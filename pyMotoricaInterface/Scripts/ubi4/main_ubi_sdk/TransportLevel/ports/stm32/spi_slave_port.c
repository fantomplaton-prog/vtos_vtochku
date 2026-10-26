/*
  ******************************************************************************
  * @file    spi_slave_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/stm32/spi_slave_port.h"

#ifdef UBI_SDK_FAMILY_STM32

#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/a_protocol.h"

#ifdef HAL_SPI_MODULE_ENABLED


void spi_slave_get_receive_buffer_size(spi_slave_port_struct* port_str,uint32_t* size)
{
	//spi_slave_get_buffered_data_len(port_str->init_struct->spi_slave_port_num, size);
}

void spi_slave_read_data_buffer(spi_slave_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	*read_bytes_size = port_str->init_struct->config->RxXferSize;
}


void spi_slave_port_start_receive(spi_slave_port_struct* port_str)
{

	uint8_t try_counter = 0;
	__HAL_SPI_ENABLE(port_str->init_struct->config);
	__HAL_SPI_CLEAR_FREFLAG(port_str->init_struct->config);
	__HAL_SPI_CLEAR_OVRFLAG(port_str->init_struct->config);

	while(HAL_SPI_Receive_DMA(port_str->init_struct->config, port_str->init_struct->rx_buffer + port_str->current_rx_offset, port_str->current_rx_len) != HAL_OK)
	{

		HAL_SPI_Abort(port_str->init_struct->config);
		try_counter++;

		if(try_counter > 100) break;
	}
	if(try_counter<100)
		port_str->wait_receive_enable = 1;
}

void spi_slave_port_wait_receive(spi_slave_port_struct* port_str, uint32_t offset, uint32_t len)
{
	uint8_t try_counter = 0;

	port_str->current_mode = SPIME_RECEIVE;
	port_str->current_rx_offset = offset;
	port_str->current_rx_len = len;
	__HAL_SPI_DISABLE(port_str->init_struct->config);


	/*
	while(HAL_SPI_Receive_DMA(port_str->init_struct->config, port_str->init_struct->rx_buffer + offset, len) != HAL_OK)
	{

		HAL_SPI_Abort(port_str->init_struct->config);
		try_counter++;

		if(try_counter > 100) break;
	}
	if(try_counter<100)
		port_str->wait_receive_enable = 1;
		*/


}


void spi_slave_port_write_cpp(spi_slave_port_struct* port_str,uint8_t *buf, uint16_t size)
{
	uint8_t try_counter = 0;
	port_str->current_mode = SPIME_TRANSMIT;

	while(HAL_SPI_Transmit_DMA(port_str->init_struct->config, buf, size) != HAL_OK)
	{
		//HAL_spi_slave_Abort(port_str->init_struct->config);
		HAL_SPI_Abort(port_str->init_struct->config);
		try_counter++;
		if(try_counter > 100) break;
	}
	if(try_counter<100)
			port_str->wait_receive_enable = 0;
}

void spi_slave_port_init(spi_slave_init_port_struct* init_struct, spi_slave_port_struct** out_port_struct)
{
	spi_slave_port_struct* port_struct = malloc(sizeof(spi_slave_port_struct));
	memset(port_struct, 0, sizeof(spi_slave_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = spi_slave_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = spi_slave_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_SPI_BUS;
	port_struct->Base.port_direction = UBI_PTE_FULL_DUPLEX;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = spi_slave_port_write_cpp;
	port_struct->Base.wait_receive = spi_slave_port_wait_receive;

	port_struct->current_receive_mode = SPIRME_RECEIVE_IRQ;

	*out_port_struct = port_struct;
}


#endif  //HAL_SPI_MODULE_ENABLED

#endif
