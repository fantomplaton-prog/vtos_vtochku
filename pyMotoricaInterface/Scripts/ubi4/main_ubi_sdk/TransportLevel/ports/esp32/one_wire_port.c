/*
  ******************************************************************************
  * @file    one_wire_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "TransportLevel/ports/esp32/one_wire_port.h"

#ifdef UBI_SDK_TARGET_ESP32

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "hal/uart_ll.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/timer.h"
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
#define TASK_STACK_SIZE 1024

#define TIMER_DIVIDER         (80)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

void init_hw_timer()
{
	/* Select and initialize basic parameters of the timer */
	timer_config_t config = {
		.divider = TIMER_DIVIDER,
		.counter_dir = TIMER_COUNT_UP,
		.counter_en = TIMER_PAUSE,
		.alarm_en = TIMER_ALARM_DIS,
		.auto_reload = true,
	}; // default clock source is APB
	timer_init(TIMER_GROUP_0, TIMER_0, &config);

	/* Timer's counter will initially start from value below.
	   Also, if auto_reload is set, this value will be automatically reload on alarm */
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);

	timer_start(TIMER_GROUP_0, TIMER_0);
}

uint8_t flag = 0;

//uint8_t one_wire_buf_tx[BUF_SIZE];
//uint8_t one_wire_buf_rx[BUF_SIZE];

//void one_wire_en_tx_direction(one_wire_port_struct* port_str);
//void one_wire_en_rx_direction(one_wire_port_struct* port_str);

volatile uint64_t start_time = 0;
volatile uint64_t finish_time = 0;
volatile uint64_t delta_time = 0;

/*
static void one_wire_receive_task(void *arg)
{
	size_t len = 0;
	uint8_t read_len = 0;
	while(1)
	{
		 uart_get_buffered_data_len(UART_PORT_NUM, &len);

		 if(len)
		 {
			// Read data from the UART
			// read_len = uart_read_bytes(UART_PORT_NUM, one_wire_buf_rx, (BUF_SIZE - 1), 20 / portTICK_RATE_MS);
			// Write data back to the UART
			//uart_write_bytes(UART_PORT_NUM, (const char *) data, len);

			port_struct ps;
			ps.port = UBI_PE_ONE_WIRE_BUS;
		//	parce(one_wire_buf_rx, read_len, ONE_WIRE_BUS);
		 }
	}
}
*/


static void one_wire_receive_task_irq(void *arg)
{
	size_t len = 0;
	uint16_t read_len = 0;
	uint16_t processed_len = 0;
	one_wire_port_struct* port_struct = 0;
	while(1)
	{
		port_struct = (one_wire_port_struct*)arg;

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



static void one_wire_wait_receive_task(void *arg)
{
	size_t len = 0;
	uint8_t read_len = 0;
	one_wire_port_struct* port= 0;

	while(1)
	{
		port = (one_wire_port_struct*)arg;

		if(xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY) == pdPASS)
		{
			port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, port->wait_time_ms/portTICK_RATE_MS);
		}
	}
}


static void one_wire_control_task(void *arg)
{

}


void one_wire_processing_data(one_wire_port_struct* port_str,uint32_t len)
{
	if(port_str->receive_size >= len)
	{
		port_str->receive_size -= len;
	}else
		port_str->receive_size = 0;
}



void one_wire_get_receive_buffer_size(one_wire_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);
	*size = port_str->receive_size;

}

void one_wire_read_data_buffer(one_wire_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	// uart_rx_fifo_rest(port_str->init_struct->uart_port_num);

//	uart_flush_input(port_str->init_struct->uart_port_num);

	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, read_bytes_size);

	//if(*read_bytes_size > 0)
	//{
//		*read_bytes_size = uart_read_bytes(port_str->init_struct->uart_port_num, out_buf, out_buf_size, 20 / portTICK_RATE_MS);
//		flag = 0;
	//}

	*read_bytes_size = port_str->receive_size;

}


void one_wire_port_wait_receive(one_wire_port_struct* port, uint32_t offset, uint32_t size)
{

	port->Base.receiver_mode = PORT_RECEIVER_IRQ_MODE;
	port->Base.rx_buffer_offset = offset;
	port->Base.rx_buffer_req_size = size;
	//port->enableRecieverTask = true;
	//xSemaphoreTake(port->rx_task_semaphore, portMAX_DELAY);

}

void one_wire_port_wait_receive_time(one_wire_port_struct* port, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;

	//port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, port->wait_time_ms/portTICK_RATE_MS);
	xSemaphoreGive(port->rx_task_semaphore);
}

int one_wire_port_wait_receive_time_blocked(one_wire_port_struct* port, uint32_t offset, uint32_t len, uint32_t time_ms)
{
	port->wait_time_ms = time_ms;
	port->Base.receiver_mode = PORT_RECEIVER_COLLECT_MODE;

//	while(port->current_mode == OWME_TRANSMIT)
//	{
//		vTaskDelay( 1 / portTICK_PERIOD_MS );
//	}



	if(len > 0)
	{
		port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, len, time_ms/portTICK_RATE_MS);
	}else
	{
		port->receive_size = uart_read_bytes(port->init_struct->uart_port_num, port->Base.rx_buffer, port->Base.rx_buffer_size, time_ms/portTICK_RATE_MS);
	}

	return port->receive_size;

}

void one_wire_port_write_cpp(void* port_str,uint8_t *buf, uint8_t size)
{
	one_wire_port_struct* port_struct = (one_wire_port_struct*)port_str;
	uint32_t read_bytes_size = 0;

	//if(flag == 1) return;

	static uint8_t firststr = 0;


	flag = 1;

	uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);
	if(read_bytes_size > 0)
		uart_flush_input(port_struct->init_struct->uart_port_num);

	//port_struct->current_mode = OWME_TRANSMIT;
	//uart_set_rx_full_threshold(port_struct->init_struct->uart_port_num, size);

	uart_clear_intr_status(port_struct->init_struct->uart_port_num,UART_INTR_TX_DONE);
	uart_enable_intr_mask(port_struct->init_struct->uart_port_num,UART_INTR_TX_DONE);
	//uart_disable_intr_mask(port_struct->init_struct->uart_port_num,UART_INTR_RXFIFO_FULL);

	//gpio_input_disable(port_struct->init_struct->uart_owd_pin);
	//gpio_input_enable(port_struct->init_struct->uart_owd_pin);
	//gpio_output_enable(port_struct->init_struct->uart_owd_pin);
	//gpio_od_enable(port_struct->init_struct->uart_owd_pin);


	//uart_set_rx_full_threshold(port_struct->init_struct->uart_port_num, size);

	/*
	if(firststr)
	{
		uint64_t temp_t = 0;
		timer_get_counter_value(TIMER_GROUP_0, TIMER_0,&temp_t);

		delta_time = temp_t - start_time;

		if(delta_time<28000)
		{
			//gpio_set_level(16, 0);
		}

		start_time = temp_t;
	}else
	{

	timer_get_counter_value(TIMER_GROUP_0, TIMER_0,&start_time);
	}

	firststr = 1;
	*/


	port_struct->current_mode = OWME_TRANSMIT;


	uart_write_bytes(port_struct->init_struct->uart_port_num, buf, size);

	//uart_wait_tx_done(port_struct->init_struct->uart_port_num, 100 / portTICK_PERIOD_MS);


	//uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);

	//if(read_bytes_size > 0)
		//	uart_flush_input(port_struct->init_struct->uart_port_num);





	//gpio_input_enable(port_struct->init_struct->uart_owd_pin);

	//port_struct->current_mode = OWME_RECEIVE;


	//gpio_output_disable(port_struct->init_struct->uart_owd_pin);
	//gpio_input_enable(port_struct->init_struct->uart_owd_pin);




//	uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);
//	if(read_bytes_size > 0)
//	{
//  	uart_flush_input(port_struct->init_struct->uart_port_num);
//	}

	//port_struct->current_mode = OWME_RECEIVE;


	//gpio_set_level(16, 0);



}

void one_wire_fwd_irq(uint32_t intr_status, one_wire_port_struct* port_struct)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(port_struct)
	{
		if(intr_status & UART_INTR_TX_DONE)
		{
			//uart_clear_intr_status(port_struct->init_struct->uart_port_num,UART_INTR_RXFIFO_FULL);
			//uart_enable_intr_mask(port_struct->init_struct->uart_port_num,UART_INTR_RXFIFO_FULL);
			//uart_set_rx_full_threshold(port_struct->init_struct->uart_port_num, 1);

			//gpio_output_disable(port_struct->init_struct->uart_owd_pin);
			//gpio_input_enable(port_struct->init_struct->uart_owd_pin);

			//gpio_set_level(16, 1);

			//timer_get_counter_value(TIMER_GROUP_0, TIMER_0,&finish_time);
			//delta_time = finish_time - start_time;


			//uint32_t read_bytes_size = 0;

			//uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);
		//	if(read_bytes_size > 0)
		//	{
		//		uart_flush_input(port_struct->init_struct->uart_port_num);
		//	}

			uart_clear_intr_status(port_struct->init_struct->uart_port_num,UART_INTR_TX_DONE);
			uart_rx_fifo_rest(port_struct->init_struct->uart_port_num);
			port_struct->current_mode = OWME_RECEIVE;

			if(port_struct->tx_wait_rx_enable)
			{
			//	 xSemaphoreGiveFromISR(port_struct->rx_task_semaphore, &xHigherPriorityTaskWoken);
			}

		}
	}
}

void one_wire_bck_irq(uint32_t intr_status, one_wire_port_struct* port_struct)
{
	if(port_struct)
	{
		if(intr_status & UART_INTR_RXFIFO_FULL)
		{

			uint32_t read_bytes_size = 0;
			uart_get_buffered_data_len(port_struct->init_struct->uart_port_num, &read_bytes_size);
			if(read_bytes_size > 0)
			{
				uart_flush_input(port_struct->init_struct->uart_port_num);
			}
			uart_disable_intr_mask(port_struct->init_struct->uart_port_num,UART_INTR_RXFIFO_FULL);
			uart_set_rx_full_threshold(port_struct->init_struct->uart_port_num, 128);
		}
	}
}


uint32_t GetTransmitTime(one_wire_port_struct* port, uint32_t num_bytes)
{
	return ((1000000*num_bytes)/port->Base.speed)+1;
}

void one_wire_port_write_and_time_receive(void* port_str,uint8_t *buf, uint8_t size, uint32_t time)
{
	one_wire_port_struct* port_struct = (one_wire_port_struct*)port_str;
	uint32_t read_bytes_size = 0;

	static uint8_t firststr = 0;

	flag = 1;

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



}

void one_wire_port_set_receiver_mode(one_wire_port_struct* port, port_receive_mode_enum mode)
{
	port->Base.receiver_mode = mode;
}

void one_wire_port_wait_tx(one_wire_port_struct* port_str)
{
	uart_wait_tx_done(port_str->init_struct->uart_port_num, 100 / portTICK_PERIOD_MS);
}


void one_wire_port_init(one_wire_init_port_struct* init_struct, one_wire_port_struct** out_port_struct)
{
	one_wire_port_struct* port_struct = malloc(sizeof(one_wire_port_struct));
	memset(port_struct, 0, sizeof(one_wire_port_struct));

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

	port_struct->Base.processing_data = one_wire_processing_data;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = one_wire_port_write_cpp;
	port_struct->Base.wait_receive = one_wire_port_wait_receive;
	port_struct->Base.wait_receive_time_blocked = one_wire_port_wait_receive_time_blocked;
	port_struct->Base.set_receiver_mode = one_wire_port_set_receiver_mode;
	port_struct->Base.wait_receive_time = one_wire_port_wait_receive_time;
	port_struct->Base.write_and_time_receive = one_wire_port_write_and_time_receive;
	port_struct->tx_wait_rx_enable = false;
	port_struct->Base.GetTransmitTime_us = GetTransmitTime;

	port_struct->Base.speed = (init_struct->uart_config.baud_rate*8)/((8+1+1+0)*8);

	port_struct->Base.wait_tx_finish = one_wire_port_wait_tx;

	*out_port_struct = port_struct;

	if(init_struct->one_wire_control_pin != -1)
	{
		gpio_reset_pin(init_struct->one_wire_control_pin);
		gpio_config_t io_conf = {};
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pin_bit_mask = 1ULL<<init_struct->one_wire_control_pin;
		io_conf.pull_down_en = 0;
		io_conf.pull_up_en = 0;
		gpio_config(&io_conf);
	}


	int intr_alloc_flags = 0;


	if(uart_driver_delete(init_struct->uart_port_num) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}

	if(uart_param_config(init_struct->uart_port_num, &init_struct->uart_config) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}

	if(uart_set_half_duplex_pin(init_struct->uart_port_num, init_struct->uart_owd_pin) != ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}
	if(uart_driver_install(init_struct->uart_port_num, init_struct->rx_buffer_size, init_struct->tx_buffer_size, 0, NULL, intr_alloc_flags)!= ESP_OK)
	{
		ESP_LOGI("UART0", "UART0 ERROR");
	}


//	uart_set_tx_idle_num(init_struct->uart_port_num,100);


	//gpio_output_disable(init_struct->uart_owd_pin);
	//gpio_input_enable(init_struct->uart_owd_pin);

	gpio_input_enable(port_struct->init_struct->uart_owd_pin);
	gpio_output_enable(port_struct->init_struct->uart_owd_pin);
	gpio_od_enable(port_struct->init_struct->uart_owd_pin);

	//gpio_set_direction(port_struct->init_struct->uart_owd_pin, GPIO_MODE_INPUT);

	//set_user_bck_irq_callback(init_struct->uart_port_num, one_wire_bck_irq, port_struct);
	set_user_fvd_irq_callback(init_struct->uart_port_num, one_wire_fwd_irq, port_struct);


	port_struct->rx_task_semaphore = xSemaphoreCreateBinary();

	//init_hw_timer();



	//uart_disable_intr_mask(init_struct->uart_port_num, 0xFFFFFFFF);


	//one_wire_en_rx_direction(port_struct);

	//one_wire_en_tx_direction(port_struct);

	//gpio_set_pull_mode(init_struct->uart_tx_pin,GPIO_PULLUP_ONLY);
	//gpio_set_direction(init_struct->uart_tx_pin, GPIO_MODE_OUTPUT_OD);

	//uart_flush_input(init_struct->uart_port_num);

	//xTaskCreate(one_wire_receive_task, "one_wire_receive_task", TASK_STACK_SIZE, NULL, 10, NULL);

	//xTaskCreate(one_wire_receive_task_irq, "one_wire_receive_task", TASK_STACK_SIZE, port_struct, 20, NULL);

	//xTaskCreate(one_wire_wait_receive_task, "one_wire_receive_task", TASK_STACK_SIZE, port_struct, 20, NULL);


}


#endif
