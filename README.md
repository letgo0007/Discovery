# Discovery
A discovery Programm based on STM32L476 discovery board.

# Source Structure

## Application
- Define task thread of different functions
- Can depend on  : `Board` / `Drivers` / `Lib`  
- Avoid depend on: Other `Applicaitons`. 
  - If 2x appilications have to run together, should be clearly noted and check dependency before running.
  - If syncornize & message is needed between applications, suggest use MESSAGE / MUTEX / SEMAPHORES provided by RTOS.
    
## Board
- Setup the board to basic work state
  - Clock, debug UART
- Define board resources
  - GPIO, I2C, UART define
- Can depend on  : `Drivers` / `Lib` 
- Avoid depend on: `Application`

## Drivers
- Devices drivers
  - ADC, EEPROM, etc..
- Can Depend on  : `Drivers` / `Lib` 
- Avoid depend on: `Application` / `Board`
  - Let board register device handler
  - Let board register IO functions for drivers
  - Do not hard code GPIO number, etc

## Project
- Project target main function
- Select application/board
- Can Depend on  : Everything

## Lib
- Liberies from vendor or 3rd party
- Can Depend on  : Should be self contained.
  - External config header file path should be handled by `Project`