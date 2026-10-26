/*
  ******************************************************************************
  * @file    main_receiver.c
  * @author  A.Alborov
  ******************************************************************************
 */

#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"
#include "Data/device_command_structs.h"
#include "Data/base_device_info.h"
//#include "update_fw.h"


#include "Data/data_verification.h"
#include "device_info.h"


#ifdef UBI_SDK_TARGET_ESP32
#include "utils/esp32/update_fw.h"
#include "string.h"
#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/ports/esp32/one_wire_port.h"
#include "TransportLevel/ports/esp32/usb_cdc_port.h"
#endif

#ifdef UBI_SDK_TARGET_STM32
#include "utils/stm32/update_fw.h"
//#include "Data/stm32/data_table.h"
#include "string.h"
#endif

#ifdef UBI_SDK_FAMALY_NRF52

#include "Data/data_table.h"
#include "string.h"
#endif



static custom_receiver_callback custom_main_receiver = 0;
static custom_command_selector_callback custom_command_selector = 0;
static device_custom_receiver_callback device_custom_receiver = 0;

//void DeviceCommandSelector(command_selector_struct command);

void set_device_custom_main_receiver(device_custom_receiver_callback custom_receiver)
{
	device_custom_receiver = custom_receiver;
}

void set_custom_main_receiver(custom_receiver_callback custom_receiver)
{
	custom_main_receiver = custom_receiver;
}

void set_custom_command_selector(custom_command_selector_callback custom_selector)
{
	custom_command_selector = custom_selector;
}



uint8_t Bbuf[10];
bool en_var_len_package = false;

//uint8_t tx_buf[256];

collect_header_struct header_arr[COLLECT_HEADER_SIZE];



uint8_t registr_header(bool brige, bool request_answer, uint8_t transmitter_address, uint8_t package_id, uint8_t command, ports_enum port)
{

	for(uint8_t i = 0; i<100; i++)
	{
		if(header_arr[i].header_id == 0)
		{
			memset(&header_arr[i],0,sizeof(collect_header_struct));
			header_arr[i].brige = brige;
			header_arr[i].request_answer =request_answer;
			header_arr[i].command = command;
			header_arr[i].package_id = package_id;
			header_arr[i].port = port;
			header_arr[i].transmitter_address = transmitter_address;
			header_arr[i].header_id = i+1;
			return header_arr[i].header_id;
		}
	}

	return 0;
}


collect_header_struct* get_header(uint8_t header_id)
{
	if(header_id < COLLECT_HEADER_SIZE + 1)
	{
		if(header_arr[header_id-1].header_id == header_id)
			return &header_arr[header_id-1];
		else
			return 0;
	}
	return 0;
}

void clear_header(uint8_t header_id)
{
	if(header_id < COLLECT_HEADER_SIZE + 1)
	{
		if(header_arr[header_id-1].header_id == header_id)
		{
			memset(&header_arr[header_id-1],0,sizeof(collect_header_struct));
		}
	}
}

collect_header_struct* find_first_header(bool brige, uint8_t address, uint8_t command)
{
	for(uint8_t i = 0; i<COLLECT_HEADER_SIZE; i++)
	{
		if(header_arr[i].brige == brige && header_arr[i].transmitter_address == address && header_arr[i].command == command)
		{
			return &header_arr[i];
		}
	}
	return 0;
}


void Echo_v2(ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str);

void DeviceMainReceiver(ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{
	if(receive_str.request_type == BPRT_COMMAND)
	{
		switch(receive_str.command)
		{
			case DEVICE_INFORMATION: 			DeviceInformationCommand_v2((base_device_information_struct*)device_info, protocol, receive_str); 	break;
			case DATA_MANAGER: 					DataManagerCommand_v2((base_device_information_struct*)device_info, protocol, receive_str); 		break;
			case WRITE_FW_COMMAND: 		/*		Update_FW_Receive(protocol, pInfo,command, Buf, size); 		*/		break;
			case DEVICE_ACCESS_COMMAND: 				break;
			case ECHO_COMMAND: 					Echo_v2(protocol, receive_str);										break;
			case SUB_DEVICE_MANAGER:			Sub_Device_Manager((base_device_information_struct*)device_info, protocol, receive_str);			break;
			case COMPLEX_PARAMETER_TRANSFER:  	Complex_Parameter_Transfer((base_device_information_struct*)device_info, protocol, receive_str); 	break;
			case GET_DEVICE_STATUS: 									break;
			case DATA_TRANSFER_SETTINGS:		Data_Transfer_Setting((base_device_information_struct*)device_info, protocol, receive_str);  break;

			default:
				{
					if(device_custom_receiver)
						device_custom_receiver(protocol, receive_str);

				}break;
		}
	}else if(receive_str.request_type == BPRT_PARAMETERS)
	{
		ParameterRequestHandler((base_device_information_struct*)device_info, protocol,receive_str);
	}
}

void Echo_v2(ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, receive_str.size);

	if(!data_segment) return;

	memcpy(data_segment->pData, receive_str.data_buf,data_segment->DataSize);


	if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
		protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);


}









