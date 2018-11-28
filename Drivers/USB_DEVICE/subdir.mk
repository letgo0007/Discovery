C_INCLUDES += \
-IDrivers/USB_DEVICE

C_INCLUDES += \
-IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc \
-IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc

C_SOURCES += \
Drivers/USB_DEVICE/usb_device.c \
Drivers/USB_DEVICE/usbd_conf.c \
Drivers/USB_DEVICE/usbd_desc.c \
Drivers/USB_DEVICE/usbd_cdc_if.c \

C_SOURCES += \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
