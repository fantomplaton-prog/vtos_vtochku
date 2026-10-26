/*
  ******************************************************************************
  * @file    usb_cdc_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef MAIN_LIB_USB_CDC_PORT_H_
#define MAIN_LIB_USB_CDC_PORT_H_

#include "TransportLevel/ports/a_port.h"
#include "stdint.h"
#include "stdbool.h"


typedef struct
{
	uint8_t* rx_buffer;
	uint16_t rx_buffer_size;

	uint8_t* tx_buffer;
	uint16_t tx_buffer_size;

}usb_cdc_init_port_struct;



typedef struct
{
	base_port_struct Base;
	usb_cdc_init_port_struct* init_struct;
	int itf;

	uint32_t last_read_size;

}usb_cdc_port_struct;


void usb_cdc_port_init(usb_cdc_init_port_struct* init_struct, usb_cdc_port_struct** out_port_struct);


//void usb_cdc_initialize_port();
//void usb_cdc_port_write(uint8_t *buf, uint16_t size);
//uint8_t* usb_cdc_port_get_tx_buf();

#endif /* MAIN_LIB_USB_CDC_PORT_H_ */
