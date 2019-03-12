C_INCLUDES += \
-IDrivers/CMSIS/Device/ST/STM32L4xx/Include \
-IDrivers/CMSIS/Include \
-IDrivers/STM32L4xx_HAL_Driver/Inc \
-IDrivers/STM32L4xx_HAL_Driver/Inc/Legacy

C_SOURCES += $(wildcard Drivers/STM32L4xx_HAL_Driver/Src/*.c)
