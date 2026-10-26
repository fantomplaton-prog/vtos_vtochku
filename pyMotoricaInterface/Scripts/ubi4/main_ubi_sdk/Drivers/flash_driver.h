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

#ifdef UBI_SDK_MANUFACTURER_NORDIC

#include "nrf_nvmc.h"
#include "nordic_common.h"
#include "nrf_fstorage_sd.h"
#include "nrf_fstorage_nvmc.h"
#include "Drivers/base_memory_driver.h"



#ifdef UBI_SDK_TARGET_NRF52832
#define FLASH_START_ADDR							0x00U
#define FLASH_END_ADDR								0x077000U
#define MIN_SECTOR_SIZE 					   	0x1000U
#endif




#define ROUND_MEMORY_CELL_SIZE(_sizeof_, _base_) (_sizeof_%_base_ ? ((_sizeof_/_base_)*_base_+_base_) : (_sizeof_))

typedef enum {
		FDSE_WRITE_OK = 0,
		FDSE_OWERWRITING_NOT_REQUERED,
		FDSE_DATA_CONTAINE_IN_FLASH_BUFFER,
		FDSE_FLASH_BUFFER_BUSY,
		FDSE_FLASH_WRITE_ERROR
	}flash_driver_status_enum;

	
	
void init_flash_driver(base_memory_driver_struct** memory_driver);
	

uint32_t readFlash32(uint32_t Address);


uint16_t readFlash16(uint32_t Address);


uint8_t readFlash8(uint32_t Address);


void* getFlashPointer(uint32_t Address);


bool WriteBytesToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size);

void* SmartWriteToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status);



bool compare_memory_val(uint32_t Address, uint8_t* val, uint32_t val_size);


bool memory_region_is_free(uint32_t Address, uint32_t Len);

void clear_flash_sector(uint32_t Address);

uint32_t getSectorNumber(uint32_t Address);

uint32_t getStartSectorAddress(uint32_t Address);

void finish_used_flash();

#endif /* UBI_SDK_MANUFACTURER_NORDIC*/

#ifdef __cplusplus
}
#endif

#endif /* INC_FLASH_DRIVER_H_ */

