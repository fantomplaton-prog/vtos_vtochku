/*
  ******************************************************************************
  * @file    usb_cdc_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef MAIN_LIB_USB_CDC_PORT_H_
#define MAIN_LIB_USB_CDC_PORT_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "UBI_SDK.h"

#ifdef UBI_SDK_TARGET_STM32
#ifdef USB_PORT_EN
#include "stdint.h"

void usb_cdc_initialize_port();
void usb_cdc_port_write(uint8_t *buf, uint16_t size);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAIN_LIB_USB_CDC_PORT_H_ */
