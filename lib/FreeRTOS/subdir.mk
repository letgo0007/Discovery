C_INCLUDES += \
-Ilib/FreeRTOS/Source/include \
-Ilib/FreeRTOS/Source/portable/GCC/ARM_CM4F \
-Ilib/FreeRTOS/Source/CMSIS_RTOS

C_SOURCES += \
lib/FreeRTOS/Source/portable/MemMang/heap_4.c \
lib/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c \
lib/FreeRTOS/Source/list.c \
lib/FreeRTOS/Source/queue.c \
lib/FreeRTOS/Source/timers.c \
lib/FreeRTOS/Source/tasks.c \
lib/FreeRTOS/Source/event_groups.c \
lib/FreeRTOS/Source/croutine.c \
lib/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c
