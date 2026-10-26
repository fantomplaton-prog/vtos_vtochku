/*
 * usb_cdc_port.c
 *
 *  Created on: 4 янв. 2023 г.
 *      Author: PC
 */

/*
  ******************************************************************************
  * @file    usb_cdc_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include <string.h>

#include "TransportLevel/ports/stm32/usb_cdc_port.h"

#ifdef UBI_SDK_TARGET_STM32
#ifdef USB_PORT_EN

#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"
#include "usbd_cdc_if.h"



void usb_cdc_port_write(uint8_t *buf, uint16_t size)
{
	CDC_Transmit_FS(buf, size);
}

void usb_cdc_initialize_port()
{

}
#endif
#endif
