/******************************************************************************
 * @file    bsp_nvram.c
 * @brief   Board Support Package for NVRAM (Non-Volatile Random Access Memory)
 *          Support the following device:
 *          - STM32L476 internal flash emulation.
 *          - External EEPROM device (TBD)
 *
 * @author  Nick Yang
 * @date    2018/04/27
 * @version V0.2
 *****************************************************************************/

#include "bsp_nvram.h"
#include "stdio.h"

#include "eeprom_emul.h"

// General Print
#define NVRAM_PRINT(msg, args...)                                                                  \
    do                                                                                             \
    {                                                                                              \
        fprintf(stdout, "<%s:%d> " msg, __FILE__, __LINE__, ##args);                               \
    } while (0)

// Error Message output, with RED color.
#define NVRAM_ERROR(msg, args...)                                                                  \
    do                                                                                             \
    {                                                                                              \
        fprintf(stderr, "\e[31m%s: " msg "\e[0m", __FILE__, ##args);                               \
    } while (0)

#define CHECK_FUNC_RET(status, func)                                                               \
    do                                                                                             \
    {                                                                                              \
        int ret = func;                                                                            \
        if (status != ret)                                                                         \
        {                                                                                          \
            fprintf(stderr, "\e[31mERROR: Return=[%d] " #func "<%s:%d>\n\e[0m", ret, __FILE__,     \
                    __LINE__);                                                                     \
            return ret;                                                                            \
        }                                                                                          \
    } while (0)

/*!@var Nvram_VirtualTab
 * Virtual address and real address look up table.
 */
const uint16_t EEP_EMUL_VirtualTab[] = {
    0x100, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10,  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20,  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30,  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40,  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50,  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60,  0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70,  0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80,  0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90,  0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0,  0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0,  0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0,  0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0,  0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0,  0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0,  0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

Nvram_DrvTypeDef Nvram_Drv = {0};

NVRAM_STATUS EEP_EMUL_Init()
{
    // Unlock Flash for EEPROM_Emul to initialize
    HAL_FLASH_Unlock();
    EE_Status status = 0;

    status = EE_Init((uint16_t *)EEP_EMUL_VirtualTab, EE_CONDITIONAL_ERASE);

    if (EE_ERROR_NOACTIVE_NORECEIVE_NOVALID_PAGE == status)
    {
        NVRAM_PRINT("\e[33mEEPROM_Emul Flash is not initialized, run EE_Format().\e[0m\n");
        HAL_FLASH_Unlock();
        status = EE_Format(EE_FORCED_ERASE);
        HAL_FLASH_Lock();
    }

    if (EE_OK == status)
    {
        NVRAM_PRINT("EEPROM_Emul Initialize success!\n");
    }
    else
    {
        NVRAM_ERROR("EEPROM_Emul Initialize Fail, error code=[%d].\nReset MCU in 10ms.\n", status);
        HAL_Delay(10);
        HAL_NVIC_SystemReset();
    }

    // Lock Flash after initialize finish.
    HAL_FLASH_Lock();

    return 0;
}

NVRAM_STATUS EEP_EMUL_DeInit()
{
    return 0;
}

NVRAM_STATUS EEP_EMUL_Erase()
{
    EE_Status ret = 0;
    HAL_FLASH_Unlock();
    ret = EE_Format(EE_FORCED_ERASE);
    HAL_FLASH_Lock();
    return ret;
}

NVRAM_STATUS EEP_EMUL_Write(uint32_t addr, uint32_t value)
{
    EE_Status ret = 0;

    HAL_FLASH_Unlock();
    ret = EE_WriteVariable32bits(addr, value);

    if (ret == EE_CLEANUP_REQUIRED)
    {
        ret = EE_CleanUp();
        ret = EE_WriteVariable32bits(addr, value);
    }

    HAL_FLASH_Lock();
    return ret;
}

NVRAM_STATUS EEP_EMUL_Read(uint32_t addr, uint32_t *value)
{
    return EE_ReadVariable32bits(addr, value);
}

NVRAM_STATUS EEP_EMUL_GetInfo(Nvram_InfoTypeDef *info)
{
    info->Interface  = "EMULATION";
    info->DevName    = "STM32L4_Flash_EMUL_EEPROM";
    info->DevChannel = 0;
    info->DevAddr    = 0;
    info->DataBit    = 32;
    info->DataVolume = NB_OF_VARIABLES;

    return NVRAM_OK;
}

NVRAM_STATUS Bsp_Nvram_Init()
{
    Nvram_Drv.Init    = EEP_EMUL_Init;
    Nvram_Drv.DeInit  = EEP_EMUL_DeInit;
    Nvram_Drv.Erase   = EEP_EMUL_Erase;
    Nvram_Drv.Write   = EEP_EMUL_Write;
    Nvram_Drv.Read    = EEP_EMUL_Read;
    Nvram_Drv.GetInfo = EEP_EMUL_GetInfo;

    if (Nvram_Drv.Init != NULL)
    {
        return Nvram_Drv.Init();
    }

    return NVRAM_OK;
}

NVRAM_STATUS Bsp_Nvram_DeInit()
{
    if (Nvram_Drv.DeInit != NULL)
    {
        Nvram_Drv.DeInit();
    }

    Nvram_Drv.Init    = NULL;
    Nvram_Drv.DeInit  = NULL;
    Nvram_Drv.Erase   = NULL;
    Nvram_Drv.Write   = NULL;
    Nvram_Drv.Read    = NULL;
    Nvram_Drv.GetInfo = NULL;

    return NVRAM_OK;
}
