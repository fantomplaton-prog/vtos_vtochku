/*
  ******************************************************************************
  * @file    flash_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "HAL_DRIVERS.h"

#include "Drivers/stm32/flash_driver.h"
//#define UBI_SDK_TARGET_STM32G4
#ifdef UBI_SDK_TARGET_STM32G4

//#include "memory_struct.h"
#include <string.h> /* memset */


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


uint32_t readFlash32(base_memory_driver_struct* memory_driver, uint32_t Address);
uint16_t readFlash16(base_memory_driver_struct* memory_driver, uint32_t Address);
uint8_t readFlash8(base_memory_driver_struct* memory_driver, uint32_t Address);
void  readData(base_memory_driver_struct* memory_driver, uint32_t Address,
                uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status);

void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address);

bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size);
void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status);

bool CommitRamToFlash(base_memory_driver_struct* memory_driver);
void finish_used_flash(base_memory_driver_struct* memory_driver);
bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len);

void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address);


uint32_t getSectorNumber(uint32_t Address);
uint32_t getStartSectorAddress(uint32_t Address);


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

	t_memory_driver->compare_memory_val = compare_memory_val;
	t_memory_driver->memory_region_is_free = memory_region_is_free;


	t_memory_driver->write_bytes = WriteBytesToFlash;
	//t_memory_driver->smart_write = SmartWriteToFlash;

	t_memory_driver->clear_sector = clear_flash_sector;
	t_memory_driver->readData = readData;
	t_memory_driver->finish_used_flash = finish_used_flash;


	*memory_driver = t_memory_driver;

}


uint64_t readFlas64(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint64_t*)Address;
}

uint32_t readFlash32(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint32_t*)Address;
}

uint16_t readFlash16(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint16_t*)Address;;
}

uint8_t readFlash8(base_memory_driver_struct* memory_driver, uint32_t Address)
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

void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return (void*)Address;
}


bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size)
{
    if(!(Address - memory_driver->RAM_Buffer.StartSectorAddress < MIN_SECTOR_SIZE))
    {
        // commit buffer to flash
        if (memory_driver->RAM_Buffer.flash_sector_used)
        {
            if (!CommitRamToFlash(memory_driver))
                return false;
        }

        //setup new ram page
        memory_driver->RAM_Buffer.flash_sector_used = true;
        memory_driver->RAM_Buffer.StartSectorAddress = Address - (Address % MIN_SECTOR_SIZE);
        memcpy(memory_driver->RAM_Buffer.flash_driver_ram_buf , getFlashPointer(memory_driver, memory_driver->RAM_Buffer.StartSectorAddress), MIN_SECTOR_SIZE);
    }

    uint32_t offset = Address - memory_driver->RAM_Buffer.StartSectorAddress;
    memcpy(memory_driver->RAM_Buffer.flash_driver_ram_buf + offset, Buf, Size);
	return true;
}

bool CommitRamToFlash(base_memory_driver_struct* memory_driver)
{
    if (!memory_driver->RAM_Buffer.flash_sector_used)
        return true;
    clear_flash_sector(memory_driver, memory_driver->RAM_Buffer.StartSectorAddress);
    HAL_FLASH_Unlock();

#ifdef FAST_FLASH_USED
    for (uint32_t i = 0; i < MIN_SECTOR_SIZE / (32 * 8) - 1; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST,
                          memory_driver->RAM_Buffer.StartSectorAddress + i * (32 * 8),
                          (uint64_t) memory_driver->RAM_Buffer.flash_driver_ram_buf + i * (32 * 8)))
            return false;

    }

    uint32_t i = MIN_SECTOR_SIZE / (32 * 8) - 1;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST_AND_LAST,
                      memory_driver->RAM_Buffer.StartSectorAddress + i * (32 * 8),
                      (uint64_t) memory_driver->RAM_Buffer.flash_driver_ram_buf + i * (32 * 8)))
        return false;
#else
    for (uint32_t i = 0; i < MIN_SECTOR_SIZE / 8; i++)
    {
        uint64_t value = *(uint64_t*) (memory_driver->RAM_Buffer.flash_driver_ram_buf + (i * 8));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                              (memory_driver->RAM_Buffer.StartSectorAddress + (i * 8)),
                              value))
            return false;
    }
#endif
    HAL_FLASH_Lock();
    memory_driver->RAM_Buffer.flash_sector_used = false;
    memory_driver->RAM_Buffer.StartSectorAddress = 0;
    return true;
}

/*
void WriteBytesToVirtualPage(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size,memory_driver_status_enum* status)
{
	if(ram_buffer_info.flash_sector_used)
	{

	}else
	{

	}
}
*/
/*
 * typedef enum {
		MDSE_OK = 0,
		MDSE_ERROR,
		MDSE_OWERWRITING_NOT_REQUERED,
		MDSE_DATA_CONTAINE_IN_RAM_BUFFER,
		MDSE_FLASH_BUFFER_BUSY

}memory_driver_status_enum;
*/
/*
void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size,memory_driver_status_enum* status)
{


	if(ram_buffer_info.flash_sector_used)
	{
		uint32_t StartSectorAddress = getStartSectorAddress(Address);
		memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

		*status = MDSE_DATA_CONTAINE_IN_RAM_BUFFER;
		return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
	}else
	{

		if(memory_region_is_free(memory_driver, Address, Size))
		{

			if(!WriteBytesToFlash(memory_driver, Address,Buf,Size))
			{
				*status = MDSE_ERROR;
				return 0;
			}

			*status = MDSE_OK;
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

					memcpy(flash_driver_ram_buf,getFlashPointer(memory_driver,StartSectorAddress),MIN_SECTOR_SIZE);
					memcpy(&flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

					*status = MDSE_DATA_CONTAINE_IN_RAM_BUFFER;
					return (void*)&flash_driver_ram_buf[Address - StartSectorAddress];
				}
			}
		}
	}
	return 0;
}
*/
void finish_used_flash(base_memory_driver_struct* memory_driver)
{
    CommitRamToFlash(memory_driver);
}




void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	uint32_t StartSectorNumber = getSectorNumber(Address);

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_config;

	erase_config.TypeErase = FLASH_TYPEERASE_PAGES;
    //erase_config.Banks = 0;
	erase_config.Page = StartSectorNumber;
	erase_config.NbPages = 1;

	uint32_t SectorError = 0;
	HAL_FLASHEx_Erase(&erase_config, &SectorError);

	HAL_FLASH_Lock();

}



uint32_t getSectorNumber(uint32_t Address)
{
    return (Address - 0x08000000)/0x800;

}


uint32_t getStartSectorAddress(uint32_t Address)
{
	return 0;
}

bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size)
{
	for(uint32_t i = 0; i<val_size; i++)
	{
		if(*(uint8_t*)(Address+i) != *(val + i))
			return false;
	}
	return true;
}

bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len)
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
