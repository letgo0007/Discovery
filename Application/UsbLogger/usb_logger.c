#include "usb_logger.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

void UsbLogger_Task(void const *arguments)
{
    /* init code for USB_DEVICE */
    osDelay(100);
    MX_USB_DEVICE_Init();
    osDelay(10);

    /* Infinite loop */
    for (;;)
    {
        CDC_Transmit_FS((uint8_t *)"Hello World\n", 12);
        osDelay(1000);
        // printf("Heart Beat!\r\n");
    }
}
