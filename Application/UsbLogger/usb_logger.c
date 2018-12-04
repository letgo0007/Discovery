#include "usb_logger.h"
#include "FreeRTOS.h"
#include "cli.h"
#include "cmsis_os.h"
#include "main.h"
#include "stdio.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

extern osMessageQId UsbTxQueue_Handle;
extern uint8_t UserRxBufferFS[];

void UsbLogger_Task(void const *arguments)
{
    /* init code for USB_DEVICE */
    osDelay(100);
    CLI_INFO("%s: Initialize Start\n", __FUNCTION__);
    MX_USB_DEVICE_Init();

    osDelay(10);
    CLI_INFO("%s: Initialize Finish\n", __FUNCTION__);

    /* Infinite loop */
    for (;;)
    {
        char *text = "Hello World\n";

        osMessagePut(UsbTxQueue_Handle, text, 100);
        osEvent event = osMessageGet(UsbTxQueue_Handle, 100);

        if (event.value.p != NULL)
        {
            //CDC_Transmit_FS(event.value.p, 13);
        }

        int len = 0;

        if(strlen(UserRxBufferFS) != 0)
        {
            //printf("%s\n",UserRxBufferFS);
        }


        osDelay(10);
    }
}
