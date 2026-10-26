/*
  ******************************************************************************
  * @file    flash_driver.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_FLASH_DRIVER_H_
#define INC_FLASH_DRIVER_H_

#include "UBI_SDK.h"

#ifdef UBI_SDK_TARGET_ESP32
#include "stdint.h"
#include "stdbool.h"
#include "esp_partition.h"


#define MIN_SECTOR_SIZE 					   0x1000U

typedef enum {
		FDSE_WRITE_OK = 0,
		FDSE_OWERWRITING_NOT_REQUERED,
		FDSE_DATA_CONTAINE_IN_FLASH_BUFFER,
		FDSE_FLASH_BUFFER_BUSY,
		FDSE_FLASH_WRITE_ERROR
	}flash_driver_status_enum;


uint32_t readFlash32(uint32_t Address);


uint16_t readFlash16(uint32_t Address);


uint8_t readFlash8(uint32_t Address);


void* getFlashPointer(uint32_t Address);


bool WriteBytesToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size);
bool WriteBytesToFlashPartition(esp_partition_t* partition, uint32_t AddressOffset, uint8_t* Buf, uint32_t Size);


void* SmartWriteToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status);
void* SmartWriteToFlashPartition(esp_partition_t* partition, void* image_pointer,void* pointer, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status);



bool compare_memory_val(uint32_t Address, uint8_t* val, uint32_t val_size);
bool compare_memory_val_cash(void* Pointer, uint8_t* val, uint32_t val_size);


bool memory_region_is_free(uint32_t Address, uint32_t Len);
bool memory_region_is_free_cash(void* Pointer, uint32_t Len);

void clear_flash_sector(uint32_t Address);

uint32_t getSectorNumber(uint32_t Address);

uint32_t getStartSectorAddress(uint32_t Address);

void finish_used_flash();

#endif

#endif /* INC_FLASH_DRIVER_H_ */
