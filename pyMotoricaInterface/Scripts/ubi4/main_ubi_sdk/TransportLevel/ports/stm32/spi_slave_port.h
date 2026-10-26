/*
  ******************************************************************************
  * @file    spi_slave_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_SPI_SLAVE_PORT_H_
#define INC_SPI_SLAVE_PORT_H_

#include "HAL_DRIVERS.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifdef HAL_SPI_MODULE_ENABLED


#include "UBI_SDK.h"
#include "TransportLevel/ports/a_port.h"

#ifdef UBI_SDK_FAMILY_STM32
#include "HAL_DRIVERS.h"
#include "stdbool.h"

#ifdef HAL_SPI_MODULE_ENABLED

 typedef enum
 {
 	SPIME_RECEIVE	= 0x00,
 	SPIME_TRANSMIT	= 0x01
 }spi_mode_enum;

 typedef enum
 {
 	SPIRME_RECEIVE_IRQ		= 0x00,
 	SPIRME_RECEIVE_COLLECT	= 0x01
 }spi_receive_mode_enum;

 typedef struct
 {
 	uint8_t* rx_buffer;
 	uint16_t rx_buffer_size;

 	uint8_t* tx_buffer;
 	uint16_t tx_buffer_size;
 	SPI_HandleTypeDef* config;

 }spi_slave_init_port_struct;

 typedef struct
 {
 	base_port_struct Base;

 	spi_slave_init_port_struct* init_struct;

 	uint8_t wait_receive_enable;

 	spi_mode_enum current_mode;
 	spi_receive_mode_enum current_receive_mode;

 	uint32_t current_rx_offset;
 	uint32_t current_rx_len;

 }spi_slave_port_struct;




void spi_slave_port_init(spi_slave_init_port_struct* init_struct, spi_slave_port_struct** out_port_struct);
void spi_slave_port_wait_receive(spi_slave_port_struct* port_str, uint32_t offset, uint32_t len);
void spi_slave_port_start_receive(spi_slave_port_struct* port_str);
void spi_slave_port_init0(SPI_HandleTypeDef* huart);

void spi_slave_port_transmit(uint8_t* Buf, uint16_t Size);

//void spi_slave_port_wait_receive(uint8_t* Buf, uint16_t Size);

void spi_slave_port_receive(SPI_HandleTypeDef* huart);

void spi_slave_port_start(void* p);

void spi_slave_port_wait_fixed_receive();
void spi_slave_port_wait_float_receive(uint16_t size);

bool spi_slave_port_receive_float_len();

#endif // HAL_SPI_MODULE_ENABLED
#endif

#endif //UBI_SDK_FAMILY_STM32

#ifdef __cplusplus
}
#endif

#endif /* INC_SPI_SLAVE_PORT_H_ */
