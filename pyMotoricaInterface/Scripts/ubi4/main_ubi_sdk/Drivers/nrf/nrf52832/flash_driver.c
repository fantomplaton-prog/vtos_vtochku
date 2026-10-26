/*
  ******************************************************************************
  * @file    flash_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "HAL_DRIVERS.h"

#include "Drivers/nrf/flash_driver.h"

#ifdef UBI_SDK_MANUFACTURER_NORDIC

#include "memory_struct.h"
#include <string.h> /* memset */


uint8_t flash_driver_ram_buf[MIN_SECTOR_SIZE];

/*
typedef struct
{
	bool flash_sector_used;
	uint32_t StartSectorAddress;
}ram_buffer_info_struct;
*/

ram_buffer_info_struct ram_buffer_info =
		{
				.flash_sector_used = false,
				.StartSectorAddress = 0x00
		};
		
		
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
	
}


NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = FLASH_START_ADDR,
    .end_addr   = FLASH_END_ADDR
};




uint64_t readFlas64(uint32_t Address)
{
	return *(uint64_t*)Address;
}

uint32_t readFlash32(uint32_t Address)
{
	return *(uint32_t*)Address;
}

uint16_t readFlash16(uint32_t Address)
{
	return *(uint16_t*)Address;
}


uint8_t readFlash8(uint32_t Address)
{
	return *(uint8_t*)Address;
}

void* getFlashPointer(uint32_t Address)
{

	return (void*)Address;
}


void init_flash_driver(base_memory_driver_struct** memory_driver)
{
	
	uint32_t rc;
	
	nrf_fstorage_api_t * p_fs_api;
	p_fs_api = &nrf_fstorage_sd;
	
	
	base_memory_driver_struct* t_memory_driver = (base_memory_driver_struct*)malloc(sizeof(base_memory_driver_struct));
	
	t_memory_driver->memory_region_is_free = memory_region_is_free;
	t_memory_driver->compare_memory_val = compare_memory_val;
	
	*memory_driver = t_memory_driver;
	
	rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
	APP_ERROR_CHECK(rc);
	
}


bool WriteBytesToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size)
{
	
	//Execute write procedure
	uint32_t rc = nrf_fstorage_write(&fstorage, Address, Buf, Size, NULL);
	APP_ERROR_CHECK(rc);
	
	while(nrf_fstorage_is_busy(&fstorage));
	
	return true;
}

void WriteBytesToVirtualPage(uint32_t Address, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status)
{
	if(ram_buffer_info.flash_sector_used)
	{

	}else
	{

	}
}


void* SmartWriteToFlash(uint32_t Address, uint8_t* Buf, uint32_t Size,flash_driver_status_enum* status)
{
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
			
			if(!WriteBytesToFlash(Address,Buf,Size))
			{
				*status = FDSE_FLASH_WRITE_ERROR;
				return 0;
			}
			
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
	return 0;
}

void finish_used_flash()
{
	if(ram_buffer_info.flash_sector_used)
	{
		clear_flash_sector(ram_buffer_info.StartSectorAddress);


		WriteBytesToFlash(ram_buffer_info.StartSectorAddress,flash_driver_ram_buf,MIN_SECTOR_SIZE);


		ram_buffer_info.flash_sector_used = false;
	}
}




void clear_flash_sector(uint32_t Address)
{
	//Execute write procedure
	uint32_t rc = nrf_fstorage_erase(&fstorage, Address, 1, NULL);;
	APP_ERROR_CHECK(rc);
	
	//Timeout handler + wait while nrf storage is busy
	while (nrf_fstorage_is_busy(&fstorage));
}

uint32_t getSectorNumber(uint32_t Address)
{
	return DATA_PAGE;
}

uint32_t getStartSectorAddress(uint32_t Address)
{
	return DATA_START_ADDRESS;
}

bool compare_memory_val(uint32_t Address, uint8_t* val, uint32_t val_size)
{
	for(uint32_t i = 0; i<val_size; i++)
	{
		if(*(uint8_t*)(Address+i) != *(val + i))
			return false;
	}
	return true;
}

bool memory_region_is_free(uint32_t Address, uint32_t Len)
{

	uint32_t StartAddress = Address;
	uint32_t ResultLen = Len;
	uint32_t ShiftBack = 0;
	if(Address%4)
	{
		StartAddress = (Address/4)*4;
		ShiftBack = Address - StartAddress;
		ResultLen = Len + ShiftBack;
	}
	if((StartAddress + ResultLen)%4)
	{
		ResultLen = ((ResultLen/4)*4) + 4;
	}

	for(uint32_t i = 0; i<ResultLen; i++)
	{
		if(*(uint8_t*)(StartAddress+i) != 0xFF)
			return false;
	}

	return true;
}

#endif
