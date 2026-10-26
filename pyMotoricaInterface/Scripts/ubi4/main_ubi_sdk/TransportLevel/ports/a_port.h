/*
  ******************************************************************************
  * @file    a_port.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef INC_A_PORT_H_
#define INC_A_PORT_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "UBI_SDK.h"
#include "stdint.h"
#include "stdbool.h"


#ifdef UBI_SDK_TARGET_ESP32

//#include "UBIv4/TransportLevel/ports/esp32/ble_gatt_server_port.h"

//#include "TransportLevel/ports/esp32/one_wire_port.h"

//#include "TransportLevel/ports/esp32/usb_cdc_port.h"

#include "stdint.h"
#include "stdbool.h"

#endif

#ifdef UBI_SDK_FAMILY_STM32

//#include "TransportLevel/ports/stm32/one_wire_port.h"

//#include "TransportLevel/ports/stm32/usb_cdc_port.h"

//#include "TransportLevel/ports/stm32/uart_port.h"

//#include "TransportLevel/ports/stm32/spi_slave_port.h"

#endif



 typedef enum {
		 	 UBI_PE_ONE_WIRE_BUS = 0,
			 UBI_PE_UART_BUS,
			 UBI_PE_SPI_BUS,
			 UBI_PE_USB,
			 UPI_PE_BLE
	}ports_enum;

typedef enum {
		 UBI_PTE_FULL_DUPLEX = 0,
		 UBI_PTE_HALF_DUPLEX
}port_type_enum;

typedef enum {
		 PORT_RECEIVER_IRQ_MODE = 0,
		 PORT_RECEIVER_COLLECT_MODE
}port_receive_mode_enum;

	 typedef struct {
		 ports_enum port;

	 }port_struct;

/*
typedef bool (*initialize_port_func)(void* init_str, void* port_str);
typedef uint8_t* (*get_port_buffer_func)(void* port_str);
typedef void (*write_port_func)(void* port_str,uint8_t *buf, uint8_t size);
typedef void (*receive_data_func)(void* port_str);
*/

 typedef struct
 {
	 ports_enum port_type;
	 port_type_enum port_direction;

	 void* init_struct;

	 uint8_t* (*get_tx_buffer)(void* init_str);
	 uint8_t* (*get_rx_buffer)(void* init_str);
	 void (*write_direct)(void* port_str, uint8_t *buf, uint16_t size);

	 void (*write_and_time_receive)(void* port_str, uint8_t *buf, uint16_t size, uint32_t time_ms);

	 int (*receive_data)(void* port_str);
	 void (*receive_bridge_func)(void* port_str, uint8_t* buf, uint32_t size);
	 void (*slave_answer_func)(void* port_str, uint8_t* buf, uint32_t size);
	 void (*receive_to_current_device_func)(void* port_str, uint8_t* buf, uint32_t size);
	 void (*receive_to_another_device_func)(void* port_str, uint8_t* buf, uint32_t size);

	 void (*get_receive_buffer_size)(void* port_str, uint32_t* size);
	 void (*read_data_buffer)(void* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size);
	 void (*start_receive_task)(void* port_str);

	 void (*processing_data)(void* port_str, uint32_t len);

	 void (*wait_receive)(void* port_str, uint32_t rx_buf_offset, uint32_t len);
	 void (*wait_tx_finish)(void* port_str);

	 void (*wait_receive_time)(void* port, uint32_t time_ms);

	 uint32_t (*wait_receive_time_blocked)(void* port, uint32_t offset, uint32_t len ,uint32_t time_ms);

	 void (*stop_receive)(void* port_str);
	 void (*set_receiver_mode)(void* port_str,port_receive_mode_enum mode);

	 uint32_t (*GetTransmitTime_us)(void* port_str, uint32_t num_bytes);

	 void (*close_port_callback)(void* protocol_str);

	 uint32_t speed;

	 void* protocol;

	 port_receive_mode_enum receiver_mode;

	 uint8_t* rx_buffer;
	 uint16_t rx_buffer_size;
	 uint16_t rx_buffer_offset;
	 uint16_t rx_buffer_req_size;
	 uint32_t wait_rx_size;

	 uint8_t* tx_buffer;
	 uint16_t tx_buffer_size;

 }base_port_struct;


void base_port_init(base_port_struct* port_str);

void base_port_ubi4_bus_receive_data(base_port_struct* port_str);

void base_port_receive_bridge(base_port_struct* port_str,uint8_t* buf, uint32_t size);
void base_port_slave_answer(base_port_struct* port_str,uint8_t* buf, uint32_t size);
void base_port_receive_to_current_device(base_port_struct* port_str,uint8_t* buf, uint32_t size);

void base_port_get_receive_buffer_size(base_port_struct* port_str, uint32_t* size);

void base_port_read_data_buffer(base_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size);

 uint8_t* port_get_tx_buffer(base_port_struct* port_str);

 uint8_t* port_get_rx_buffer(base_port_struct* port_str);


#ifdef __cplusplus
}
#endif

#endif /* INC_A_PORT_H_ */
