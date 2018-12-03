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

typedef enum {
    NVRAM_OK        = 0,
    NVRAM_FAIL      = -1,
    NVRAM_ERR_PARAM = 1,
    NVRAM_ERR_IF    = 2,
} NVRAM_STATUS;

typedef struct {
    char *   Interface;  //!> I2C / SPI / EMULATION
    char *   DevName;    //!> Device name string
    uint8_t  DevChannel; //!> Device I2C/SPI bus channel
    uint8_t  DevAddr;    //!> Device I2C/SPI address
    uint8_t  DataBit;    //!> Device Data bit, 8/16/32
    uint32_t DataVolume; //!> Maximum data volume in the device.
} Nvram_InfoTypeDef;

typedef struct {
    /*! Basic Function */
    NVRAM_STATUS (*Init)(void);
    NVRAM_STATUS (*DeInit)(void);
    NVRAM_STATUS (*Erase)(void);

    /*! Register R/W Function */
    NVRAM_STATUS (*Write)(uint32_t addr, uint32_t value);
    NVRAM_STATUS (*Read)(uint32_t addr, uint32_t *value);
    NVRAM_STATUS (*WriteEx)(uint8_t reg, uint8_t *value, uint16_t len);
    NVRAM_STATUS (*ReadEx)(uint8_t reg, uint8_t *value, uint16_t len);

    /*! Get Value*/
    NVRAM_STATUS (*GetInfo)(Nvram_InfoTypeDef *info);
} Nvram_DrvTypeDef;

Nvram_DrvTypeDef Nvram_Drv;

NVRAM_STATUS Bsp_Nvram_Init();
NVRAM_STATUS Bsp_Nvram_DeInit();

#endif /* INC_BSP_BSP_NVRAM_H_ */
