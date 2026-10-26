/*
  ******************************************************************************
  * @file    uart_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/stm32/uart_port.h"

#ifdef UBI_SDK_FAMILY_STM32
#ifdef UART_PORT_EN
#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/a_protocol.h"




void uart_get_receive_buffer_size(uart_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);
}

void uart_read_data_buffer(uart_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	*read_bytes_size = port_str->init_struct->uart_config->RxXferSize;
}

void uart_port_wait_receive(uart_port_struct* port_str, uint32_t offset, uint32_t len)
{
	uint8_t try_counter = 0;
	while(HAL_UART_Receive_DMA(port_str->init_struct->uart_config, port_str->init_struct->rx_buffer + offset, len) != HAL_OK)
	{

		HAL_UART_AbortReceive(port_str->init_struct->uart_config);
		try_counter++;

		if(try_counter > 100) break;
	}
	if(try_counter<100)
		port_str->wait_receive_enable = 1;


}

void uart_port_wait_tx(uart_port_struct* port_str)
{

}


void uart_port_write_cpp(uart_port_struct* port_str,uint8_t *buf, uint16_t size)
{
	uint8_t try_counter = 0;
	while(HAL_UART_Transmit_DMA(port_str->init_struct->uart_config, buf, size) != HAL_OK)
	{
		//HAL_UART_Abort(port_str->init_struct->uart_config);
		HAL_UART_AbortTransmit(port_str->init_struct->uart_config);
		try_counter++;
		if(try_counter > 100) break;
	}
}

void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct)
{
	uart_port_struct* port_struct = malloc(sizeof(uart_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = uart_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = uart_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_UART_BUS;
	port_struct->Base.port_direction = UBI_PTE_FULL_DUPLEX;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = uart_port_write_cpp;
	port_struct->Base.wait_receive = uart_port_wait_receive;
	port_struct->Base.wait_tx_finish = uart_port_wait_tx;

	*out_port_struct = port_struct;
}




bool FinishWriteToBuffer(buffer_queue_struct* buffer_queue, uint16_t Size)
{
	if((buffer_queue->size - (buffer_queue->write_counter - buffer_queue->read_counter)) >= Size)
	{
		buffer_queue->write_counter += Size;
		return true;
	}else
		return false;

}

bool ReadBufferQueue(buffer_queue_struct* buffer_queue, uint16_t Size)
{
	if(buffer_queue->write_counter - buffer_queue->read_counter)
	{
		buffer_queue->read_counter += Size;

		if(buffer_queue->write_counter == buffer_queue->read_counter)
		{
			buffer_queue->write_counter = 0;
			buffer_queue->read_counter = 0;
		}
		return true;

	}else
		return false;
}

uint8_t* GetQueueBufferPointer(buffer_queue_struct* buffer_queue, uint16_t* Size)
{
	*Size = buffer_queue->write_counter - buffer_queue->read_counter;

	return &buffer_queue->queue_array[buffer_queue->write_counter];
}

uint16_t QueueSize(buffer_queue_struct* buffer_queue)
{
	return buffer_queue->write_counter - buffer_queue->read_counter;
}


uint8_t* ReadQueue(buffer_queue_struct* buffer_queue, uint16_t size)
{
	if(size <= buffer_queue->write_counter - buffer_queue->read_counter)
	{

		uint8_t* point = buffer_queue->queue_array + buffer_queue->read_counter;
		buffer_queue->read_counter += size;
		if(buffer_queue->write_counter == buffer_queue->read_counter)
		{
			buffer_queue->write_counter = 0;
			buffer_queue->read_counter = 0;
		}

		return point;
	}
	return 0;
}

#endif
#endif
