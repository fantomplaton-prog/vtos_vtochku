/*
  ******************************************************************************
  * @file    uart_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef UART_PORT_H_
#define UART_PORT_H_


#include "TransportLevel/ports/a_port.h"

#ifdef UBI_SDK_TARGET_ESP32

#include "stdint.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"




typedef struct
{
	int uart_port_num;
	int uart_rx_pin;
	int uart_tx_pin;

	uint8_t* rx_buffer;
	uint16_t rx_buffer_size;

	uint8_t* tx_buffer;
	uint16_t tx_buffer_size;

	uart_config_t uart_config;

	ports_enum port_type;


}uart_init_port_struct;



typedef struct
{
	base_port_struct Base;
	TaskHandle_t  ReceiveTask;
	bool enableRecieverTask;
	uart_init_port_struct* init_struct;

	xSemaphoreHandle rx_task_semaphore;
	uint32_t wait_time_ms;
	int receive_size;
	int last_rx_delta_size;

}uart_port_struct;


void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct);

#endif
#endif /* MAIN_ONE_WIRE_PORT_H_ */
