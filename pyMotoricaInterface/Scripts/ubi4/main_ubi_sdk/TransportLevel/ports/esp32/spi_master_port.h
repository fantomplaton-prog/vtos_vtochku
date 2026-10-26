/*
  ******************************************************************************
  * @file    spi_master_port.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef MAIN_SPI_MASTER_PORT_H_
#define MAIN_SPI_MASTER_PORT_H_


#include "TransportLevel/ports/a_port.h"

//#define UBI_SDK_TARGET_ESP32


#ifdef UBI_SDK_TARGET_ESP32

#include "stdint.h"
#include "driver/spi_master.h"




typedef struct
{
	spi_host_device_t spi_host;
	int mosi_pin;
	int miso_pin;
	int sclk_pin;
	int cs_pin;

	int clock_speed_hz;

	uint8_t* rx_buffer;
	uint16_t rx_buffer_size;

	uint8_t* tx_buffer;
	uint16_t tx_buffer_size;

	spi_bus_config_t  config;

	ports_enum port_type;


}spi_master_init_port_struct;



typedef struct
{
	base_port_struct Base;

	//TaskHandle_t  ReceiveTask;
	bool enableRecieverTask;
	spi_master_init_port_struct* init_struct;
	spi_device_handle_t spi;

	spi_device_interface_config_t device_spi_config;

//	uart_isr_handle_t isr_handle;

//	xSemaphoreHandle rx_task_semaphore;
	uint32_t wait_time_ms;
	int receive_size;
	int last_rx_delta_size;

	bool tx_wait_rx_enable;

}spi_master_port_struct;


void spi_set_cs(spi_master_port_struct* spi_port_struct, uint8_t state);


void spi_master_port_init(spi_master_init_port_struct* init_struct, spi_master_port_struct** spi_port_struct);


#endif
#endif /* MAIN_SPI_MASTER_PORT_H_ */
