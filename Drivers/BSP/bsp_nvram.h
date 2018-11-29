/*
 * bsp_nvram.h
 *
 *  Created on: 2018年8月1日
 *      Author: nickyang
 */

#ifndef INC_BSP_BSP_NVRAM_H_
#define INC_BSP_BSP_NVRAM_H_

#include "eeprom_emul.h"
#include "stdint.h"

#define NVRAM_TYPE_INTERNAL_FLASH 0 //!>Use Internal flash emulate EEPROM
#define NVRAM_TYPE_I2C_EEPROM 1     //!>User External EEPROM

#define NVRAM_TYPE NVRAM_TYPE_INTERNAL_FLASH

/*!@defgroup Internal Flash EMUL EEPROM parameters
 *
 */
#if NVRAM_TYPE == NVRAM_TYPE_INTERNAL_FLASH
#define NVRAM_MODEL "STM32L4 Flash EMUL EEPROM"
#define NVRAM_DATA_BIT 32               //!>NVRAM data bit, default is 32bit
#define NVRAM_DATA_BYTE 4               //!>NVRAM data bit, default is 32bit = 4byte
#define NVRAM_DATA_SIZE NB_OF_VARIABLES //!>NVRAM data size
#endif

/*!@defgroup External EEPROM parameters
 *
 */
#if NVRAM_TYPE == NVRAM_TYPE_I2C_EEPROM
#define NVRAM_MODEL "25W16"
#define NVRAM_DATA_BIT 8        //!>NVRAM data length in bit
#define NVRAM_DATA_BYTE 1       //!>NVRAM data length in byte
#define NVRAM_DATA_SIZE 1024    //!>NVRAM size
#define NVRAM_I2C_HANDLE hi2c3  //!>I2C channel
#define NVRAM_I2C_SLV_ADDR 0x50 //!>I2C slave address
#endif

typedef struct {
    const char *name;
    uint8_t     DataBit;
    uint8_t     DataSize;
} Nvram_Info;

typedef struct {
    /*! Basic Function */
    void (*Init)(void);
    void (*DeInit)(void);
    void (*Erase)(void);

    /*! Register R/W Function */
    void (*Write)(uint32_t addr, uint32_t value);
    uint8_t (*Read)(uint32_t addr);
    void (*WriteEx)(uint8_t reg, uint8_t *value, uint16_t len);
    void (*ReadEx)(uint8_t reg, uint8_t *value, uint16_t len);

    /*! Get Value*/
    void (*GetInfo)(Nvram_Info *info);
} Nvram_DrvTypeDef;

int  Bsp_Nvram_init();
int  Bsp_Nvram_deinit();
int  Bsp_Nvram_erase();
int  Bsp_Nvram_write(uint16_t addr, uint32_t value);
int  Bsp_Nvram_read(uint16_t addr, uint32_t *value);
void Bsp_Nvram_info();

#endif /* INC_BSP_BSP_NVRAM_H_ */
