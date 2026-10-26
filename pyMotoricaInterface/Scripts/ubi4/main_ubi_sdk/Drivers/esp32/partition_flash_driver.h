/*
  ******************************************************************************
  * @file    partition_flash_driver.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef DRIVERS_ESP32_PARTITION_FLASH_DRIVER_H_
#define DRIVERS_ESP32_PARTITION_FLASH_DRIVER_H_

#include "Drivers/base_memory_driver.h"

#include"UBI_SDK.h"

#ifdef UBI_SDK_FAMILY_ESP32

#include "esp_partition.h"


typedef struct
 {

	base_memory_driver_struct Base;
	esp_partition_t* partition;
	void (*clear_partition)(void* memory_driver);


 }partition_flash_driver_struct;

void init_partition_flash_driver(partition_flash_driver_struct** memory_driver);


#endif /* UBI_SDK_FAMILY_ESP32 */
#endif /* DRIVERS_ESP32_PARTITION_FLASH_DRIVER_H_ */
