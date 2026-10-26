/*
  ******************************************************************************
  * @file    p2p_protocol.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef INC_P2P_PROTOCOL_H_
#define INC_P2P_PROTOCOL_H_


#include "TransportLevel/a_protocol.h"
#include "stdint.h"


typedef struct
{
	uint8_t port:3;
	uint8_t exist_uid: 1;
	uint8_t package_type: 2;
	uint8_t direction: 1;
	uint8_t bridge: 1;

	uint8_t addr;
	uint8_t package_code;
	uint8_t Data[6];
	uint8_t crc;
}__attribute__((__packed__)) p2p_port_header;


//P2P high level port

typedef struct
{
	uint8_t port:5;
	uint8_t waiting_answer: 	1;
	uint8_t request_type:		1;
	uint8_t bridge: 				1;
	uint8_t package_code;
	uint8_t package_id;
	uint16_t package_size;
	uint8_t crc;
	uint8_t address;
}__attribute__((__packed__)) p2p_high_level_port_header;

/*
typedef struct
{
    uint8_t port:7;
    uint8_t brige: 1;
    uint8_t package_code;
    uint8_t package_id;
    uint16_t package_size;
    uint8_t crc;
    uint8_t address;
}__attribute__((__packed__)) p2p_brige_high_level_port_header;
*/



typedef struct
{
	uint16_t ID;
	p2p_high_level_port_header header;
}__attribute__((__packed__))usb_package_info_struct;

void p2p_hight_level_protocol_init(base_port_struct* port, ubi4_bus_protocol_obj_struct** protocol_obj_str);



#endif /* INC_P2P_PROTOCOL_H_ */
