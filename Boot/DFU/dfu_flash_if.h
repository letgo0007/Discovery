/*
 * dfu_flash_if.h
 *
 *  Created on: Dec 15, 2018
 *      Author: nickyang
 */

#ifndef DFU_FLASH_IF_H_
#define DFU_FLASH_IF_H_

#include "stm32l4xx_hal.h"

/*!@defgroup  FLASH Address define.
 *
 * STM32L476 1 MB Dual bank organization, <RM0351> page97
 *
 * ---Main Memory---
 * Bank 1   0x08000000 - 0x0807FFFF     512K    Flash Memory (2K x 256)
 * Bank 2   0x08080000 - 0x080FFFFF     512K    Flash Memory (2K x 256)
 *
 * ---Information Memory---
 * Bank 1   0x1FFF0000 - 0x1FFF6FFF     28 K    System memory
 * Bank 1   0x1FFF7000 - 0x1FFF73FF     1  K    OTP area
 * Bank 1   0x1FFF7800 - 0x1FFF780F     16 B    Option bytes
 *
 * Bank 2   0x1FFF8000 - 0x1FFFEFFF     28 K    System memory
 * Bank 2   0x1FFFF800 - 0x1FFFF80F     16 B    Option bytes
 */

#define FLASH_BANK_ADDR_CURRENT         FLASH_BASE
#define FLASH_BANK_ADDR_NEXT            FLASH_BASE + FLASH_BANK_SIZE

#define SYSTEM_MEMORY_ADDR_BANK1        0x1FFF0000
#define SYSTEM_MEMORY_ADDR_BANK2        0x1FFF8000

#define OPTION_BYTE_ADDR_BANK1          0x1FFF7800
#define OPTION_BYTE_ADDR_BANK2          0x1FFFF800

#define FLASH_OTP_ADDR                  0x1FFF7000
#define FLASH_OTP_DATABYTE              8
#define FLASH_OTP_SIZE                  1024

uint32_t Flash_setActiveBank(uint32_t flashbank);
uint32_t Flash_getActiveBank();
uint32_t Flash_getPage(uint32_t address);
uint32_t Flash_getBank(uint32_t address);
uint32_t Flash_getAddress(uint32_t bank, uint32_t page);
uint32_t Flash_checkBankUsage(uint32_t bank);
uint32_t Flash_checkPageUsage(uint32_t bank, uint8_t page);
uint32_t Flash_erasePage(uint32_t bank, uint32_t start_page, uint32_t num_of_page);
uint32_t Flash_eraseBank(uint32_t bank);
uint32_t Flash_cmpPage(uint32_t SrcBank, uint32_t SrcPage, uint32_t DstBank, uint32_t DstPage);
uint32_t Flash_copyPage(uint32_t SrcBank, uint32_t SrcPage, uint32_t DstBank, uint32_t DstPage);
uint32_t Flash_copyBank(uint32_t SrcBank, uint32_t DstBank);
uint32_t Flash_program_8bit(uint32_t addr, uint8_t *ptr, uint32_t len);
uint32_t Flash_Otp_write(uint16_t idx, uint64_t value);
uint32_t Flash_Otp_read(uint16_t idx, uint64_t *value);

#endif /* DFU_FLASH_IF_H_ */
