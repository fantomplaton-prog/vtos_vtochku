/*
  ******************************************************************************
  * @file    flash_driver.c
  * @authors A.Alborov, A.Ovodov
  ******************************************************************************
*/

#include "HAL_DRIVERS.h"

#include "Drivers/stm32/flash_driver.h"

#ifdef UBI_SDK_TARGET_STM32L1

#include "memory_struct.h"
#include <string.h> /* memset */
#include "stdlib.h"

uint8_t flash_driver_ram_buf[MIN_SECTOR_SIZE];

/*
typedef struct
{
	bool flash_sector_used;
	uint32_t StartSectorAddress;
}ram_buffer_info_struct_;

ram_buffer_info_struct_ ram_buffer_info =
{
		.flash_sector_used = false,
		.StartSectorAddress = 0x00
};
*/

static uint32_t readFlash32(base_memory_driver_struct* memory_driver, uint32_t Address);
static uint16_t readFlash16(base_memory_driver_struct* memory_driver, uint32_t Address);
static uint8_t readFlash8(base_memory_driver_struct* memory_driver, uint32_t Address);
void  readData(base_memory_driver_struct* memory_driver, uint32_t Address,
                uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status);

static void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address);

static bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size);
static void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status);

static bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
static bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len);


static void finish_used_flash(base_memory_driver_struct* memory_driver);
static void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address);

static uint32_t getSectorNumber(uint32_t Address);
static uint32_t getStartSectorAddress(uint32_t Address);


void init_flash_driver(base_memory_driver_struct** memory_driver)
{
	base_memory_driver_struct* t_memory_driver = malloc(sizeof(base_memory_driver_struct));
    t_memory_driver->RAM_Buffer.flash_sector_used = false;
    t_memory_driver->RAM_Buffer.ram_buffer_size = MIN_SECTOR_SIZE;
    t_memory_driver->RAM_Buffer.StartSectorAddress = 0;
    t_memory_driver->RAM_Buffer.flash_driver_ram_buf = flash_driver_ram_buf;

	t_memory_driver->memory_type = MTE_INTERNAL_FLASH;

	t_memory_driver->get_pointer = getFlashPointer;
	t_memory_driver->read8 = readFlash8;
	t_memory_driver->read16 = readFlash16;
	t_memory_driver->read32 = readFlash32;
	t_memory_driver->finish_used_flash = finish_used_flash;

	t_memory_driver->compare_memory_val = compare_memory_val;
	t_memory_driver->memory_region_is_free = memory_region_is_free;

	t_memory_driver->readData = readData;
	t_memory_driver->write_bytes = WriteBytesToFlash;
	t_memory_driver->smart_write = SmartWriteToFlash;

	t_memory_driver->clear_sector = clear_flash_sector;


	*memory_driver = t_memory_driver;
}


static uint64_t readFlas64(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint64_t*)Address;
}

static uint32_t readFlash32(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint32_t*)Address;
}

static uint16_t readFlash16(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint16_t*)Address;;
}


static uint8_t readFlash8(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint8_t*)Address;;
}

void  readData(base_memory_driver_struct* memory_driver, uint32_t Address,
                uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status)
{

    if(!(Address - memory_driver->RAM_Buffer.StartSectorAddress < MIN_SECTOR_SIZE))
    {
        memcpy(readBuffer, getFlashPointer(memory_driver, Address), len);
    } else
    {
        memcpy(readBuffer, memory_driver->RAM_Buffer.flash_driver_ram_buf + Address, len);
    }
    *status = MDSE_OK;
}


static void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address)
{

	return (void*)Address;
}


static bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size)
{
	uint32_t tData = 0xFFFFFFFF;
	uint32_t shift = 0;

	HAL_FLASH_Unlock();

	for(uint32_t i = 0; i<Size/4; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address+i*4, *(uint32_t*)&Buf[i*4]) != HAL_OK)
			return false;
	}

	if(Size%4)
	{
		shift = (Size/4)*4;

	 tData = 0xFFFFFFFFFFFFFFFF;

		memcpy((uint8_t*)&tData,Buf+shift,Size-((Size/4)*4));


		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address+shift, tData) != HAL_OK)
					return false;
	}


	HAL_FLASH_Lock();

	return true;
}

static void WriteBytesToVirtualPage(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size,memory_driver_status_enum* status)
{

}


static void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status)
{

/*
	if(ram_buffer_info.flash_sector_used)
	{
		uint32_t StartSectorAddress = getStartSectorAddress(Address);
		memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

		//*status = MDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
		return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
	}else
	{

		if(memory_region_is_free(memory_driver, Address, Size))
		{

			if(!WriteBytesToFlash(memory_driver, Address,Buf,Size))
			{
				//*status = MDSE_FLASH_WRITE_ERROR;
				return 0;
			}

			//*status = MDSE_WRITE_OK;
			return getFlashPointer(memory_driver, Address);
		}else
		{
			if(compare_memory_val(memory_driver, Address,Buf,Size))
			{
				*status = MDSE_OWERWRITING_NOT_REQUERED;
				return getFlashPointer(memory_driver, Address);
			}else
			{
				if(!ram_buffer_info.flash_sector_used)
				{
					ram_buffer_info.flash_sector_used = true;
					memset(flash_driver_ram_buf,0xFFFFFFFF,MIN_SECTOR_SIZE);
					uint32_t StartSectorAddress = getStartSectorAddress(Address);
					ram_buffer_info.StartSectorAddress = StartSectorAddress;

					memcpy(flash_driver_ram_buf,getFlashPointer(memory_driver, StartSectorAddress),MIN_SECTOR_SIZE);
					memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

					//*status = MDSE_DATA_CONTAINE_IN_FLASH_BUFFER;
					return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
				}
			}
		}
	}
 */
	return 0;
}

static void finish_used_flash(base_memory_driver_struct* memory_driver)
{
/*	if(ram_buffer_info.flash_sector_used)
	{
		clear_flash_sector(memory_driver, ram_buffer_info.StartSectorAddress);


		WriteBytesToFlash(memory_driver, ram_buffer_info.StartSectorAddress,flash_driver_ram_buf,MIN_SECTOR_SIZE);


		ram_buffer_info.flash_sector_used = false;
	}
*/
}




static void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	//uint32_t StartSectorNumber = getSectorNumber(Address);

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_config;

	erase_config.TypeErase = FLASH_TYPEERASE_PAGES;
	erase_config.PageAddress = Address;
	erase_config.NbPages = 1;

	uint32_t SectorError = 0;
	HAL_FLASHEx_Erase(&erase_config, &SectorError);

	HAL_FLASH_Lock();

}

static uint32_t getSectorNumber(uint32_t Address)
{
	return ((Address - DATA_START_ADDRESS) / DATA_SIZE) + 511;
}

static uint32_t getStartSectorAddress(uint32_t Address)
{
	return DATA_START_ADDRESS;
}

static bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size)
{
	for(uint32_t i = 0; i<val_size; i++)
	{
		if(*(uint8_t*)(Address+i) != *(val + i))
			return false;
	}
	return true;
}

static bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len)
{

	uint32_t StartAddress = Address;
	uint32_t ResultLen = Len;
	uint32_t ShiftBack = 0;
	if(Address%8)
	{
		StartAddress = (Address/8)*8;
		ShiftBack = Address - StartAddress;
		ResultLen = Len + ShiftBack;
	}
	if((StartAddress + ResultLen)%8)
	{
		ResultLen = ((ResultLen/8)*8) + 8;
	}

	for(uint32_t i = 0; i<ResultLen; i++)
	{
		if(*(uint8_t*)(StartAddress+i) != 0xFF)
			return false;
	}

	return true;
}

#endif
