/*
  ******************************************************************************
  * @file    spi_master_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/esp32/spi_master_port.h"


//#define UBI_SDK_TARGET_ESP32

#ifdef UBI_SDK_TARGET_ESP32

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "hal/uart_ll.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/timer.h"
#include "string.h"

#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"




static void spi_master_receive_task_irq(void *arg)
{
	size_t len = 0;
	uint16_t read_len = 0;
	uint16_t processed_len = 0;
	spi_master_port_struct* port_struct = 0;
	while(1)
	{
		port_struct = (spi_master_port_struct*)arg;

		/*
		if(port_struct->Base.receiver_mode == PORT_RECEIVER_IRQ_MODE)
		{
			uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &len);

			if(port_struct->Base.rx_buffer_req_size != 0)
			{
				if((len + port_struct->last_rx_delta_size) >= port_struct->Base.rx_buffer_req_size)
				{
					uint8_t* BufPos = port_struct->Base.rx_buffer + port_struct->Base.rx_buffer_offset;
					//uint32_t size = port_struct->Base.rx_buffer_size + port_struct->last_rx_delta_size;
					read_len = uart_read_bytes(port_struct->init_struct->uart_port_num, BufPos, port_struct->Base.rx_buffer_size, 1);
					port_struct->receive_size = read_len;
					processed_len = port_struct->Base.receive_data(port_struct);

					if(read_len + port_struct->last_rx_delta_size != processed_len)
					{
						port_struct->Base.rx_buffer_offset = processed_len;
						port_struct->last_rx_delta_size = read_len-processed_len;
					}else
					{
						port_struct->Base.rx_buffer_offset = 0;
						port_struct->last_rx_delta_size = 0;
					}

					//port_struct->Base.receive_data(port_struct);
				}

				if(port_struct->Base.receiver_mode == PORT_RECEIVER_COLLECT_MODE)
				{
					//vTaskSuspend(NULL);
				}
			}
		}
		*/

		vTaskDelay( 5 / portTICK_PERIOD_MS );
	}
}



static void spi_master_wait_receive_task(void *arg)
{
	size_t len = 0;
	uint8_t read_len = 0;
	spi_master_port_struct* port= 0;

	while(1)
	{
		port = (spi_master_port_struct*)arg;

		/*
		if(xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY) == pdPASS)
		{
			port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, port->wait_time_ms/portTICK_RATE_MS);
		}*/
	}
}


static void spi_master_control_task(void *arg)
{

}


void spi_master_processing_data(spi_master_port_struct* port_str,uint32_t len)
{
	if(port_str->receive_size >= len)
	{
		port_str->receive_size -= len;
	}else
		port_str->receive_size = 0;
}



void spi_master_get_receive_buffer_size(spi_master_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);
	*size = port_str->receive_size;

}

void spi_master_read_data_buffer(spi_master_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	// uart_rx_fifo_rest(port_str->init_struct->uart_port_num);

//	uart_flush_input(port_str->init_struct->uart_port_num);

	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, read_bytes_size);

	//if(*read_bytes_size > 0)
	//{
//		*read_bytes_size = uart_read_bytes(port_str->init_struct->uart_port_num, out_buf, out_buf_size, 20 / portTICK_RATE_MS);
//		flag = 0;
	//}

	*read_bytes_size = port_str->receive_size + port_str->Base.rx_buffer_offset;

}


void spi_master_port_wait_receive(spi_master_port_struct* port, uint32_t offset, uint32_t size)
{

	port->Base.receiver_mode = PORT_RECEIVER_IRQ_MODE;
	port->Base.rx_buffer_offset = offset;
	port->Base.rx_buffer_req_size = size;
	//port->enableRecieverTask = true;
	//xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY);

}

void spi_master_port_wait_receive_time(spi_master_port_struct* port, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;

	//port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, port->wait_time_ms/portTICK_RATE_MS);
	//xSemaphoreGive(port->rx_task_semaphore);
}

int spi_master_port_wait_receive_time_blocked(spi_master_port_struct* port, uint32_t offset, uint32_t len, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;
	port->Base.rx_buffer_offset = offset;


	esp_err_t err = 0;

	if(len > 0)
	{
		spi_transaction_t t = {
			.cmd = 0,
			.length = len*8,
			.rxlength = len*8,
			.flags = 0,
			.rx_buffer = (void*)port->Base.rx_buffer + offset,
			.user = port,
		};

		err = spi_device_polling_transmit(port->spi,  &t);

		if(err == ESP_OK)
		{
			port->receive_size = len;
		}

	}else
	{
		uint32_t receive_bytes = (port->init_struct->clock_speed_hz/1000)*time_ms;

		if(receive_bytes > port->Base.rx_buffer_size) receive_bytes = port->Base.rx_buffer_size;


		spi_transaction_t t = {
			.cmd = 0,
			.length = len*8,
			.rxlength = len*8,
			.flags = 0,
			.rx_buffer = (void*)port->Base.rx_buffer + offset,
			.user = port,
		};



		err = spi_device_polling_transmit(port->spi,  &t);

		if(err == ESP_OK)
		{
			port->receive_size = receive_bytes;
		}
	}

	return port->receive_size;

}

void spi_master_port_write_cpp(void* port_str,uint8_t *buf, uint8_t size)
{
	spi_master_port_struct* port_struct = (spi_master_port_struct*)port_str;

	spi_transaction_t t = {
	        .cmd = 0,
	        .length = size*8,
	        .flags = 0,
	        .tx_buffer = (void*)buf,
	        .user = port_str,
	    };
	  //  err = spi_device_polling_transmit(ctx->spi, &t);

	//spi_device_transmit(port_struct->spi, &t);

	spi_device_polling_transmit(port_struct->spi, &t);
}




uint32_t spi_get_transmit_time(spi_master_port_struct* port, uint32_t num_bytes)
{
	return ((1000000*num_bytes)/port->Base.speed)+1;
}

void spi_master_port_write_and_time_receive(void* port_str,uint8_t *buf, uint8_t size, uint32_t time)
{
	spi_master_port_struct* port_struct = (spi_master_port_struct*)port_str;
	uint32_t read_bytes_size = 0;

	static uint8_t firststr = 0;

//flag = 1;

	/*

	uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);
	if(read_bytes_size > 0)
		uart_flush_input(port_struct->init_struct->uart_port_num);


	uart_clear_intr_status(port_struct->init_struct->uart_port_num,UART_INTR_TX_DONE);
	uart_enable_intr_mask(port_struct->init_struct->uart_port_num,UART_INTR_TX_DONE);

	port_struct->current_mode = OWME_TRANSMIT;
	port_struct->tx_wait_rx_enable = true;
	port_struct->wait_time_ms = time;

	uart_write_bytes(port_struct->init_struct->uart_port_num, buf, size);
	//uart_wait_tx_done(port_struct->init_struct->uart_port_num, 100 / portTICK_PERIOD_MS);

	//uart_rx_fifo_rest(port_struct->init_struct->uart_port_num);
	//port_struct->current_mode = OWME_RECEIVE;

	//port_struct->receive_size = uart_read_bytes(port_struct->init_struct->uart_port_num, port_struct->Base.rx_buffer, port_struct->Base.rx_buffer_size, port_struct->wait_time_ms/portTICK_RATE_MS);
	xSemaphoreGive(port_struct->rx_task_semaphore);

	*/



}

void spi_master_port_set_receiver_mode(spi_master_port_struct* port, port_receive_mode_enum mode)
{
	port->Base.receiver_mode = mode;
}

static void cs_high(spi_transaction_t* t)
{
    gpio_set_level(((spi_master_port_struct*)t->user)->init_struct->cs_pin, 1);
}

static void cs_low(spi_transaction_t* t)
{
    gpio_set_level(((spi_master_port_struct*)t->user)->init_struct->cs_pin, 0);
}

void spi_set_cs(spi_master_port_struct* spi_port_struct, uint8_t state)
{
	gpio_set_level(spi_port_struct->init_struct->cs_pin, state);
}


void spi_master_port_init(spi_master_init_port_struct* init_struct, spi_master_port_struct** spi_port_struct)
{
	spi_master_port_struct* port_struct = malloc(sizeof(spi_master_port_struct));
	memset(port_struct, 0, sizeof(spi_master_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = spi_master_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = spi_master_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_SPI_BUS;
	port_struct->Base.port_direction = UBI_PTE_HALF_DUPLEX;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.processing_data = spi_master_processing_data;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = spi_master_port_write_cpp;
	port_struct->Base.wait_receive = spi_master_port_wait_receive;
	port_struct->Base.wait_receive_time_blocked = spi_master_port_wait_receive_time_blocked;
	port_struct->Base.set_receiver_mode = spi_master_port_set_receiver_mode;
	port_struct->Base.wait_receive_time = spi_master_port_wait_receive_time;
	port_struct->Base.write_and_time_receive = spi_master_port_write_and_time_receive;
	port_struct->tx_wait_rx_enable = false;
	port_struct->Base.GetTransmitTime_us = spi_get_transmit_time;

	port_struct->Base.speed = (init_struct->clock_speed_hz)/8;

	*spi_port_struct = port_struct;

	int intr_alloc_flags = 0;

	esp_err_t err = ESP_OK;

	init_struct->config.mosi_io_num = init_struct->mosi_pin;
	init_struct->config.miso_io_num = init_struct->miso_pin;
	init_struct->config.sclk_io_num = init_struct->sclk_pin;
	init_struct->config.quadwp_io_num = -1;
	init_struct->config.quadhd_io_num = -1;
	init_struct->config.max_transfer_sz = init_struct->tx_buffer_size;
	init_struct->config.flags = 0;

	err = spi_bus_initialize(init_struct->spi_host, &init_struct->config, SPI_DMA_CH_AUTO);


	port_struct->device_spi_config.command_bits = 0;
	port_struct->device_spi_config.clock_speed_hz = init_struct->clock_speed_hz;
	port_struct->device_spi_config.mode = 0;          //SPI mode 0
/*
* The timing requirements to read the busy signal from the EEPROM cannot be easily emulated
* by SPI transactions. We need to control CS pin by SW to check the busy signal manually.
*/
	port_struct->device_spi_config.spics_io_num = -1;
	port_struct->device_spi_config.queue_size = 1;
	port_struct->device_spi_config.flags = SPI_DEVICE_NO_DUMMY;

	port_struct->device_spi_config.cs_ena_pretrans = 	((init_struct->clock_speed_hz * 50)/1000000) - 1;
	port_struct->device_spi_config.cs_ena_posttrans =	((init_struct->clock_speed_hz * 50)/1000000) - 1;
	port_struct->device_spi_config.pre_cb = cs_low;
	port_struct->device_spi_config.post_cb = cs_high;
	port_struct->device_spi_config.command_bits = 0;
	port_struct->device_spi_config.address_bits = 0;
	port_struct->device_spi_config.dummy_bits = 0;
	port_struct->device_spi_config.input_delay_ns = 1000;  //the EEPROM output the data half a SPI clock behind.
	//Attach the EEPROM to the SPI bus
	err = spi_bus_add_device(init_struct->spi_host, &port_struct->device_spi_config, &port_struct->spi);

	gpio_set_level(init_struct->cs_pin, 1);

	gpio_reset_pin(init_struct->cs_pin);
	gpio_config_t cs_io_conf = {};
	cs_io_conf.intr_type = GPIO_INTR_DISABLE;
	cs_io_conf.mode = GPIO_MODE_OUTPUT;
	cs_io_conf.pin_bit_mask = BIT64(init_struct->cs_pin);
	cs_io_conf.pull_down_en = 0;
	cs_io_conf.pull_up_en = 0;
	gpio_config(&cs_io_conf);

	gpio_set_level(init_struct->cs_pin, 1);


	//port_struct->rx_task_semaphore = xSemaphoreCreateBinary();

	//init_hw_timer();



	//uart_disable_intr_mask(init_struct->uart_port_num, 0xFFFFFFFF);


	//spi_master_en_rx_direction(port_struct);

	//spi_master_en_tx_direction(port_struct);

	//gpio_set_pull_mode(init_struct->uart_tx_pin,GPIO_PULLUP_ONLY);
	//gpio_set_direction(init_struct->uart_tx_pin, GPIO_MODE_OUTPUT_OD);

	//uart_flush_input(init_struct->uart_port_num);

	//xTaskCreate(spi_master_receive_task, "spi_master_receive_task", TASK_STACK_SIZE, NULL, 10, NULL);

	//xTaskCreate(spi_master_receive_task_irq, "spi_master_receive_task", TASK_STACK_SIZE, port_struct, 20, NULL);

	//xTaskCreate(spi_master_wait_receive_task, "spi_master_receive_task", 2048, port_struct, 20, NULL);


}


#endif
