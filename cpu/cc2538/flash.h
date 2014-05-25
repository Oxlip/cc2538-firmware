#ifndef __FLASH_HW_REG__
#define __FLASH_HW_REG__

#define  FLASH_CTRL_CACHE_MODE_DISABLE          0x0
#define  FLASH_CTRL_CACHE_MODE_ENABLE           0x4
#define  FLASH_CTRL_CACHE_MODE_PREFETCH_ENABLE  0x8
#define  FLASH_CTRL_CACHE_MODE_REALTIME         0xc

#define FLASH_CTRL_FCTL         0x400D3008
#define FLASH_CTRL_FADDR        0x400D300C
#define FLASH_CTRL_FCTL_CM_M    0x0000000C
#define FLASH_CTRL_FCTL_BUSY    0x00000080

static inline uint32_t
flash_get_cache_mode(void)
{
  return(REG(FLASH_CTRL_FCTL) & FLASH_CTRL_FCTL_CM_M);
}

static inline void
flash_set_cache_mode(uint32_t ui32CacheMode)
{
  uint32_t ui32Busy;
  uint32_t ui32TempValue;

  /* Wait until FLASH is not busy. */
  ui32Busy = 1;
  while(ui32Busy)
  {
      ui32TempValue = REG(FLASH_CTRL_FCTL);
      ui32Busy      = ui32TempValue & FLASH_CTRL_FCTL_BUSY;
  }

  ui32TempValue           &= ~FLASH_CTRL_FCTL_CM_M;
  REG(FLASH_CTRL_FCTL) = ui32TempValue | ui32CacheMode;
}

#endif