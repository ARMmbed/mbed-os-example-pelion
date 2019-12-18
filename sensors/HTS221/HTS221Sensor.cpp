/**
 ******************************************************************************
 * @file    HTS221Sensor.cpp
 * @author  CLab
 * @version V1.0.0
 * @date    5 August 2016
 * @brief   Implementation of an HTS221 Humidity and Temperature sensor.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */


/* Includes ------------------------------------------------------------------*/


#include "HTS221Sensor.h"


/* Class Implementation ------------------------------------------------------*/

HTS221Sensor::HTS221Sensor(SPI *spi, PinName cs_pin, PinName drdy_pin) : 
                           _dev_spi(spi), _cs_pin(cs_pin), _drdy_pin(drdy_pin)  // SPI3W ONLY
{    
    assert(spi); 
    _dev_i2c = NULL;
};

/** Constructor
 * @param i2c object of an helper class which handles the I2C peripheral
 * @param address the address of the component's instance
 */
HTS221Sensor::HTS221Sensor(DevI2C *i2c, uint8_t address, PinName drdy_pin) :
                           _dev_i2c(i2c), _address(address), _cs_pin(NC), _drdy_pin(drdy_pin)
{
    assert(i2c);
    _dev_spi = NULL;
};

/**
 * @brief     Initializing the component.
 * @param[in] init pointer to device specific initalization structure.
 * @retval    "0" in case of success, an error code otherwise.
 */
int HTS221Sensor::init(void *init)
{
  /* Power down the device */
  if ( HTS221_DeActivate( (void *)this ) == HTS221_ERROR )
  {
    return 1;
  }

  /* Enable BDU */
  if ( HTS221_Set_BduMode( (void *)this, HTS221_ENABLE ) == HTS221_ERROR )
  {
    return 1;
  }
  
  if(set_odr(1.0f) == 1)
  {
    return 1;
  }
  
  return 0;
}

/**
 * @brief  Enable HTS221
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::enable(void)
{
  /* Power up the device */
  if ( HTS221_Activate( (void *)this ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}

/**
 * @brief  Disable HTS221
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::disable(void)
{
  /* Power up the device */
  if ( HTS221_DeActivate( (void *)this ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}

/**
 * @brief  Read ID address of HTS221
 * @param  id the pointer where the ID of the device is stored
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::read_id(uint8_t *id)
{
  if(!id)
  { 
    return 1;
  }
  
  /* Read WHO AM I register */
  if ( HTS221_Get_DeviceID( (void *)this, id ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}

/**
 * @brief  Reboot memory content of HTS221
 * @param  None
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::reset(void)
{
    uint8_t tmpreg;

    /* Read CTRL_REG2 register */
    if (read_reg(HTS221_CTRL_REG2, &tmpreg) != 0)
    {
      return 1;
    }

    /* Enable or Disable the reboot memory */
    tmpreg |= (0x01 << HTS221_BOOT_BIT);

    /* Write value to MEMS CTRL_REG2 regsister */
    if (write_reg(HTS221_CTRL_REG2, tmpreg) != 0)
    {
      return 1;
    }
    
    return 0;
}

/**
 * @brief  Read HTS221 output register, and calculate the humidity
 * @param  pfData the pointer to data output
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::get_humidity(float* pfData)
{
  uint16_t uint16data = 0;

  /* Read data from HTS221. */
  if ( HTS221_Get_Humidity( (void *)this, &uint16data ) == HTS221_ERROR )
  {
    return 1;
  }

  *pfData = ( float )uint16data / 10.0f;

  return 0;
}

/**
 * @brief  Read HTS221 output register, and calculate the temperature
 * @param  pfData the pointer to data output
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::get_temperature(float* pfData)
{
  int16_t int16data = 0;

  /* Read data from HTS221. */
  if ( HTS221_Get_Temperature( (void *)this, &int16data ) == HTS221_ERROR )
  {
    return 1;
  }

  *pfData = ( float )int16data / 10.0f;

  return 0;
}

/**
 * @brief  Read HTS221 output register, and calculate the humidity
 * @param  odr the pointer to the output data rate
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::get_odr(float* odr)
{
  HTS221_Odr_et odr_low_level;

  if ( HTS221_Get_Odr( (void *)this, &odr_low_level ) == HTS221_ERROR )
  {
    return 1;
  }

  switch( odr_low_level )
  {
    case HTS221_ODR_ONE_SHOT:
      *odr =  0.0f;
      break;
    case HTS221_ODR_1HZ     :
      *odr =  1.0f;
      break;
    case HTS221_ODR_7HZ     :
      *odr =  7.0f;
      break;
    case HTS221_ODR_12_5HZ  :
      *odr = 12.5f;
      break;
    default                 :
      *odr = -1.0f;
      return 1;
  }

  return 0;
}

/**
 * @brief  Set ODR
 * @param  odr the output data rate to be set
 * @retval 0 in case of success, an error code otherwise
 */
int HTS221Sensor::set_odr(float odr)
{
  HTS221_Odr_et new_odr;

  new_odr = ( odr <= 1.0f ) ? HTS221_ODR_1HZ
          : ( odr <= 7.0f ) ? HTS221_ODR_7HZ
          :                   HTS221_ODR_12_5HZ;

  if ( HTS221_Set_Odr( (void *)this, new_odr ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}


/**
 * @brief Read the data from register
 * @param reg register address
 * @param data register data
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
int HTS221Sensor::read_reg( uint8_t reg, uint8_t *data )
{

  if ( HTS221_read_reg( (void *)this, reg, 1, data ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}

/**
 * @brief Write the data to register
 * @param reg register address
 * @param data register data
 * @retval 0 in case of success
 * @retval 1 in case of failure
 */
int HTS221Sensor::write_reg( uint8_t reg, uint8_t data )
{

  if ( HTS221_write_reg( (void *)this, reg, 1, &data ) == HTS221_ERROR )
  {
    return 1;
  }

  return 0;
}

uint8_t HTS221_io_write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite )
{
  return ((HTS221Sensor *)handle)->io_write(pBuffer, WriteAddr, nBytesToWrite);
}

uint8_t HTS221_io_read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead )
{
  return ((HTS221Sensor *)handle)->io_read(pBuffer, ReadAddr, nBytesToRead);
}
