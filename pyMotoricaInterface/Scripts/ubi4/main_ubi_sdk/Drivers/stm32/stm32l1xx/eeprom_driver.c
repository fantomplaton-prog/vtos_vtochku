/*
  ******************************************************************************
  * @file    eeprom_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "HAL_DRIVERS.h"

#include "Drivers/stm32/eeprom_driver.h"



#ifdef UBI_SDK_TARGET_STM32L1

#include <string.h> /* memset */




static uint32_t readEEPROM32(base_memory_driver_struct* memory_driver, uint32_t Address);
static uint16_t readEEPROM16(base_memory_driver_struct* memory_driver, uint32_t Address);
static uint8_t readEEPROM8(base_memory_driver_struct* memory_driver, uint32_t Address);
static void* getEEPROMPointer(base_memory_driver_struct* memory_driver, uint32_t Address);

static bool WriteBytesToEEPROM(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size);
static void* SmartWriteToEEPROM(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status);

static bool compare_memory_val(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
static bool memory_region_is_free(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len);

static void clear_eeprom_sector(base_memory_driver_struct* memory_driver, uint32_t Address);
static void clear_eeprom_segment(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len);


void init_eeprom_driver(base_memory_driver_struct** memory_driver)
{
	base_memory_driver_struct* t_memory_driver = malloc(sizeof(base_memory_driver_struct));


	t_memory_driver->memory_type = MTE_INTERNAL_EEPROM;

	t_memory_driver->get_pointer = getEEPROMPointer;
	t_memory_driver->read8 = readEEPROM8;
	t_memory_driver->read16 = readEEPROM16;
	t_memory_driver->read32 = readEEPROM32;

	t_memory_driver->compare_memory_val = compare_memory_val;
	t_memory_driver->memory_region_is_free = memory_region_is_free;


	t_memory_driver->write_bytes = WriteBytesToEEPROM;
	t_memory_driver->smart_write = SmartWriteToEEPROM;

	t_memory_driver->clear_sector = clear_eeprom_sector;
	t_memory_driver->clear_sgment = clear_eeprom_segment;



}


static uint64_t readEEPROM64(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint64_t*)Address;
}

static uint32_t readEEPROM32(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint32_t*)Address;
}

static uint16_t readEEPROM16(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint16_t*)Address;
}


static uint8_t readEEPROM8(base_memory_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint8_t*)Address;
}

static void* getEEPROMPointer(base_memory_driver_struct* memory_driver, uint32_t Address)
{

	return (void*)Address;
}


static bool WriteBytesToEEPROM(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size)
{

	HAL_FLASHEx_DATAEEPROM_Unlock();

	for(uint32_t i = 0; i<Size; i++)
	{
		HAL_FLASHEx_DATAEEPROM_Program (FLASH_TYPEPROGRAMDATA_FASTBYTE, Address + i, Buf[i]);
	}

	HAL_FLASHEx_DATAEEPROM_Lock();

	return true;
}


static void* SmartWriteToEEPROM(base_memory_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status)
{
	HAL_FLASHEx_DATAEEPROM_Unlock();

	for(uint32_t i = 0; i<Size; i++)
	{
		if(*(uint8_t*)(Address + i) != 0)
			HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_BYTE, Address+i);

		HAL_FLASHEx_DATAEEPROM_Program (FLASH_TYPEPROGRAMDATA_FASTBYTE, Address + i, Buf[i]);
	}

	HAL_FLASHEx_DATAEEPROM_Lock();

	return 0;
}

static void finish_used_flash(base_memory_driver_struct* memory_driver)
{

}




static void clear_eeprom_sector(base_memory_driver_struct* memory_driver, uint32_t Address)
{


}

static void clear_eeprom_segment(base_memory_driver_struct* memory_driver, uint32_t Address, uint32_t Len)
{

	HAL_FLASHEx_DATAEEPROM_Unlock();

	for(uint32_t i = 0; i<Len; i++)
	{
		HAL_FLASHEx_DATAEEPROM_Erase (FLASH_TYPEERASEDATA_BYTE, Address+i);
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
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
	for(uint32_t i = 0; i<Len; i++)
	{
		if(*(uint8_t*)(Address+i) != 0x00)
			return false;
	}
	return true;
}

#endif
