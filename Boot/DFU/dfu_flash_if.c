/******************************************************************************
 * @file    hal_flash.c
 * @brief   Hardware Abstract Layer for STM32L4 internal Flash.
 *
 *          Supplementary to standard STM32L4xx_HAL_Driver, provide more usable
 *          Interface to Dual-Bank flash application.
 *
 *          Provide API for some functions ST Driver does not provide.
 *          - Dual Bank switch
 *          - Dual Bank address / bank / page convert.
 *          - Flash bank/page usage check.
 *          - Flash bank/page erase/copy/compare.
 *
 * @author  Nick Yang
 * @date    2018/08/12
 * @version V0.2
 *****************************************************************************/

#include "dfu_flash_if.h"
#include "stm32l4xx_hal.h"

#define HWREG64(x) (*((volatile uint64_t *)((uint32_t)x)))
#define HWREG32(x) (*((volatile uint32_t *)((uint32_t)x)))
#define HWREG16(x) (*((volatile uint16_t *)((uint32_t)x)))
#define HWREG8(x) (*((volatile uint8_t *)((uint32_t)x)))

#define IS_SRAM1_ADDRESS(ADDRESS)                                                                  \
    ((ADDRESS >= SRAM1_BASE) && (ADDRESS <= SRAM1_BASE + SRAM1_SIZE_MAX))
#define IS_SRAM2_ADDRESS(ADDRESS) ((ADDRESS >= SRAM2_BASE) && (ADDRESS <= SRAM2_BASE + SRAM2_SIZE))
#define IS_SRAM_ADDRESS(ADDRESS) (IS_SRAM1_ADDRESS(ADDRESS) || IS_SRAM2_ADDRESS(ADDRESS))

/*!@brief   Set active Flash bank to boot.
 *
 * @param   bank    FLASH_BANK_1 or FLASH_BANK_2, Refer to @defgroup FLASH_Banks FLASH Banks
 * @return          [0]     Success
 *                  [-1]    Failure. Target Flash bank dont have valid reset vector.
 */
uint32_t Flash_setActiveBank(uint32_t bank)
{
    // Check Parameter
    assert_param(IS_FLASH_BANK(bank));

    // Check if target bank has valid reset vector & stack.
    //.word   _estack           -> End of stack, must be in the SRAM
    //.word   Reset_Handler     -> Reset Vector, must be in the FLASH
    uint32_t estack_addr = Flash_getAddress(bank, 0);
    uint32_t reset_addr  = estack_addr + 4;

    assert_param(IS_SRAM_ADDRESS(HWREG32(estack_addr)));
    assert_param(IS_FLASH_MAIN_MEM_ADDRESS(HWREG32(reset_addr)));

    if (IS_SRAM_ADDRESS(HWREG32(estack_addr)) && IS_FLASH_MAIN_MEM_ADDRESS(HWREG32(reset_addr)))
    {
        /* Allow Access to Flash control registers and user Flash */
        FLASH_OBProgramInitTypeDef OBInit;
        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
        HAL_FLASH_OB_Unlock();

        /* Get the Dual boot configuration status */
        HAL_FLASHEx_OBGetConfig(&OBInit);

        /* Enable/Disable dual boot feature */
        switch (bank)
        {
        case FLASH_BANK_1:
        {
            OBInit.OptionType = OPTIONBYTE_USER; //!< USER option byte configuration
            OBInit.USERType   = OB_USER_BFB2;    //!< Dual-bank boot
            OBInit.USERConfig = OB_BFB2_DISABLE; //!< Dual-bank boot disable
            break;
        }
        case FLASH_BANK_2:
        {
            OBInit.OptionType = OPTIONBYTE_USER; //!< USER option byte configuration
            OBInit.USERType   = OB_USER_BFB2;    //!< Dual-bank boot
            OBInit.USERConfig = OB_BFB2_ENABLE;  //!< Dual-bank boot enable
            break;
        }
        default:
        {
            printf("\e[31mERROR: Flash bank [%d] not supported.\e[0m\n", (uint8_t)bank);
            return -1;
        }
        }

        /* Start the Option Bytes programming process */
        HAL_FLASHEx_OBProgram(&OBInit);
        HAL_FLASH_OB_Launch();
    }
    else
    {
        printf("\e[31mERROR: Invalid Vector on Flash bank [%d]! EndStack=[0x%08lX], "
               "ResetVector=[0x%08lX]\e[0m\n",
               (uint8_t)bank, HWREG32(estack_addr), HWREG32(reset_addr));
        return -1;
    }

    return 0;
}
/*!@brief   Get current working flash bank.
 *
 * @return  FLASH_BANK_1    Current is working on Flash bank 1
 *          FLASH_BANK_2    Current is working on Flash bank 2
 */
uint32_t Flash_getActiveBank(void)
{
    // Check if Memory Re-Map function is enabled.
    if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0U)
    {
        return FLASH_BANK_1;
    }
    else
    {
        return FLASH_BANK_2;
    }
}

/*!@brief Get flash page number from Address.
 *
 * @param address
 * @return 0~255
 */
uint32_t Flash_getPageNum(uint32_t address)
{
    // Check valid Flash address
    assert_param(IS_FLASH_PROGRAM_ADDRESS(address));

    return (address < (FLASH_BASE + FLASH_BANK_SIZE))
               ? (address - FLASH_BASE) / FLASH_PAGE_SIZE
               : (address - FLASH_BASE - FLASH_BANK_SIZE) / FLASH_PAGE_SIZE;
}

/*!@brief Get flash bank from Address.
 *
 * @param address
 * @return FLASH_BANK_1 or FLASH_BANK_2
 */
uint32_t Flash_getBankNum(uint32_t address)
{
    // Check valid Flash address
    assert_param(IS_FLASH_PROGRAM_ADDRESS(address));

    if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0U)
    {
        return (address < (FLASH_BASE + FLASH_BANK_SIZE)) ? FLASH_BANK_1 : FLASH_BANK_2;
    }
    else
    {
        return (address < (FLASH_BASE + FLASH_BANK_SIZE)) ? FLASH_BANK_2 : FLASH_BANK_1;
    }
}

/*!@brief Get Flash address from given flash bank & page
 *
 * @param bank      FLASH_BANK_1 or FLASH_BANK_2
 * @param page      [0~255] Page number in the bank
 * @return          [0x8000000~0x80FFFFF] Flash address
 */
uint32_t Flash_getAddress(uint32_t bank, uint32_t page)
{
    // Check valid flash bank and page
    assert_param(IS_FLASH_BANK_EXCLUSIVE(bank));
    assert_param(IS_FLASH_PAGE(page));

    uint32_t address = -1;

    if (READ_BIT(SYSCFG->MEMRMP, SYSCFG_MEMRMP_FB_MODE) == 0U)
    {
        // No Bank Swap case, BNAK1 @ 0x8000000, BANK2 @ 0x8080000
        if (bank == FLASH_BANK_1)
        {
            address = FLASH_BASE + FLASH_PAGE_SIZE * page;
        }
        else
        {
            address = FLASH_BASE + FLASH_BANK_SIZE + FLASH_PAGE_SIZE * page;
        }
    }
    else
    {
        // Bank Swap case, BNAK2 @ 0x8000000, BANK1 @ 0x8080000
        if (bank == FLASH_BANK_2)
        {
            address = FLASH_BASE + FLASH_PAGE_SIZE * page;
        }
        else
        {
            address = FLASH_BASE + FLASH_BANK_SIZE + FLASH_PAGE_SIZE * page;
        }
    }

    return address;
}

/*!@brief   Check if a flash page is used.
 *
 * @param   bank    @def FLASH_BANK_1 or @def FLASH_BANK_2
 * @param   page    [0~255] Page number in the flash bank
 * @return          [0~2048] Number of bytes is used in a flash bank.
 */
uint32_t Flash_checkPageUsage(uint32_t bank, uint8_t page)
{
    // Check Parameters
    assert_param(IS_FLASH_BANK_EXCLUSIVE(bank));
    assert_param(IS_FLASH_PAGE(page));

    // Calculate flash address
    uint32_t StartAddr = Flash_getAddress(bank, page);
    uint32_t UsedCount = 0;

    // Check page content and return used byte count.
    for (int i = 0; i < FLASH_PAGE_SIZE; i = i + 4)
    {
        if (HWREG32(StartAddr + i) != 0xFFFFFFFF)
        {
            UsedCount += 4;
        }
    }

    return UsedCount;
}

/*!@brief   Check how many page in a Flash bank is used.
 *
 * @param   bank    @def FLASH_BANK_1 or @def FLASH_BANK_2
 * @return          [0~255]Number of page used
 */
uint32_t Flash_checkBankUsage(uint32_t bank)
{
    // Check Parameter
    assert_param(IS_FLASH_BANK_EXCLUSIVE(bank));

    // Check all page in a Bank
    uint32_t TotalPage = FLASH_BANK_SIZE / FLASH_PAGE_SIZE;
    uint32_t UsedCount = 0;

    for (int i = 0; i < TotalPage; i++)
    {
        if (Flash_checkPageUsage(bank, i) == 0) // Used page
        {
            UsedCount += 1;
        }
    }

    return UsedCount;
}

/*!@brief Erase selected Flash pages.
 *
 * @param bank          FLASH_BANK_1 or FLASH_BANK_2
 * @param start_page    [0~255] Page to start erase.
 * @param num_of_page   [0~255] Number of Pages to erase.
 * @return
 */
uint32_t Flash_erasePage(uint32_t bank, uint32_t start_page, uint32_t num_of_page)
{
    // Check Parameters
    assert_param(IS_FLASH_BANK_EXCLUSIVE(bank));
    assert_param(IS_FLASH_PAGE(start_page));
    assert_param(IS_FLASH_PAGE(start_page + num_of_page));

    FLASH_EraseInitTypeDef erase_param = {0};
    HAL_StatusTypeDef      ret         = HAL_OK;
    uint32_t               page_error  = 0;
    erase_param.TypeErase              = FLASH_TYPEERASE_PAGES;
    erase_param.Banks                  = bank;
    erase_param.Page                   = start_page;
    erase_param.NbPages                = num_of_page;

    HAL_FLASH_Unlock();
    ret = HAL_FLASHEx_Erase(&erase_param, &page_error);
    HAL_FLASH_Lock();

    if (ret != HAL_OK)
    {
        uint32_t usage = Flash_checkPageUsage(bank, start_page);
        printf("Flash_erasePage, status = [%d], error_page = [%lX], usage = [%ld] \n", ret,
               page_error, usage);
    }

    return ret;
}

/*!@brief Mass erase on an entire Flash bank
 *
 * @param bank  FLASH_BANK_1 or FLASH_BANK_2
 * @return
 */
uint32_t Flash_eraseBank(uint32_t bank)
{
    // Check Parameters
    assert_param(IS_FLASH_BANK_EXCLUSIVE(bank));

    // Set up Mass Erase parameters
    FLASH_EraseInitTypeDef erase_param = {0};
    HAL_StatusTypeDef      ret         = HAL_OK;
    uint32_t               page_error  = 0;
    erase_param.TypeErase              = FLASH_TYPEERASE_MASSERASE;
    erase_param.Banks                  = bank;
    erase_param.Page                   = 0;
    erase_param.NbPages                = 256;

    // Do Mass erase
    HAL_FLASH_Unlock();
    ret = HAL_FLASHEx_Erase(&erase_param, &page_error);
    HAL_FLASH_Lock();

    printf("Flash_eraseBank [%ld], ret =[%d], page_error=[0x%lX]\n", bank, ret, page_error);

    return ret;
}

/*!@brief Compare 2x flash page content and return with difference count.
 *
 * @param SrcBank   FLASH_BANK_1 or FLASH_BANK_2, Source Flash bank
 * @param SrcPage   [0~255], Source Flash page
 * @param DstBank   FLASH_BANK_1 or FLASH_BANK_2, Destination Flash Bank
 * @param DstPage   [0~255], Destination Flash page
 * @return          [0~2048] Number of bytes that is different on 2x Flash pages.
 */
uint32_t Flash_cmpPage(uint32_t SrcBank, uint32_t SrcPage, uint32_t DstBank, uint32_t DstPage)
{
    // Check Parameters
    assert_param(IS_FLASH_BANK_EXCLUSIVE(SrcBank));
    assert_param(IS_FLASH_BANK_EXCLUSIVE(DstBank));
    assert_param(IS_FLASH_PAGE(SrcPage));
    assert_param(IS_FLASH_PAGE(DstPage));

    // Compare Page content
    uint32_t SrcAddr   = Flash_getAddress(SrcBank, SrcPage);
    uint32_t DstAddr   = Flash_getAddress(DstBank, DstPage);
    uint32_t DiffCount = 0;

    // Check page content and return difference count.
    for (int i = 0; i < FLASH_PAGE_SIZE; i = i + 4)
    {
        if (HWREG32(SrcAddr + i) != HWREG32(DstAddr + i))
        {
            DiffCount += 4;
        }
    }

    return DiffCount;
}

/*!@brief Copy a flash page content and another.
 *
 * @param SrcBank   FLASH_BANK_1 or FLASH_BANK_2, Source Flash bank
 * @param SrcPage   [0~255], Source Flash page
 * @param DstBank   FLASH_BANK_1 or FLASH_BANK_2, Destination Flash Bank
 * @param DstPage   [0~255], Destination Flash page
 * @return
 */
uint32_t Flash_copyPage(uint32_t SrcBank, uint32_t SrcPage, uint32_t DstBank, uint32_t DstPage)
{
    // Check Parameters
    assert_param(IS_FLASH_BANK_EXCLUSIVE(SrcBank));
    assert_param(IS_FLASH_BANK_EXCLUSIVE(DstBank));
    assert_param(IS_FLASH_PAGE(SrcPage));
    assert_param(IS_FLASH_PAGE(DstPage));

    // Get Address
    uint32_t SrcAddr = Flash_getAddress(SrcBank, SrcPage);
    uint32_t DstAddr = Flash_getAddress(DstBank, DstPage);
    uint32_t Ret     = 0;

    // Erase Destination Page
    HAL_FLASH_Unlock();
    Flash_erasePage(DstBank, DstPage, 1);
    HAL_FLASH_Lock();

    // Write Destination Page
    HAL_FLASH_Unlock();
    for (int i = 0; i < FLASH_PAGE_SIZE; i = i + 8)
    {
        uint64_t data = HWREG64(SrcAddr + i);
        Ret           = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, DstAddr + i, data);

        if (Ret != HAL_OK)
        {
            printf("\e[31mERROR: Flash write fail @ [0x%lX]\n\e[0m", DstAddr + i);
            HAL_FLASH_Lock();
            return Ret;
        }
    }

    HAL_FLASH_Lock();
    return Ret;
}

/*!@brief Copy a bank to another.
 *
 * @param SrcBank   FLASH_BANK_1 or FLASH_BANK_2, Source bank
 * @param DstBank   FLASH_BANK_1 or FLASH_BANK_2, Destination bank
 * @return
 */
uint32_t Flash_copyBank(uint32_t SrcBank, uint32_t DstBank)
{
    uint16_t page = 0;
    for (page = 0; page < FLASH_BANK_SIZE / FLASH_PAGE_SIZE; page++)
    {
        // Check if a flash page is used.
        uint32_t PageUsage = Flash_checkPageUsage(SrcBank, page);

        if (PageUsage > 0)
        {
            Flash_erasePage(DstBank, page, 1);
            Flash_copyPage(SrcBank, page, DstBank, page);
        }
    }
    return HAL_OK;
}

uint32_t Flash_program_8bit(uint32_t addr, uint8_t *pu8, uint32_t len)
{
    // b*Dfu_runHexLine
    HAL_FLASH_Unlock();

    for (int i = 0; i < len; i = i + 8)
    {
        // Convert 8x U8 to 1x U64
        uint64_t temp = 0;
        for (int j = 7; j >= 0; j--)
        {
            temp = (temp << 8) + pu8[i + j];
        }

        // Write Flash
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, temp);
    }

    HAL_FLASH_Lock();

    return 0;
}

uint32_t Flash_Otp_write(uint16_t idx, uint64_t value)
{
    /* Calculate otp address */
    uint32_t otp_addr = FLASH_OTP_ADDR + idx * sizeof(value);
    assert_param(IS_FLASH_OTP_ADDRESS(otp_addr));

    /* Unlock FLASH */
    HAL_FLASH_Unlock();

    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, otp_addr, value);

    /* Lock FLASH */
    HAL_FLASH_Lock();

    return status;
}

uint32_t Flash_Otp_read(uint16_t idx, uint64_t *value)
{
    /* Calculate otp address */
    uint32_t otp_addr = FLASH_OTP_ADDR + idx * sizeof(*value);
    assert_param(IS_FLASH_OTP_ADDRESS(otp_addr));

    *value = HWREG64(otp_addr);

    return 0;
}
