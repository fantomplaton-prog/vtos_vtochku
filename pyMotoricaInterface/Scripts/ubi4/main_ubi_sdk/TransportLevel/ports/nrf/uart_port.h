/*
  ******************************************************************************
  * @file    uart_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_UART_PORT_H_
#define INC_UART_PORT_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "UBI_SDK.h"
#include "TransportLevel/ports/a_port.h"

#ifdef UBI_SDK_FAMALY_NRF52
#include "HAL_DRIVERS.h"
#include "stdbool.h"

 typedef struct
 {
 	uint8_t* queue_array;
 	uint16_t read_counter;
 	uint16_t write_counter;
 	uint16_t size;
 }buffer_queue_struct;


 typedef struct
 {
 	uint8_t* rx_buffer;
 	uint16_t rx_buffer_size;

 	uint8_t* tx_buffer;
 	uint16_t tx_buffer_size;
	 
	void (*uart_port_rx_callback)(base_port_struct* port);
	 
	void (*uart_port_tx_callback)(base_port_struct* port);

 	app_uart_comm_params_t uart_config;
	 
	app_irq_priority_t irq_priority; 

 }uart_init_port_struct;

 typedef struct
 {
 	base_port_struct Base;

 	uart_init_port_struct* init_struct;
	 
	uint8_t* last_data_rx_p;
	uint32_t last_data_rx_size;

 }uart_port_struct;


/*

 extern buffer_queue_struct tx_queue;
 extern buffer_queue_struct rx_queue;
*/

 void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct);
 void uart_port_wait_receive(uart_port_struct* port_str, uint32_t offset, uint32_t len);

void uart_port_init0(void* huart);

void uart_port_transmit(uint8_t* Buf, uint16_t Size);

//void uart_port_wait_receive(uint8_t* Buf, uint16_t Size);

void uart_port_receive(void* huart);

void uart_port_start(void* p);

void uart_port_wait_fixed_receive();
void uart_port_wait_float_receive(uint16_t size);

bool uart_port_receive_float_len();


uint8_t* ReadQueue(buffer_queue_struct* buffer_queue, uint16_t size);

uint16_t QueueSize(buffer_queue_struct* buffer_queue);

uint8_t* GetQueueBufferPointer(buffer_queue_struct* buffer_queue, uint16_t* size);
bool FinishWriteToBuffer(buffer_queue_struct* buffer_queue, uint16_t Size);

#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_UART_PORT_H_ */
