#include "usb_logger.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"


extern QueueHandle_t xQueueUsbTx;

void UsbLogger_Task(void const *arguments)
{
    /* init code for USB_DEVICE */
    osDelay(100);
    printf("[%8ld]Init Start:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);
    MX_USB_DEVICE_Init();
    osDelay(10);
    printf("[%8ld]Init Finish:\t%s\t%s:%d\n", HAL_GetTick(), __FUNCTION__, __FILE__, __LINE__);

    /* Infinite loop */
    for (;;)
    {
        uint8_t *ptr[1] = {NULL};
        BaseType_t xResult = xQueueReceive(xQueueUsbTx, ptr, 10);

        if ((xResult == pdPASS) && (*ptr != NULL))
        {
            CDC_Transmit_FS(*ptr, strlen(*ptr));
        }

        osDelay(10);
    }
}
