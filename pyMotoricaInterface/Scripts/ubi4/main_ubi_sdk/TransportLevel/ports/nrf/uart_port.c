/*
  ******************************************************************************
  * @file    uart_port.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "TransportLevel/ports/nrf/uart_port.h"

#ifdef UBI_SDK_FAMALY_NRF52

#include "utils/base_soft_timer.h"
#include "TransportLevel/ports/a_port.h"
#include "TransportLevel/a_protocol.h"
#include "stdlib.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"

static nrf_drv_uart_t app_uart_inst = NRF_DRV_UART_INSTANCE(APP_UART_DRIVER_INSTANCE);

void* uart_port_inst;

//extern TIM_HandleTypeDef htim3;

bool enable_float_len_uart = false;

/*
uint8_t uart_port_rx_buffer[1024];
uint8_t uart_port_tx_buffer[1024];





buffer_queue_struct tx_queue = {
		.queue_array = uart_port_tx_buffer,
		.read_counter = 0,
		.write_counter = 0,
		.size = 1024

};

buffer_queue_struct rx_queue = {
		.queue_array = uart_port_rx_buffer,
		.read_counter = 0,
		.write_counter = 0,
		.size = 1024
};
*/




void uart_get_receive_buffer_size(uart_port_struct* port_str,uint32_t* size)
{
	//uart_get_buffered_data_len(port_str->init_struct->uart_port_num, size);
	*size = port_str->last_data_rx_size;
}

void uart_read_data_buffer(uart_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	*read_bytes_size = port_str->last_data_rx_size;
}

void uart_port_wait_receive(uart_port_struct* port_str, uint32_t offset, uint32_t len)
{
	nrf_drv_uart_rx(&app_uart_inst, (uint8_t*)(port_str->Base.rx_buffer + offset), (uint8_t)len);
}


void uart_port_write_cpp(uart_port_struct* port_str,uint8_t *buf, uint16_t size)
{
		nrf_gpio_cfg_default(port_str->init_struct->uart_config.tx_pin_no);
		nrf_gpio_pin_set(port_str->init_struct->uart_config.tx_pin_no);
    nrf_gpio_cfg_output(port_str->init_struct->uart_config.tx_pin_no);
		nrf_uart_txrx_pins_set(app_uart_inst.uart.p_reg, port_str->init_struct->uart_config.tx_pin_no, port_str->init_struct->uart_config.rx_pin_no);
		
    ret_code_t ret =  nrf_drv_uart_tx(&app_uart_inst, buf, size);
}

/*
	UART Event handler
*/
static void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context)
{
    if (p_event->type == NRF_DRV_UART_EVT_RX_DONE)
    {
        // Received bytes counter has to be checked, because there could be event from RXTO interrupt
        if (p_event->data.rxtx.bytes)
        {
            app_uart_evt_t app_uart_event;
            app_uart_event.evt_type   = APP_UART_DATA;
            app_uart_event.data.value = p_event->data.rxtx.p_data[0];
					
					//((uart_port_struct*)p_context)->Base.receive_data
       //     rx_done = true;
       //     m_event_handler(&app_uart_event);
					
					((uart_port_struct*)p_context)->last_data_rx_p = p_event->data.rxtx.p_data;
					((uart_port_struct*)p_context)->last_data_rx_size = p_event->data.rxtx.bytes;
					((uart_port_struct*)p_context)->init_struct->uart_port_rx_callback((base_port_struct*)p_context);
					
        }
    }
    else if (p_event->type == NRF_DRV_UART_EVT_ERROR)
    {
        app_uart_evt_t app_uart_event;
        app_uart_event.evt_type                 = APP_UART_COMMUNICATION_ERROR;
        app_uart_event.data.error_communication = p_event->data.error.error_mask;
    //    (void)nrf_drv_uart_rx(&app_uart_inst, rx_buffer, 1);
    //    m_event_handler(&app_uart_event);
    }
    else if (p_event->type == NRF_DRV_UART_EVT_TX_DONE)
    {
       // Last byte from FIFO transmitted, notify the application.
       // Notify that new data is available if this was first byte put in the buffer.
       app_uart_evt_t app_uart_event;
       app_uart_event.evt_type = APP_UART_TX_EMPTY;
			
			nrf_uart_txrx_pins_set(app_uart_inst.uart.p_reg, NRF_UART_PSEL_DISCONNECTED, ((uart_port_struct*)p_context)->init_struct->uart_config.rx_pin_no);
		
			nrf_gpio_cfg_input(((uart_port_struct*)p_context)->init_struct->uart_config.tx_pin_no, NRF_GPIO_PIN_PULLUP);
			
			
  //     m_event_handler(&app_uart_event);
    }
}


void uart_port_init(uart_init_port_struct* init_struct, uart_port_struct** out_port_struct)
{
	uart_port_struct* port_struct = malloc(sizeof(uart_port_struct));

	base_port_init((base_port_struct*)port_struct);

	port_struct->Base.get_receive_buffer_size = uart_get_receive_buffer_size;
	port_struct->Base.read_data_buffer = uart_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_UART_BUS;
	port_struct->Base.port_direction = UBI_PTE_FULL_DUPLEX;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = uart_port_write_cpp;
	port_struct->Base.wait_receive = uart_port_wait_receive;
	
	
	uint32_t err_code;
	
	 nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
	 
	 config.interrupt_priority = init_struct->irq_priority;
	 
	 config.p_context = port_struct;
	 
	 config.baudrate = (nrf_uart_baudrate_t)init_struct->uart_config.baud_rate;
   config.parity = init_struct->uart_config.use_parity ? NRF_UART_PARITY_INCLUDED : NRF_UART_PARITY_EXCLUDED;
	 config.hwfc = (init_struct->uart_config.flow_control == APP_UART_FLOW_CONTROL_DISABLED) ? NRF_UART_HWFC_DISABLED : NRF_UART_HWFC_ENABLED;

   config.pselrxd = init_struct->uart_config.rx_pin_no;
   config.pseltxd = init_struct->uart_config.tx_pin_no;
	 config.pselcts = init_struct->uart_config.cts_pin_no;
   config.pselrts =  init_struct->uart_config.rts_pin_no;
	 
	 config.use_easy_dma = true;
	 
	 

	  err_code = nrf_drv_uart_init(&app_uart_inst, &config, uart_event_handler);
		
		nrf_uart_txrx_pins_set(app_uart_inst.uart.p_reg, NRF_UART_PSEL_DISCONNECTED, config.pselrxd);
		
		nrf_gpio_cfg_input(config.pseltxd, NRF_GPIO_PIN_PULLUP);
		 
		// nrf_gpio_input_disconnect(config.pseltxd);
   // VERIFY_SUCCESS(err_code);
	

	

	*out_port_struct = port_struct;
}



void uart_port_start(void* p)
{

	uart_port_wait_fixed_receive();
	//stop_soft_timer(uart_port_start);

}


void uart_port_transmit(uint8_t* Buf, uint16_t Size)
{

	
//	HAL_UART_Transmit_DMA(uart_port_inst, Buf, Size);
//	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,GPIO_PIN_RESET);
}

bool FinishWriteToBuffer(buffer_queue_struct* buffer_queue, uint16_t Size)
{
	if((buffer_queue->size - (buffer_queue->write_counter - buffer_queue->read_counter)) >= Size)
	{
		buffer_queue->write_counter += Size;
		return true;
	}else
		return false;

}

bool ReadBufferQueue(buffer_queue_struct* buffer_queue, uint16_t Size)
{
	if(buffer_queue->write_counter - buffer_queue->read_counter)
	{
		buffer_queue->read_counter += Size;

		if(buffer_queue->write_counter == buffer_queue->read_counter)
		{
			buffer_queue->write_counter = 0;
			buffer_queue->read_counter = 0;
		}
		return true;

	}else
		return false;
}

uint8_t* GetQueueBufferPointer(buffer_queue_struct* buffer_queue, uint16_t* Size)
{
	*Size = buffer_queue->write_counter - buffer_queue->read_counter;

	return &buffer_queue->queue_array[buffer_queue->write_counter];
}

uint16_t QueueSize(buffer_queue_struct* buffer_queue)
{
	return buffer_queue->write_counter - buffer_queue->read_counter;
}


uint8_t* ReadQueue(buffer_queue_struct* buffer_queue, uint16_t size)
{
	if(size <= buffer_queue->write_counter - buffer_queue->read_counter)
	{

		uint8_t* point = buffer_queue->queue_array + buffer_queue->read_counter;
		buffer_queue->read_counter += size;
		if(buffer_queue->write_counter == buffer_queue->read_counter)
		{
			buffer_queue->write_counter = 0;
			buffer_queue->read_counter = 0;
		}

		return point;
	}
	return 0;
}

/*
void uart_port_wait_receive(uint8_t* Buf, uint16_t Size)
{
	if(HAL_UART_Receive_DMA(uart_port_inst, Buf, Size) != HAL_OK)
	{
		HAL_UART_DMAStop(uart_port_inst);
		HAL_UART_Abort(uart_port_inst);
		HAL_UART_Receive_DMA(uart_port_inst, Buf, Size);
	}
}*/

void uart_port_wait_fixed_receive()
{
	enable_float_len_uart = false;

	uint16_t Size = 0;

//	uint8_t* rx_pointer = GetQueueBufferPointer(&rx_queue,&Size);

	
}

void uart_port_wait_float_receive(uint16_t size)
{
	enable_float_len_uart = true;
	uint16_t Size = 0;

//	uint8_t* rx_pointer = GetQueueBufferPointer(&rx_queue,&Size);

//	HAL_UART_Receive_DMA(uart_port_inst, rx_pointer, size);
	//start_soft_timer(uart_port_start,10,0,false,SOFT_TIMER_EXEC_IRQ_MODE);

}


void uart_port_receive(void* huart)
{
	if(huart == uart_port_inst)
	{
	
//		FinishWriteToBuffer(&rx_queue,huart->RxXferSize);

	//	parce(huart->pRxBuffPtr,huart->RxXferSize,UBI_PE_UART_BUS);



	//	start_soft_timer(uart_port_start,6,0,false,SOFT_TIMER_EXEC_IRQ_MODE);
	}
}

bool uart_port_receive_float_len()
{
	return enable_float_len_uart;
}

#endif
