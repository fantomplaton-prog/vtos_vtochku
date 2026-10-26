/*
  ******************************************************************************
  * @file    base_parameter.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef DATA_BASE_PARAMETER_H_
#define DATA_BASE_PARAMETER_H_

#include "stdint.h"
#include "stdbool.h"

#include "utils/static_conteiner.h"

typedef enum {
    PARTE_UNKNOW = 0,
    PARTE_BOOL_TYPE,
    PARTE_BOOL_ARRAY_TYPE,
    PARTE_BOOL_MAP_TYPE,

	// INTEGER TYPE
    PARTE_INT32_TYPE,
    PARTE_INT32_ARRAY_TYPE,
    PARTE_INT32_MAP_TYPE,

    PARTE_INT16_TYPE,
    PARTE_INT16_ARRAY_TYPE,
    PARTE_INT16_MAP_TYPE,

    PARTE_INT8_TYPE,
    PARTE_INT8_ARRAY_TYPE,
    PARTE_INT8_MAP_TYPE,

	// UNSIGNED INTEGER TYPE
    PARTE_UINT32_TYPE,
    PARTE_UINT32_ARRAY_TYPE,
    PARTE_UINT32_MAP_TYPE,

	PARTE_UINT16_TYPE,
	PARTE_UINT16_ARRAY_TYPE,
	PARTE_UINT16_MAP_TYPE,

	PARTE_UINT8_TYPE,
	PARTE_UINT8_ARRAY_TYPE,
	PARTE_UINT8_MAP_TYPE,

	// FLOAT TYPE
    PARTE_FLOAT_TYPE,
    PARTE_FLOAT_ARRAY_TYPE,
    PARTE_FLOAT_MAP_TYPE,

	// STRUCT TYPE
    PARTE_STRUCT_TYPE,
    PARTE_STRUCT_ARRAY_TYPE,
    PARTE_STRUCT_MAP_TYPE,

    PARTE_FIX_LEN_CHAR_TYPE,
	PARTE_VAR_LEN_CHAR_TYPE,

	PARTE_FIX_DATA_TRANSFER_TYPE,


    PARTE_NUM

} parameter_type_enum;

typedef enum {
    PAITE_UNKNOW 								= 0,
    PAITE_BLE_CUSTOM_CHAR 						= 1,
    PAITE_BLE_AUTO_CHAR 						= 2,
    PAITE_BLE_CUSTOM_CHAR_STRUCT_ARRAY_ITEM		= 3,
    PAITE_BLE_AUTO_CHAR_STRUCT_ARRAY_ITEM		= 4,
	PAITE_WIDGET								= 5,
	PAITE_VALUE_LIMIT_MIN_MAX					= 6,
	PAITE_READ_DATA_TABLE_SLOT					= 7,
	PAITE_ADD_PARAM_TO_WIDGET					= 8,

} parameter_addinfo_type_enum;


typedef enum {
    PDTE_UNKNOW = 0,
	PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM

} parameter_dependence_type_enum;



typedef enum {
    PSSE_DONT_SAVE = 0,
    PSSE_SAVE
} parameter_save_setting_enum;

typedef enum {
    PSDE_MASTER_TO_SLAVE = 0,
    PSDE_SLAVE_TO_MASTER,
    PSDE_BIDIRECTIONAL,
    PSDE_BIDIRECTIONAL_TWO_PARAMETERS
} parameter_synchronization_direction_enum;

/*
 typedef enum {
 PSDE_MASTER_TO_SLAVE = 0,
 PSDE_SLAVE_TO_MASTER,
 PSDE_BIDIRECTIONAL,
 PSDE_BIDIRECTIONAL_TWO_PARAMETERS
 }parameter_synchronization_direction_enum;
 */

typedef enum {
    PSTE_READ_ONLY = 0,
    PSTE_WRITE_ONLY,
    PSTE_STREAMING,
    PSTE_NOTIFICATION,
	PSTE_READ_WRITE
} parameter_synchronization_type_enum;

typedef enum {
    PLTE_AUTO		 = 0,
    PLTE_DYNAMIC
} parameter_link_type_enum;

typedef enum {
    PLTE_LIMIT_NO_LIMIT = 0,
    PLTE_LIMIT_BY_TYPE = 1,
    PLTE_LIMIT_CUSTOM = 2,
    PLTE_LIMIT_100 = 3,
    PLTE_LIMIT_NUM
} parameter_limite_type_enum;


typedef enum {
    PROE_REGULAR_STREAM  = 0,
	PROE_ACTIVATED_STREAM = 1,
} parameter_request_optimisation_enum;




typedef struct {

    uint8_t ID;
    uint8_t BroadcastID;
    uint8_t DataCode;
    uint8_t DataInstance;
    uint16_t parameter_size;
    uint8_t flag_shift;

    uint8_t optimisation_transfer:	1;
    uint8_t optimisation_stream:	7;

    uint8_t value_limit;

    uint8_t init_read:				1;
    uint8_t init_write:				1;
    uint8_t synch_type:				3;
    uint8_t synch_direction:		3;
    uint8_t synch_period;


    uint8_t type;
    uint8_t save_in_master:			1;
    uint8_t save_in_slave:			1;
    uint8_t main_consumer:			1;
    uint8_t link_type:				5;

    uint8_t additional_info_size;

    uint8_t related_parameter_ID;
    uint8_t related_DataCode;

} __attribute__((__packed__)) base_parameter_info_struct;


union u_ble_uuid {
    uint16_t uuid16; /*!< 16bit UUID */
    uint32_t uuid32; /*!< 32bit UUID */
    uint8_t uuid128[16]; /*!< 128bit UUID */
} __attribute__((__packed__));

typedef enum {
    BULE_LEN_16 = 0,
    BULE_LEN_32,
    BULE_LEN_128
} ble_uuid_len_enum;

typedef struct {
    union u_ble_uuid uuid;
    uint8_t uuid_len :2;
    uint8_t read_per :1;
    uint8_t write_per :1;
    uint8_t notify :1;

} __attribute__((__packed__)) param_addinfo_custom_ble_char_struct;


typedef struct {
	uint8_t read_slot;
} __attribute__((__packed__)) param_addinfo_read_data_table_struct;

typedef struct {
    uint16_t ArraySize;
    uint16_t ItemSize;
    uint8_t ArrayCRC;
} __attribute__((__packed__)) StructArrayAddInfo_struct;

typedef struct {
    uint8_t write_type;
    uint16_t Index;
    uint8_t *Data;
} __attribute__((__packed__)) StructArray_write_item_header_struct;



typedef enum {
    SAWCE_ERROR = 0, SAWCE_ITEM_WRITE
} StructArray_write_command_enum;

/*
 typedef struct
 {

 uint8_t* parameter_data_p;

 void* related_parameter_p;

 uint8_t* flag_buffer;

 void (*change_notification)(void* param, uint8_t* newVal);

 void* device_owner;

 void** parameters_consumers;

 }array_parameter_atribute_struct;
 */
typedef struct {
    uint16_t info_type;
    uint16_t info_size;
    uint8_t *info;
} __attribute__((__packed__))base_parameter_addinfoseg_struct;

typedef struct {
    uint8_t info_type;
    uint8_t info_instance;
    uint16_t info_size;
    void *info;
} __attribute__((__packed__))dependence_info_struct;



typedef struct {
	uint8_t smart_parameter_write: 1;
	uint8_t update_external_source_param: 1;
	uint8_t reserv: 6;

}enable_function_struct;


typedef void (*change_notification_callback_t)(void *param, uint8_t *newVal);
typedef void (*tx_notificaion_callback_t)(void *param);
typedef void (*pre_read_callback_t)(void *param);
typedef void (*pre_read_data_transfer_callback_t)(void *param, uint8_t* args, uint16_t size);

typedef struct {

    uint8_t *parameter_data_p;

    void *related_parameter_p;

    uint8_t *flag_buffer;

    change_notification_callback_t change_notification;
    pre_read_callback_t pre_read_callback;
    tx_notificaion_callback_t tx_notification;

    void *device_owner;

    struct parameter_consumer_array_struct *parameters_consumers;

    void *source_parameter;

    base_parameter_addinfoseg_struct *add_info_segments;

    struct dependence_info_array_struct* dependence_info;

    uint8_t enable_transfer;


    enable_function_struct enable_function;

} base_parameter_pointers_struct;

typedef struct {
    base_parameter_info_struct *info;
    base_parameter_pointers_struct *pointers;
} base_parameter_struct;


struct parameter_consumer_array_struct{
	uint8_t size;
	uint8_t counter;
	base_parameter_struct* array;

};

struct dependence_info_array_struct{
	uint8_t size;
	uint8_t counter;
	dependence_info_struct* array;

};


typedef struct {
    static_conteiner_struct *change_parameter_conteiner;
    base_parameter_struct *parameters;
    uint8_t param_num;
    void (*change_parameter_callback)(base_parameter_struct parameter,uint8_t newVal, uint16_t size);
} parameter_manager_struct;



typedef struct {
	uint8_t Address;
	uint8_t Command;
} parameter_stream_filter_struct;



typedef struct
{
	void (*external_stream_add_param_callback)(base_parameter_struct* param, void* object);
	base_parameter_struct** param_array;
	uint16_t array_size;
	uint16_t array_counter;

	parameter_stream_filter_struct* filter_array;
	uint8_t filter_array_size;
	uint8_t filter_array_counter;

	void* protocol;
}external_device_parameter_stream__struct;



void init_parameter_manager(parameter_manager_struct **param_manager,
        base_parameter_struct *parameters, uint8_t param_num);

void parameter_write_data(parameter_manager_struct *param_manager,
        uint8_t data_code, uint8_t instance, void *data);
void parameter_write_data_obj(parameter_manager_struct *param_manager,
        base_parameter_struct *param, uint8_t *data);
void parameter_write_data_info(parameter_manager_struct *param_manager,
        base_parameter_info_struct *param_info, void *data);

void parameter_write_master_to_slave_simple(base_parameter_struct *param,
        uint8_t *Data);

void parameter_write_master_to_slave_struct_array(base_parameter_struct *param,
        uint8_t *Data);

void parameter_write_master_to_slave_data_transfer(base_parameter_struct* param, uint8_t* Data, uint16_t size);


void create_parameter_consumer_array(base_parameter_pointers_struct *param_pointers, uint8_t size);

void parameter_add_consumer(base_parameter_pointers_struct *source_param_pointers, base_parameter_struct param);

bool parameter_add_dependence(base_parameter_pointers_struct *param_pointers, uint8_t type, uint8_t instance, void* dependence_pointer, uint32_t dependence_size);


//void parameter_write_data(base_parameter_struct* param, void* val, uint32_t size);
//void* parameter_get_data_pointer(base_parameter_struct* param);

#endif /* DATA_BASE_PARAMETER_H_ */
