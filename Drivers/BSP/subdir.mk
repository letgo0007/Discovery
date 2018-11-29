#STM32L476_Discovery board component list
#Audio../ Components / cs43l22 / cs43l22.h
#Compass../ Components / lsm303c / lsm303c.h
#Gyro../ Components / l3gd20 / l3gd20.h
#MFX IDD../ Components / mfxstm32l152 / mfxstm32l152.h
#QSPI Flash../ Components / n25q128a / n25q128a.h

C_SOURCES += $(wildcard Drivers/BSP/STM32L476G-Discovery/*.c) \
Drivers/BSP/Components/cs43l22/cs43l22.c 	\
Drivers/BSP/Components/lsm303c/lsm303c.c 	\
Drivers/BSP/Components/l3gd20/l3gd20.c 		\
Drivers/BSP/Components/mfxstm32l152/mfxstm32l152.c

C_INCLUDES += \
-IDrivers/BSP/ \
-IDrivers/BSP/STM32L476G-Discovery/ 	\
-IDrivers/BSP/Components/Common/ 	\
-IDrivers/BSP/Components/cs43l22/	\
-IDrivers/BSP/Components/lsm303c/ 	\
-IDrivers/BSP/Components/l3gd20/ 	\
-IDrivers/BSP/Components/mfxstm32l152/	\
-IDrivers/BSP/Components/n25q128a/

C_SOURCES += \
Drivers/BSP/bsp_nvram.c
