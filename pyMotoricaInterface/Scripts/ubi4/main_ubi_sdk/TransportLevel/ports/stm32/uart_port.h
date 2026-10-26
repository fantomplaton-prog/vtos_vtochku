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

#ifdef UBI_SDK_FAMILY_STM32
#ifdef UART_PORT_EN

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

 	UART_HandleTypeDef* uart_config;

 }uart_init_port_struct;

 typedef struct
 {
 	base_port_struct Base;

 	uart_init_port_struct* init_struct;

 	uint8_t wait_receive_enable;

 }uart_port_struct;




 extern buffer_queue_struct tx_queue;
 extern buffer_queue_struct rx_queue;


 void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct);
 void uart_port_wait_receive(uart_port_struct* port_str, uint32_t offset, uint32_t len);

void uart_port_init0(UART_HandleTypeDef* huart);

void uart_port_transmit(uint8_t* Buf, uint16_t Size);

//void uart_port_wait_receive(uint8_t* Buf, uint16_t Size);

void uart_port_receive(UART_HandleTypeDef* huart);

void uart_port_start(void* p);

void uart_port_wait_fixed_receive();
void uart_port_wait_float_receive(uint16_t size);

bool uart_port_receive_float_len();


uint8_t* ReadQueue(buffer_queue_struct* buffer_queue, uint16_t size);

uint16_t QueueSize(buffer_queue_struct* buffer_queue);

uint8_t* GetQueueBufferPointer(buffer_queue_struct* buffer_queue, uint16_t* size);
bool FinishWriteToBuffer(buffer_queue_struct* buffer_queue, uint16_t Size);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_UART_PORT_H_ */
