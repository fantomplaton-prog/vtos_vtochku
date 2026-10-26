/*
  ******************************************************************************
  * @file    data_table_structs.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_DATA_TABLE_STRUCTS_H_
#define INC_DATA_TABLE_STRUCTS_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"



typedef enum {
		DTE_UNKNOW_TYPE= 0,
		DTE_STRUCT,
		DTE_STRUCT_ARRAY,
		DTE_STRUCT_ARRAY_1SET_BYTE,
		DTE_ARRAY,
		DTE_STRING
	}data_table_slots_type_enum ;


typedef struct
	{
		uint32_t init_code;
		uint32_t reserv;
		uint16_t data_table_size;
		uint8_t data_table_version;
		uint8_t data_table_crc;
	}__attribute__((__packed__))data_table_info_struct;




typedef struct
{
	uint8_t DataCode;
	uint8_t DataType;
	uint8_t DataTypeVersion;
	uint8_t DataTypeSubVersion;
	uint16_t DataSize;
	uint16_t StartAddressShift;
	uint8_t crc;
	uint8_t reserv1;
	uint8_t reserv2;
	uint8_t reserv3;
}__attribute__((__packed__))data_table_slot_struct;

typedef struct
{
	uint8_t ItemType;
	uint8_t ItemTypeVersion;
	uint8_t ItemTypeSubVersion;
	uint16_t ItemSize;
	uint16_t ItemNum;

}__attribute__((__packed__))data_table_array_slot_additional_struct;



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
	DTRD_READ_RAM_IMAGE,
	DTRD_SLOT_NOT_FOUND
	}data_table_read_data_status_enum;

typedef enum {
		DTRD_ALREADY_INITIALIZED = 0,
		DTRD_FIRST_INITIALIZE,
		DTRD_INITIALIZE_ERROR
		}data_table_initialize_status_enum;


typedef enum {
		DTSTE_OPEN = 0,
		DTSTE_CRC_OPEN_PROBLEM,
		DTSTE_ALREADY_OPEN,
		DTSTE_DATA_CODE_ERROR,
		DTSTE_ERROR
		}data_table_open_status_enum;

typedef enum {
		DTRDRE_OPEN = 0,
		DTRDRE_CRC_ERROR,
		DTRDRE_ERROR
		}data_table_read_data_result_enum;


#ifdef __cplusplus
}
#endif

#endif /* INC_DATA_TABLE_STRUCTS_H_*/
