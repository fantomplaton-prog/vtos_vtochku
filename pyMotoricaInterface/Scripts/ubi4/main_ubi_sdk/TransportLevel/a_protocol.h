/*
  ******************************************************************************
  * @file    a_protocol.h
  * @author  A.Alborov
  ******************************************************************************
 */



#ifndef INC_A_PROTOCOL_H_
#define INC_A_PROTOCOL_H_


#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/data_sniffer.h"
#include "utils/custom_heap.h"
#include "TransportLevel/data_sniffer.h"
#include "TransportLevel/a_protocol_struct.h"
#include "utils/static_conteiner.h"
#include "utils/base_soft_timer.h"
#include "stdint.h"


 typedef enum {
	 	PACKAGE_TYPE_FIX_LEN = 0,
		PACKAGE_TYPE_VAR_LEN,
		PE_RES1,
		PE_RES2
	}package_type_enum;

typedef enum
{
	SDT_DRIVER			= 0x00,
	SDT_SENSORE			= 0x01,
	SDT_HID				= 0x02,
	SDT_ALL_DEVICES		= 0x03,
	SDT_NUM
}simple_device_type_enum;



typedef enum
{
	BPPT_FIX_LEN_PACKAGE	= 0x00,
	BPPT_FLOAT_LEN_PACKAGE	= 0x01,
	BPPT_RES1				= 0x02,
	BPPT_RES2				= 0x03
}bus_protocol_package_type_enum;

typedef enum
{
	BPRT_COMMAND	= 0x00,
	BPRT_PARAMETERS	= 0x01
}bus_protocol_request_type_enum;

/*
typedef struct
{
	uint8_t port:3;
	uint8_t exist_uid: 1;
	uint8_t package_type: 2;
	uint8_t direction: 1;
	uint8_t bridge: 1;

	uint8_t addr;
	uint8_t package_code;
	uint8_t Data[6];
	uint8_t crc;
}__attribute__((__packed__)) p2p_port_header;


//P2P high level port

typedef struct
{
	uint8_t res:7;
	uint8_t bridge: 1;
	uint8_t package_code;
	uint8_t package_id;
	uint16_t package_size;
	uint8_t crc;
}__attribute__((__packed__)) p2p_high_level_port_header;


typedef struct
{
    uint8_t port:7;
    uint8_t bridge: 1;
    uint8_t package_code;
    uint8_t package_id;
    uint16_t package_size;
    uint8_t crc;
    uint8_t address;
}__attribute__((__packed__)) p2p_bridge_high_level_port_header;

*/

struct device_address_struct{
	 uint8_t addr: 6;
	 uint8_t simple_device_type: 2;
}__attribute__((__packed__));


union u_device_address{
  uint8_t    full_address;
  struct device_address_struct split_address;
}__attribute__((__packed__));

struct var_len_payload_struct{
	 uint16_t size;
	 uint8_t Data[4];
}__attribute__((__packed__));


union u_bus_res_payload{
  uint8_t    Data[6];
  struct var_len_payload_struct var_len_header;
}__attribute__((__packed__));

typedef struct
{
	uint8_t last_package: 								1;
	uint8_t waiting_answer: 							1;
	uint8_t request_type: 								1;
	uint8_t size_type: 										2;
	uint8_t exist_uid_or_param_com_size: 	1; //waiting answer or parameter command size
	uint8_t bridge:												1;
	uint8_t request: 											1;
}__attribute__((__packed__)) base_bus_transmit_settings_struct;


typedef struct
{
	uint8_t last_package: 								1;
	uint8_t waiting_answer: 							1;
	uint8_t request_type: 								1;
	uint8_t size_type: 										2;
	uint8_t exist_uid_or_param_com_size: 	1; //waiting answer or parameter command size
	uint8_t bridge:												1;
	uint8_t request: 											1;

	union u_device_address addr;

	uint8_t package_code;
	union u_bus_res_payload Data;
	uint8_t crc;

}__attribute__((__packed__)) bus_main_module_header;

typedef enum
{
	PSSE_FREE			= 0x00,
	PSSE_BUSY

}package_info_slot_status_enum;

typedef enum
{
	PDSSE_FREE			= 0x00,
	PDSSE_BUSY

}protocol_data_segment_status_enum;

typedef enum
{
	PPSE_PARCE_OK			= 0x00,
	PPSE_SHORT_PACKAGE,
	PPSE_CRC_FAIL
}protocol_parce_status_enum;

typedef enum
{
	PISE_ID			= 0x00,
	PISE_BRIGE,
	PISE_WAIT_ANSWER,
	PISE_PACKAGE_CODE,
	PISE_PACKAGE_ID,
	PISE_PACKAGE_SIZE,
	PISE_PACKAGE_CRC,
	PISE_ADDRESS,
	PISE_REQUEST_TYPE
}pack_info_specificator_enum;




typedef enum
{
	QTTE_UNKNOW			= 0x00,
	QTTE_DIRECT,
	QTTE_BRIDGE
}queue_transmit_type_enum;


typedef enum
{
	PRME_RECEIVE_IRQ = 0x00,
	PRME_RECEIVE_FULL_BUFFER

}protocol_receive_mode_enum;

typedef struct
{
	uint16_t dataSegmentID;
	uint8_t* pBuf;
	uint16_t BufSize;

	uint8_t* pData;
	uint16_t DataSize;

	custom_heap_obj_struct* heap;
	custom_heap_slot_struct* heapSlot;


	protocol_data_segment_status_enum slot_status;

	void* object;
	void (*data_tx_callback)(void* object);
}__attribute__((__packed__))base_protocol_data_segment_struct;



typedef struct
{
	uint16_t pack_info_ID;
	uint8_t last_package: 1;
	uint8_t waiting_answer: 1;
	uint8_t request_type: 1;
	uint8_t size_type: 2;
	uint8_t exist_uid:1;
	uint8_t bridge:1;
	uint8_t request: 1;
	union u_device_address addr;
	uint8_t package_code;
	uint8_t* pData;
	uint16_t DataSize;
	uint8_t request_queue_pos;

	base_protocol_data_segment_struct* answer_data_segment;

	package_info_slot_status_enum slot_status;


}__attribute__((__packed__))ubi4_bus_package_info_struct;

typedef struct
{
	uint16_t pack_info_ID;
	uint8_t Address;
	uint8_t command;
	void* source_pack_info;
	void* source_protocol;

}__attribute__((__packed__))ubi4_bridge_request_info_struct;

typedef struct
{
	uint8_t status;
	void* device;
	void* last_bridge_pack_info;
	void* last_bridge_protocol;
}bridge_protocol_info_struct;

typedef struct
{
	uint16_t dataSegmentID;

	uint8_t waiting_answer: 	1;
	uint8_t request_type:		1;
	uint8_t reserv: 			6;

	uint8_t command;

	union u_device_address addr;

	queue_transmit_type_enum queue_transmit_type;
	ubi4_bus_package_info_struct* pack_info;
	base_protocol_data_segment_struct* data_segment;

	ubi4_bus_package_info_struct* source_pack_info;
	void* source_protocol_str;

}__attribute__((__packed__))base_protocol_queue_segment_struct;

typedef struct
{
	uint8_t rx_package_counter;
	bus_protocol_package_type_enum last_package_type;
	bus_main_module_header* last_header;

	uint32_t buffer_offset;
	int reset_timer_instance;
	int answer_us_timer_instance;
	base_port_struct* port;
	soft_timer_class_struct* ms_soft_timer;
	soft_timer_class_struct* us_soft_timer;


	/*
	uint16_t package_info_array_size;
	uint16_t package_info_array_count;
	ubi4_bus_package_info_struct* package_info_array;
	 */

	static_conteiner_struct* package_info_conteiner;
	static_conteiner_struct* data_segment_conteiner;

	static_conteiner_struct* transmit_queue_conteiner;

	//static_conteiner_struct* bridge_request_conteiner;

	sniffer_conteiner_struct* sniffer_conteiner;

	custom_heap_obj_struct* heap;

	uint8_t answer_queue_counter;

	uint8_t enable_half_duplex_transmit;

	uint8_t wait_answer;

	uint8_t* var_len_package_start;

	uint32_t last_rx_time;

	void* port_device_manager;

	void (*receive_data_func)(void* protocol_str);

	void (*receive_bridge_func)(void* port_str, uint8_t* buf, uint32_t size);
	void (*slave_answer_func)(void* protocol_str, uint8_t* buf, uint32_t size);

	void (*remove_pack_info)(void* protocol_str, void* pack_info);

	//bridge_protocol_info_struct (*check_device_bridge_info) (void* port_device_manager, uint8_t Address);

	base_protocol_data_segment_struct* (*get_data_segment_func)(void* protocol_str,uint16_t size);

	protocol_parce_status_enum (*get_data_from_package)(void* protocol, uint8_t* buf, uint32_t size, ubi4_bus_package_info_struct* pack_info_out, uint8_t** buf_out, uint32_t* len);

	void (*receive_to_current_device_func)(void* protocol_str, uint8_t* buf, uint32_t size);
	void (*receive_to_another_device_func)(void* protocol_str, uint8_t* buf, uint32_t size);
	void (*protocol_answer_func)(void* protocol_str,void* pack_info, uint8_t* data_buf, uint16_t size);
	void (*protocol_answer_obj_data_func)(void* protocol_str,void* pack_info, base_protocol_data_segment_struct* data_segment);

	void (*device_main_receiver)(void* protocol_str, protocol_receive_struct receive_str);

	void (*protocol_bridge_receive_callback)(void* protocol_str, protocol_receive_struct receive_str);

	void (*transmit_data_segment_to_master)(void* protocol_str, uint8_t command, base_protocol_data_segment_struct* data_segment);

	void (*transmit_to_slave)(void* protocol_str,uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* data_buf, uint16_t size);

	int (*request_from_slave_blocked)(void* protocol_str,uint8_t address, uint8_t command,  base_bus_transmit_settings_struct* tx_settings, uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms, ubi4_bus_package_info_struct* pack_info_out, uint8_t** buf_out, uint32_t* len);
	int (*request_handler_blocked)(void* protocol_str,uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms);

	void (*transmit_bridge_to_slave)(void* protocol_str,uint8_t address, uint8_t command, bool last_package, void* source_protocol_str, void* source_pack_info, base_protocol_data_segment_struct* data_segment);

	void (*transmit_bridge_to_slave_queue)(void* protocol_str,uint8_t address, uint8_t command, bool last_package, void* source_protocol_str, void* source_pack_info, base_protocol_data_segment_struct* data_segment);

	void* (*get_specific_pack_info)(void* protocol_str, void* pack_info, pack_info_specificator_enum specificator, uint8_t* status);
	uint8_t (*get_specific_byte_pack_info)(void* protocol_str, void* pack_info, pack_info_specificator_enum specificator, uint8_t* status);

	void (*parce_preamble)(void* protocol_str, uint8_t* data, uint16_t size);

	void (*protocol_close_port_callback)(void* protocol_str);

	protocol_receive_mode_enum receive_mode;


}ubi4_bus_protocol_obj_struct;








typedef struct
{
	soft_timer_class_struct* ms_soft_timer;
	soft_timer_class_struct* us_soft_timer;
}ubi4_bus_protocol_init_struct;

typedef void (*receiver_typedef)(void* protocol_str, protocol_receive_struct receive_str);

//ubi4_bus_package_info_struct* create_package_info(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* buf, uint16_t size);
//void remove_package_info(ubi4_bus_protocol_obj_struct* protocol_str, ubi4_bus_package_info_struct* package_info);

//base_protocol_data_segment_struct* create_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, uint16_t size);
//void remove_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, base_protocol_data_segment_struct* data_segment);

void parce(uint8_t* Buf, uint16_t size,ports_enum port);
int ubi4_bus_receive_data_full_buffer(base_port_struct* port_str);
int ubi4_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str);
void protocol_tx_complete(ubi4_bus_protocol_obj_struct* protocol_str);


void ubi4_master_receiver_init(base_port_struct* port, ubi4_bus_protocol_obj_struct** protocol_obj_str);

void ubi4_remove_pack_info(ubi4_bus_protocol_obj_struct* protocol_str, ubi4_bus_package_info_struct* pack_info);

void ubi4_slave_receiver_reset(ubi4_bus_protocol_obj_struct* protocol_str);
void ubi4_slave_receiver_init(base_port_struct* port, ubi4_bus_protocol_init_struct init_struct, ubi4_bus_protocol_obj_struct** protocol_obj_str);
void ubi4_spi_slave_receiver_init(base_port_struct* port, ubi4_bus_protocol_init_struct init_struct, ubi4_bus_protocol_obj_struct** protocol_obj_str);
void ubi4_slave_receiver_start(ubi4_bus_protocol_obj_struct* protocol_obj_str);
void protocol_set_receiver(ubi4_bus_protocol_obj_struct* protocol_str, receiver_typedef receiver);


void protocol_set_bridge_callback(ubi4_bus_protocol_obj_struct* protocol_str, void (*receiver)(void* protocol_str, protocol_receive_struct receive_str));


bridge_protocol_info_struct Check_Device_Bridge_Info(void* port_device_manager, uint8_t Address);

uint8_t Get_Device_Address(uint8_t* status);
void protocol_delay(uint32_t time_ms);

#endif /* INC_A_PROTOCOL_H_ */
