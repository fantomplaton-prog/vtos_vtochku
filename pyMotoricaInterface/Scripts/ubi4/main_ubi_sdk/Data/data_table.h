/*
  ******************************************************************************
  * @file    data_table.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_DATA_TABLE_H_
#define INC_DATA_TABLE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "TransportLevel/main_receiver.h"
#include "Data/data_table_structs.h"
#include "Drivers/base_memory_driver.h"

#define DATA_TABLE_INIT_CODE 0x513429EF

 typedef struct
 {
 	uint8_t DataCode;

 	uint8_t exist_change: 1;
 	uint8_t ram_image_init: 1;
 	uint8_t reserv: 6;
 	uint8_t* ram_image;
 }data_table_ram_image_pointer_struct;


 typedef struct
 {
	 data_table_slot_struct* slots;
	 uint8_t slot_num;
	 uint8_t** data_pointer_buffer;

 }data_table_default_struct;


 typedef struct
 {
 	uint32_t DataTableStartAddres;

 	data_table_info_struct* info;
 	data_table_slot_struct* slots;
 	uint8_t slots_num;

 	data_table_default_struct default_data_table;

 	uint8_t version;
 	void* DataPointer;

 	data_table_ram_image_pointer_struct* ram_image_map;
 	uint8_t ram_image_map_size;


 	void (*clear_data_table)(void* data_table);

 	base_memory_driver_struct* memory_driver;
 }data_table_struct;




/*
typedef enum {
		DTE_UNKNOW = 0,
		DTE_BOOTLOADER_INFO_TYPE,
		DTE_FW_INFO_TYPE,
		DTE_DEVICE_INFO_TYPE,
		DTE_BOARD_INFO_TYPE,
		DTE_PRODUCT_INFO_TYPE,
		DTE_SERVICE_INFO,
		DTE_FREE_SLOT = 0xFF,
	}data_table_slots_enum;


typedef struct
	{
		uint8_t data_table_version;
		uint8_t data_table_crc;
		uint16_t data_table_size;
	}data_table_info_struct;



typedef struct
{
	data_table_slots_enum DataType;
	uint8_t DataTypeVersion;
	uint8_t DataTypeSubVersion;
	uint8_t crc;
	uint16_t DataSize;
	uint16_t StartAddressShift;
}data_table_slot_struct;


typedef enum {
		DTWS_WRITE_SLOT_OK = 0,
		DTWS_OWERWRITING_NOT_REQUERED,
		DTWS_DATA_CONTAINE_IN_FLASH_BUFFER,
		DTWS_SLOT_EXIST,
		DTWS_ALL_SLOT_BUSY
	}data_table_write_new_slot_status_enum;

typedef enum {
		DTRS_READ_SLOT_OK = 0,
		DTRS_SLOT_NOT_FOUND
	}data_table_read_slot_status_enum;

typedef enum {
	DTWD_WRITE_DATA_OK = 0,
	DTWD_OWERWRITING_NOT_REQUERED,
	DTWD_DATA_CONTAINE_IN_FLASH_BUFFER,
	DTWD_FLASH_BUFFER_BUSY,
	DTWD_SLOT_NOT_FOUND,
	DTWD_SLOT_ERROR
	}data_table_write_data_status_enum;

typedef enum {
	DTRD_READ_DATA_OK = 0,
	DTRD_SLOT_NOT_FOUND
	}data_table_read_data_status_enum;

*/


#define DATA_TABLE_SIZE 	16
#define DATA_TABLE_VERSION 	1


 	data_table_initialize_status_enum DataTableInit(data_table_struct* data_table, base_memory_driver_struct* memory_driver);

 	data_table_open_status_enum DataTable_Open(data_table_struct* data_table);
 	void DataTable_Close(data_table_struct* data_table);
 	void DataTable_Save(data_table_struct* data_table);
 	void DataTableSetRamImageFlag(data_table_struct* data_table, uint8_t slot);

 	void DataTable_Init_Default_RAM(data_table_struct* data_table);


 	void DataTableInitialiseRamImage(data_table_struct* data_table);
	void DataTableInitFinish(data_table_struct* data_table);

	data_table_slot_struct* DataTable_WriteNewSlot(data_table_struct* data_table, data_table_slot_struct* Row, data_table_write_new_slot_status_enum* status);
	data_table_slot_struct* Data_Table_ReadSlot(data_table_struct* data_table, uint8_t slot, data_table_read_slot_status_enum* status);

	void DataTable_WriteData_RAM(data_table_struct* data_table, uint8_t slot, uint8_t* Data, data_table_write_data_status_enum* status);
	void* DataTable_ReadData_RAM(data_table_struct* data_table, uint8_t slot, data_table_read_data_status_enum* status);

	void DataTableWriteData(data_table_struct* data_table, uint8_t slot,uint8_t* Data, data_table_write_data_status_enum* status);
	void* Data_Table_ReadData(data_table_struct* data_table, uint8_t slot, data_table_read_data_status_enum* status);

	void ReadAvailableSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
	void ReadSlot(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
	void ReadDataInSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);
	void WriteDataInSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);

	void ReadAvailableSlot(data_table_struct* data_table, command_selector_struct command);
	void ReadDataInSlot(data_table_struct* data_table, command_selector_struct command);
	void WriteDataInSlot(data_table_struct* data_table, command_selector_struct command);


	void DataTable_Clear(data_table_struct* data_table);
	void DataTable_Clear_RAM(data_table_struct* data_table);

	void Data_Table_Reset(data_table_struct* data_table);

	void DataTable_Calc_Image_CRC(data_table_struct* data_table);


	extern uint32_t Data_Table_TableLocation;
	extern data_table_info_struct* Data_Table_Info;

#ifdef __cplusplus
}
#endif


#endif /* INC_DATA_TABLE_H_ */
