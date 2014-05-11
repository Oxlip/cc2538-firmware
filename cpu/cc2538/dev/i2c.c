/*
 * Copyright (c) 2013, elarm Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
 /**
 * \addtogroup cc2538-i2c
 * @{
 *
 * \file
 * C file for the cc2538 i2c driver
 *
 * \author
 *          Adam Rea <areairs@gmail.com>
*/

#include "contiki.h"
#include "dev/i2c.h"
#include "sys/energest.h"
#include "dev/sys-ctrl.h"
#include "dev/ioc.h"
#include "dev/gpio.h"
#include "reg.h"
#include <stdio.h>


/*
 * These values were lifted from cc2538 User Guied p. 438 based on 16MHz clock.
 */
#define I2C_SPEED_400K  0x01
#define I2C_SPEED_100K  0x07

/*---------------------------------------------------------------------------*/
void
i2c_init(void)
{
  /* Turn on the i2c clock in all states */
  REG(SYS_CTRL_RCGCI2C) |= 0x0001;
  REG(SYS_CTRL_SCGCI2C) |= 0x0001;
  REG(SYS_CTRL_DCGCI2C) |= 0x0001;

  /* Setup the SDA and SCL to the right port functions and map
   *  the port:pin to the I2CMSSxx registers to grab those pins
   *  Set the pins to periphreal mode (SCL and SDA)
   */
  REG(I2C_SCL_PORT_BASE + GPIO_AFSEL) |= (0x0001 << I2C_SCL_PIN);
  ioc_set_sel(I2C_SCL_PORT, I2C_SCL_PIN, IOC_PXX_SEL_I2C_CMSSCL);
  ioc_set_over(I2C_SCL_PORT, I2C_SCL_PIN,IOC_OVERRIDE_DIS);
  REG(IOC_I2CMSSCL) |= ( (I2C_SCL_PORT << 3) + I2C_SCL_PIN);

  REG(I2C_SDA_PORT_BASE + GPIO_AFSEL) |= (0x0001 << I2C_SDA_PIN);
  ioc_set_sel(I2C_SDA_PORT, I2C_SDA_PIN, IOC_PXX_SEL_I2C_CMSSDA);
  ioc_set_over(I2C_SDA_PORT, I2C_SDA_PIN,IOC_OVERRIDE_DIS);
  REG(IOC_I2CMSSDA) |= ( (I2C_SDA_PORT << 3) + I2C_SDA_PIN);

  /*
   * Enable the master block.
   */
  REG(I2CM_CR) |= I2CM_CR_MFE;

  /*
   *  Get the desired SCL speed. Can be set with _CONF_'s in project-conf.h
   */

#if I2C_CONF_HI_SPEED /* 400k */
  REG(I2CM_TPR) = I2C_SPEED_400K;
#else /*  I2C_CONF_HI_SPEED */ /* 100k */
  REG(I2CM_TPR) = I2C_SPEED_100K;
#endif /*  I2C_CONF_HI_SPEED */

  /* Should be ready to go as a master */
}

/*---------------------------------------------------------------------------*/
static inline int
i2c_busy_wait()
{
  uint8_t stat;
  /* wait on busy then check error flag */
  do {
    stat = REG(I2CM_STAT);
  } while (stat & I2CM_STAT_BUSY);

  /* return failure if error was occured in the last operation*/
  if (stat & I2CM_STAT_ERROR) {
    return 1;
  }
#define I2C_WAIT_FOR_RIS
#ifdef I2C_WAIT_FOR_RIS
/* This was _not_ according to data sheet.  Just waiting on the busy flag
 *  was resulting in weird offsets and random behavior.  Even if you are
 *  not using interrupts directly, if you spin on the RIS, it will synch
 *  your results.
 */
  while(!REG(I2CM_RIS));
  REG(I2CM_ICR) |= 0x01;
#endif

  return 0;
}


#define I2C_BUSY_WAIT_RETURN_ON_FAILURE(return_value)                       \
do {                                                                        \
  if (i2c_busy_wait()) {                                                    \
    printf("i2c_busy_wait() failed with %d at %s:%d \n", return_value,      \
           __func__, __LINE__);                                             \
    return return_value;                                                    \
  }                                                                         \
} while(0)

/*---------------------------------------------------------------------------*/
uint8_t
i2c_write_byte(uint8_t slave_address, uint8_t value)
{
  /* check that slaveaddr only has 7 bits active */
  if (slave_address & 0x80){
    return 1;
  }

  /* Set slave addr (data sheet says 0:6 are address with a s/r bit but
   *  the diagram shows address is 1:7 with the s/r bit in 0...I''m trusting
   *  the picture - makes most sense. Also matches the calcualtion on pp 453.
   *  Send is bit 0 low Rx is bit 0 high
   *  now that we checked the addr put it in to write
   */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);

  REG(I2CM_DR) = value;
  REG(I2CM_CTRL) = I2C_MASTER_CMD_SINGLE_SEND;

  /* wait on busy then check error flag */
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(0);

  return 0;
}

/*---------------------------------------------------------------------------*/
uint8_t
i2c_write_bytes(uint8_t slave_address, uint8_t* buffer, uint8_t len)
{
  /* check that slave address only has 7 bits active */
  if (slave_address & 0x80){
    return 1;
  }

  /* Set slave addr (data sheet says 0:6 are address with a s/r bit but
   *  the diagram shows address is 1:7 with the s/r bit in 0...I''m trusting
   *  the picture - makes most sense. Also matches the calcualtion on pp 453.
   *  Send is bit 0 low Rx is bit 0 high
   *  now that we checked the addr put it in to write
   */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);

  if (len == 1) {
    return i2c_write_byte(slave_address, buffer[0]);
  } else {
    uint8_t c;

    for(c = 0; c < len; c++) {
      REG(I2CM_DR) = buffer[c];
      if(c == 0) {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_START;
      } else if (c == len - 1) {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_FINISH;
      } else {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_CONT;
      }

      I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);
    }
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
uint8_t
i2c_read_byte(uint8_t slave_address, uint8_t *result)
{
  /*check that slave_address only has 7 bits active */
  if (slave_address & 0x80){
    return 1;
  }

  /* Set slave addr (data sheet says 0:6 are address with a s/r bit but
   *  the diagram shows address is 1:7 with the s/r bit in 0...I''m trusting
   *  the picture - makes most sense. Also matches the calcualtion on pp 453.
   *  Send is bit 0 low.  Rx is bit 0 high
   *  now that we checked the addr put it in to read
   */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_RECEIVE(slave_address);

  REG(I2CM_CTRL) = I2C_MASTER_CMD_SINGLE_RECEIVE;

   /* wait on busy then check error flag */
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(0);

  /* return the byte in the DR register */
  *result = REG(I2CM_DR);

  return 0;
}

/*---------------------------------------------------------------------------*/
uint8_t
i2c_read_bytes(uint8_t slave_address, uint8_t* buffer, uint8_t len)
{
  uint8_t c;

  /*check that slave_address only has 7 bits active */
  if (slave_address & 0x80){
    return 1;
  }

  /* Set slave addr (data sheet says 0:6 are address with a s/r bit but
   *  the diagram shows address is 1:7 with the s/r bit in 0...I''m trusting
   *  the picture - makes most sense. Also matches the calcualtion on pp 453.
   *  Send is bit 0 low.  Rx is bit 0 high
   *  now that we checked the addr put it in to read
   */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_RECEIVE(slave_address);

  if (len == 1) {
    return i2c_read_byte(slave_address, buffer);
  } else {
    for(c = 0; c < len; c++) {
      buffer[c] = 0;
      if(c == 0) {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_START;
      } else if (c == len - 1) {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_FINISH;
      } else {
        REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_CONT;
      }

      /* wait on busy then check error flag */
      I2C_BUSY_WAIT_RETURN_ON_FAILURE(0);

      buffer[c] = REG(I2CM_DR);
    }
  }
  return 0;
}

/*
  1. Send a start sequence
  2. Send slave address(write)
  3. Send register offset
  4. Send a start sequence again (repeated start)
  5. Send slave address | 1(read)
  6. Read data byte
  7. Send the stop sequence.
 */
uint8_t
i2c_smb_read_byte(uint8_t slave_address, uint8_t offset, uint8_t *result)
{
  *result = 0;
  if (slave_address & 0x80) {
    return 1;
  }

  /* Set slave address */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);
  /* Set the offset */
  REG(I2CM_DR) = offset;
  /* Start sequence */
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);

  /* Set slave address and resume sequence(start sequene again) */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_RECEIVE(slave_address);
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(3);
  /* Read databyte */
  *result = REG(I2CM_DR);

  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_FINISH;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(3);

  return 0;
}

/*
1. Send a start sequence
2. Send the I2C address of the slave with the R/W bit low (even address)
3. Send the internal register number you want to write to
4. Send the data byte
6. Send the stop sequence.
 */
uint8_t
i2c_smb_write_byte(uint8_t slave_address, uint8_t offset, uint8_t value)
{
  if (slave_address & 0x80) {
    return 1;
  }

  /* Set slave address */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);
  /* Write the offset */
  REG(I2CM_DR) = offset;
  /* Start sequence */
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);

  /* Write the value */
  REG(I2CM_DR) = value;
  /* stop sequence*/
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_FINISH;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(3);

  return 0;
}


uint8_t
i2c_smb_read_word(uint8_t slave_address, uint8_t offset, uint16_t *result)
{
  *result = 0;
  if (slave_address & 0x80) {
    return 1;
  }

  /* Set slave address */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);
  /* Set the offset */
  REG(I2CM_DR) = offset;
  /* Start sequence */
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);

  /* Set slave address and resume sequence(start sequene again) */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_RECEIVE(slave_address);
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(3);
  /* Read data low byte */
  *result = REG(I2CM_DR);

  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_RECEIVE_FINISH;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(4);
  /* Read data high byte */
  *result |= REG(I2CM_DR) << 8;

  return 0;
}

uint8_t
i2c_smb_write_word(uint8_t slave_address, uint8_t offset, uint16_t value)
{
  if (slave_address & 0x80) {
    return 1;
  }

  /* Set slave address */
  REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_SEND(slave_address);
  /* Write the offset */
  REG(I2CM_DR) = offset;
  /* Start sequence */
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_START;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);

  /* Write the low byte */
  REG(I2CM_DR) = (uint8_t)(value & 0xff);
  /* Start sequence */
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_CONT;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(2);

  /* Write the high byte */
  REG(I2CM_DR) = (uint8_t)(value >> 8);;
  /* stop sequence*/
  REG(I2CM_CTRL) = I2C_MASTER_CMD_BURST_SEND_FINISH;
  I2C_BUSY_WAIT_RETURN_ON_FAILURE(3);

  return 0;
}

#ifdef I2C_DEBUG

#define TSL2561_ADDRESS 0x39
#define TSL2561_COMMAND(reg)  (0x80 | reg)
void
i2c_test()
{
  uint8_t i, value;
  uint16_t value16;

  //start the device
  i2c_smb_write_byte(TSL2561_ADDRESS, TSL2561_COMMAND(0), 0b11);
  //read the value back to confirm
  i2c_smb_read_byte(TSL2561_ADDRESS, TSL2561_COMMAND(0), &value);
  printf("Power %d\n", value);

  i2c_smb_read_byte(TSL2561_ADDRESS, TSL2561_COMMAND(0xa), &value);
  printf("ID %d\n", value);

  for(i=0xc; i <= 0xf; i++) {
    i2c_smb_read_byte(TSL2561_ADDRESS, TSL2561_COMMAND(i), &value);
    printf("smb read (offset = %x, val=%d)\n", i, value);
  }

  i2c_smb_read_word(TSL2561_ADDRESS, TSL2561_COMMAND(0x20 | 0xc), &value16);
  printf("16bit value %d\n", value16);
}

void
i2c_scan()
{
  uint8_t slaveaddr, stat, total=0;

  printf("Scanning for i2c devices:\n");
  for(slaveaddr=1; slaveaddr < 127; slaveaddr++) {
    REG(I2CM_SA) = I2CM_SLAVE_ADDRESS_FOR_RECEIVE(slaveaddr);
    REG(I2CM_CTRL) = 0b111;

    /* wait on busy then check error flag */
    do {
      stat = REG(I2CM_STAT);
    } while (stat & I2CM_STAT_BUSY);
    uint8_t unused  __attribute__((unused))= REG(I2CM_DR);
    if (!(stat & I2CM_STAT_ADRACK)) {
      total++;
      printf("slave found at %d\n", slaveaddr);
      continue;
    }
  }
  printf("%d i2c devices found \n", total);
}
#endif

/** @} */
