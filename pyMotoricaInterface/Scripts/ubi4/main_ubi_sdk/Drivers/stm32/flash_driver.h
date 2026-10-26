/*
  ******************************************************************************
  * @file    flash_driver.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_FLASH_DRIVER_H_
#define INC_FLASH_DRIVER_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "UBI_SDK.h"

#include "Drivers/base_memory_driver.h"


#ifdef UBI_SDK_TARGET_STM32L1
#define MIN_SECTOR_SIZE 					   0x100U
#endif


#ifdef UBI_SDK_TARGET_STM32F4
#define MIN_SECTOR_SIZE 					   0x4000U
#endif

#ifdef UBI_SDK_TARGET_STM32G0
#define MIN_SECTOR_SIZE 					   0x800U
#endif

#ifdef UBI_SDK_TARGET_STM32G4
#define MIN_SECTOR_SIZE                        0x800U
#endif



 void init_flash_driver(base_memory_driver_struct** memory_driver);



 /*
typedef enum {
		FDSE_WRITE_OK = 0,
		FDSE_OWERWRITING_NOT_REQUERED,
		FDSE_DATA_CONTAINE_IN_FLASH_BUFFER,
		FDSE_FLASH_BUFFER_BUSY,
		FDSE_FLASH_WRITE_ERROR
	}flash_driver_status_enum;




void clear_flash_sector(uint32_t Address);

uint32_t getSectorNumber(uint32_t Address);

uint32_t getStartSectorAddress(uint32_t Address);

void finish_used_flash();
*/

#ifdef __cplusplus
}
#endif

#endif /* INC_FLASH_DRIVER_H_ */
