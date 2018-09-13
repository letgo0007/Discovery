C_INCLUDES += \
-IDrivers/EEPROM_Emul/Core/ \
-IDrivers/EEPROM_Emul/Porting/STM32L4

C_SOURCES += \
Drivers/EEPROM_Emul/Core/eeprom_emul.c \
Drivers/EEPROM_Emul/Porting/STM32L4/flash_interface.c