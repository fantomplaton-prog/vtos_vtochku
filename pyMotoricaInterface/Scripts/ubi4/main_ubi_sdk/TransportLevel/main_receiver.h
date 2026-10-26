/*
  ******************************************************************************
  * @file    main_receiver.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef INC_MAIN_RECEIVER_H_
#define INC_MAIN_RECEIVER_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "UBI_SDK.h"

#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/a_protocol.h"
#include "TransportLevel/a_protocol_struct.h"

#include "stdint.h"
#include "stdbool.h"

 typedef enum {
	DEVICE_INFORMATION 			= 1,
	DATA_MANAGER 				= 2,
	WRITE_FW_COMMAND 			= 3,
	DEVICE_ACCESS_COMMAND 		= 4,
	ECHO_COMMAND 				= 5,
	SUB_DEVICE_MANAGER 			= 6,
	GET_DEVICE_STATUS 			= 7,
	DATA_TRANSFER_SETTINGS 		= 8,
	COMPLEX_PARAMETER_TRANSFER 	= 9,
	POWER_CONTROL				= 10,
	PROTOCOL_PING				= 11,

	}Command_enum;


typedef enum {
	READ_AVAILABLE_SLOTS = 1,
	WRITE_SLOT,
	READ_DATA,
	WRITE_DATA,
	RESET_TO_FACTORY,
	SAVE_DATA,
	READ_SLOT
}DataManagerCommand_enum;

typedef enum {
	ADDRESS_REQUEST = 1,
	ADDRESS_TEST_REQUEST,
	SET_BUS_BLOKING_REGISTR,
	GET_BUS_BLOKING_REGISTR,
	SET_NEW_DEVICE_ADDRESS
}DeviceAccessCommand_enum;

typedef enum {
GET_ALL_SUB_DEVICE = 1,
ADD_SUB_DEVICE,
REMOVE_SUB_DEVICE
}DevicesManagerCommand_enum;


typedef enum {

	INICIALIZE_INFORMATION 						= 1,
	READ_DEVICE_PARAMETERS 						= 2,
	READ_DEVICE_ADDITIONAL_PARAMETER			= 3,
	READ_DEVICE_ADDITIONAL_PARAMETER_SIZE		= 16,
	
	READ_SUB_DEVICES_FIRST_INFO					= 4,
	READ_SUB_DEVICE_INFO 						= 5,
	READ_SUB_DEVICE_PARAMETERS					= 6,
	READ_SUB_DEVICE_ADDITIONAL_PARAMETER		= 7,
	READ_SUB_DEVICE_ADDITIONAL_PARAMETER_SIZE	= 17,
	SUB_DEVICE_PARAMETER_INIT_READ				= 8,
	SUB_DEVICE_PARAMETER_INIT_WRITE				= 9,

	GET_SERIAL_NUMBER							= 10,
	SET_SERIAL_NUMBER							= 11,

	GET_DEVICE_NAME								= 12,
	SET_DEVICE_NAME								= 13,

	GET_DEVICE_ROLE								= 14,
	SET_DEVICE_ROLE								= 15

}DeviceInformationCommand_enum;

typedef enum {

	DTSE_START_PARAMETER_STREAM = 1,
	DTSE_STOP_PARAMETER_STREAM,
	DTSE_PROTOCOL_PREAMBLE,
	DTSE_ADD_PARAMETER_STREAM_FILTER

}DataTransferSetting_enum;



typedef enum {

	SET_BRIGE_MODE = 1,


}DataTransferSettingsCommand_enum;


typedef struct
{
	Command_enum command;
	uint8_t* data_buf;
	uint16_t data_size;
	ports_enum port;
	uint16_t header_id;
}command_selector_struct;

typedef struct
{
	uint8_t res:6;
	uint8_t request_answer:1;
	uint8_t brige:1;
	uint8_t transmitter_address;
	uint8_t package_id;
	uint8_t header_id;
	uint8_t command;
	ports_enum port;
}__attribute__((__packed__))collect_header_struct;






typedef bool (* custom_receiver_callback)(uint8_t* Buf, uint16_t size,  ports_enum port);
typedef bool (* custom_command_selector_callback)(command_selector_struct command);
typedef void (* device_custom_receiver_callback)(ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);


#define COLLECT_HEADER_SIZE 100
extern collect_header_struct header_arr[COLLECT_HEADER_SIZE];

void DeviceMainReceiver(ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);
void MainReceiver(uint8_t* Buf, uint16_t size, ports_enum port);


void CommandSelector(command_selector_struct command);


uint8_t registr_header(bool brige, bool request_answer, uint8_t transmitter_address, uint8_t package_id, uint8_t command, ports_enum port);
collect_header_struct* find_first_header(bool brige, uint8_t address, uint8_t command);

void set_custom_main_receiver(custom_receiver_callback custom_receiver);
void set_custom_command_selector(custom_command_selector_callback custom_selector);
void set_device_custom_main_receiver(device_custom_receiver_callback custom_receiver);

#ifdef __cplusplus
}
#endif

#endif /* INC_MAIN_RECEIVER_H_ */
