/*
  ******************************************************************************
  * @file    one_wire_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/stm32/one_wire_port.h"

#ifdef UBI_SDK_FAMILY_STM32
#ifdef UART_PORT_EN
#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"

#include "main.h"


void one_wire_get_receive_buffer_size(one_wire_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);
}

void one_wire_read_data_buffer(one_wire_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	*read_bytes_size = port_str->init_struct->uart_config->RxXferSize;
}

void one_wire_port_wait_receive(one_wire_port_struct* port_str, uint32_t offset, uint32_t len)
{

	uint8_t try_counter = 0;


//	while(!__HAL_UART_GET_FLAG(port_str->init_struct->uart_config,UART_FLAG_IDLE));
//	HAL_UART_DMAStop(port_str->init_struct->uart_config);

//	HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *huart);
//	HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart);

	//HAL_UART_Abort(port_str->init_struct->uart_config);

	HAL_HalfDuplex_EnableReceiver(port_str->init_struct->uart_config);

	while(HAL_UART_Receive_DMA(port_str->init_struct->uart_config, port_str->init_struct->rx_buffer + offset, len) != HAL_OK)
	{

		HAL_UART_DMAStop(port_str->init_struct->uart_config);
		HAL_UART_Abort(port_str->init_struct->uart_config);
		HAL_HalfDuplex_EnableReceiver(port_str->init_struct->uart_config);
		try_counter++;
		if(try_counter > 100) break;
	}
	if(try_counter<100)
			port_str->wait_receive_enable = 1;
}


uint8_t buf11[10] = {0, 7, 32, 0x20, 0xD0, 0, 0, 0, 0, 3};


void one_wire_port_write_cpp(one_wire_port_struct* port_str,uint8_t *buf, uint16_t size)
{
	uint8_t try_counter = 0;
//	HAL_UART_DMAStop(port_str->init_struct->uart_config);
//	HAL_UART_Abort(port_str->init_struct->uart_config);

	//HAL_GPIO_WritePin(PROBE_GPIO_Port, PROBE_Pin, GPIO_PIN_SET);

	HAL_HalfDuplex_EnableTransmitter(port_str->init_struct->uart_config);
	while(HAL_UART_Transmit_DMA(port_str->init_struct->uart_config, buf, size) != HAL_OK)
	{
		HAL_UART_DMAStop(port_str->init_struct->uart_config);
		HAL_UART_Abort(port_str->init_struct->uart_config);
		//HAL_UART_AbortTransmit(port_str->init_struct->uart_config);
		HAL_HalfDuplex_EnableTransmitter(port_str->init_struct->uart_config);
		try_counter++;
		if(try_counter > 100) break;
	}
}

void one_wire_port_init(one_wire_init_port_struct* init_struct, one_wire_port_struct** out_port_struct)
{
	one_wire_port_struct* port_struct = malloc(sizeof(one_wire_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = one_wire_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = one_wire_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_ONE_WIRE_BUS;
	port_struct->Base.port_direction = UBI_PTE_HALF_DUPLEX;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = one_wire_port_write_cpp;
	port_struct->Base.wait_receive = one_wire_port_wait_receive;

	*out_port_struct = port_struct;


}


#endif
#endif
