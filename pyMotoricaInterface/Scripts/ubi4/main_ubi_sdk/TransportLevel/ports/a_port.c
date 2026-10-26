/*
  ******************************************************************************
  * @file    a_port.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/a_protocol.h"
#include "Data/data_verification.h"



void base_port_init(base_port_struct* port_str)
{
	if(port_str)
	{
		port_str->get_rx_buffer = port_get_rx_buffer;
		port_str->get_tx_buffer = port_get_tx_buffer;
		port_str->get_receive_buffer_size = 0;
		port_str->read_data_buffer = 0;
		port_str->receive_data = ubi4_bus_receive_data_full_buffer;
		port_str->receive_bridge_func = base_port_receive_bridge;
		port_str->slave_answer_func = base_port_slave_answer;
		port_str->receive_to_current_device_func = base_port_receive_to_current_device;
	}
}



void base_port_receive_bridge(base_port_struct* port_str,uint8_t* buf, uint32_t size)
{

}

void base_port_slave_answer(base_port_struct* port_str,uint8_t* buf, uint32_t size)
{

}
void base_port_receive_to_current_device(base_port_struct* port_str,uint8_t* buf, uint32_t size)
{

}

void base_port_get_receive_buffer_size(base_port_struct* port_str, uint32_t* size)
{

}

void base_port_read_data_buffer(base_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{

}

 uint8_t* port_get_tx_buffer(base_port_struct* port_str)
 {
	 return port_str->tx_buffer;
 }

 uint8_t* port_get_rx_buffer(base_port_struct* port_str)
 {
	 return port_str->rx_buffer;
 }



