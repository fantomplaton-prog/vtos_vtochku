/*
  ******************************************************************************
  * @file    ble_gatt_server_port.c
  * @author  A.Alborov
  ******************************************************************************
*/

#ifdef UBI_SDK_TARGET_ESP32
#include "ble_gatt_server_port.h"

//#include "ble_gatt_server.h"


uint8_t service_uuid[16] = {0xfb, 0x34, 0x9b, 0x7f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00};

uint8_t character1_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x00};
uint8_t character1_desc_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00};

uint8_t character2_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF1, 0x00, 0x00, 0x00};
uint8_t character2_desc_uuid[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00};




static struct characteristics_param_struct characters[1] = {
		[0] = {
				.char_uuid.len = ESP_UUID_LEN_128,
				.char_uuid.uuid = {0},
				.descr_uuid.len = ESP_UUID_LEN_128,
				.descr_uuid.uuid = {0}
		}
};

static struct service_param_struct services[1] = {
		[0] = {
				.service_id.id.uuid.len = ESP_UUID_LEN_128,
				.service_id.id.uuid.uuid = {0},
				.service_id.is_primary = true,
				.characteristics_num = 2,
				.characteristics_arr = characters
		}
};


static struct gatts_default_profile_inst  custom_profile = {
		.service_arr = services,
		.service_num = 1
};





void ble_gatt_server_initialize_port()
{

	for(uint8_t i = 0; i<16; i++)
	{
		custom_profile.service_arr[0].service_id.id.uuid.uuid.uuid128[i] = service_uuid[i];
		custom_profile.service_arr[0].characteristics_arr[0].char_uuid.uuid.uuid128[i] = character1_uuid[i];
		custom_profile.service_arr[0].characteristics_arr[0].descr_uuid.uuid.uuid128[i] = character1_desc_uuid[i];

		custom_profile.service_arr[0].characteristics_arr[1].char_uuid.uuid.uuid128[i] = character2_uuid[i];
		custom_profile.service_arr[0].characteristics_arr[1].descr_uuid.uuid.uuid128[i] = character2_desc_uuid[i];


	}


	ble_gatt_server_init();
}

#endif
