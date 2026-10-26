/*
  ******************************************************************************
  * @file    p2p_protocol.c
  * @author  A.Alborov
  ******************************************************************************
 */

#include "TransportLevel/p2p_protocol.h"
#include "TransportLevel/a_protocol.h"
#include "TransportLevel/main_receiver.h"
#include "TransportLevel/data_sniffer.h"

#include "TransportLevel/ports/a_port.h"
#include "Data/data_verification.h"


#ifdef UBI_SDK_FAMILY_STM32
//#include "transport_handler.h"
#include "utils/base_soft_timer.h"
#endif

#ifdef UBI_SDK_FAMALY_NRF52
//#include "transport_handler.h"
#include "utils/nrf/soft_timer.h"
#endif

#ifdef UBI_SDK_TARGET_ESP32
#include "TransportLevel/ports/esp32/one_wire_port.h"
#include "TransportLevel/ports/esp32/usb_cdc_port.h"
#endif

#include "string.h"

static base_protocol_data_segment_struct* create_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, uint16_t size);

void p2p_hight_level_receiver_exec(ubi4_bus_protocol_obj_struct* protocol)
{
	base_port_struct* port_str = protocol->port;
	p2p_high_level_port_header* header = (p2p_high_level_port_header*)(port_str->rx_buffer);


	usb_package_info_struct* package_info = StaticConteiner_addData(protocol->package_info_conteiner, header);

	if(!package_info)
					return;


	uint8_t* Data_p = (uint8_t*)header + sizeof(p2p_high_level_port_header);

	if(header->bridge && protocol->protocol_bridge_receive_callback)
	{
		protocol_receive_struct receive_str;

		receive_str.command = header->package_code;
		receive_str.request_type = header->request_type;
		receive_str.pack_info = package_info;
		receive_str.data_buf = Data_p;
		receive_str.size = header->package_size;
	//	receive_str.wait_answer = header->waiting_answer;

		protocol->protocol_bridge_receive_callback(protocol, receive_str);

	}else
	{
		if(protocol->device_main_receiver)
		{
			protocol_receive_struct receive_str;

			receive_str.command = header->package_code;
			receive_str.request_type = header->request_type;
			receive_str.pack_info = package_info;
			receive_str.data_buf = Data_p;
			receive_str.size = header->package_size;

			protocol->device_main_receiver(protocol, receive_str);
		}
	}

}

void p2p_hight_level_to_ubi_bridge()
{

}

void p2p_hight_level_receiver(ubi4_bus_protocol_obj_struct* protocol)
{

	uint32_t receive_size = 0;
	p2p_high_level_port_header* header = 0;

	base_port_struct* port_str = protocol->port;

	//port_str->read_data_buffer(port_str,port_str->rx_buffer,port_str->rx_buffer_size,(uint32_t*)&receive_size);

	if(port_str->rx_buffer_offset == 0)
	{
		header = (p2p_high_level_port_header*)port_str->rx_buffer;

		port_str->get_receive_buffer_size(port_str, &receive_size);

		if(receive_size < sizeof(p2p_high_level_port_header))
														 return;

		header = (p2p_high_level_port_header*)(port_str->rx_buffer);


		if(receive_size == sizeof(p2p_high_level_port_header) + header->package_size)
		{
			p2p_hight_level_receiver_exec(protocol);
			port_str->wait_receive(port_str,0,sizeof(p2p_high_level_port_header));
		}else
		{
			port_str->wait_receive(port_str,receive_size, (header->package_size + sizeof(p2p_high_level_port_header))- receive_size);
		}

	}else
	{
		port_str->get_receive_buffer_size(port_str, &receive_size);


		if(receive_size == port_str->wait_rx_size)
		{
			p2p_hight_level_receiver_exec(protocol);
			port_str->wait_receive(port_str,0,sizeof(p2p_high_level_port_header));
			port_str->rx_buffer_offset = 0;

		}else
		{
			port_str->wait_receive(port_str,receive_size, (port_str->wait_rx_size - receive_size));
		}

	}
}


void p2p_hight_level_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,usb_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment)
{

		p2p_high_level_port_header* header = (p2p_high_level_port_header*)data_segment->pBuf;

		header->bridge = pack_info->header.bridge;
		header->crc = 0;
		header->package_code = pack_info->header.package_code;
		header->package_id = pack_info->header.package_id;
		header->package_size = data_segment->DataSize;
		header->request_type = pack_info->header.request_type;
		header->waiting_answer = pack_info->header.waiting_answer;


		//if(header->bridge)
			header->address = pack_info->header.address;
		//else
		//	header->address = 0;

		header->crc = getCRC((uint8_t*)header, data_segment->BufSize);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,data_segment->BufSize);

		free_heap_buffer(data_segment->heap,data_segment->heapSlot);
		StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
		StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->ID);
}

static void addDataToPackInfoConteiner(static_conteiner_struct* conteiner_obj, usb_package_info_struct* conteiner_p, p2p_high_level_port_header* inputData, uint16_t ID)
{

	conteiner_p->header.bridge = inputData->bridge;
	conteiner_p->header.package_code = inputData->package_code;
	conteiner_p->header.package_id = inputData->package_id;
	conteiner_p->header.package_size = inputData->package_size;
	conteiner_p->header.port = inputData->port;
	conteiner_p->header.waiting_answer = inputData->waiting_answer;
	conteiner_p->header.request_type = inputData->request_type;

	//if(conteiner_p->header.bridge)
		conteiner_p->header.address = inputData->address;

}

static void addDataToDataSegmentConteiner(static_conteiner_struct* conteiner_obj, base_protocol_data_segment_struct* conteiner_p, uint16_t* size_p, uint16_t ID)
{

	uint16_t size = *size_p;

	ubi4_bus_protocol_obj_struct* protocol_str = conteiner_obj->owner;


	conteiner_p->BufSize = sizeof(p2p_high_level_port_header) + size;


	conteiner_p->DataSize = size;
	conteiner_p->heap = protocol_str->heap;

	conteiner_p->heapSlot = get_heap_buffer(protocol_str->heap,conteiner_p->BufSize);
	if(conteiner_p->heapSlot == 0) return;

	p2p_high_level_port_header* header = (p2p_high_level_port_header*)conteiner_p->heapSlot->p;

	conteiner_p->pBuf = (uint8_t*)header;

	memset(conteiner_p->pBuf, 0, conteiner_p->BufSize);

	conteiner_p->pData = (uint8_t*)header + sizeof(p2p_high_level_port_header);


}


static static_conteiner_init_struct pack_info_conteiner_init =
{
		.ArraySize = 100,
		.DataSize = sizeof(usb_package_info_struct),
		.addDataCallback = 0
};

static static_conteiner_init_struct data_segment_conteiner_init =
{
		.ArraySize = 100,
		.DataSize = sizeof(ubi4_bus_package_info_struct),
		.addDataCallback = 0
};



uint8_t get_specific_byte_pack_info(ubi4_bus_protocol_obj_struct* protocol_str, usb_package_info_struct* pack_info,  pack_info_specificator_enum specificator, uint8_t* status)
{
	switch(specificator)
	{
		case PISE_BRIGE:
		{
			*status = 1;
			return pack_info->header.bridge;
			break;
		}
		case PISE_PACKAGE_CODE:
		{
			*status = 1;
			return pack_info->header.package_code;
			break;
		}
		case PISE_WAIT_ANSWER:
		{
			*status = 1;
			return pack_info->header.waiting_answer;
			break;
		}
		case PISE_PACKAGE_ID:
		{
			*status = 1;
			return pack_info->header.package_id;
			break;
		}
		case PISE_PACKAGE_CRC:
		{
			*status = 1;
			return pack_info->header.crc;
			break;
		}
		case PISE_ADDRESS:
		{
			*status = 1;
			return pack_info->header.address;
			break;
		}
		case PISE_REQUEST_TYPE:
		{
			*status = 1;
			return pack_info->header.request_type;
			break;
		}
		default:
		{
			*status = 0;
			return 0;
			break;
		}
	}





	return 0;
}

void* get_specific_pack_info(ubi4_bus_protocol_obj_struct* protocol_str, usb_package_info_struct* pack_info,  pack_info_specificator_enum specificator, uint8_t* status)
{
	switch(specificator)
	{
		case PISE_ID:
		{
			*status = 1;
			return &pack_info->ID;
			break;
		}
		case PISE_PACKAGE_CODE:
		{
			*status = 1;
			return &pack_info->header.package_code;
			break;
		}
		case PISE_PACKAGE_ID:
		{
			*status = 1;
			return &pack_info->header.package_id;
			break;
		}
		case PISE_PACKAGE_SIZE:
		{
			*status = 1;
			return &pack_info->header.package_size;
			break;
		}
		case PISE_PACKAGE_CRC:
		{
			*status = 1;
			return &pack_info->header.crc;
			break;
		}
		case PISE_ADDRESS:
		{
			*status = 1;
			return &pack_info->header.address;
			break;
		}
		default:
		{
			*status = 0;
			return 0;
			break;
		}
	}

	*status = 0;
	return 0;
}


void p2p_transmit_data_segment_to_master(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t command, base_protocol_data_segment_struct* data_segment)
{

	p2p_high_level_port_header* header = (bus_main_module_header*)data_segment->pBuf;

	header->bridge = 0;
	header->address = 0;
	header->waiting_answer = 0;
	header->package_code = command;
	header->request_type = 0;
	header->package_size = data_segment->DataSize;

	header->crc = getCRC(data_segment->pData, data_segment->DataSize);

	protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,data_segment->BufSize);

	free_heap_buffer(data_segment->heap,data_segment->heapSlot);
	StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);

}


void close_port_callback(base_port_struct* port, ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str)
	{
		if(protocol_str->protocol_close_port_callback)
			protocol_str->protocol_close_port_callback(protocol_str);
	}
}



void p2p_hight_level_protocol_init(base_port_struct* port, ubi4_bus_protocol_obj_struct** protocol_obj_str)
{
	ubi4_bus_protocol_obj_struct* protocol_obj_str_temp = (ubi4_bus_protocol_obj_struct*)malloc(sizeof(ubi4_bus_protocol_obj_struct));
	memset(protocol_obj_str_temp, 0, sizeof(ubi4_bus_protocol_obj_struct));

	protocol_obj_str_temp->port = port;
	protocol_obj_str_temp->port->receive_data = p2p_hight_level_receiver;
	protocol_obj_str_temp->port->protocol = protocol_obj_str_temp;

	protocol_obj_str_temp->protocol_answer_func = 0;
	protocol_obj_str_temp->receive_data_func = 0;
	protocol_obj_str_temp->protocol_answer_obj_data_func = p2p_hight_level_protocol_answer_obj_data;
	protocol_obj_str_temp->slave_answer_func = 0;
	protocol_obj_str_temp->transmit_to_slave = 0;
	protocol_obj_str_temp->get_data_segment_func = create_data_segment;
	protocol_obj_str_temp->remove_pack_info = ubi4_remove_pack_info;
	protocol_obj_str_temp->get_specific_pack_info = get_specific_pack_info;
	protocol_obj_str_temp->get_specific_byte_pack_info = get_specific_byte_pack_info;
	protocol_obj_str_temp->transmit_data_segment_to_master = p2p_transmit_data_segment_to_master;

	port->close_port_callback = close_port_callback;



	protocol_obj_str_temp->port->wait_receive(protocol_obj_str_temp->port,0,sizeof(p2p_high_level_port_header));

	init_sniffer_conteiner(&protocol_obj_str_temp->sniffer_conteiner, 10);

	protocol_obj_str_temp->get_data_segment_func = create_data_segment;

	pack_info_conteiner_init.addDataCallback = addDataToPackInfoConteiner;
	init_static_conteiner(&pack_info_conteiner_init, &protocol_obj_str_temp->package_info_conteiner);
	protocol_obj_str_temp->package_info_conteiner->owner = protocol_obj_str_temp;


	data_segment_conteiner_init.addDataCallback = addDataToDataSegmentConteiner;
	init_static_conteiner(&data_segment_conteiner_init, &protocol_obj_str_temp->data_segment_conteiner);
	protocol_obj_str_temp->data_segment_conteiner->owner = protocol_obj_str_temp;





	protocol_obj_str_temp->heap = create_heap(port->tx_buffer,port->tx_buffer_size, 50);


	*protocol_obj_str = protocol_obj_str_temp;
	//ubi4_slave_receiver_reset(protocol_obj_str_temp);

}


static base_protocol_data_segment_struct* create_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, uint16_t size)
{
	return (base_protocol_data_segment_struct*)StaticConteiner_addData(protocol_str->data_segment_conteiner, &size);
}
