/*
  ******************************************************************************
  * @file    flash_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "HAL_DRIVERS.h"

#include "Drivers/stm32/flash_driver.h"

#ifdef UBI_SDK_TARGET_STM32F4

//#include "memory_struct.h"
#include <string.h> /* memset */


uint8_t flash_driver_ram_buf[MIN_SECTOR_SIZE];

uint32_t readFlash32(base_memory_driver_struct* memory_driver, uint32_t Address);
uint16_t readFlash16(base_memory_driver_struct* memory_driver, uint32_t Address);
uint8_t readFlash8(base_memory_driver_struct* memory_driver, uint32_t Address);
void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address);

bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size);
void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status);

bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len);

void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address);
void ReadData(void* memory_driver, uint32_t Address, uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status);


uint32_t getSectorNumber(uint32_t Address);
uint32_t getStartSectorAddress(uint32_t Address);

void init_flash_driver(base_memory_driver_struct** memory_driver)
{
	base_memory_driver_struct* t_memory_driver = malloc(sizeof(base_memory_driver_struct));


	t_memory_driver->memory_type = MTE_INTERNAL_FLASH;

	t_memory_driver->get_pointer = getFlashPointer;
	t_memory_driver->read8 = readFlash8;
	t_memory_driver->read16 = readFlash16;
	t_memory_driver->read32 = readFlash32;

	t_memory_driver->readData = ReadData;

	t_memory_driver->compare_memory_val = compare_memory_val;
	t_memory_driver->memory_region_is_free = memory_region_is_free;


	t_memory_driver->write_bytes = WriteBytesToFlash;
	t_memory_driver->smart_write = SmartWriteToFlash;

	t_memory_driver->clear_sector = clear_flash_sector;

	t_memory_driver->RAM_Buffer.StartSectorAddress = 0;
	t_memory_driver->RAM_Buffer.flash_sector_used = false;
	t_memory_driver->RAM_Buffer.ram_buffer_size = MIN_SECTOR_SIZE;
	t_memory_driver->RAM_Buffer.flash_driver_ram_buf = flash_driver_ram_buf;

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

void* getFlashPointer(base_memory_driver_struct* memory_driver, uint32_t Address)
{

	return (void*)Address;
}

void ReadData(void* memory_driver, uint32_t Address, uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status)
{
	if(memory_driver == 0) return;

	uint8_t* data = (uint8_t*)getFlashPointer(memory_driver, Address);

	memcpy(readBuffer, data, len);

	*status = MDSE_OK;

}



bool WriteBytesToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size)
{


	uint32_t tData = 0xFFFFFFFF;
	uint32_t shift = 0;

	HAL_FLASH_Unlock();


	for(uint32_t i = 0; i<Size/4; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address+i*4, *(uint64_t*)&Buf[i*4]) != HAL_OK)
			return false;
	}

	if(Size%4)
	{
		shift = (Size/4)*4;

	 tData = 0xFFFFFFFF;

		memcpy((uint8_t*)&tData,Buf+shift,Size-((Size/4)*4));


		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address+shift, tData) != HAL_OK)
					return false;
	}


	HAL_FLASH_Lock();

	return true;

	/*
	uint64_t tData = 0xFFFFFFFFFFFFFFFF;
	uint32_t shift = 0;

	HAL_FLASH_Unlock();


	for(uint32_t i = 0; i<Size/8; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address+i*8, *(uint64_t*)&Buf[i*8]) != HAL_OK)
			return false;
	}

	if(Size%8)
	{
		shift = (Size/8)*8;

	 tData = 0xFFFFFFFFFFFFFFFF;

		memcpy((uint8_t*)&tData,Buf+shift,Size-((Size/8)*8));


		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, Address+shift, tData) != HAL_OK)
					return false;
	}


	HAL_FLASH_Lock();

	return true;
	*/
}


void* SmartWriteToFlash(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size,memory_driver_status_enum* status)
{


	if(memory_driver->RAM_Buffer.flash_sector_used)
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
				if(!memory_driver->RAM_Buffer.flash_sector_used)
				{
					memory_driver->RAM_Buffer.flash_sector_used = true;
					memset(flash_driver_ram_buf,0xFFFFFFFF,MIN_SECTOR_SIZE);
					uint32_t StartSectorAddress = getStartSectorAddress(Address);
					memory_driver->RAM_Buffer.StartSectorAddress = StartSectorAddress;

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

void finish_used_flash(base_memory_driver_struct* memory_driver)
{
	if(memory_driver->RAM_Buffer.flash_sector_used)
	{
		clear_flash_sector(memory_driver, memory_driver->RAM_Buffer.StartSectorAddress);

		/*
		HAL_FLASH_Unlock();
		for(uint32_t i = 0; i<MIN_SECTOR_SIZE; i++)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, ram_buffer_info.StartSectorAddress+i,flash_driver_ram_buf[i]);

		}
		HAL_FLASH_Lock();
		*/

		WriteBytesToFlash(memory_driver, memory_driver->RAM_Buffer.StartSectorAddress,memory_driver->RAM_Buffer.ram_buffer_size,memory_driver->RAM_Buffer.ram_buffer_size);


		memory_driver->RAM_Buffer.flash_sector_used = false;
	}
}




void clear_flash_sector(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	uint32_t StartSectorNumber = getSectorNumber(Address);

	if(StartSectorNumber == 0xFF)return;

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_config;

	erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_config.NbSectors = 1;
	erase_config.Sector = StartSectorNumber;
	erase_config.Banks = FLASH_BANK_1;
	erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3;


	uint32_t SectorError = 0;
	HAL_FLASHEx_Erase(&erase_config, &SectorError);

	HAL_FLASH_Lock();

}

uint32_t getSectorNumber(uint32_t Address)
{

	if(Address >= 0x08000000 && Address <= 0x08003FFF)
		return 0;
	else if(Address >= 0x08004000 && Address <= 0x08007FFF)
		return 1;
	else if(Address >= 0x08008000 && Address <= 0x0800BFFF)
		return 2;
	else if(Address >= 0x0800C000 && Address <= 0x0800FFFF)
		return 3;
	else if(Address >= 0x08010000 && Address <= 0x0801FFFF)
		return 4;
	else if(Address >= 0x08020000 && Address <= 0x0803FFFF)
		return 5;
	else if(Address >= 0x08040000 && Address <= 0x0805FFFF )
		return 6;
	else if(Address >= 0x08060000 && Address <= 0x0807FFFF)
		return 7;

	return 0xFF;
}

uint32_t getStartSectorAddress(uint32_t Address)
{
//	return DATA_START_ADDRESS;
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
