/*
 * Copyright (c) 2013, Texas Instruments Incorporated - http://www.ti.com/
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * cc2538 linker configuration file. This is not the actual file used at link
 * stage. Rather, it is used as input for the auto-generation of the actual
 * ld script, which is called cc2538.ld and will be in the project directory
 */
#if (LPM_CONF_MAX_PM==2) && (LPM_CONF_ENABLE != 0)
#define NRSRAM_START    0x20000000
#define NRSRAM_LEN      0x00004000
#define SRAM_START      0x20004000
#define SRAM_LEN        0x00004000
#else
#define SRAM_START      0x20000000
#define SRAM_LEN        0x00008000
#endif

#define FLASH_CCA_SIZE          44
#define FLASH_CCA_START_ADDR    (FLASH_START_ADDR + FLASH_SIZE - FLASH_CCA_SIZE)

MEMORY
{
    FLASH (rx) : ORIGIN = FLASH_START_ADDR, LENGTH = FLASH_SIZE - FLASH_CCA_SIZE
#if FLASH_CONF_WRITE_CCA
    FLASH_CCA (RX) : ORIGIN = FLASH_CCA_START_ADDR, LENGTH = FLASH_CCA_SIZE
#endif
#if (LPM_CONF_MAX_PM==2) && (LPM_CONF_ENABLE != 0)
    NRSRAM (RWX) : ORIGIN = NRSRAM_START, LENGTH = NRSRAM_LEN
#endif
    SRAM (RWX) : ORIGIN = SRAM_START, LENGTH = SRAM_LEN
}

SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH= 0

    .socdata (NOLOAD) :
    {
        *(.udma_channel_control_table)
    } > SRAM

    .data :
    {
        _data = .;
        *(.data*)
        _edata = .;
    } > SRAM AT > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    .bss :
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

#if (LPM_CONF_MAX_PM==2) && (LPM_CONF_ENABLE != 0)
    .nrdata (NOLOAD) :
    {
        _nrdata = .;
        *(.nrdata*)
        _enrdata = .;
    } > NRSRAM
#endif

#if FLASH_CONF_WRITE_CCA
    .flashcca :
    {
        KEEP(*(.flashcca))
    } > FLASH_CCA
#endif
}
