/*
 * bsp_drivers.h
 *
 *  Created on: 2018年9月20日
 *      Author: nickyang
 */

#ifndef DRIVERS_BSP_BSP_COMMON_H_
#define DRIVERS_BSP_BSP_COMMON_H_

#include "stdint.h"

#include "stm32l476g_discovery.h"

typedef struct
{
    /*! Basic Function */
    void (*Init)(void);
    void (*DeInit)(void);
    void (*Reset)(void);

    /*! Register R/W Function */
    void (*Write)(uint8_t reg, uint8_t value);
    uint8_t (*Read)(uint8_t reg);
    void (*WriteEx)(uint8_t reg, uint8_t *value, uint16_t len);
    void (*ReadEx)(uint8_t reg, uint8_t *value, uint16_t len);

    /*! Get Value*/
    void (*GetDevId)(int16_t *);
    void (*GetXYZ)(int16_t *);

} BSP_Accel_DrvTypeDef;

typedef struct
{
    /*! Basic Function */
    void (*Init)(void);
    void (*DeInit)(void);
    void (*Reset)(void);

    /*! Register R/W Function */
    void (*Write)(uint8_t reg, uint8_t value);
    uint8_t (*Read)(uint8_t reg);
    void (*WriteEx)(uint8_t reg, uint8_t *value, uint16_t len);
    void (*ReadEx)(uint8_t reg, uint8_t *value, uint16_t len);

    /*! Get Value*/
    void (*GetDevId)(int16_t *);
    void (*GetXYZ)(int16_t *);
} BSP_Gyro_DrvTypeDef;

typedef struct
{
    /*! Basic Function */
    void (*Init)(void);
    void (*DeInit)(void);
    void (*Reset)(void);

    /*! Register R/W Function */
    void (*Write)(uint8_t reg, uint8_t value);
    uint8_t (*Read)(uint8_t reg);
    void (*WriteEx)(uint8_t reg, uint8_t *value, uint16_t len);
    void (*ReadEx)(uint8_t reg, uint8_t *value, uint16_t len);

    /*! Get Value*/
    void (*GetDevId)(int16_t *);
    void (*GetXYZ)(int16_t *);
} BSP_Compass_DrvTypeDef;

typedef struct
{

} BSP_SpiFlash_DrvTypeDef;

#endif /* DRIVERS_BSP_BSP_COMMON_H_ */
