/*
  ******************************************************************************
  * @file    eeprom_driver.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef DRIVERS_STM32_EEPROM_DRIVER_H_
#define DRIVERS_STM32_EEPROM_DRIVER_H_


#include "UBI_SDK.h"
#include "stdint.h"
#include "stdbool.h"
#include "Drivers/base_memory_driver.h"



void init_eeprom_driver(base_memory_driver_struct** memory_driver);


#endif /* DRIVERS_STM32_EEPROM_DRIVER_H_ */
