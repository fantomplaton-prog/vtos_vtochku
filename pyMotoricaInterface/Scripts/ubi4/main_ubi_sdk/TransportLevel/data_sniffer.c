/*
  ******************************************************************************
  * @file    data_sniffer.c
  * @author  A.Alborov
  ******************************************************************************
 */


#include "TransportLevel/data_sniffer.h"
#include "string.h"

void init_sniffer_conteiner(sniffer_conteiner_struct** sniffer_conteiner, uint8_t size)
{
	sniffer_conteiner_struct* t_sniffer_conteiner = (sniffer_conteiner_struct*)malloc(sizeof(sniffer_conteiner_struct));

	t_sniffer_conteiner->snifferArray = (sniffer_struct*)malloc(sizeof(sniffer_struct)*size);
	t_sniffer_conteiner->snifferArraySize = size;
	t_sniffer_conteiner->snifferCounter = 0;

	memset(t_sniffer_conteiner->snifferArray,0,sizeof(sniffer_struct)*size);

	*sniffer_conteiner = t_sniffer_conteiner;
}

sniffer_struct* create_sniffer(sniffer_conteiner_struct* sniffer_conteiner)
{
	int index = -1;
	for(uint16_t i = sniffer_conteiner->snifferCounter; i<sniffer_conteiner->snifferArraySize; i++)
	{
		if(sniffer_conteiner->snifferArray[i].UUID == 0)
		{
			index = i;
			break;
		}
	}

	if(index == -1)
	{
		for(uint16_t i = 0; i<sniffer_conteiner->snifferCounter; i++)
		{
			if(sniffer_conteiner->snifferArray[i].UUID == 0)
			{
					index = i;
					break;
			}
		}
	}

	if(index != -1)
	{
		sniffer_conteiner->snifferArray[index].UUID = index + 1;
		sniffer_conteiner->snifferCounter++;
		return &sniffer_conteiner->snifferArray[index];
	} return 0;
}

void exec_sniffer_conteiner(sniffer_conteiner_struct* sniffer_conteiner, void* protocol_str,void* pack_info, uint8_t Address, uint8_t command, uint8_t* data_buf, uint16_t size)
{

	uint8_t l_counter = 0;

	for(uint8_t i = 0; i<sniffer_conteiner->snifferArraySize; i++)
	{
		if(sniffer_conteiner->snifferArray[i].UUID != 0)
		{

			l_counter++;

			sniffer_struct* sniffer = &sniffer_conteiner->snifferArray[i];

			if(sniffer->FilterByAddress)
				if(sniffer->AddressFilter != Address) continue;

			if(sniffer->FilterByCommand)
				if(sniffer->CommandFilter != command) continue;

			if(sniffer->FilterBySubCommand)
			{
				for(uint8_t n = 0; n<sniffer->SubCommandSizeFilter; n++)
				{
					if(sniffer->SubCommandFilter[n] != data_buf[n]) continue;
				}
			}

			if(sniffer->FilterByData)
			{
				for(uint8_t n = 0; n<sniffer->DataSizeFilter; n++)
				{
					if(sniffer->DataFilter[n] != data_buf[n + sniffer->SubCommandSizeFilter]) continue;
				}
			}

			if(sniffer->receiver_callback)
				sniffer->receiver_callback(sniffer, protocol_str, pack_info,command,data_buf+sniffer->SubCommandSizeFilter,size-sniffer->SubCommandSizeFilter);
		}

		if(sniffer_conteiner->snifferCounter >= l_counter) return;



	}
}

void remove_sniffer(sniffer_conteiner_struct* sniffer_conteiner, sniffer_struct* sniffer)
{
	if(sniffer->UUID != 0)
	{
		//memset((uint8_t*)sniffer,0,sizeof(sniffer_struct));
		sniffer->UUID = 0;
		sniffer_conteiner->snifferCounter--;
	}
}

sniffer_struct* get_sniffer(sniffer_conteiner_struct* sniffer_conteiner, uint8_t sniffer_uuid)
{
	return &sniffer_conteiner->snifferArray[sniffer_uuid - 1];
}
