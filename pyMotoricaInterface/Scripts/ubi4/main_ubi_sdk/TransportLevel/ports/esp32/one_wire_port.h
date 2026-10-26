/*
  ******************************************************************************
  * @file    one_wire_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef MAIN_ONE_WIRE_PORT_H_
#define MAIN_ONE_WIRE_PORT_H_


#include "TransportLevel/ports/a_port.h"

#ifdef UBI_SDK_TARGET_ESP32

#include "stdint.h"
#include "driver/uart.h"




typedef struct
{
	int uart_port_num;
	int uart_owd_pin;
	int one_wire_control_pin;

	uint8_t* rx_buffer;
	uint16_t rx_buffer_size;

	uint8_t* tx_buffer;
	uint16_t tx_buffer_size;

	uart_config_t uart_config;

	ports_enum port_type;


}one_wire_init_port_struct;


typedef enum
{
	OWME_RECEIVE	= 0x00,
	OWME_TRANSMIT	= 0x01
}one_wire_mode_enum;


typedef struct
{
	base_port_struct Base;
	TaskHandle_t  ReceiveTask;
	bool enableRecieverTask;
	one_wire_init_port_struct* init_struct;

	uart_isr_handle_t isr_handle;

	one_wire_mode_enum current_mode;

	xSemaphoreHandle rx_task_semaphore;
	uint32_t wait_time_ms;
	int receive_size;
	int last_rx_delta_size;

	bool tx_wait_rx_enable;

}one_wire_port_struct;



void one_wire_initialize_port();
uint8_t* one_wire_port_get_buffer();
void one_wire_port_write(uint8_t *buf, uint8_t size);

void one_wire_enable_rx(one_wire_port_struct* out_port_struct);


void one_wire_port_init(one_wire_init_port_struct* init_struct, one_wire_port_struct** out_port_struct);


#endif
#endif /* MAIN_ONE_WIRE_PORT_H_ */
