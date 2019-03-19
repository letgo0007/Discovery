C_INCLUDES += \
-Ilib/EEPROM_Emul/Core/ \
-Ilib/EEPROM_Emul/Porting/STM32L4

C_SOURCES += \
lib/EEPROM_Emul/Core/eeprom_emul.c \
lib/EEPROM_Emul/Porting/STM32L4/flash_interface.c