/*
  ******************************************************************************
  * @file    while_sheduller.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "utils/while_sheduller.h"
#include "stdbool.h"

typedef void (* while_sheduller_event_handler_t)(void* p);

static uint8_t count = 0;

typedef struct 
{
	while_sheduller_event_handler_t handler;
	void* p;
	uint8_t repeat: 1;
	
}sheduller_handler_struct;

#define SHEDULLER_BUFFER_SIZE 50

sheduller_handler_struct sheduller_buffer_data[SHEDULLER_BUFFER_SIZE];


void sheduller_exece()
{
	uint8_t lcnt = 0;
	for(uint8_t i = 0; i<SHEDULLER_BUFFER_SIZE; i++)
	{
		if(sheduller_buffer_data[i].handler != 0)
		{
			sheduller_buffer_data[i].handler(sheduller_buffer_data[i].p);
			lcnt++;
			if((!sheduller_buffer_data[i].repeat) && (sheduller_buffer_data[i].handler != 0))
			{
				sheduller_buffer_data[i].handler = 0;
				sheduller_buffer_data[i].p = 0;
				count--;
				lcnt--;
			}
		}
		if(lcnt >= count) return;
	}
}

bool while_sheduller_add_func(void (*handler)(void* p),void* p, bool repeat)
{
	if(handler == 0) return false;
	
	uint8_t lcnt = 0;
	
	for(uint8_t i = 0; i<SHEDULLER_BUFFER_SIZE; i++)
	{
		if(sheduller_buffer_data[i].handler != 0)
			lcnt++;
		
		if(sheduller_buffer_data[i].handler == handler)
		{
			sheduller_buffer_data[i].p = p;
			sheduller_buffer_data[i].repeat = repeat;
			return true;
		}
		if(lcnt >= count) break;
	}
	
	for(uint8_t i = 0; i<SHEDULLER_BUFFER_SIZE; i++)
	{
		if(sheduller_buffer_data[i].handler == 0)
		{
			sheduller_buffer_data[i].handler = handler;
			sheduller_buffer_data[i].p = p;
			sheduller_buffer_data[i].repeat = repeat;
			count++;
			return true;
		}
	}
		return false;
}
