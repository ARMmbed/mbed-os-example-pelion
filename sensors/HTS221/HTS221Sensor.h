/**
 ******************************************************************************
 * @file    HTS221Sensor.h
 * @author  CLab
 * @version V1.0.0
 * @date    5 August 2016
 * @brief   Abstract class of an HTS221 Humidity and Temperature sensor.
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


/* Prevent recursive inclusion -----------------------------------------------*/

#ifndef __HTS221Sensor_H__
#define __HTS221Sensor_H__


/* Includes ------------------------------------------------------------------*/

#include "DevI2C.h"
#include "HTS221_driver.h"
#include "HumiditySensor.h"
#include "TempSensor.h"
#include <assert.h>

/* Class Declaration ---------------------------------------------------------*/

/**
 * Abstract class of an HTS221 Humidity and Temperature sensor.
 */
class HTS221Sensor : public HumiditySensor, public TempSensor
{
  public:
    HTS221Sensor(SPI *spi, PinName cs_pin=NC, PinName drdy_pin=NC);    // SPI3W ONLY
    HTS221Sensor(DevI2C *i2c, uint8_t address=HTS221_I2C_ADDRESS, PinName drdy_pin=NC);
    virtual int init(void *init);
    virtual int read_id(uint8_t *id);
    virtual int get_humidity(float *pfData);
    virtual int get_temperature(float *pfData);
    int enable(void);
    int disable(void);
    int reset(void);
    int get_odr(float *odr);
    int set_odr(float odr);
    int read_reg(uint8_t reg, uint8_t *data);
    int write_reg(uint8_t reg, uint8_t data);
    /**
     * @brief Utility function to read data.
     * @param  pBuffer: pointer to data to be read.
     * @param  RegisterAddr: specifies internal address register to be read.
     * @param  NumByteToRead: number of bytes to be read.
     * @retval 0 if ok, an error code otherwise.
     */
    uint8_t io_read(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToRead)
    {
        if (_dev_spi) {
        /* Write Reg Address */
            _dev_spi->lock();
            _cs_pin = 0;           
            /* Write RD Reg Address with RD bit*/
            uint8_t TxByte = RegisterAddr | 0x80;    
            _dev_spi->write((char *)&TxByte, 1, (char *)pBuffer, (int) NumByteToRead);
            _cs_pin = 1;
            _dev_spi->unlock(); 
            return 0;
        }                       
        if (_dev_i2c) return (uint8_t) _dev_i2c->i2c_read(pBuffer, _address, RegisterAddr, NumByteToRead);
        return 1;
    }
    
    /**
     * @brief Utility function to write data.
     * @param  pBuffer: pointer to data to be written.
     * @param  RegisterAddr: specifies internal address register to be written.
     * @param  NumByteToWrite: number of bytes to write.
     * @retval 0 if ok, an error code otherwise.
     */
    uint8_t io_write(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToWrite)
    {
        if (_dev_spi) { 
            _dev_spi->lock();
            _cs_pin = 0;
            int data = _dev_spi->write(RegisterAddr);                    
            _dev_spi->write((char *)pBuffer, (int) NumByteToWrite, NULL, 0);                     
            _cs_pin = 1;                    
            _dev_spi->unlock();
            return data;                    
        }        
        if (_dev_i2c) return (uint8_t) _dev_i2c->i2c_write(pBuffer, _address, RegisterAddr, NumByteToWrite);    
        return 1;
    }

  private:

    /* Helper classes. */
    DevI2C *_dev_i2c;
    SPI    * _dev_spi;
    
    /* Configuration */
    uint8_t _address;
    DigitalOut  _cs_pin;        
    InterruptIn _drdy_pin;    
};

#ifdef __cplusplus
 extern "C" {
#endif
uint8_t HTS221_io_write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite );
uint8_t HTS221_io_read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead );
#ifdef __cplusplus
  }
#endif

#endif
