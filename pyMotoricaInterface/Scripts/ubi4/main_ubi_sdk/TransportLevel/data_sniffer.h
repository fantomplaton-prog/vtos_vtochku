/*
  ******************************************************************************
  * @file    data_sniffer.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef TRANSPORTLEVEL_DATA_SNIFFER_H_
#define TRANSPORTLEVEL_DATA_SNIFFER_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
	uint8_t FilterByAddress: 	1;
	uint8_t FilterByCommand: 	1;
	uint8_t FilterBySubCommand: 1;
	uint8_t FilterByData:	 	1;
	uint8_t Reserved:		 	4;

	uint8_t UUID;

	uint8_t AddressFilter;
	uint8_t CommandFilter;

	uint8_t* SubCommandFilter;
	uint8_t SubCommandSizeFilter;

	uint8_t* DataFilter;
	uint8_t DataSizeFilter;

	void* ObjectBuffer;
	uint16_t ObjectBufferSize;


	void (*receiver_callback)(void* sniffer, void* protocol_str,void* pack_info, uint8_t command, uint8_t* data_buf, uint16_t size);

}sniffer_struct;


typedef struct
{
	uint8_t snifferArraySize;
	uint8_t snifferCounter;
	sniffer_struct* snifferArray;
	void* owner;
}sniffer_conteiner_struct;


void init_sniffer_conteiner(sniffer_conteiner_struct** sniffer_conteiner, uint8_t size);

void exec_sniffer_conteiner(sniffer_conteiner_struct* sniffer_conteiner, void* protocol_str,void* pack_info, uint8_t Address, uint8_t command, uint8_t* data_buf, uint16_t size);

sniffer_struct* create_sniffer(sniffer_conteiner_struct* sniffer_conteiner);
void remove_sniffer(sniffer_conteiner_struct* sniffer_conteiner, sniffer_struct* sniffer);
sniffer_struct* get_sniffer(sniffer_conteiner_struct* sniffer_conteiner, uint8_t sniffer_uuid);


#endif /* TRANSPORTLEVEL_DATA_SNIFFER_H_ */
