/*
  ******************************************************************************
  * @file    usb_cdc_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include <string.h>

#include "TransportLevel/ports/esp32/usb_cdc_port.h"

#ifdef UBI_SDK_TARGET_ESP32

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"


#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "TransportLevel/main_receiver.h"
#include "TransportLevel/a_protocol.h"
#include "TransportLevel/p2p_protocol.h"


static const char *TAG = "example";


static usb_cdc_port_struct* this_usb_port = 0;

//static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
//static uint8_t tx_buf[CONFIG_TINYUSB_CDC_TX_BUFSIZE + 1];

//uint8_t long_package = 0;

//size_t rx_buffer_offset = 0;


/*
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{

	size_t rx_size = 0;


    esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf+rx_buffer_offset, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK) {
       // buf[rx_buffer_offset] = '\0';
    	rx_buffer_offset += rx_size;

    static p2p_high_level_port_header* usb_header = 0;
    static uint8_t headerSize = 0;


    if(long_package == 0)
    {

		usb_header = (p2p_high_level_port_header*)rx_buf;

		if(usb_header->brige)
			headerSize = sizeof(p2p_brige_high_level_port_header);
		else
			headerSize = sizeof(p2p_high_level_port_header);

		if(rx_size == usb_header->package_size + headerSize)
		{
			rx_size = rx_buffer_offset;

			long_package = 0;
			usb_header = 0;
			headerSize = 0;
			rx_buffer_offset = 0;

			parce(rx_buf, rx_size, UBI_PE_USB);

		}else
		{
			long_package = 1;
		}
    }else
    {
    	if(rx_buffer_offset == usb_header->package_size + headerSize)
		{
    		rx_size = rx_buffer_offset;

			long_package = 0;
			usb_header = 0;
			headerSize = 0;
			rx_buffer_offset = 0;

			parce(rx_buf, rx_size, UBI_PE_USB);

		}else
		{
			long_package = 1;
		}
    }

    ESP_LOGI(TAG, "Got data (%d bytes): %s", rx_size, rx_buf);
  } else {
      ESP_LOGE(TAG, "Read error");
  }
}
*/

volatile uint8_t usb_rx_counter = 0;

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
	if(this_usb_port)
	{
		if(this_usb_port->Base.wait_rx_size == 0) return;

		usb_rx_counter++;

		size_t rx_size = 0;

		esp_err_t ret = tinyusb_cdcacm_read(itf, this_usb_port->Base.rx_buffer + this_usb_port->Base.rx_buffer_offset, this_usb_port->Base.rx_buffer_size - this_usb_port->Base.rx_buffer_offset, &rx_size);
		if(ret == ESP_OK)
		{
			if(rx_size >= this_usb_port->Base.wait_rx_size)
			{
				this_usb_port->last_read_size = rx_size;
				this_usb_port->Base.receive_data(this_usb_port->Base.protocol);
			}else
			{
				this_usb_port->Base.rx_buffer_offset = rx_size;
			}
		}

	}
}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rst = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed! dtr:%d, rst:%d", dtr, rst);
}


static void usb_cdc_port_write(usb_cdc_port_struct* port, uint8_t *buf, uint16_t size)
{
	tinyusb_cdcacm_write_queue(port->itf, buf, size);
	tinyusb_cdcacm_write_flush(port->itf, 0);
}

static void usb_read_data_buffer(usb_cdc_port_struct* port_str, uint8_t* out_buf, uint32_t out_buf_size, uint32_t* read_bytes_size)
{
	esp_err_t ret = tinyusb_cdcacm_read(port_str->itf, out_buf, out_buf_size, &read_bytes_size);
}


static void wait_receive(usb_cdc_port_struct* port_str, uint32_t buffer_offset, uint32_t len)
{
	port_str->Base.rx_buffer_offset = port_str->Base.rx_buffer_offset+buffer_offset;
	port_str->Base.wait_rx_size = len;
}

static void get_read_size(usb_cdc_port_struct* port_struct, uint32_t* len)
{
	*len = port_struct->last_read_size;
}

void usb_cdc_port_init(usb_cdc_init_port_struct* init_struct, usb_cdc_port_struct** out_port_struct)
{

	usb_cdc_port_struct* port_struct = malloc(sizeof(usb_cdc_port_struct));

	base_port_init(port_struct);

	port_struct->Base.get_receive_buffer_size = 0;
	port_struct->Base.read_data_buffer = usb_read_data_buffer;
	port_struct->init_struct = init_struct;

	port_struct->Base.init_struct = init_struct;

	port_struct->Base.port_type = UBI_PE_USB;

	port_struct->Base.rx_buffer = init_struct->rx_buffer;
	port_struct->Base.rx_buffer_size = init_struct->rx_buffer_size;
	port_struct->Base.tx_buffer = init_struct->tx_buffer;
	port_struct->Base.tx_buffer_size = init_struct->tx_buffer_size;

	port_struct->Base.get_receive_buffer_size = get_read_size;

	port_struct->Base.rx_buffer_offset = 0;

	port_struct->Base.get_rx_buffer = port_get_rx_buffer;
	port_struct->Base.get_tx_buffer = port_get_tx_buffer;
	port_struct->Base.write_direct = usb_cdc_port_write;
	port_struct->Base.wait_receive = wait_receive;
//	port_struct->Base.stop_receive = uart_port_stop_receive;

	port_struct->itf = TINYUSB_CDC_ACM_0;

	*out_port_struct = port_struct;
	this_usb_port = port_struct;



	ESP_LOGI(TAG, "USB initialization");
	tinyusb_config_t tusb_cfg = {}; // the configuration using default values
	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

	tinyusb_config_cdcacm_t acm_cfg = {
		.usb_dev = TINYUSB_USBDEV_0,
		.cdc_port = TINYUSB_CDC_ACM_0,
		.rx_unread_buf_sz = 64,
		.callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
		.callback_rx_wanted_char = NULL,
		.callback_line_state_changed = NULL,
		.callback_line_coding_changed = NULL
	};

	ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
	/* the second way to register a callback */
	ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
						TINYUSB_CDC_ACM_0,
						CDC_EVENT_LINE_STATE_CHANGED,
						&tinyusb_cdc_line_state_changed_callback));
	ESP_LOGI(TAG, "USB initialization DONE");
}
/*
void usb_cdc_initialize_port()
{
	ESP_LOGI(TAG, "USB initialization");
	    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
	    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

	    tinyusb_config_cdcacm_t acm_cfg = {
	        .usb_dev = TINYUSB_USBDEV_0,
	        .cdc_port = TINYUSB_CDC_ACM_0,
	        .rx_unread_buf_sz = 64,
	        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
	        .callback_rx_wanted_char = NULL,
	        .callback_line_state_changed = NULL,
	        .callback_line_coding_changed = NULL
	    };

	    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

	    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
	                        TINYUSB_CDC_ACM_0,
	                        CDC_EVENT_LINE_STATE_CHANGED,
	                        &tinyusb_cdc_line_state_changed_callback));
	    ESP_LOGI(TAG, "USB initialization DONE");

}
*/
#endif
