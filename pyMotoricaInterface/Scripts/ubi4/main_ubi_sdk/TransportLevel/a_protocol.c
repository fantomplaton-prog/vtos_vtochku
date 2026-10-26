/*
  ******************************************************************************
  * @file    a_protocol.c
  * @author  A.Alborov
  ******************************************************************************
 */

#include "TransportLevel/a_protocol.h"
#include "TransportLevel/main_receiver.h"
#include "TransportLevel/data_sniffer.h"
#include "device_info.h"   					// [PROBLEM] убрать зависимость от  device_info
//#include "stm32f4xx_hal.h"

#include "TransportLevel/ports/a_port.h"
#include "Data/data_verification.h"

#include "UBI_SDK.h"

//#include "TransportLevel/a_spi_protocol.h"

#ifdef UBI_SDK_FAMILY_STM32
//#include "transport_handler.h"

#endif

#ifdef UBI_SDK_FAMALY_NRF52
//#include "transport_handler.h"
#include "utils/base_soft_timer.h"
#endif

#ifdef UBI_SDK_TARGET_ESP32
#include "TransportLevel/ports/esp32/one_wire_port.h"
#include "TransportLevel/ports/esp32/usb_cdc_port.h"
#include "TransportLevel/ports/esp32/spi_master_port.h"
#endif

uint32_t crc_error = 0; //(temp)

//#include "main.h"
#include "string.h"
#include "stdlib.h"

//extern UART_HandleTypeDef huart1;

void ubi4_bus_protocol_answer(ubi4_bus_protocol_obj_struct* protocol_str, ubi4_bus_package_info_struct* pacckage_info, uint8_t* dataBuf, uint16_t dataSize);
void ubi4_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment);

void full_duplex_answer_task(soft_timer_class_struct* soft_timer_obj, int instance, ubi4_bus_protocol_obj_struct* protocol_str);
int ubi4_full_duplex_bus_receive_data_rx_irq_exec_new(ubi4_bus_protocol_obj_struct* protocol_str);



void ubi4_receive_to_current_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize);
void ubi4_receive_to_another_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize);
void ubi4_spi_receive_to_another_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize);
static base_protocol_data_segment_struct* create_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, uint16_t size);

void answer_task_start(ubi4_bus_protocol_obj_struct* protocol_str);
void answer_task(soft_timer_class_struct* soft_timer_obj, int instance,ubi4_bus_protocol_obj_struct* protocol_str);
void ubi4_bridge_receive(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t* buf, uint32_t size);


int ubi4_full_duplex_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str);
int ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str);
int ubi4_spi_full_duplex_bus_receive_data_full_buffer_exec(ubi4_bus_protocol_obj_struct* protocol_str);

void ubi4_full_duplex_bus_receive_to_current_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize);
void ubi4_full_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment);
void ubi4_half_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment);
void ubi4_spi_slave_full_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment);
#ifdef USB_PORT_EN
void parceUSB(uint8_t* Buf, uint16_t size,ports_enum port);
#endif

#ifdef ONE_WIRE_PORT_EN
void OneWireParce(uint8_t* dataBuf, uint16_t dataLen, ports_enum port);
#endif


void ubi4_bus_base_slave_answer(ubi4_bus_protocol_obj_struct* protocol, uint8_t* buf, uint32_t size)
{

	//ubi4_bus_package_info_struct* package_info = create_package_info(protocol, buf, size);

	//	if(!package_info)
	//					return;

	bus_main_module_header* bus_header = (bus_main_module_header*)buf;


	if(bus_header->size_type == BPPT_FIX_LEN_PACKAGE)
		exec_sniffer_conteiner(protocol->sniffer_conteiner, protocol, 0, bus_header->addr.full_address, bus_header->package_code, bus_header->Data.Data, 6);
	else if(bus_header->size_type == BPPT_FLOAT_LEN_PACKAGE)
		exec_sniffer_conteiner(protocol->sniffer_conteiner, protocol, 0, bus_header->addr.full_address, bus_header->package_code, buf + sizeof(bus_main_module_header), size);
}


protocol_parce_status_enum ubi4_bus_get_data_from_package(ubi4_bus_protocol_obj_struct* protocol, uint8_t* buf, uint32_t size, ubi4_bus_package_info_struct* pack_info_out, uint8_t** buf_out, uint32_t* len)
{

	if(size< sizeof(bus_main_module_header))
	{
		*len = 0;
		return PPSE_SHORT_PACKAGE;
	}


	bus_main_module_header* bus_header = (bus_main_module_header*)buf;

	if(getCRC((uint8_t*)bus_header, 9) != bus_header->crc)
	{
		*len = 0;
		return PPSE_CRC_FAIL;
	}

	pack_info_out->request = bus_header->request;
	pack_info_out->bridge = bus_header->bridge;
	pack_info_out->exist_uid = bus_header->exist_uid_or_param_com_size;
	pack_info_out->size_type = bus_header->size_type;
	pack_info_out->request_type = bus_header->request_type;
	pack_info_out->waiting_answer = bus_header->waiting_answer;
	pack_info_out->last_package = bus_header->last_package;

	pack_info_out->addr.full_address = bus_header->addr.full_address;
	pack_info_out->package_code = bus_header->package_code;

	if(pack_info_out->size_type == BPPT_FIX_LEN_PACKAGE)
	{
		pack_info_out->DataSize = 6;
		*buf_out = bus_header->Data.Data;
		*len = 6;
	}else
	{
		pack_info_out->DataSize = bus_header->Data.var_len_header.size;
		*buf_out = buf + sizeof(bus_main_module_header);
		*len = bus_header->Data.var_len_header.size;
	}

	return PPSE_PARCE_OK;
}


int ubi4_bus_receive_data_full_buffer_exec(ubi4_bus_protocol_obj_struct* protocol)
{
	int receive_size = 0;
	int processed_len = 0;
	bus_main_module_header* bus_header = 0;


	base_port_struct* port_str = protocol->port;

	//if(device_manager->start_scheduller == 0)
	port_str->read_data_buffer(port_str,port_str->rx_buffer,port_str->rx_buffer_size,(uint32_t*)&receive_size);

	if(receive_size < sizeof(bus_main_module_header))
												 return 0;

	int number_of_bytes_processed = 0;
	while((receive_size-number_of_bytes_processed) >= sizeof(bus_main_module_header))
	{
		bus_header = (bus_main_module_header*)(port_str->rx_buffer + number_of_bytes_processed);

		if(getCRC((uint8_t*)bus_header, 9) != bus_header->crc)
		{
			number_of_bytes_processed++;
			crc_error++;
			return 0;
			//continue;
		}

		/*
		if(bus_header->addr.full_address == 0xFF)
		{
			number_of_bytes_processed +=sizeof(bus_main_module_header);
			continue;
		}
		*/

		if(bus_header->size_type == BPPT_FIX_LEN_PACKAGE)
		{
			if(bus_header->bridge)
			{
				if(protocol->receive_bridge_func)
				{
					protocol->receive_bridge_func(protocol,(uint8_t*)bus_header,sizeof(bus_main_module_header));
					//port_str->receive_bridge_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header));
					protocol->port->processing_data(protocol->port,sizeof(bus_main_module_header));
					number_of_bytes_processed+=sizeof(bus_main_module_header);
				}
			}else
			{
				if(bus_header->request)
				{
					if(port_str->receive_to_current_device_func)
					{
						port_str->receive_to_current_device_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header));
						protocol->port->processing_data(protocol->port,sizeof(bus_main_module_header));
						number_of_bytes_processed+=sizeof(bus_main_module_header);
					}
				}else
				{
					if(port_str->slave_answer_func)
					{
						port_str->slave_answer_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header));
						protocol->port->processing_data(protocol->port,sizeof(bus_main_module_header));
						number_of_bytes_processed+=sizeof(bus_main_module_header);
					}
				}
			}
		}else if(bus_header->size_type == BPPT_FLOAT_LEN_PACKAGE)
		{
			if(bus_header->bridge)
			{
				if(port_str->receive_bridge_func)
				{
					//port_str->receive_bridge_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header));

					protocol->receive_bridge_func(protocol,(uint8_t*)bus_header,sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
					protocol->port->processing_data(protocol->port,sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
					number_of_bytes_processed+=sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size;
				}
			}else
			{
				if(bus_header->request)
				{
					if(port_str->receive_to_current_device_func)
					{
					//	port_str->receive_to_current_device_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header));
						number_of_bytes_processed+=sizeof(bus_main_module_header);
					}
				}else
				{
					if(protocol->slave_answer_func)
					{
						//protocol->slave_answer_func(protocol,(uint8_t*)bus_header,bus_header->Data.var_len_header.size);
						port_str->slave_answer_func(port_str,(uint8_t*)bus_header,sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
						protocol->port->processing_data(protocol->port,sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
						number_of_bytes_processed+=sizeof(bus_main_module_header) + bus_header->Data.var_len_header.size;
					}
				}
			}
		}
	}
		return number_of_bytes_processed;
}

int ubi4_bus_receive_data_full_buffer(base_port_struct* port_str)
{

	if(port_str != 0)
	{
		if(port_str->get_receive_buffer_size != 0)
		{
			uint32_t receive_size = 0;
			port_str->get_receive_buffer_size(port_str,&receive_size);
			if(receive_size > 0)
			{
				if(port_str->read_data_buffer != 0)
				{
					return ubi4_bus_receive_data_full_buffer_exec(port_str->protocol);
				}
			}
		}else
		{
			if(port_str->read_data_buffer != 0)
			{
				return ubi4_bus_receive_data_full_buffer_exec(port_str->protocol);
			}
		}
	}
	return 0;
}

void ubi4_slave_receiver_reset(ubi4_bus_protocol_obj_struct* protocol_str)
{
	protocol_str->buffer_offset = 0;
	protocol_str->last_package_type = BPPT_FIX_LEN_PACKAGE;
	protocol_str->rx_package_counter = 0;
	protocol_str->answer_queue_counter = 0;
	protocol_str->wait_answer = 0;
	protocol_str->enable_half_duplex_transmit = 0;

	if(protocol_str->port->port_type == UBI_PE_SPI_BUS)
	{
		protocol_str->receive_mode = PRME_RECEIVE_IRQ;
		protocol_str->protocol_answer_obj_data_func = ubi4_spi_slave_full_duplex_bus_protocol_answer_obj_data;
		protocol_str->port->receive_data = ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec;
	}


}



void ubi4_slave_receive_task(soft_timer_class_struct* soft_timer_obj, int timer_instance, ubi4_bus_protocol_obj_struct* protocol_str)
{

	if((protocol_str->ms_soft_timer->init_struct->GetTick() - protocol_str->last_rx_time) > 20)
	{
		ubi4_slave_receiver_reset(protocol_str);
		protocol_str->port->wait_receive(protocol_str->port,0,sizeof(bus_main_module_header));
	}

#ifdef UBI_SDK_FAMILY_STM32
	//start_instance_soft_timer(protocol_str->reset_timer_instance,100);
#endif

}

void ubi4_transmit_to_slave(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* data_buf, uint16_t size)
{


	if(size <= 6)
	{
		bus_main_module_header header;

		memset((uint8_t*)&header, 0, sizeof(bus_main_module_header));


		header.bridge = tx_settings->bridge;
		header.exist_uid_or_param_com_size = tx_settings->exist_uid_or_param_com_size;
		header.last_package = tx_settings->last_package;
		header.request = tx_settings->request;
		header.waiting_answer = tx_settings->waiting_answer;
		header.request_type = tx_settings->request_type;
		header.addr.full_address = address;
		header.package_code = command;
		header.size_type = BPPT_FIX_LEN_PACKAGE;

		if(data_buf != 0 && size>0)
			memcpy(header.Data.Data,data_buf,size);

		header.crc = getCRC((uint8_t*)&header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)&header,sizeof(bus_main_module_header));
	}else
	{
		uint8_t* tx_buf = malloc(sizeof(bus_main_module_header) + size);
		memset(tx_buf, 0, sizeof(bus_main_module_header)+size);

		if(tx_buf == 0) return;

		bus_main_module_header* header = (bus_main_module_header*)tx_buf;

		header->bridge = tx_settings->bridge;
		header->exist_uid_or_param_com_size = tx_settings->exist_uid_or_param_com_size;
		header->last_package = tx_settings->last_package;
		header->request = tx_settings->request;
		header->waiting_answer = tx_settings->waiting_answer;
		header->request_type = tx_settings->request_type;
		header->addr.full_address = address;
		header->package_code = command;
		header->size_type = BPPT_FLOAT_LEN_PACKAGE;
		header->Data.var_len_header.size = size;

		if(data_buf != 0 && size>0)
					memcpy(tx_buf+sizeof(bus_main_module_header),data_buf,size);

		header->crc = getCRC((uint8_t*)header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,sizeof(bus_main_module_header)+size);

		free(tx_buf);
	}
}

void ubi4_spi_transmit_to_slave(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* data_buf, uint16_t size)
{

	if(size <= 6)
	{
		bus_main_module_header header;

		memset((uint8_t*)&header, 0, sizeof(bus_main_module_header));


		header.bridge = tx_settings->bridge;
		header.exist_uid_or_param_com_size = tx_settings->exist_uid_or_param_com_size;
		header.last_package = tx_settings->last_package;
		header.request = tx_settings->request;
		header.waiting_answer = tx_settings->waiting_answer;
		header.request_type = tx_settings->request_type;
		header.addr.full_address = address;
		header.package_code = command;
		header.size_type = BPPT_FIX_LEN_PACKAGE;

		if(data_buf != 0 && size>0)
			memcpy(header.Data.Data,data_buf,size);

		header.crc = getCRC((uint8_t*)&header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)&header,sizeof(bus_main_module_header));
	}else
	{
		uint8_t* tx_buf = malloc(sizeof(bus_main_module_header) + size);
		memset(tx_buf, 0, sizeof(bus_main_module_header)+size);

		if(tx_buf == 0) return;

		bus_main_module_header* header = (bus_main_module_header*)tx_buf;

		header->bridge = tx_settings->bridge;
		header->exist_uid_or_param_com_size = tx_settings->exist_uid_or_param_com_size;
		header->last_package = tx_settings->last_package;
		header->request = tx_settings->request;
		header->waiting_answer = tx_settings->waiting_answer;
		header->request_type = tx_settings->request_type;
		header->addr.full_address = address;
		header->package_code = command;
		header->size_type = BPPT_FLOAT_LEN_PACKAGE;
		header->Data.var_len_header.size = size;
		header->crc = getCRC((uint8_t*)&header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,sizeof(bus_main_module_header));

		protocol_delay(1);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header + sizeof(bus_main_module_header), size);

		free(tx_buf);
	}
}




void ubi4_transmit_bridge_to_slave(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, uint8_t command, bool last_package, ubi4_bus_protocol_obj_struct* source_protocol_str, void* source_pack_info, base_protocol_data_segment_struct* data_segment)
{


	uint8_t wait_answer = 0;

	bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;


	header->bridge = 1;
	header->exist_uid_or_param_com_size = 0;
	header->last_package = last_package;
	header->request = 1;
	header->waiting_answer = 0;
	header->request_type = 0;
	header->addr.full_address = address;
	header->package_code = command;

	if(source_protocol_str)
	{
		if(source_protocol_str->get_specific_byte_pack_info)
		{
			uint8_t status = 0;
			uint8_t wait_answer = source_protocol_str->get_specific_byte_pack_info(source_protocol_str,source_pack_info, PISE_WAIT_ANSWER,&status);

			if(status)
			{
				if(wait_answer)
				{
					//ubi4_bridge_request_info_struct t_bridge_request_info;

					//t_bridge_request_info.Address = address;
					//t_bridge_request_info.command = command;
					//t_bridge_request_info.source_pack_info = source_pack_info;
					//t_bridge_request_info.source_protocol = source_protocol_str;

					//ubi4_bridge_request_info_struct* bridge_request_info = (ubi4_bridge_request_info_struct*)StaticConteiner_addData(protocol_str->bridge_request_conteiner, &t_bridge_request_info);

					header->waiting_answer = 1;
				}
			}
		}
	}

	if(wait_answer == 0)
	{
		source_protocol_str->remove_pack_info(source_protocol_str, source_pack_info);
	}


	if(data_segment->DataSize <= 6)
	{
		header->size_type = BPPT_FIX_LEN_PACKAGE;

		header->crc = getCRC((uint8_t*)header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,10);
	}else
	{
		header->size_type = BPPT_FLOAT_LEN_PACKAGE;
		header->Data.var_len_header.size = data_segment->DataSize;

		header->crc = getCRC((uint8_t*)header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header, data_segment->BufSize);
	}


	free_heap_buffer(data_segment->heap,data_segment->heapSlot);
	StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
}


void ubi4_transmit_bridge_to_slave_queue(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, uint8_t command, bool last_package, ubi4_bus_protocol_obj_struct* source_protocol_str, void* source_pack_info, base_protocol_data_segment_struct* data_segment)
{
	uint8_t wait_answer = 0;

	bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;


	header->bridge = 1;
	header->exist_uid_or_param_com_size = 0;
	header->last_package = last_package;
	header->request = 1;
	header->waiting_answer = 0;
	header->request_type = 0;
	header->addr.full_address = address;
	header->package_code = command;

	if(source_protocol_str)
	{
		if(source_protocol_str->get_specific_byte_pack_info)
		{
			uint8_t status = 0;
			uint8_t wait_answer = source_protocol_str->get_specific_byte_pack_info(source_protocol_str,source_pack_info, PISE_WAIT_ANSWER,&status);

			if(status)
				if(wait_answer)
					header->waiting_answer = 1;

			status = 0;

			uint8_t request_type = source_protocol_str->get_specific_byte_pack_info(source_protocol_str,source_pack_info, PISE_REQUEST_TYPE,&status);

			if(status)
				if(request_type)
					header->request_type = request_type;



		}
	}

	base_protocol_queue_segment_struct queue_segment;

	queue_segment.dataSegmentID = 0;
	queue_segment.data_segment = data_segment;
	queue_segment.pack_info = source_pack_info;
	queue_segment.command = command;
	queue_segment.addr.full_address = address;
	queue_segment.request_type = header->request_type;

	queue_segment.source_pack_info = source_pack_info;
	queue_segment.source_protocol_str = source_protocol_str;
	queue_segment.queue_transmit_type = QTTE_BRIDGE;
	queue_segment.waiting_answer = header->waiting_answer;



	if(wait_answer == 0)
	{
		source_protocol_str->remove_pack_info(source_protocol_str, source_pack_info);
		queue_segment.pack_info = 0;
	}


	if(data_segment->DataSize <= 6)
	{
		header->size_type = BPPT_FIX_LEN_PACKAGE;
		header->crc = getCRC((uint8_t*)header, 9);
	}else
	{
		header->size_type = BPPT_FLOAT_LEN_PACKAGE;
		header->Data.var_len_header.size = data_segment->DataSize;
		header->crc = getCRC((uint8_t*)header, 9);
	}


	StaticConteiner_addData(protocol_str->transmit_queue_conteiner, &queue_segment);
}



void ubi4_parameter_read_request(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, base_parameter_info_struct* param_info, bool last_package)
{
	bus_main_module_header header;


	header.bridge = 0;
	header.exist_uid_or_param_com_size = 0;
	header.last_package = last_package;
	header.request = 1;
	header.waiting_answer = 1;
	header.request_type = BPRT_PARAMETERS;

	header.addr.full_address = address;
	header.package_code = param_info->ID;



	if(param_info->parameter_size <= 6)
	{
		header.size_type = BPPT_FIX_LEN_PACKAGE;

		if(param_info->parameter_size < 6)
			memset(header.Data.Data, 0, 6);

		//memcpy(header.Data.Data,data_buf,size);

		header.crc = getCRC((uint8_t*)&header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)&header,10);
	}
}

void ubi4_parameter_write_request(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t address, uint8_t command, bool last_package, uint8_t* data_buf, uint16_t size)
{
	bus_main_module_header header;


	header.bridge = 0;
	header.exist_uid_or_param_com_size = 0;
	header.last_package = last_package;
	header.request = 1;
	header.waiting_answer = 1;
	header.request_type = 0;

	header.addr.full_address = address;
	header.package_code = command;

	if(size <= 6)
	{
		header.size_type = BPPT_FIX_LEN_PACKAGE;

		if(size < 6)
			memset(header.Data.Data, 0, 6);

		memcpy(header.Data.Data,data_buf,size);

		header.crc = getCRC((uint8_t*)&header, 9);

		protocol_str->port->write_direct(protocol_str->port,(uint8_t*)&header,10);
	}
}





static void addDataToPackInfoConteiner(static_conteiner_struct* conteiner_obj, ubi4_bus_package_info_struct* conteiner_p, bus_main_module_header* inputData, uint16_t ID)
{

	conteiner_p->bridge = inputData->bridge;
	conteiner_p->exist_uid = inputData->exist_uid_or_param_com_size;
	conteiner_p->last_package = inputData->last_package;
	conteiner_p->package_code = inputData->package_code;
	conteiner_p->size_type = inputData->size_type;
	conteiner_p->request_type = inputData->request_type;
	conteiner_p->request = inputData->request;
	conteiner_p->waiting_answer = inputData->waiting_answer;

	if(inputData->size_type == BPPT_FIX_LEN_PACKAGE)
	{
		conteiner_p->pData = inputData->Data.Data;
		conteiner_p->DataSize = 6;
	}else if(inputData->size_type == BPPT_FLOAT_LEN_PACKAGE)
	{
		conteiner_p->pData = ((uint8_t*)inputData)+sizeof(bus_main_module_header);
		conteiner_p->DataSize = inputData->Data.var_len_header.size;
	}
}




static void addDataToDataSegmentConteiner(static_conteiner_struct* conteiner_obj, base_protocol_data_segment_struct* conteiner_p, uint16_t* size_p, uint16_t ID)
{

	uint16_t size = *size_p;

	ubi4_bus_protocol_obj_struct* protocol_str = conteiner_obj->owner;

	if(size > 6)
		conteiner_p->BufSize = sizeof(bus_main_module_header) + size;
	else
		conteiner_p->BufSize = sizeof(bus_main_module_header);

	conteiner_p->DataSize = size;
	conteiner_p->heap = protocol_str->heap;

	conteiner_p->heapSlot = get_heap_buffer(protocol_str->heap,conteiner_p->BufSize);
	if(conteiner_p->heapSlot == 0) return;

	bus_main_module_header* header = (bus_main_module_header*)conteiner_p->heapSlot->p;

	conteiner_p->pBuf = (uint8_t*)header;

	memset(conteiner_p->pBuf, 0, conteiner_p->BufSize);

	if(size > 6)
	{
		conteiner_p->pData = (uint8_t*)header + sizeof(bus_main_module_header);
	}else{
		conteiner_p->pData = header->Data.Data;
	}
}


static static_conteiner_init_struct pack_info_conteiner_init =
{
		.ArraySize = 32,
		.DataSize = sizeof(ubi4_bus_package_info_struct),
		.addDataCallback = 0,
};

static static_conteiner_init_struct data_segment_conteiner_init =
{
		.ArraySize = 32,
		.DataSize = sizeof(base_protocol_data_segment_struct), ////????
		.addDataCallback = 0
};

static static_conteiner_init_struct transmit_queue_conteiner_init =
{
		.ArraySize = 32,
		.DataSize = sizeof(base_protocol_queue_segment_struct), ////????
		.addDataCallback = 0
};

/*
static static_conteiner_init_struct bridge_request_conteiner_init =
{
		.ArraySize = 16,
		.DataSize = sizeof(ubi4_bridge_request_info_struct),
		.addDataCallback = 0
};
*/


int base_request_from_slave_blocked(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t address, uint8_t command,  base_bus_transmit_settings_struct* tx_settings, uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms, ubi4_bus_package_info_struct* pack_info_out, uint8_t** buf_out, uint32_t* len)
{

	tx_settings->exist_uid_or_param_com_size = 0;
	tx_settings->last_package = 1;
	tx_settings->waiting_answer = 1;
	tx_settings->request = 1;


	protocol_str->transmit_to_slave(protocol_str, address, command, tx_settings ,tx_data_buf, tx_size);

	uint32_t protocol_wait_rx = 0;

	if(wait_rx_size != 0 )
	{
		if(wait_rx_size > 6) protocol_wait_rx = wait_rx_size + sizeof(bus_main_module_header);
		else protocol_wait_rx = sizeof(bus_main_module_header);
	}


	uint32_t rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, protocol_wait_rx, wait_time_ms);

	*len = rx_size;

	if(rx_size == 0)
			return 0;

	if(protocol_str->get_data_from_package(protocol_str, protocol_str->port->rx_buffer, rx_size, pack_info_out, buf_out, len) == PPSE_PARCE_OK)
		return 1;
	else
		return 0;
}


int spi_request_from_slave_blocked(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings,
		uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms, ubi4_bus_package_info_struct* pack_info_out, uint8_t** buf_out, uint32_t* len)
{
	tx_settings->exist_uid_or_param_com_size = 0;
	tx_settings->last_package = 1;
	tx_settings->waiting_answer = 1;
	tx_settings->request = 1;

	protocol_str->transmit_to_slave(protocol_str, address, command, tx_settings ,tx_data_buf, tx_size);

	protocol_delay(1);

	uint32_t protocol_wait_rx = 0;

	if(wait_rx_size != 0 )
	{
		if(wait_rx_size > 6) protocol_wait_rx = wait_rx_size + sizeof(bus_main_module_header);
		else protocol_wait_rx = sizeof(bus_main_module_header);
	}


	uint32_t rx_size = 0;

	if(protocol_wait_rx > 0)
	{
		rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, protocol_wait_rx, wait_time_ms);


		if(rx_size == 0)
					return 0;

		protocol_str->get_data_from_package(protocol_str, protocol_str->port->rx_buffer, rx_size, pack_info_out, buf_out, len);

		if(*len>0)return 1;
		else return 0;

	}else
	{
		rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, 10, wait_time_ms);

		protocol_str->get_data_from_package(protocol_str, protocol_str->port->rx_buffer, rx_size, pack_info_out, buf_out, len);

		if(pack_info_out->size_type  == BPPT_FIX_LEN_PACKAGE)
		{
			if(*len>0)return 1;
			else return 0;

		}else if(pack_info_out->size_type  == BPPT_FLOAT_LEN_PACKAGE)
		{

			rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 10, pack_info_out->DataSize, wait_time_ms);

			protocol_str->get_data_from_package(protocol_str, protocol_str->port->rx_buffer, rx_size, pack_info_out, buf_out, len);

			if(*len>0)return 1;
			else return 0;
		}
	}

	return 1;
}

int base_request_handler_blocked(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms)
{
	tx_settings->exist_uid_or_param_com_size = 0;
	tx_settings->last_package = 1;
	tx_settings->request = 1;

	protocol_str->transmit_to_slave(protocol_str, address, command, tx_settings ,tx_data_buf, tx_size);


	if(tx_settings->waiting_answer == 0) return 1;

	uint32_t protocol_wait_rx = 0;

	if(wait_rx_size != 0 )
	{
		if(wait_rx_size > 6) protocol_wait_rx = wait_rx_size + sizeof(bus_main_module_header);
		else protocol_wait_rx = sizeof(bus_main_module_header);
	}


	uint32_t rx_size = 0;

	rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, protocol_wait_rx, wait_time_ms);

			if(rx_size == 0)
						return 0;

			protocol_str->receive_data_func(protocol_str);
	return 1;
}



int spi_request_handler_blocked(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t address, uint8_t command, base_bus_transmit_settings_struct* tx_settings, uint8_t* tx_data_buf, uint16_t tx_size, uint32_t wait_rx_size, uint32_t wait_time_ms)
{
	tx_settings->exist_uid_or_param_com_size = 0;
	tx_settings->last_package = 1;
	tx_settings->request = 1;

	protocol_str->transmit_to_slave(protocol_str, address, command, tx_settings ,tx_data_buf, tx_size);


	if(tx_settings->waiting_answer == 0) return 1;


	protocol_delay(1);

	uint32_t protocol_wait_rx = 0;

	if(wait_rx_size != 0 )
	{
		if(wait_rx_size > 6) protocol_wait_rx = wait_rx_size + sizeof(bus_main_module_header);
		else protocol_wait_rx = sizeof(bus_main_module_header);
	}


	uint32_t rx_size = 0;

	if(protocol_wait_rx > 0)
	{
		rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, protocol_wait_rx, wait_time_ms);

		if(rx_size == 0)
					return 0;

		protocol_str->receive_data_func(protocol_str);

		return 1;
	}else
	{
		ubi4_bus_package_info_struct pack_info_out;
		uint8_t* buf_out = 0;
		uint32_t len = 0;

		rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 0, 10, wait_time_ms);

		if(protocol_str->get_data_from_package(protocol_str, protocol_str->port->rx_buffer, rx_size, &pack_info_out, &buf_out, &len) != PPSE_PARCE_OK) return 0;


		if(pack_info_out.size_type  == BPPT_FIX_LEN_PACKAGE)
		{

			protocol_str->receive_data_func(protocol_str);
			return 1;

		}else if(pack_info_out.size_type  == BPPT_FLOAT_LEN_PACKAGE)
		{

			rx_size = protocol_str->port->wait_receive_time_blocked(protocol_str->port, 10, pack_info_out.DataSize, wait_time_ms);

			protocol_str->receive_data_func(protocol_str);
			return 1;
		}
	}

	return 1;
}





void ubi4_master_receiver_init(base_port_struct* port, ubi4_bus_protocol_obj_struct** protocol_obj_str)
{
	ubi4_bus_protocol_obj_struct* protocol_obj_str_temp = (ubi4_bus_protocol_obj_struct*)malloc(sizeof(ubi4_bus_protocol_obj_struct));
	memset(protocol_obj_str_temp, 0, sizeof(ubi4_bus_protocol_obj_struct));
	protocol_obj_str_temp->port = port;
	protocol_obj_str_temp->port->receive_data = ubi4_bus_receive_data_full_buffer;
	protocol_obj_str_temp->port->protocol = protocol_obj_str_temp;

	protocol_obj_str_temp->protocol_answer_func = ubi4_bus_protocol_answer;
	protocol_obj_str_temp->receive_data_func = ubi4_bus_receive_data_full_buffer_exec;
	protocol_obj_str_temp->protocol_answer_obj_data_func = ubi4_bus_protocol_answer_obj_data;
	protocol_obj_str_temp->slave_answer_func = ubi4_bus_base_slave_answer;
	protocol_obj_str_temp->transmit_bridge_to_slave = ubi4_transmit_bridge_to_slave;
	protocol_obj_str_temp->transmit_bridge_to_slave_queue = ubi4_transmit_bridge_to_slave_queue;
	protocol_obj_str_temp->receive_bridge_func = ubi4_bridge_receive;
	protocol_obj_str_temp->get_data_from_package = ubi4_bus_get_data_from_package;
	protocol_obj_str_temp->transmit_data_segment_to_master = 0;

	protocol_obj_str_temp->get_data_segment_func = create_data_segment;   // под вопросом, возможно лучше использовать статическую функцию
	protocol_obj_str_temp->remove_pack_info = ubi4_remove_pack_info;		// под вопросом, возможно лучше использовать статическую функцию


	protocol_obj_str_temp->request_from_slave_blocked = base_request_from_slave_blocked;
	protocol_obj_str_temp->request_handler_blocked = base_request_handler_blocked;
	protocol_obj_str_temp->transmit_to_slave = ubi4_transmit_to_slave;

	if(port->port_type == UBI_PE_SPI_BUS)
	{
		protocol_obj_str_temp->request_from_slave_blocked = spi_request_from_slave_blocked;
		protocol_obj_str_temp->request_handler_blocked = spi_request_handler_blocked;
		protocol_obj_str_temp->transmit_to_slave = ubi4_spi_transmit_to_slave;
	}

	protocol_obj_str_temp->port->wait_receive(protocol_obj_str_temp->port,0,10);

	init_sniffer_conteiner(&protocol_obj_str_temp->sniffer_conteiner, 10);

	pack_info_conteiner_init.addDataCallback = addDataToPackInfoConteiner;
	init_static_conteiner(&pack_info_conteiner_init, &protocol_obj_str_temp->package_info_conteiner);
	protocol_obj_str_temp->package_info_conteiner->owner = protocol_obj_str_temp;

	data_segment_conteiner_init.addDataCallback = addDataToDataSegmentConteiner;
	init_static_conteiner(&data_segment_conteiner_init, &protocol_obj_str_temp->data_segment_conteiner);
	protocol_obj_str_temp->data_segment_conteiner->owner = protocol_obj_str_temp;


	init_static_conteiner(&transmit_queue_conteiner_init, &protocol_obj_str_temp->transmit_queue_conteiner);
	protocol_obj_str_temp->transmit_queue_conteiner->owner = protocol_obj_str_temp;




	//init_static_conteiner(&bridge_request_conteiner_init, &protocol_obj_str_temp->bridge_request_conteiner);
	//protocol_obj_str_temp->bridge_request_conteiner->owner = protocol_obj_str_temp;

	protocol_obj_str_temp->heap = create_heap(port->tx_buffer,port->tx_buffer_size, 50);


	*protocol_obj_str = protocol_obj_str_temp;
	//ubi4_slave_receiver_reset(protocol_obj_str_temp);

}



void ubi4_base_parce_preamble(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* data, uint16_t size)
{

}

void ubi4_spi_parce_preamble(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* data, uint16_t size)
{
	uint16_t wait_size = *(uint16_t*)data;

	ubi4_slave_receiver_reset(protocol_str);

	StaticConteiner_clear(protocol_str->data_segment_conteiner);
	StaticConteiner_clear(protocol_str->package_info_conteiner);
	//StaticConteiner_clear(protocol_str->transmit_queue_conteiner);
	free_heap(protocol_str->heap);

	protocol_str->receive_mode = PRME_RECEIVE_FULL_BUFFER;
	protocol_str->protocol_answer_obj_data_func = ubi4_half_duplex_bus_protocol_answer_obj_data;
	protocol_str->port->receive_data = ubi4_spi_full_duplex_bus_receive_data_full_buffer_exec;
	protocol_str->port->wait_receive(protocol_str->port, 0, wait_size);
}


void ubi4_slave_receiver_init(base_port_struct* port, ubi4_bus_protocol_init_struct init_struct, ubi4_bus_protocol_obj_struct** protocol_obj_str)
{
	ubi4_bus_protocol_obj_struct* protocol_obj_str_temp = (ubi4_bus_protocol_obj_struct*)malloc(sizeof(ubi4_bus_protocol_obj_struct));
	memset(protocol_obj_str_temp, 0, sizeof(ubi4_bus_protocol_obj_struct));

	protocol_obj_str_temp->port = port;
	protocol_obj_str_temp->us_soft_timer = init_struct.us_soft_timer;
	protocol_obj_str_temp->ms_soft_timer = init_struct.ms_soft_timer;

	if(port->port_direction == UBI_PTE_FULL_DUPLEX)
	{
		protocol_obj_str_temp->protocol_answer_func = ubi4_bus_protocol_answer;
		protocol_obj_str_temp->protocol_answer_obj_data_func = ubi4_full_duplex_bus_protocol_answer_obj_data;

		protocol_obj_str_temp->receive_data_func = ubi4_full_duplex_bus_receive_data_rx_irq_exec_new;
		protocol_obj_str_temp->receive_to_current_device_func = ubi4_receive_to_current_device;
		protocol_obj_str_temp->receive_to_another_device_func = ubi4_receive_to_another_device;
		protocol_obj_str_temp->port->receive_data = ubi4_full_duplex_bus_receive_data_rx_irq_exec_new;

		protocol_obj_str_temp->wait_answer = 0;
		protocol_obj_str_temp->answer_queue_counter = 0;

	}else if(port->port_direction == UBI_PTE_HALF_DUPLEX)
	{
		protocol_obj_str_temp->protocol_answer_func = ubi4_bus_protocol_answer;
		protocol_obj_str_temp->protocol_answer_obj_data_func = ubi4_half_duplex_bus_protocol_answer_obj_data;

		protocol_obj_str_temp->receive_data_func = ubi4_full_duplex_bus_receive_data_rx_irq_exec_new;
		protocol_obj_str_temp->receive_to_current_device_func = ubi4_receive_to_current_device;
		protocol_obj_str_temp->receive_to_another_device_func = ubi4_receive_to_another_device;
		protocol_obj_str_temp->port->receive_data = ubi4_full_duplex_bus_receive_data_rx_irq_exec_new;

		protocol_obj_str_temp->wait_answer = 0;
		protocol_obj_str_temp->answer_queue_counter = 0;
	}

	protocol_obj_str_temp->receive_mode = PRME_RECEIVE_IRQ;

	protocol_obj_str_temp->enable_half_duplex_transmit = 0;
	protocol_obj_str_temp->remove_pack_info = ubi4_remove_pack_info;
	protocol_obj_str_temp->get_data_segment_func = create_data_segment;
	protocol_obj_str_temp->parce_preamble = ubi4_base_parce_preamble;

	pack_info_conteiner_init.addDataCallback = addDataToPackInfoConteiner;
	init_static_conteiner(&pack_info_conteiner_init, &protocol_obj_str_temp->package_info_conteiner);
	protocol_obj_str_temp->package_info_conteiner->owner = protocol_obj_str_temp;


	data_segment_conteiner_init.addDataCallback = addDataToDataSegmentConteiner;
	init_static_conteiner(&data_segment_conteiner_init, &protocol_obj_str_temp->data_segment_conteiner);
	protocol_obj_str_temp->data_segment_conteiner->owner = protocol_obj_str_temp;


	protocol_obj_str_temp->heap = create_heap(port->tx_buffer,port->tx_buffer_size, 50);

	*protocol_obj_str = protocol_obj_str_temp;
	ubi4_slave_receiver_reset(protocol_obj_str_temp);

//#ifdef UBI_SDK_FAMILY_STM32 UBI_SDK_FAMALY_NRF52
#if defined(UBI_SDK_FAMILY_STM32) || defined(UBI_SDK_FAMALY_NRF52)

	if(port->port_direction == UBI_PTE_FULL_DUPLEX)
	{
	    protocol_obj_str_temp->reset_timer_instance = create_soft_timer(protocol_obj_str_temp->ms_soft_timer, ubi4_slave_receive_task, protocol_obj_str_temp, true, false,SOFT_TIMER_EXEC_IRQ_MODE);
	    protocol_obj_str_temp->answer_us_timer_instance = create_soft_timer(protocol_obj_str_temp->us_soft_timer, full_duplex_answer_task, protocol_obj_str_temp, false,false,SOFT_TIMER_EXEC_IRQ_MODE);
	}else if(port->port_direction == UBI_PTE_HALF_DUPLEX)
	{
		protocol_obj_str_temp->answer_us_timer_instance = create_soft_timer(protocol_obj_str_temp->us_soft_timer, answer_task, protocol_obj_str_temp, false,false,SOFT_TIMER_EXEC_IRQ_MODE);
	}

#endif
}


void ubi4_spi_slave_receiver_init(base_port_struct* port, ubi4_bus_protocol_init_struct init_struct, ubi4_bus_protocol_obj_struct** protocol_obj_str)
{
	if(port->port_type != UBI_PE_SPI_BUS) return;

	ubi4_bus_protocol_obj_struct* protocol_obj_str_temp = (ubi4_bus_protocol_obj_struct*)malloc(sizeof(ubi4_bus_protocol_obj_struct));
	memset(protocol_obj_str_temp, 0, sizeof(ubi4_bus_protocol_obj_struct));

	protocol_obj_str_temp->port = port;
	protocol_obj_str_temp->us_soft_timer = init_struct.us_soft_timer;
	protocol_obj_str_temp->ms_soft_timer = init_struct.ms_soft_timer;


	protocol_obj_str_temp->protocol_answer_func = ubi4_bus_protocol_answer;
	protocol_obj_str_temp->protocol_answer_obj_data_func = ubi4_spi_slave_full_duplex_bus_protocol_answer_obj_data;

	protocol_obj_str_temp->receive_data_func = ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec;
	protocol_obj_str_temp->receive_to_current_device_func = ubi4_receive_to_current_device;
	protocol_obj_str_temp->receive_to_another_device_func = ubi4_spi_receive_to_another_device;
	protocol_obj_str_temp->port->receive_data = ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec;

	protocol_obj_str_temp->wait_answer = 0;
	protocol_obj_str_temp->answer_queue_counter = 0;
	protocol_obj_str_temp->receive_mode = PRME_RECEIVE_IRQ;

	protocol_obj_str_temp->enable_half_duplex_transmit = 0;
	protocol_obj_str_temp->remove_pack_info = ubi4_remove_pack_info;
	protocol_obj_str_temp->get_data_segment_func = create_data_segment;
	protocol_obj_str_temp->parce_preamble = ubi4_spi_parce_preamble;

	pack_info_conteiner_init.addDataCallback = addDataToPackInfoConteiner;
	init_static_conteiner(&pack_info_conteiner_init, &protocol_obj_str_temp->package_info_conteiner);
	protocol_obj_str_temp->package_info_conteiner->owner = protocol_obj_str_temp;


	data_segment_conteiner_init.addDataCallback = addDataToDataSegmentConteiner;
	init_static_conteiner(&data_segment_conteiner_init, &protocol_obj_str_temp->data_segment_conteiner);
	protocol_obj_str_temp->data_segment_conteiner->owner = protocol_obj_str_temp;


	protocol_obj_str_temp->heap = create_heap(port->tx_buffer,port->tx_buffer_size, 50);

	*protocol_obj_str = protocol_obj_str_temp;
	ubi4_slave_receiver_reset(protocol_obj_str_temp);



#if defined(UBI_SDK_FAMILY_STM32) || defined(UBI_SDK_FAMALY_NRF52)
	 protocol_obj_str_temp->reset_timer_instance = create_soft_timer(protocol_obj_str_temp->ms_soft_timer, ubi4_slave_receive_task, protocol_obj_str_temp, true, false,SOFT_TIMER_EXEC_IRQ_MODE);
#endif


}




void protocol_set_receiver(ubi4_bus_protocol_obj_struct* protocol_str, receiver_typedef receiver)
{
	protocol_str->device_main_receiver = receiver;
}


void protocol_set_bridge_callback(ubi4_bus_protocol_obj_struct* protocol_str, void (*receiver)(void* protocol_str, protocol_receive_struct receive_str))
{
	protocol_str->protocol_bridge_receive_callback = receiver;
}


void ubi4_spi_slave_receiver_start(ubi4_bus_protocol_obj_struct* protocol_str)
{
	protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();
	protocol_str->port->wait_receive(protocol_str->port,0,sizeof(bus_main_module_header));
}

void ubi4_slave_receiver_start(ubi4_bus_protocol_obj_struct* protocol_str)
{
	ubi4_slave_receiver_reset(protocol_str);

	if(protocol_str->port->port_type != UBI_PE_SPI_BUS)
	{
		protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();

#ifdef UBI_SDK_FAMILY_STM32
		if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX && protocol_str->port->port_type != UBI_PE_SPI_BUS)
			start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif

    	protocol_str->port->wait_receive(protocol_str->port,0,sizeof(bus_main_module_header));
	}else if(protocol_str->port->port_type == UBI_PE_SPI_BUS)
	{
#ifdef UBI_SDK_FAMILY_STM32
		start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif
		ubi4_spi_slave_receiver_start(protocol_str);
	}
}

void ubi4_parce_fix_len_package()
{

}

base_protocol_data_segment_struct* ubi4_get_data_segment(ubi4_bus_protocol_obj_struct* protocol_str,uint16_t size)
{

	return 0;
}

void ubi4_bridge_receive(ubi4_bus_protocol_obj_struct* protocol_str,uint8_t* buf, uint32_t size)
{


	bus_main_module_header* header = (bus_main_module_header*)buf;

//	if(protocol_str->check_device_bridge_info)
//	{
	//	bridge_protocol_info_struct device_bridge_info = protocol_str->check_device_bridge_info(protocol_str->port_device_manager, header->addr.full_address);

	bridge_protocol_info_struct device_bridge_info = Check_Device_Bridge_Info(protocol_str->port_device_manager, header->addr.full_address);

		if(device_bridge_info.status)
		{

			ubi4_bus_protocol_obj_struct* source_protocol = (ubi4_bus_protocol_obj_struct*)device_bridge_info.last_bridge_protocol;

			uint16_t d_size = 0;;
			uint8_t* dp = 0;

			if(header->size_type == BPPT_FIX_LEN_PACKAGE)
			{
				d_size = 6;
				dp = header->Data.Data;
			}
			else if(header->size_type ==  BPPT_FLOAT_LEN_PACKAGE)
			{
				d_size = header->Data.var_len_header.size;
				dp = buf+sizeof(bus_main_module_header);
			}


			base_protocol_data_segment_struct* data_segment = source_protocol->get_data_segment_func(source_protocol, d_size);

			if(!data_segment) return;

			memcpy(data_segment->pData, dp,data_segment->DataSize);


			if(source_protocol->protocol_answer_obj_data_func && device_bridge_info.last_bridge_pack_info && data_segment)
				source_protocol->protocol_answer_obj_data_func(source_protocol, device_bridge_info.last_bridge_pack_info, data_segment);


	//	}

	}

	/*
	if(protocol_str->bridge_request_conteiner->Counter)
	{


		uint8_t seg_iterator = 0;

		for(uint16_t i = 0; i<protocol_str->bridge_request_conteiner->init_struct->ArraySize; i++ )
		{
			ubi4_bridge_request_info_struct* bridge_request_info = StaticConteiner_getSegment(protocol_str->bridge_request_conteiner, i+1);

			if(bridge_request_info->pack_info_ID != 0)
			{
				seg_iterator++;

				bus_main_module_header* header = (bus_main_module_header*)buf;

				if(bridge_request_info->Address == header->addr.full_address && bridge_request_info->command == header->package_code)
				{
					ubi4_bus_protocol_obj_struct* source_protocol = (ubi4_bus_protocol_obj_struct*)bridge_request_info->source_protocol;


					uint16_t d_size = 0;;
					uint8_t* dp = 0;

					if(header->size_type == BPPT_FIX_LEN_PACKAGE)
					{
						d_size = 6;
						dp = header->Data.Data;
					}
					else if(header->size_type ==  BPPT_FLOAT_LEN_PACKAGE)
					{
						d_size = header->Data.var_len_header.size;
						dp = buf+sizeof(bus_main_module_header);
					}


					base_protocol_data_segment_struct* data_segment = source_protocol->get_data_segment_func(source_protocol, d_size);

					if(!data_segment) return;

					memcpy(data_segment->pData, dp,data_segment->DataSize);


					if(source_protocol->protocol_answer_obj_data_func && bridge_request_info->source_pack_info && data_segment)
						source_protocol->protocol_answer_obj_data_func(source_protocol, bridge_request_info->source_pack_info, data_segment);

					StaticConteiner_removeData(protocol_str->bridge_request_conteiner, bridge_request_info->pack_info_ID);

					if(seg_iterator >= protocol_str->bridge_request_conteiner->Counter) break;


				}
			}
		}



	}
	*/

}


void ubi4_full_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment)
{
	if(!pack_info->waiting_answer)
	{
		free_heap_buffer(data_segment->heap,data_segment->heapSlot);
		StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
		StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
		return;
	}


		bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;

		header->bridge = pack_info->bridge;
		header->exist_uid_or_param_com_size = pack_info->exist_uid;
		header->waiting_answer = 0;
		header->request = 0;
		header->last_package = 0;
		header->package_code = pack_info->package_code;
		header->request_type = pack_info->request_type;

		pack_info->answer_data_segment = data_segment;


		uint8_t a_status = 0;
		header->addr.full_address = Get_Device_Address(&a_status);
		if(a_status == 0) return;


		if(data_segment->DataSize > 6)
		{
			header->size_type = PACKAGE_TYPE_VAR_LEN;
			header->Data.var_len_header.size = data_segment->DataSize;
			header->crc = getCRC((uint8_t*)header, 9);
		}else
		{
			header->size_type = PACKAGE_TYPE_FIX_LEN;
			header->crc = getCRC((uint8_t*)header, 9);
		}


		if(1|| protocol_str->answer_queue_counter  == 0)
		{


		    for (volatile uint16_t i = 0; i < protocol_str->answer_queue_counter * CPU_FREQ_MHZ * 5; i++) {};



			protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,data_segment->BufSize);
			if (data_segment->data_tx_callback)
			{
			    data_segment->data_tx_callback(data_segment->object);
			}
			free_heap_buffer(data_segment->heap,data_segment->heapSlot);
			StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
			StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
            protocol_str->answer_queue_counter = 0;

		}else
		{

			if(!soft_timer_is_run(protocol_str->us_soft_timer, protocol_str->answer_us_timer_instance))
						start_soft_timer(protocol_str->us_soft_timer, protocol_str->answer_us_timer_instance,20*(protocol_str->answer_queue_counter));

		}

}

void ubi4_spi_slave_full_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment)
{
	if(!pack_info->waiting_answer)
	{
		free_heap_buffer(data_segment->heap,data_segment->heapSlot);
		StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
		StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
		return;
	}


		bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;

		header->bridge = pack_info->bridge;
		header->exist_uid_or_param_com_size = pack_info->exist_uid;
		header->waiting_answer = 0;
		header->request = 0;
		header->last_package = 0;
		header->package_code = pack_info->package_code;
		header->request_type = pack_info->request_type;


		uint8_t a_status = 0;
		header->addr.full_address = Get_Device_Address(&a_status);
		if(a_status == 0) return;


		if(data_segment->DataSize > 6)
		{
			header->size_type = PACKAGE_TYPE_VAR_LEN;
			header->Data.var_len_header.size = data_segment->DataSize;
			header->crc = getCRC((uint8_t*)header, 9);
		}else
		{
			header->size_type = PACKAGE_TYPE_FIX_LEN;
			header->crc = getCRC((uint8_t*)header, 9);
		}


			protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,data_segment->BufSize);

			if (data_segment->data_tx_callback)
			{
			    data_segment->data_tx_callback(data_segment->object);
			  //  data_segment->data_tx_callback = 0;
			}
			free_heap_buffer(data_segment->heap,data_segment->heapSlot);
			StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
			StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);


}




void ubi4_half_duplex_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment)
{
	if(!pack_info->waiting_answer)
	{
		free_heap_buffer(data_segment->heap,data_segment->heapSlot);
		StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
		StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
		return;
	}


		bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;

		header->bridge = pack_info->bridge;
		header->exist_uid_or_param_com_size = pack_info->exist_uid;
		header->waiting_answer = 0;
		header->request = 0;
		header->last_package = 0;
		header->package_code = pack_info->package_code;
		header->request_type = pack_info->request_type;



		uint8_t a_status = 0;
		header->addr.full_address = Get_Device_Address(&a_status);
		if(a_status == 0) return;


		if(data_segment->DataSize > 6)
		{
			header->size_type = PACKAGE_TYPE_VAR_LEN;
			header->Data.var_len_header.size = data_segment->DataSize;
			header->crc = getCRC((uint8_t*)header, 9);
		}else
		{
			header->size_type = PACKAGE_TYPE_FIX_LEN;
			header->crc = getCRC((uint8_t*)header, 9);
		}

		pack_info->answer_data_segment = data_segment;

		if(pack_info->last_package && protocol_str->answer_queue_counter == 0)
		{

			pack_info->answer_data_segment = data_segment;
			protocol_str->wait_answer = 1;


		}else
		{
			pack_info->answer_data_segment = data_segment;

			protocol_str->wait_answer = 1;
		}

		/*
		if(data_segment->data_tx_callback)
		{
			data_segment->data_tx_callback(data_segment->object);

			data_segment->data_tx_callback = 0;
		}
		*/


}



void ubi4_bus_protocol_answer_obj_data(ubi4_bus_protocol_obj_struct* protocol_str,ubi4_bus_package_info_struct* pack_info, base_protocol_data_segment_struct* data_segment)
{

	if(!pack_info->waiting_answer)
	{
		free_heap_buffer(data_segment->heap,data_segment->heapSlot);
		StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
		StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
		return;
	}




		bus_main_module_header* header = (bus_main_module_header*)data_segment->pBuf;

		header->bridge = pack_info->bridge;
		header->exist_uid_or_param_com_size = pack_info->exist_uid;
		header->waiting_answer = 0;
		header->request = 0;
		header->last_package = 0;
		header->package_code = pack_info->package_code;
		header->request_type = pack_info->request_type;

		uint8_t a_status = 0;
		header->addr.full_address = Get_Device_Address(&a_status);
		if(a_status == 0) return;

		if(data_segment->DataSize > 6)
		{
			header->size_type = PACKAGE_TYPE_VAR_LEN;
			header->Data.var_len_header.size = data_segment->DataSize;
			header->crc = getCRC((uint8_t*)header, 9);
		}else
		{
			header->size_type = PACKAGE_TYPE_FIX_LEN;
			header->crc = getCRC((uint8_t*)header, 9);
		}




		if(pack_info->last_package && protocol_str->answer_queue_counter == 0)
		{
			protocol_str->port->write_direct(protocol_str->port,(uint8_t*)header,data_segment->BufSize);


			free_heap_buffer(data_segment->heap,data_segment->heapSlot);
			StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
			StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);

		}else
		{
			pack_info->answer_data_segment = data_segment;

			protocol_str->wait_answer = 1;
		}
}

void ubi4_bus_protocol_answer(ubi4_bus_protocol_obj_struct* protocol_str, ubi4_bus_package_info_struct* pacckage_info, uint8_t* dataBuf, uint16_t dataSize)
{
	//protocol_str->port->write_direct
}

void ubi4_receive_to_another_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize)
{

}

void ubi4_spi_receive_to_another_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize)
{
	bus_main_module_header* header = (bus_main_module_header*)dataBuf;

	if(header->waiting_answer)
	{
		custom_heap_slot_struct* heap_slot = get_heap_buffer(protocol_str->heap, sizeof(bus_main_module_header));
		memset(heap_slot->p, 0xFF, heap_slot->size);
	}

}


void ubi4_remove_pack_info(ubi4_bus_protocol_obj_struct* protocol_str, ubi4_bus_package_info_struct* pack_info)
{
	StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
}

void ubi4_receive_to_current_device(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t* dataBuf, uint16_t dataSize)
{
	 ubi4_bus_package_info_struct* package_info = StaticConteiner_addData(protocol_str->package_info_conteiner, dataBuf);



	if(!package_info)
					return;

	package_info->request_queue_pos = protocol_str->answer_queue_counter;

	if(protocol_str->device_main_receiver)
	{
		protocol_receive_struct receive_str;

		receive_str.command = package_info->package_code;
		receive_str.pack_info = package_info;
		receive_str.data_buf = package_info->pData;
		receive_str.size = package_info->DataSize;
		receive_str.request_type = package_info->request_type;

		protocol_str->device_main_receiver(protocol_str,receive_str);
	}


}

static base_protocol_data_segment_struct* create_data_segment(ubi4_bus_protocol_obj_struct* protocol_str, uint16_t size)
{
	return (base_protocol_data_segment_struct*)StaticConteiner_addData(protocol_str->data_segment_conteiner, &size);
}




void full_duplex_transmit_part(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str->package_info_conteiner->Counter)
	{
		ubi4_bus_package_info_struct* pack_info = (ubi4_bus_package_info_struct*)StaticConteiner_getFirstSegment(protocol_str->package_info_conteiner);

		if(pack_info)
		{
			base_protocol_data_segment_struct* data_segment = pack_info->answer_data_segment;

			if(data_segment != 0)
			{
				protocol_str->port->write_direct(protocol_str->port,(uint8_t*)protocol_str->heap->heap,protocol_str->heap->currentHeapSize);

				if(data_segment->data_tx_callback)
							data_segment->data_tx_callback(data_segment->object);

			free_heap_buffer(data_segment->heap,data_segment->heapSlot);
			StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
			}

			StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
		}
	}
}


void full_duplex_answer_task(soft_timer_class_struct* soft_timer_obj, int instance, ubi4_bus_protocol_obj_struct* protocol_str)
{
	full_duplex_transmit_part(protocol_str);

}




// WARNING: Проверить на адекватность
void answer_task(soft_timer_class_struct* soft_timer_obj, int instance,ubi4_bus_protocol_obj_struct* protocol_str)
{

	protocol_str->port->write_direct(protocol_str->port,(uint8_t*)protocol_str->heap->heap,protocol_str->heap->currentHeapSize);

	if(soft_timer_obj->init_struct->isTimerEnable && soft_timer_obj->init_struct->StopTickTimer && soft_timer_obj->init_struct->StartTickTimer)
	{
		soft_timer_obj->init_struct->StopTickTimer();
	}

	StaticConteiner_clear(protocol_str->data_segment_conteiner);
	StaticConteiner_clear(protocol_str->package_info_conteiner);
	free_heap(protocol_str->heap);

	if(protocol_str->package_info_conteiner->Counter > 0)
		protocol_str->enable_half_duplex_transmit = 1;
	else
		protocol_str->enable_half_duplex_transmit = 0;
}



void protocol_tx_complete(ubi4_bus_protocol_obj_struct* protocol_str)
{

	if(protocol_str->port->port_type == UBI_PE_UART_BUS)
	{
		full_duplex_transmit_part(protocol_str);

	}else if(protocol_str->port->port_type == UBI_PE_ONE_WIRE_BUS)
	{
		if(protocol_str->enable_half_duplex_transmit)
		{
			answer_task_start(protocol_str);
		}else
		{
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
		}
	}else if(protocol_str->port->port_type == UBI_PE_SPI_BUS)
	{
		if(protocol_str->receive_mode == PRME_RECEIVE_FULL_BUFFER)
		{



			while(protocol_str->package_info_conteiner->Counter)
			{
					ubi4_bus_package_info_struct* pack_info = (ubi4_bus_package_info_struct*)StaticConteiner_getFirstSegment(protocol_str->package_info_conteiner);

					if(pack_info)
					{
						base_protocol_data_segment_struct* data_segment = pack_info->answer_data_segment;
						if(data_segment != 0)
						{
							if(data_segment->data_tx_callback)
										data_segment->data_tx_callback(data_segment->object);

						free_heap_buffer(data_segment->heap,data_segment->heapSlot);
						StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
						}
						StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);
					}
			}


			StaticConteiner_clear(protocol_str->data_segment_conteiner);
			StaticConteiner_clear(protocol_str->package_info_conteiner);
			free_heap(protocol_str->heap);

			protocol_str->receive_mode = PRME_RECEIVE_IRQ;

			protocol_str->protocol_answer_obj_data_func = ubi4_spi_slave_full_duplex_bus_protocol_answer_obj_data;
			protocol_str->port->receive_data = ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec;

			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port, 0, sizeof(bus_main_module_header));

		}else if(protocol_str->receive_mode == PRME_RECEIVE_IRQ)
		{
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port, 0 ,sizeof(bus_main_module_header));
		}

	}
}







//Переписать процесс ответа
// Данный процесс не учитывает вероятности пробелов между пакетами от одного устройства



void answer_task_start(ubi4_bus_protocol_obj_struct* protocol_str)
{

	if(protocol_str->package_info_conteiner->Counter > 0)
	{
		ubi4_bus_package_info_struct* pack_info = (ubi4_bus_package_info_struct*)StaticConteiner_getFirstSegment(protocol_str->package_info_conteiner);

		if(pack_info)
		{

			if(pack_info->request_queue_pos == 0)
			{

				base_protocol_data_segment_struct* data_segment = pack_info->answer_data_segment;

				if(data_segment != 0)
				{
				protocol_str->port->write_direct(protocol_str->port,data_segment->pBuf,data_segment->BufSize);


				if(data_segment->data_tx_callback)
							data_segment->data_tx_callback(data_segment->object);

				free_heap_buffer(data_segment->heap,data_segment->heapSlot);
				StaticConteiner_removeData(protocol_str->data_segment_conteiner, data_segment->dataSegmentID);
				}

				StaticConteiner_removeData(protocol_str->package_info_conteiner, pack_info->pack_info_ID);

				if(protocol_str->package_info_conteiner->Counter > 0)
									protocol_str->enable_half_duplex_transmit = 1;
				else
					protocol_str->enable_half_duplex_transmit = 0;


			}else
				start_soft_timer(protocol_str->us_soft_timer, protocol_str->answer_us_timer_instance,90*protocol_str->answer_queue_counter);
		}else
		{
			protocol_str->enable_half_duplex_transmit = 0;
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
		}


	}else
	{
		ubi4_slave_receiver_reset(protocol_str);
		protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
	}

}

extern uint8_t debug_action_request_array[4];
extern uint8_t debug_action_request_counter;

void spi_answer_task_start(ubi4_bus_protocol_obj_struct* protocol_str)
{

	if(protocol_str->package_info_conteiner->Counter > 0)
	{
		protocol_str->port->write_direct(protocol_str->port,protocol_str->heap->heap,protocol_str->heap->currentHeapSize);
	}
}



int ubi4_full_duplex_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str != 0)
	{
		int receive_size = 0;
		bus_main_module_header* bus_header = 0;

#if defined(UBI_SDK_FAMILY_STM32) || defined(UBI_SDK_FAMALY_NRF52)
//	start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif



		protocol_str->port->read_data_buffer(protocol_str->port,protocol_str->port->rx_buffer + protocol_str->buffer_offset, protocol_str->port->rx_buffer_size,(uint32_t*)&receive_size);



		if(receive_size == 0)
		{
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
			return 0;
		}

		switch(protocol_str->last_package_type)
		{
			case BPPT_FIX_LEN_PACKAGE:
			{

				bus_header = (bus_main_module_header*)(protocol_str->port->rx_buffer + protocol_str->buffer_offset);

				volatile uint8_t input_crc = getCRC((uint8_t*)bus_header, 9);

				if( input_crc != bus_header->crc)
				{
					ubi4_slave_receiver_reset(protocol_str);
					protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));


					return 0;
				}

#if defined(UBI_SDK_FAMILY_STM32)
				if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
				    start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif

				protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();


				switch(bus_header->size_type)
				{
					case BPPT_FIX_LEN_PACKAGE:
					{
						uint32_t current_buffer_offset = protocol_str->buffer_offset;

						uint8_t a_status = 0;
						uint8_t device_address = Get_Device_Address(&a_status);
						if(a_status == 0) return 0;

						if(bus_header->addr.full_address == device_address )//|| (bus_header->addr.full_address == 0xFF))
						{
 							if(bus_header->last_package)
 							{
 								if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
 									ubi4_slave_receiver_reset(protocol_str);
 								else if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
 									ubi4_slave_receiver_reset(protocol_str);
 							}
							else
							{
								if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
									protocol_str->buffer_offset += receive_size;
								else if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
									protocol_str->buffer_offset += receive_size;
							}

							protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

							//if(bus_header->package_code == 0x85 && bus_header->request_type == 1)
							//	protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

							protocol_str->receive_to_current_device_func(protocol_str, protocol_str->port->rx_buffer + current_buffer_offset, sizeof(bus_main_module_header));

						}else
						{
							//if(bus_header->last_package)
							if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
							{
								ubi4_slave_receiver_reset(protocol_str);
								protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							}else if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
							{
								if(bus_header->waiting_answer)
											protocol_str->answer_queue_counter++;

								ubi4_slave_receiver_reset(protocol_str);
								protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							}
						}

						//ubi4_slave_receiver_reset(protocol_str);
						//protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

						if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
						{
							if(bus_header->last_package) answer_task_start(protocol_str);
						}

						return 0;

					}break;
					case BPPT_FLOAT_LEN_PACKAGE:
					{
						protocol_str->buffer_offset += sizeof(bus_main_module_header);
						protocol_str->var_len_package_start = (uint8_t*)bus_header;
						protocol_str->last_package_type = BPPT_FLOAT_LEN_PACKAGE;
						protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,bus_header->Data.var_len_header.size);
						return 0;


					}break;
				}

			}break;



			case BPPT_FLOAT_LEN_PACKAGE:{

				if(bus_header->last_package)
					ubi4_slave_receiver_reset(protocol_str);
				else
					protocol_str->buffer_offset += receive_size;

				protocol_str->last_package_type = BPPT_FIX_LEN_PACKAGE;
				protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

				uint8_t a_status = 0;
                uint8_t device_address = Get_Device_Address(&a_status);
                if(a_status == 0) return 0;

                if(protocol_str->var_len_package_start && ((bus_main_module_header*)protocol_str->var_len_package_start)->addr.full_address == device_address )//|| (bus_header->addr.full_address == 0xFF))
                {
                    protocol_str->receive_to_current_device_func(protocol_str, protocol_str->var_len_package_start, sizeof(bus_main_module_header));
                }


			}break;
			default: break;
		}


	}
	return 0;
}



int package_handler(ubi4_bus_protocol_obj_struct* protocol_str, uint8_t device_address, bus_main_module_header* bus_header, uint16_t size)
{

    uint8_t l_answer_counter = protocol_str->answer_queue_counter;

	if(bus_header->last_package)
		ubi4_slave_receiver_reset(protocol_str);
	else
		protocol_str->buffer_offset += size;

	protocol_str->last_package_type = BPPT_FIX_LEN_PACKAGE;
	protocol_str->port->wait_receive(protocol_str->port, protocol_str->buffer_offset, sizeof(bus_main_module_header));

	protocol_str->answer_queue_counter = l_answer_counter;

	if((bus_header->addr.full_address == device_address ) || (bus_header->addr.full_address == 0xFF))
	{

		protocol_str->receive_to_current_device_func(protocol_str, (uint8_t*)bus_header, size);

	}else
	{
		if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
		{
			if(bus_header->waiting_answer)
				protocol_str->answer_queue_counter++;
			else
				protocol_str->answer_queue_counter = 0;
		}

		if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
			if(bus_header->waiting_answer)
						protocol_str->answer_queue_counter++;
	}

	if(bus_header->last_package && protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
	                                                                    protocol_str->answer_queue_counter = 0;

	if(protocol_str->port->port_direction == UBI_PTE_HALF_DUPLEX)
		if(bus_header->last_package) answer_task_start(protocol_str);


	return 0;
}






int ubi4_full_duplex_bus_receive_data_rx_irq_exec_new(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str != 0)
	{
		int receive_size = 0;
		bus_main_module_header* bus_header = 0;


		protocol_str->port->read_data_buffer(protocol_str->port,protocol_str->port->rx_buffer + protocol_str->buffer_offset, protocol_str->port->rx_buffer_size,(uint32_t*)&receive_size);

		if(receive_size == 0)
		{
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
			return 0;
		}

		uint8_t a_status = 0;
		uint8_t device_address = Get_Device_Address(&a_status);
		if(a_status == 0) return 0;


		switch(protocol_str->last_package_type)
		{
			case BPPT_FIX_LEN_PACKAGE:
			{

				bus_header = (bus_main_module_header*)(protocol_str->port->rx_buffer + protocol_str->buffer_offset);

				volatile uint8_t input_crc = getCRC((uint8_t*)bus_header, 9);

				if( input_crc != bus_header->crc)
				{
					ubi4_slave_receiver_reset(protocol_str);
					protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
					return 0;
				}

#if defined(UBI_SDK_FAMILY_STM32)
		if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
			start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif

				protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();

				switch(bus_header->size_type)
				{
					case BPPT_FIX_LEN_PACKAGE:
					{
						package_handler(protocol_str, device_address, bus_header, sizeof(bus_main_module_header));

					}break;
					case BPPT_FLOAT_LEN_PACKAGE:
					{
						protocol_str->buffer_offset += sizeof(bus_main_module_header);
						protocol_str->var_len_package_start = (uint8_t*)bus_header;
						protocol_str->last_package_type = BPPT_FLOAT_LEN_PACKAGE;
						protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,bus_header->Data.var_len_header.size);
						return 0;

					}break;
				}

			}break;

			case BPPT_FLOAT_LEN_PACKAGE:{

				bus_header = (bus_main_module_header*)(protocol_str->var_len_package_start);

#if defined(UBI_SDK_FAMILY_STM32)
		if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
			start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif

				protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();

				package_handler(protocol_str, device_address, bus_header, sizeof(bus_main_module_header));

			}break;
			default: break;
		}
	}
	return 0;
}





int ubi4_spi_full_duplex_bus_receive_data_full_buffer_exec(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str != 0)
	{
		int receive_size = 0;
		bus_main_module_header* bus_header = 0;
		int number_of_bytes_processed = 0;
		uint8_t* data_buffer_p = protocol_str->port->rx_buffer + protocol_str->buffer_offset;

		uint8_t a_status = 0;
		uint8_t device_address = Get_Device_Address(&a_status);
		if(a_status == 0 || device_address == 0)
		{
receive_data_finish:
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->receive_mode = PRME_RECEIVE_IRQ;
			protocol_str->port->receive_data = ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec;
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
			return number_of_bytes_processed;
		}


		protocol_str->port->read_data_buffer(protocol_str->port, data_buffer_p, protocol_str->port->rx_buffer_size,(uint32_t*)&receive_size);

		if(receive_size < sizeof(bus_main_module_header))
												goto receive_data_finish;




		while(receive_size-number_of_bytes_processed >= sizeof(bus_main_module_header))
		{
			bus_header = (bus_main_module_header*)(data_buffer_p + number_of_bytes_processed);

			if(getCRC((uint8_t*)bus_header, 9) != bus_header->crc)
			{
				number_of_bytes_processed++;
				continue;
			}



			if(bus_header->addr.full_address == device_address || (bus_header->addr.full_address == 0xFF))
			{

				if( bus_header->size_type == BPPT_FIX_LEN_PACKAGE)
				{
					protocol_str->receive_to_current_device_func(protocol_str, (uint8_t*)bus_header, sizeof(bus_main_module_header));
					number_of_bytes_processed += sizeof(bus_main_module_header);
				}
				else if(bus_header->size_type == BPPT_FLOAT_LEN_PACKAGE)
				{
					protocol_str->receive_to_current_device_func(protocol_str, (uint8_t*)bus_header, sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
					number_of_bytes_processed += sizeof(bus_main_module_header)+ bus_header->Data.var_len_header.size;
				}

			}else
			{
				if( bus_header->size_type == BPPT_FIX_LEN_PACKAGE)
				{
					protocol_str->receive_to_another_device_func(protocol_str, (uint8_t*)bus_header, sizeof(bus_main_module_header));
					number_of_bytes_processed += sizeof(bus_main_module_header);
				}
				else if(bus_header->size_type == BPPT_FLOAT_LEN_PACKAGE)
				{
					protocol_str->receive_to_another_device_func(protocol_str, (uint8_t*)bus_header, sizeof(bus_main_module_header)+bus_header->Data.var_len_header.size);
					number_of_bytes_processed += sizeof(bus_main_module_header)+ bus_header->Data.var_len_header.size;
				}
				else
				{
				    break;
				}
			}

			if(bus_header->last_package)
								spi_answer_task_start(protocol_str);
		}
		//goto receive_data_finish;

		return number_of_bytes_processed;

	}
	return 0;
}


int ubi4_spi_full_duplex_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str != 0)
	{
		int receive_size = 0;
		bus_main_module_header* bus_header = 0;

#if defined(UBI_SDK_FAMILY_STM32) || defined(UBI_SDK_FAMALY_NRF52)
//	start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif



		protocol_str->port->read_data_buffer(protocol_str->port,protocol_str->port->rx_buffer + protocol_str->buffer_offset, protocol_str->port->rx_buffer_size,(uint32_t*)&receive_size);



		if(receive_size == 0)
		{
			ubi4_slave_receiver_reset(protocol_str);

			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
			return 0;
		}



		switch(protocol_str->last_package_type)
		{
			case BPPT_FIX_LEN_PACKAGE:
			{

				bus_header = (bus_main_module_header*)(protocol_str->port->rx_buffer + protocol_str->buffer_offset);

				volatile uint8_t input_crc = getCRC((uint8_t*)bus_header, 9);

				if( input_crc != bus_header->crc)
				{
					ubi4_slave_receiver_reset(protocol_str);
					protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

					crc_error++;
					return 0;
				}

#if defined(UBI_SDK_FAMILY_STM32)
				    start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif

				protocol_str->last_rx_time = protocol_str->ms_soft_timer->init_struct->GetTick();

				switch(bus_header->size_type)
				{
					case BPPT_FIX_LEN_PACKAGE:
					{
						uint32_t current_buffer_offset = protocol_str->buffer_offset;

						uint8_t a_status = 0;
						uint8_t device_address = Get_Device_Address(&a_status);
						if(a_status == 0) return 0;

						if(bus_header->addr.full_address == device_address || (bus_header->addr.full_address == 0xFF))
						{
 							if(bus_header->last_package)
 							{
 								ubi4_slave_receiver_reset(protocol_str);
 							}
							else
							{
								protocol_str->buffer_offset += receive_size;
							}

						//	protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

							//if(bus_header->package_code == 0x85 && bus_header->request_type == 1)
							//	protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

							protocol_str->receive_to_current_device_func(protocol_str, protocol_str->port->rx_buffer + current_buffer_offset, sizeof(bus_main_module_header));

						}else
						{

								ubi4_slave_receiver_reset(protocol_str);
								protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));


						}


						return 0;

					}break;
					case BPPT_FLOAT_LEN_PACKAGE:
					{
						protocol_str->buffer_offset += sizeof(bus_main_module_header);
						protocol_str->var_len_package_start = (uint8_t*)bus_header;
						protocol_str->last_package_type = BPPT_FLOAT_LEN_PACKAGE;
						protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,bus_header->Data.var_len_header.size);
						return 0;


					}break;
				}

			}break;



			case BPPT_FLOAT_LEN_PACKAGE:{

				if(bus_header->last_package)
					ubi4_slave_receiver_reset(protocol_str);
				else
					protocol_str->buffer_offset += receive_size;

				protocol_str->last_package_type = BPPT_FIX_LEN_PACKAGE;
				protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

				protocol_str->receive_to_current_device_func(protocol_str, protocol_str->var_len_package_start, sizeof(bus_main_module_header));


			}break;
			default: break;
		}


	}
	return 0;
}



int ubi4_bus_receive_data_rx_irq_exec(ubi4_bus_protocol_obj_struct* protocol_str)
{
	if(protocol_str != 0)
	{
		int receive_size = 0;
		bus_main_module_header* bus_header = 0;

#if defined(UBI_SDK_FAMILY_STM32) || defined(UBI_SDK_FAMALY_NRF52)
	start_soft_timer(protocol_str->ms_soft_timer, protocol_str->reset_timer_instance,100);
#endif


		protocol_str->port->read_data_buffer(protocol_str->port,protocol_str->port->rx_buffer + protocol_str->buffer_offset, protocol_str->port->rx_buffer_size,(uint32_t*)&receive_size);



		if(receive_size == 0)
		{
			ubi4_slave_receiver_reset(protocol_str);
			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
			return 0;
		}



		switch(protocol_str->last_package_type)
		{
			case BPPT_FIX_LEN_PACKAGE:
			{

				bus_header = (bus_main_module_header*)(protocol_str->port->rx_buffer + protocol_str->buffer_offset);
				
				volatile uint8_t input_crc = getCRC((uint8_t*)bus_header, 9);

				if( input_crc != bus_header->crc)
				{
					ubi4_slave_receiver_reset(protocol_str);
					protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));

                    crc_error++;
					return 0;
				}

				switch(bus_header->size_type)
				{
					case BPPT_FIX_LEN_PACKAGE:
					{
						uint32_t current_buffer_offset = protocol_str->buffer_offset;
					//	if(protocol_str->port->port_direction == UBI_PTE_FULL_DUPLEX)
					//	{
							
							//ubi4_slave_receiver_reset(protocol_str);
							//protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							
					//		if(bus_header->last_package)
						//	{

							//	ubi4_slave_receiver_reset(protocol_str);
							//	protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							//}else
							//{
								
						//	}
					//	}

						uint8_t a_status = 0;
						uint8_t device_address = Get_Device_Address(&a_status);
						if(a_status == 0) return 0;

						if(bus_header->addr.full_address == device_address )//|| (bus_header->addr.full_address == 0xFF))
						{
							protocol_str->buffer_offset += receive_size;
							protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							
							protocol_str->receive_to_current_device_func(protocol_str, protocol_str->port->rx_buffer + current_buffer_offset, sizeof(bus_main_module_header));
							
						//	if(protocol_str->answer_queue_counter == 0)
					//		{
					//			ubi4_slave_receiver_reset(protocol_str);
					//			protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
					//		}
							
						}else
						{
							//protocol_str->receive_to_another_device_func(protocol_str, protocol_str->port->rx_buffer + current_buffer_offset, sizeof(bus_main_module_header));
							if(bus_header->waiting_answer)
									protocol_str->answer_queue_counter++;
							
							if(bus_header->last_package)
							{
								ubi4_slave_receiver_reset(protocol_str);
								protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							}else
							{
								protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,sizeof(bus_main_module_header));
							}

						}

						if(protocol_str->wait_answer && bus_header->last_package)
																							answer_task_start(protocol_str);
						
						
						


						return 0;

					}break;
					case BPPT_FLOAT_LEN_PACKAGE:
					{
						protocol_str->buffer_offset += sizeof(bus_main_module_header);
						protocol_str->last_package_type = BPPT_FLOAT_LEN_PACKAGE;
						protocol_str->port->wait_receive(protocol_str->port,protocol_str->buffer_offset,bus_header->Data.var_len_header.size);
						return 0;


					}break;
				}

			}break;



			case BPPT_FLOAT_LEN_PACKAGE:{



			}break;
			default: break;
		}

	}
	return 0;
}


__attribute__((weak))
bridge_protocol_info_struct Check_Device_Bridge_Info(void* port_device_manager, uint8_t Address)
{

	bridge_protocol_info_struct t;
	t.device = 0;
	t.last_bridge_pack_info = 0;
	t.last_bridge_protocol = 0;
	t.status = 0;

	return t;
}




__attribute__((weak))
uint8_t Get_Device_Address(uint8_t* status)
{
	while(1)
	{
		*status = 0;
	}
	return 0;
}

__attribute__((weak))
void protocol_delay(uint32_t time_ms)
{

}


void ubi4_bus_receive_data_rx_irq(ubi4_bus_protocol_obj_struct* protocol)
{
	base_port_struct* port_str = protocol->port;
	
	if(port_str != 0)
	{
		if(port_str->get_receive_buffer_size != 0)
		{
			uint32_t receive_size = 0;
			port_str->get_receive_buffer_size(port_str,&receive_size);
			if(receive_size > 0)
			{
				if(port_str->read_data_buffer != 0)
				{
					ubi4_bus_receive_data_full_buffer_exec(protocol);
				}
			}
		}else
		{
			if(port_str->read_data_buffer != 0)
			{
				ubi4_bus_receive_data_full_buffer_exec(protocol);
			}
		}
	}
}




