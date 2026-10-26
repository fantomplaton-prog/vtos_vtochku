/*
  ******************************************************************************
  * @file    partition_flash_driver.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "Drivers/esp32/partition_flash_driver.h"
#include <string.h>

#ifdef UBI_SDK_FAMILY_ESP32

static uint32_t read32(partition_flash_driver_struct* memory_driver, uint32_t Address);
static uint16_t read16(partition_flash_driver_struct* memory_driver, uint32_t Address);
static uint8_t read8(partition_flash_driver_struct* memory_driver, uint32_t Address);
static void* getPointer(partition_flash_driver_struct* memory_driver, uint32_t Address, int size, memory_driver_status_enum* status);
static void readData(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status);

static bool WriteBytesToFlash(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size);
static void* SmartWriteToFlash(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status);

static bool compare_memory_val(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size);
static bool memory_region_is_free(partition_flash_driver_struct* memory_driver, uint32_t Address, uint32_t Len);

static void clear_flash_sector(partition_flash_driver_struct* memory_driver, uint32_t Address);
void clear_partition(partition_flash_driver_struct* memory_driver);
//static void clear_flash_segment(partition_flash_driver_struct* memory_driver, uint32_t Address, uint32_t Len);

static uint32_t getStartSectorAddress(uint32_t Address);



void init_partition_flash_driver(partition_flash_driver_struct** memory_driver)
{
	partition_flash_driver_struct* t_memory_driver = malloc(sizeof(partition_flash_driver_struct));

	t_memory_driver->partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,"data_table");

    //esp_partition_mmap(t_memory_driver->partition,0,DATA_SIZE,SPI_FLASH_MMAP_DATA,&DataTable_TablePointer,&DataTable_TableMemoryHandle);

	t_memory_driver->Base.memory_type = MTE_EXTERNAL_FLASH;

	t_memory_driver->Base.get_pointer = getPointer;
	t_memory_driver->Base.read8 = read8;
	t_memory_driver->Base.read16 = read16;
	t_memory_driver->Base.read32 = read32;
	t_memory_driver->Base.readData = readData;

	t_memory_driver->Base.compare_memory_val = compare_memory_val;
	t_memory_driver->Base.memory_region_is_free = memory_region_is_free;


	t_memory_driver->Base.write_bytes = WriteBytesToFlash;
	t_memory_driver->Base.smart_write = SmartWriteToFlash;

	//t_memory_driver->Base.




	t_memory_driver->Base.clear_sector = clear_flash_sector;
	t_memory_driver->clear_partition = clear_partition;
	//t_memory_driver->Base.clear_sgment = clear_flash_segment;





	*memory_driver = t_memory_driver;



}


static void readData(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* readBuffer, uint32_t len, memory_driver_status_enum* status)
{

	esp_err_t t_status = esp_partition_read(memory_driver->partition, Address, readBuffer, len);
	if(t_status == ESP_OK)
	{
		*status = MDSE_OK;
		return;
	}else
	{
		*status = MDSE_ERROR;
		return;
	}
}

static uint64_t read64(partition_flash_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint64_t*)Address;
}

static uint32_t read32(partition_flash_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint32_t*)Address;
}

static uint16_t read16(partition_flash_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint16_t*)Address;
}


static uint8_t read8(partition_flash_driver_struct* memory_driver, uint32_t Address)
{
	return *(uint8_t*)Address;
}

static void* getPointer(partition_flash_driver_struct* memory_driver, uint32_t Address, int size, memory_driver_status_enum* status)
{
	uint8_t* readBuffer = malloc(size);
	*status = MDSE_ERROR;

	if(readBuffer == 0) return 0;

	esp_err_t t_status = esp_partition_read(memory_driver->partition, Address, readBuffer, size);

	if(t_status != ESP_OK)
	{
		free(readBuffer);
		return 0;
	}

	*status = MDSE_DATA_CONTAINE_IN_RAM_BUFFER;
	return (void*)readBuffer;
}


static bool WriteBytesToFlash(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size)
{

	if(esp_partition_write(memory_driver->partition,Address,Buf,Size) == ESP_OK)
		return true;
	else
		return false;
}




static void* SmartWriteToFlash(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* Buf, uint32_t Size, memory_driver_status_enum* status)
{



	ram_buffer_info_struct* RAM_Buffer = &memory_driver->Base.RAM_Buffer;



	if(RAM_Buffer->flash_sector_used)
	{

		if((RAM_Buffer->StartSectorAddress+RAM_Buffer->ram_buffer_size) > (Address+Size))
		{
			uint32_t StartSectorAddress = RAM_Buffer->StartSectorAddress;

			memcpy(&RAM_Buffer->flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

			*status = MDSE_DATA_CONTAINE_IN_RAM_BUFFER;

			return (void*)&RAM_Buffer->flash_driver_ram_buf[Address - StartSectorAddress];
		}

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

			return 0;// getFlashPointer(memory_driver, Address);
		}else
		{
			if(compare_memory_val(memory_driver, Address,Buf,Size))
			{
				*status = MDSE_OWERWRITING_NOT_REQUERED;
				return 0; //getFlashPointer(memory_driver, Address);
			}else
			{
				if(!RAM_Buffer->flash_sector_used)
				{
					RAM_Buffer->flash_sector_used = true;

					memset(RAM_Buffer->flash_driver_ram_buf,0xFFFFFFFF,RAM_Buffer->ram_buffer_size);

					uint32_t StartSectorAddress = getStartSectorAddress(Address);

					RAM_Buffer->StartSectorAddress = StartSectorAddress;

				//	memcpy(RAM_Buffer->flash_driver_ram_buf,getFlashPointer(memory_driver, StartSectorAddress),RAM_Buffer->ram_buffer_size);

					memcpy(&RAM_Buffer->flash_driver_ram_buf[Address - StartSectorAddress],Buf,Size);

					*status = MDSE_DATA_CONTAINE_IN_RAM_BUFFER;

					return (void*)&RAM_Buffer->flash_driver_ram_buf[Address - StartSectorAddress];
				}
			}
		}
	}
	return 0;
}

static void finish_used_flash(partition_flash_driver_struct* memory_driver)
{

	if(memory_driver->Base.RAM_Buffer.flash_sector_used)
	{
		clear_flash_sector(memory_driver, memory_driver->Base.RAM_Buffer.StartSectorAddress);

		WriteBytesToFlash(memory_driver, memory_driver->Base.RAM_Buffer.StartSectorAddress,memory_driver->Base.RAM_Buffer.flash_driver_ram_buf, SPI_FLASH_SEC_SIZE);

		memory_driver->Base.RAM_Buffer.flash_sector_used = false;
	}
}




static void clear_flash_sector(partition_flash_driver_struct* memory_driver, uint32_t Address)
{

	size_t offset = getStartSectorAddress(Address);

	esp_partition_erase_range(memory_driver->partition, offset, SPI_FLASH_SEC_SIZE);

}

void clear_partition(partition_flash_driver_struct* memory_driver)
{
	esp_partition_erase_range(memory_driver->partition,0,memory_driver->partition->size);
}

static uint32_t getSectorNumber(uint32_t Address)
{
	return 0;
}

static uint32_t getStartSectorAddress(uint32_t Address)
{
	return (Address/SPI_FLASH_SEC_SIZE)*SPI_FLASH_SEC_SIZE;
}
/*
static bool compare_memory_val(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size)
{
	uint8_t* readBuffer = malloc(val_size);

	if(readBuffer == 0) return false;

	esp_err_t t_status = esp_partition_read(memory_driver->partition, Address, readBuffer, val_size);

	if(t_status != ESP_OK)
	{
		free(readBuffer);
		return false;
	}


	for(uint32_t i = 0; i<val_size; i++)
	{
		if(readBuffer[i] != val[i])
		{
			free(readBuffer);
			return false;
		}
	}

	free(readBuffer);
	return true;
}
*/

static bool compare_memory_val(partition_flash_driver_struct* memory_driver, uint32_t Address, uint8_t* val, uint32_t val_size)
{
	uint32_t read_size = 32;
	uint8_t* readBuffer = malloc(read_size);
	if(readBuffer == 0) return false;

	uint32_t length = val_size;
	uint32_t offset = 0;
	esp_err_t t_status;


	while(length)
	{

		if(length > read_size)
		{
			t_status = esp_partition_read(memory_driver->partition, Address + offset, readBuffer, read_size);
			if(t_status != ESP_OK)
			{
				free(readBuffer);
				return false;
			}

			length-= read_size;
			offset+= read_size;

			for(uint32_t i = 0; i<read_size; i++)
			{
				if(readBuffer[i] != val[offset+i])
				{
					free(readBuffer);
					return false;
				}
			}

		}else
		{
			t_status = esp_partition_read(memory_driver->partition, Address + offset, readBuffer, length);
			if(t_status != ESP_OK)
			{
				free(readBuffer);
				return false;
			}

			length = 0;

			for(uint32_t i = 0; i<read_size; i++)
			{
				if(readBuffer[i] != val[offset+i])
				{
					free(readBuffer);
					return false;
				}
			}
		}
	}

	free(readBuffer);
	return true;
}

// Для оптимизации сделать чтение и проверку порциями
static bool memory_region_is_free(partition_flash_driver_struct* memory_driver, uint32_t Address, uint32_t Len)
{

	uint32_t read_size = 32;
	uint8_t* readBuffer = malloc(read_size);
	if(readBuffer == 0) return false;

	uint32_t length = Len;
	uint32_t offset = 0;
	esp_err_t t_status;


	while(length)
	{

		if(length > read_size)
		{
			t_status = esp_partition_read(memory_driver->partition, Address + offset, readBuffer, read_size);
			if(t_status != ESP_OK)
			{
				free(readBuffer);
				return false;
			}

			length-= read_size;
			offset+= read_size;

			for(uint32_t i = 0; i<read_size; i++)
			{
				if(readBuffer[i] != 0xFF)
				{
					free(readBuffer);
					return false;
				}
			}

		}else
		{
			t_status = esp_partition_read(memory_driver->partition, Address + offset, readBuffer, length);
			if(t_status != ESP_OK)
			{
				free(readBuffer);
				return false;
			}

			length = 0;

			for(uint32_t i = 0; i<read_size; i++)
			{
				if(readBuffer[i] != 0xFF)
				{
					free(readBuffer);
					return false;
				}
			}
		}
	}

	free(readBuffer);
	return true;
}

#endif
