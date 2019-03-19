C_INCLUDES += \
-Ilib/CMSIS/Device/ST/STM32L4xx/Include \
-Ilib/CMSIS/Include \
-Ilib/STM32L4xx_HAL_Driver/Inc \
-Ilib/STM32L4xx_HAL_Driver/Inc/Legacy

C_SOURCES += $(wildcard lib/STM32L4xx_HAL_Driver/Src/*.c)
