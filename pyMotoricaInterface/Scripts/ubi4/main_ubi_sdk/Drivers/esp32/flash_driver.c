/*
  ******************************************************************************
  * @file    flash_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/

//#include "stm32f4xx_hal.h"

#include "Drivers/esp32/flash_driver.h"

#ifdef UBI_SDK_TARGET_ESP32
//#include "Data/esp32/memory_struct.h"

#include <string.h> /* memset */


uint8_t flash_driver_ram_buf[MIN_SECTOR_SIZE];

typedef struct
{
	bool flash_sector_used;
	uint32_t StartSectorAddress;
	esp_partition_t* partition;
}ram_buffer_info_struct;


ram_buffer_info_struct ram_buffer_info =
		{
				.flash_sector_used = false,
				.StartSectorAddress = 0x00,
				.partition = 0
		};

uint32_t readFlash32(uint32_t Address)
{
	return *(uint32_t*)Address;
}

uint16_t readFlash16(uint32_t Address)
{
	return *(uint16_t*)Address;;
}


uint8_t readFlash8(uint32_t Address)
{
	return *(uint8_t*)Address;;
}

void* getFlashPointer(uint32_t Address)
{

	return (void*)Address;
}


bool WriteBytesToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size)
{
	/*
	HAL_FLASH_Unlock();
	for(uint32_t i = 0; i<Size; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address+i, Buf[i]) != HAL_OK)
			return false;
	}
	HAL_FLASH_Lock();
	*/
	return true;
}



bool WriteBytesToFlashPartition(esp_partition_t* partition, uint32_t AddressOffset, uint8_t* Buf, uint32_t Size)
{


	esp_partition_write(partition,AddressOffset,Buf,Size);

	return true;
}


void* SmartWriteToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status)
{

	/*
	if(ram_buffer_info.flash_sector_used)
	{
		uint32_t StartSectorAddress = getStartSectorAddress(Address);
		memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

		*status = FDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
		return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
	}else
	{

		if(memory_region_is_free(Address, Size))
		{
			HAL_FLASH_Unlock();
			for(uint32_t i = 0; i<Size; i++)
			{
				if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, Address+i, Buf[i]) != HAL_OK)
				{
					*status = FDSE_FLASH_WRITE_ERROR;
					return 0;
				}
			}
			HAL_FLASH_Lock();
			*status = FDSE_WRITE_OK;
			return getFlashPointer(Address);
		}else
		{
			if(compare_memory_val(Address,Buf,Size))
			{
				*status = FDSE_OWERWRITING_NOT_REQUERED;
				return getFlashPointer(Address);
			}else
			{
				if(!ram_buffer_info.flash_sector_used)
				{
					ram_buffer_info.flash_sector_used = true;
					memset(flash_driver_ram_buf,0xFFFFFFFF,MIN_SECTOR_SIZE);
					uint32_t StartSectorAddress = getStartSectorAddress(Address);
					ram_buffer_info.StartSectorAddress = StartSectorAddress;

					memcpy(flash_driver_ram_buf,getFlashPointer(StartSectorAddress),MIN_SECTOR_SIZE);
					memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

					*status = FDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
					return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
				}
			}
		}
	}
	*/
	return 0;
}


void* SmartWriteToFlashPartition(esp_partition_t* partition, void* image_pointer,void* pointer, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status)
{


	if(ram_buffer_info.flash_sector_used)
	{
		//uint32_t StartSectorAddress = getStartSectorAddress(Address);
		//memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

		*status = FDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
		//return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
		return pointer;
	}else
	{

		if(memory_region_is_free_cash(pointer, Size))
		{
			esp_partition_write(partition,(uint32_t)(pointer-image_pointer),Buf,Size);

			*status = FDSE_WRITE_OK;
			return pointer;
		}else
		{
			if(compare_memory_val_cash(pointer,Buf,Size))
			{
				*status = FDSE_OWERWRITING_NOT_REQUERED;
				return pointer;
			}else
			{
				if(!ram_buffer_info.flash_sector_used)
				{
					ram_buffer_info.flash_sector_used = true;
					memset(flash_driver_ram_buf,0xFFFFFFFF,MIN_SECTOR_SIZE);

					ram_buffer_info.StartSectorAddress = ((uint32_t)(pointer-image_pointer)/MIN_SECTOR_SIZE)*MIN_SECTOR_SIZE;

					ram_buffer_info.partition = partition;

					uint32_t offset = pointer- image_pointer;

					memcpy(flash_driver_ram_buf,image_pointer+ram_buffer_info.StartSectorAddress,MIN_SECTOR_SIZE);

					memcpy(&flash_driver_ram_buf[(uint32_t)(pointer- image_pointer)],Buf,Size);

					*status = FDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
					return (void*)&flash_driver_ram_buf[(uint32_t)(pointer- image_pointer)];

					return pointer;
				}
			}
		}
	}

	return 0;
}



void finish_used_flash()
{

	if(ram_buffer_info.flash_sector_used && ram_buffer_info.partition != 0)
	{

		uint32_t StartSectorAddress = (ram_buffer_info.StartSectorAddress/MIN_SECTOR_SIZE)*MIN_SECTOR_SIZE;

		esp_partition_erase_range(ram_buffer_info.partition,StartSectorAddress,MIN_SECTOR_SIZE);

		esp_partition_write(ram_buffer_info.partition,StartSectorAddress,flash_driver_ram_buf,MIN_SECTOR_SIZE);


		ram_buffer_info.flash_sector_used = false;
		ram_buffer_info.StartSectorAddress = 0;
		ram_buffer_info.partition = 0;
	}

}




void clear_flash_sector(uint32_t Address)
{
	/*
	uint32_t StartSectorNumber = getSectorNumber(Address);

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_config;

	erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_config.Sector = StartSectorNumber;
	erase_config.NbSectors = 1;
	erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	uint32_t SectorError = 0;
	HAL_FLASHEx_Erase(&erase_config, &SectorError);

	HAL_FLASH_Lock();
	*/

}

void ClearFlashSectorPartition(esp_partition_t* partition, uint32_t Address)
{

	uint32_t StartSectorAddress = (Address/MIN_SECTOR_SIZE)*MIN_SECTOR_SIZE;

	esp_partition_erase_range(partition,Address,MIN_SECTOR_SIZE);

}

/*
uint32_t getSectorNumber(uint32_t Address)
{
	return DATA_SECTOR;
}

uint32_t getStartSectorAddress(uint32_t Address)
{
	return DATA_START_ADDRESS;
}
*/

bool compare_memory_val(uint32_t Address, uint8_t* val, uint32_t val_size)
{
	for(uint32_t i = 0; i<val_size; i++)
	{
		if(*(uint8_t*)(Address+i) != *(val + i))
			return false;
	}
	return true;
}

bool compare_memory_val_cash(void* Pointer, uint8_t* val, uint32_t val_size)
{
	for(uint32_t i = 0; i<val_size; i++)
	{
		if(*(uint8_t*)(Pointer+i) != *(val + i))
			return false;
	}
	return true;
}

bool memory_region_is_free(uint32_t Address, uint32_t Len)
{
	for(uint32_t i = 0; i<Len; i++)
	{
		if(*(uint8_t*)(Address+i) != 0xFF)
			return false;
	}
	return true;
}

bool memory_region_is_free_cash(void* Pointer, uint32_t Len)
{
	for(uint32_t i = 0; i<Len; i++)
	{
		if(*(uint8_t*)(Pointer+i) != 0xFF)
			return false;
	}
	return true;
}

#endif
