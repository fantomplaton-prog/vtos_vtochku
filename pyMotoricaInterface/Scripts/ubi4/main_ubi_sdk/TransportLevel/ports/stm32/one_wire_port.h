/*
  ******************************************************************************
  * @file    one_wire_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef MAIN_ONE_WIRE_PORT_H_
#define MAIN_ONE_WIRE_PORT_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "UBI_SDK.h"

#include "TransportLevel/ports/a_port.h"

#ifdef UBI_SDK_FAMILY_STM32
#ifdef UART_PORT_EN


#include "HAL_DRIVERS.h"

#include "stdint.h"


 typedef struct
 {
 	uint8_t* rx_buffer;
 	uint16_t rx_buffer_size;

 	uint8_t* tx_buffer;
 	uint16_t tx_buffer_size;

 	UART_HandleTypeDef* uart_config;

 }one_wire_init_port_struct;



 typedef struct
 {
 	base_port_struct Base;

 	one_wire_init_port_struct* init_struct;

 	uint8_t wait_receive_enable;

 }one_wire_port_struct;



 void one_wire_port_init(one_wire_init_port_struct* init_struct, one_wire_port_struct** out_port_struct);


#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAIN_ONE_WIRE_PORT_H_ */
