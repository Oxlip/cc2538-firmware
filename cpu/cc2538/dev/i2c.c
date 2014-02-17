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
  } while (stat  & I2CM_STAT_BUSY);

  /* return failure if error was occured in the last operation*/
  if (stat & I2CM_STAT_ERROR) {
    return 1;
  }

  i2c_wait_for_ris_clear();

  return 0;
}

/* This was _not_ according to data sheet.  Just waiting on the busy flag
 *  was resulting in weird offsets and random behavior.  Even if you are
 *  not using interrupts directly, if you spin on the RIS, it will synch
 *  your results.
 */
static inline void
i2c_wait_for_ris_clear()
{
  while(!REG(I2CM_RIS));
  REG(I2CM_ICR) |= 0x01;
}


#define I2C_BUSY_WAIT_RETURN_ON_FAILURE(return_value)                       \
do {                                                                        \
  if (i2c_busy_wait()) {                                                    \
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

/** @} */