/*
  ******************************************************************************
  * @file    uart_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/esp32/uart_port.h"

#ifdef UBI_SDK_TARGET_ESP32

#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "string.h"

#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"



#define UART_PORT_NUM 0
#define UART_BAUD_RATE 115200
#define BUF_SIZE 1024
#define UART_TX 43
#define UART_RX 44
#define UART_RTS -1
#define UART_CTS -1
#define TASK_STACK_SIZE 2048


//uint8_t uart_buf_tx[BUF_SIZE];
//uint8_t uart_buf_rx[BUF_SIZE];

static void uart_receive_task_irq(void *arg)
{
	size_t len = 0;
	uint16_t read_len = 0;
	uint16_t processed_len = 0;
	uart_port_struct* port_struct = 0;
	while(1)
	{
		port_struct = (uart_port_struct*)arg;

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

		vTaskDelay( 5 / portTICK_PERIOD_MS );
	}
}


static void uart_wait_receive_task(void *arg)
{
	size_t len = 0;
	uint8_t read_len = 0;
	uart_port_struct* port= 0;

	while(1)
	{
		port = (uart_port_struct*)arg;

		if(xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY) == pdPASS)
		{
			port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, port->wait_time_ms/portTICK_RATE_MS);
		}
	}
}



void uart_get_receive_buffer_size(uart_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);

	*size = port_str->receive_size;
}

void uart_read_data_buffer(uart_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	*read_bytes_size = port_str->receive_size;

	//if(out_buf == port_str->Base.rx_buffer)

}


void uart_processing_data(uart_port_struct* port_str,uint32_t len)
{
	if(port_str->receive_size >= len)
	{
		port_str->receive_size -= len;
	}else
		port_str->receive_size = 0;
}


void uart_port_write(void* port_str,uint8_t *buf, uint16_t size)
{
	  uart_write_bytes(((uart_port_struct*)port_str)->init_struct->uart_port_num, buf, size);
}

void uart_port_wait_receive(uart_port_struct* port, uint32_t offset, uint32_t size)
{

	port->Base.receiver_mode = PORT_RECEIVER_IRQ_MODE;
	port->Base.rx_buffer_offset = offset;
	port->Base.rx_buffer_req_size = size;
	//port->enableRecieverTask = true;
	//xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY);

}

void uart_port_wait_receive_time(uart_port_struct* port, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;
	xSemaphoreGive(port->rx_task_semaphore);
}

int uart_port_wait_receive_time_blocked(uart_port_struct* port, uint32_t offset, uint32_t len, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;

	if(len > 0)
	{
		port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, len, time_ms/portTICK_RATE_MS);
	}else
	{
		port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, time_ms/portTICK_RATE_MS);
	}

	return port->receive_size;

}


void uart_port_set_receiver_mode(uart_port_struct* port, port_receive_mode_enum mode)
{
	port->Base.receiver_mode = mode;
}

void uart_port_stop_receive(uart_port_struct* port)
{
	port->enableRecieverTask = false;
	//vTaskDelete();
	//vTaskSuspend(NULL);
}


void uart_port_wait_tx(uart_port_struct* port_str)
{
	uart_wait_tx_done(port_str->init_struct->uart_port_num, 100 / portTICK_PERIOD_MS);
}


void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct)
{
	uart_port_struct* port_struct = malloc(sizeof(uart_port_struct));

	memset(port_struct, 0, sizeof(uart_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = uart_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = uart_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_UART_BUS;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.processing_data = uart_processing_data;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = uart_port_write;
	port_struct->Base.wait_receive = uart_port_wait_receive;
	port_struct->Base.wait_receive_time = uart_port_wait_receive_time;
	port_struct->Base.stop_receive = uart_port_stop_receive;
	port_struct->Base.set_receiver_mode = uart_port_set_receiver_mode;
	port_struct->Base.wait_receive_time_blocked = uart_port_wait_receive_time_blocked;

	port_struct->Base.receiver_mode = PORT_RECEIVER_IRQ_MODE;

	port_struct->last_rx_delta_size = 0;
	port_struct->receive_size = 0;
	port_struct->wait_time_ms = 0;

	port_struct->Base.rx_buffer_offset = 0;
	port_struct->Base.rx_buffer_req_size = 0;
	port_struct->Base.wait_tx_finish = uart_port_wait_tx;

	port_struct->rx_task_semaphore = xSemaphoreCreateBinary();

	*out_port_struct = port_struct;


	int intr_alloc_flags = 0;

	if(uart_driver_delete(init_struct->uart_port_num) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}

	if(uart_param_config(init_struct->uart_port_num, &init_struct->uart_config) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}

	if(uart_set_pin(init_struct->uart_port_num, init_struct->uart_tx_pin, init_struct->uart_rx_pin, UART_RTS, UART_CTS) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}
	if(uart_driver_install(init_struct->uart_port_num, init_struct->rx_buffer_size, init_struct->tx_buffer_size, 0, NULL, intr_alloc_flags)!= ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}


	//xTaskCreate(uart_receive_task_irq, "uart_receive_task_irq", TASK_STACK_SIZE, port_struct, 10, &port_struct->ReceiveTask);

	//xTaskCreate(uart_wait_receive_task, "uart_wait_receive_task", TASK_STACK_SIZE, port_struct, 20, NULL);

	//gpio_set_direction(UART_TX, GPIO_MODE_OUTPUT_OD);

//	xTaskCreate(one_wire_receive_task, "one_wire_receive_task", TASK_STACK_SIZE, NULL, 10, NULL);

}

#endif
