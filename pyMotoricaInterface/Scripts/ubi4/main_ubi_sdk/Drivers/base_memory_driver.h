/*
 * base_memory_driver.h
 *
 *  Created on: Aug 4, 2023
 *      Author: PC
 */

#ifndef BASE_MEMORY_DRIVER_H_
#define BASE_MEMORY_DRIVER_H_

#include "stdint.h"
#include "stdbool.h"

#define ROUND_MEMORY_CELL_SIZE(_sizeof_, _base_) (_sizeof_%_base_ ? ((_sizeof_/_base_)*_base_+_base_) : (_sizeof_))

typedef enum {
		MDSE_OK = 0,
		MDSE_ERROR,
		MDSE_OWERWRITING_NOT_REQUERED,
		MDSE_DATA_CONTAINE_IN_RAM_BUFFER,
		MDSE_FLASH_BUFFER_BUSY

}memory_driver_status_enum;

typedef enum {
		MTE_INTERNAL_FLASH = 0,
		MTE_INTERNAL_EEPROM,
		MTE_INTERNAL_RAM,
		MTE_EXTERNAL_FLASH,
		MTE_EXTERNAL_EEPROM,
		MTE_EXTERNAL_RAM
}memory_type_enum;


typedef struct
{
	bool flash_sector_used;
	uint32_t StartSectorAddress;
	uint32_t ram_buffer_size;
	uint8_t* flash_driver_ram_buf;
}ram_buffer_info_struct;

/*
typedef struct
{

}memory_feature_struct;
*/

typedef struct
 {

	memory_type_enum memory_type;
	bool (*write_bytes)(void* memory_driver,uint32_t Address, uint8_t* Buf, uint32_t Size);
	void* (*smart_write)(void* memory_driver,uint32_t Address, uint8_t* Buf, uint32_t Size,memory_driver_status_enum* status);

	bool (*compare_memory_val)(void* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
	bool (*memory_region_is_free)(void* memory_driver,uint32_t Address, uint32_t Len);

	void (*clear_sector)(void* memory_driver, uint32_t Address);
	void (*clear_sgment)(void* memory_driver, uint32_t Address, uint32_t Len);


	void (*finish_used_flash)(void* memory_driver);

	void* (*get_pointer)(void* memory_driver, uint32_t Address, int size, memory_driver_status_enum* status);
	void  (*readData)(void* memory_driver, uint32_t Address, uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status);
	uint32_t (*read32)(void* memory_driver, uint32_t Address);
	uint16_t (*read16)(void* memory_driver, uint32_t Address);
	uint8_t (*read8)(void* memory_driver, uint32_t Address);

	ram_buffer_info_struct RAM_Buffer;



 }base_memory_driver_struct;



#endif /* DATA_BASE_MEMORY_DRIVER_H_ */
