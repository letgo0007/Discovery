C_INCLUDES += \
-Ilib/STM32_USB_Device_Library/Core/Inc \
-Ilib/STM32_USB_Device_Library/Class/CDC/Inc

C_SOURCES += \
lib/STM32_USB_Device_Library/Core/Src/usbd_core.c \
lib/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
lib/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
lib/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
