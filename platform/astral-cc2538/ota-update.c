/**
 * OTA firmwre update.
 *
 * Over The Air firmware update is done using usual two partition method.
 * The flash memory is (virtually) split into two equal portions. The
 * firware runs on one partition and when a update is pushed it is
 * stored in the other partition and flashCCA is updated to point the
 * next partition as active partition, in the next boot CC2538 will boot
 * from updated firmware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"

#include "er-coap-13.h"
#include "erbium.h"
#include "rplinfo.h"
#include "rom.h"
#include "flash.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/**
 * We need Two partitons - One active, another one for updating.
 * So max size of firmware can be (512K/2)-2K.
 * 512K - Size of flash memory in CC2538.
 * 2 - Number of partitions.
 * 2K - Flash page size in CC2538.
 * Note - 1 flash page size is 2K and last page is reserved for CCA.
 */
#define FLASH_START_ADDR                0x200000
#define FLASH_TOTAL_SIZE                (512 * 1024)
#define FLASH_PAGE_SIZE                 2048
#define OTA_UPDATE_FIRMWARE_MAX_SIZE    ((FLASH_TOTAL_SIZE / 2) - FLASH_PAGE_SIZE)

#define FLASH_CCA_SIZE          44
#define FLASH_CCA_START_ADDR    (FLASH_START_ADDR + FLASH_TOTAL_SIZE - FLASH_CCA_SIZE)

/* Refer CC2538 ROM bootloader documenation for details about this structure. */
typedef struct {
  uint32_t      enable;
  uint32_t      valid;
  uint32_t      text;
} flash_cca_t;

typedef struct {
  /* Where to start writing the new firmware in the flash.
   * There could be two possibilities - first half or second half.
   */
  uint32_t start_address;
  /* Size of the new firmware. */
  uint32_t length;
  /* Checksum of the firmware excluding this header. */
  uint32_t crc32;
  /* Version of the new firmware. */
  uint16_t version;
  /* Flags related to upload process. */
  uint16_t flags;
} ota_update_header_t;

/* Only one update process can be running at a time.
   This structure caches the header that was send in the first update packet.
 */
static ota_update_header_t ota_header;

extern uint32_t _text;

/* Helper function to set response with failure error code.
 */
static void
ota_update_failure(void* response, char *error_msg)
{
  memset(&ota_header, 0, sizeof(ota_header));
  REST.set_response_payload(response, error_msg, strlen(error_msg));
  REST.set_response_status(response, REST.status.BAD_REQUEST);
}

#define OTA_UPDATE_FAILURE(err_msg)                       \
  do {                                                    \
    ota_update_failure(response, err_msg);                \
    return;                                               \
  } while(0);

/* Erases the given page.
 *
 * This function uses a recent erased page array to cache last erased pages.
 * This is NOT for optimization only but for correctness of the program.
 * Since the coap blocks are going to be 16byte to 128byte it will take
 * multiple coap requests before a whole page can be filled. If page
 * erase is performed for each request then it will erase previously
 * written content.
 */
static int
erase_flash_page(uint32_t page_no)
{
  static int recent_erase_pages[2] = {-1, -1};
  uint32_t cache_mode;
  int result;
  uint32_t address = page_no * FLASH_PAGE_SIZE;

  if (page_no == recent_erase_pages[0] || page_no == recent_erase_pages[1]) {
    return 0;
  }

  PRINTF("Erasing flash page %lx\n", address);
  cache_mode = flash_get_cache_mode();
  result = ROM_PageErase(address, FLASH_PAGE_SIZE);
  flash_set_cache_mode(cache_mode);
  if (result) {
    PRINTF("Erase error %d\n", result);
    return result;
  }
  recent_erase_pages[0] = recent_erase_pages[1];
  recent_erase_pages[1] = page_no;

  return 0;
}

/* Writes given buffer to given flash address.
 *
 * This function will take care of page erasing and verification after
 * writing.
 */
static int
write_to_flash(uint8_t *buffer, uint32_t address, uint32_t length)
{
  int result, retry_count = 0;
  uint32_t cache_mode, start_page, end_page;

  start_page = address / FLASH_PAGE_SIZE;
  end_page = (address + length) / FLASH_PAGE_SIZE;
  if (erase_flash_page(start_page)) {
    return 1;
  }
  if (erase_flash_page(end_page)) {
    return 2;
  }

  /* Foundation firmware code says cache mode might be modified by page_erase and write.
   * So lets preserve and restore the cache mode.
   */
  cache_mode = flash_get_cache_mode();

retry:
  if (retry_count > 5) {
    PRINTF("Too many retries, aborting\n");
    return 3;
  }
  PRINTF("Writing to %lx size %lx\n", (uint32_t)address, length);
  result = ROM_ProgramFlash((void *)buffer, (uint32_t)address, length);
  flash_set_cache_mode(cache_mode);
  retry_count++;
  if (result) {
    PRINTF("Flash programming error %d, retrying...\n", result);
    goto retry;
  }

  /* Verify the page. */
  if (ROM_Memcmp((void *)buffer, (void *)address, length)) {
    PRINTF("Flash verification failed, retrying...\n");
    goto retry;
  }

  return 0;
}

/*
 * When all the packets are written to the flash, dynamically update
 * the CCA with new start address. It verifies the new firmware's
 * checksum before making it active.
 */
static int
write_cca(void* response)
{
  uint32_t calc_crc32;
  flash_cca_t cca = {
    0xFFFFFFFF,                  /* Bootloader backdoor enabled. */
    0,                           /* Image valid. */
    ota_header.start_address
  };

  calc_crc32 = ROM_Crc32((void *)ota_header.start_address, ota_header.length);
  if (calc_crc32 != ota_header.crc32) {
    PRINTF("checksum mismatch %lx %lx\n", calc_crc32, ota_header.crc32);
    ota_update_failure(response, "Checksum mismatch.");
    return 1;
  }

  if (write_to_flash((uint8_t *)&cca, FLASH_CCA_START_ADDR, sizeof(cca))) {
    ota_update_failure(response, "Flash CCA programming error.");
    return 1;
  }
  return 0;
}

/*
 * CoAP resource for firmware update.
 * Note - It worked fine with block size of 128bytes. I am not sure about
 * other sizes.
 */

RESOURCE(ota_update, METHOD_GET | METHOD_PUT, "debug/update", "title=\"Firmware update\";rt=\"block\"");
void
ota_update_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_packet_t *const coap_req = (coap_packet_t *) request;
  uint8_t *incoming = NULL;
  size_t len = 0, prev_len, write_len;
  uint8_t method = REST.get_method_type(request);
  unsigned int ct = REST.get_header_content_type(request);
  uint32_t start_address = 0;

  if (method & METHOD_GET) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_response_payload(response, buffer, snprintf((char *)buffer, 64, "Part:%d", 1));
    return;
  }

  if (ct != APPLICATION_OCTET_STREAM) {
    OTA_UPDATE_FAILURE("Invalid Content-Type.");
  }

  len = REST.get_request_payload(request, (const uint8_t **) &incoming);
  if (len == 0) {
    OTA_UPDATE_FAILURE("Missing firmware.");
  }

  prev_len = coap_req->block1_num * coap_req->block1_size;
  if (prev_len == 0) {
    /* First packet in the update request - should have the header. */
    memcpy(&ota_header, incoming, sizeof(ota_header));
    start_address = ota_header.start_address;
    write_len = len - sizeof(ota_header);
    incoming += sizeof(ota_header);
    if (start_address == (uint32_t)&_text ||
        start_address < FLASH_START_ADDR ||
        start_address > (FLASH_START_ADDR + FLASH_TOTAL_SIZE) ||
        ota_header.length > OTA_UPDATE_FIRMWARE_MAX_SIZE) {
      OTA_UPDATE_FAILURE("Invalid firmware image.");
    }
  } else {
    if (ota_header.start_address == 0) {
      OTA_UPDATE_FAILURE("Restart OTA update.");
    }
    start_address = ota_header.start_address + prev_len - sizeof(ota_header);
    write_len = len;
  }
  if (prev_len + len > OTA_UPDATE_FIRMWARE_MAX_SIZE) {
    OTA_UPDATE_FAILURE("Invalid firmware.");
  }

  /* Write the firmware contents.*/
  if (write_len > 0) {
    if (write_to_flash(incoming, start_address, write_len)) {
      OTA_UPDATE_FAILURE("Flash programming error.");
    }
  }

  /* If last packet - write CCA and complete the update. */
  if (prev_len + write_len - sizeof(ota_header) == ota_header.length) {
    if (write_cca(response)) {
      return;
    }
  }

  REST.set_response_status(response, REST.status.CHANGED);
  coap_set_header_block1(response, coap_req->block1_num, 0, coap_req->block1_size);
}

void
ota_update_enable()
{
  rest_activate_resource(&resource_ota_update);
}

