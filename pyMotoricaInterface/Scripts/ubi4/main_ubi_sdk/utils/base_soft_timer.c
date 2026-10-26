/*
  ******************************************************************************
  * @file    base_soft_timer.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "utils/base_soft_timer.h"
#include "HAL_DRIVERS.h"
#include "stdlib.h"


#include "utils/while_sheduller.h"



//static uint8_t soft_timer_count = 0;

//#define SOFT_TIMER_BUFFER_SIZE 50
//soft_timer_handler_struct soft_timer_buffer_data[SOFT_TIMER_BUFFER_SIZE];

//#define TEMPLATE_BUFFER_NAME soft_timer_buffer
//#define TEMPLATE_BUFFER_DATA_T soft_timer_handler_struct
//#define TEMPLATE_BUFFER_SIZE SOFT_TIMER_BUFFER_SIZE  
//#include "FIFO_buffer.h"    



void soft_timer_init(soft_timer_init_struct* init_struct, soft_timer_class_struct** soft_timer_obj)
{

	soft_timer_class_struct* t_soft_timer_obj = (soft_timer_class_struct*)malloc(sizeof(soft_timer_class_struct));
	t_soft_timer_obj->soft_timer_buffer = (soft_timer_handler_struct*)malloc(sizeof(soft_timer_handler_struct)*init_struct->timers_size);

	t_soft_timer_obj->init_struct = init_struct;

	for(uint16_t i = 0; i < init_struct->timers_size; i++)
	{
		memset(&t_soft_timer_obj->soft_timer_buffer[i], 0, sizeof(soft_timer_handler_struct));
		t_soft_timer_obj->soft_timer_buffer[i].instance = -1;
	}
	t_soft_timer_obj->soft_timer_count = 0;

	*soft_timer_obj = t_soft_timer_obj;
}



void timer_owerflow(soft_timer_class_struct* soft_timer_obj)
{
	for(uint8_t i = 0; i<soft_timer_obj->init_struct->timers_size; i++)
	{
		if(soft_timer_obj->soft_timer_buffer[i].instance != -1)
			soft_timer_obj->soft_timer_buffer[i].overflow = false;
	}
}


int create_soft_timer(soft_timer_class_struct* soft_timer_obj, void (*handler)(void* timer_obj, int instance, void* p),void* p, bool repeat,bool delete_on_completion,soft_timer_mode_enum handler_exec_mode)
{
	uint32_t global_time_ms = soft_timer_obj->init_struct->GetTick();

	if(handler!=0 && soft_timer_obj->soft_timer_count<soft_timer_obj->init_struct->timers_size)
	{

		for(uint16_t i = soft_timer_obj->soft_timer_count; i<soft_timer_obj->init_struct->timers_size; i++)
		{
			if(soft_timer_obj->soft_timer_buffer[i].instance == -1)
			{
				soft_timer_obj->soft_timer_buffer[i].handler = handler;
				soft_timer_obj->soft_timer_buffer[i].p = p;
				soft_timer_obj->soft_timer_buffer[i].mode = handler_exec_mode;
				soft_timer_obj->soft_timer_buffer[i].repeat = repeat;
				soft_timer_obj->soft_timer_buffer[i].delete_on_completion = delete_on_completion;
				soft_timer_obj->soft_timer_buffer[i].instance = i;

				soft_timer_obj->soft_timer_count++;

				return i;
			}
		}

		for(uint16_t i = 0; i< soft_timer_obj->soft_timer_count; i++)
		{
			if(soft_timer_obj->soft_timer_buffer[i].instance == -1)
			{
				soft_timer_obj->soft_timer_buffer[i].handler = handler;
				soft_timer_obj->soft_timer_buffer[i].p = p;
				soft_timer_obj->soft_timer_buffer[i].mode = handler_exec_mode;
				soft_timer_obj->soft_timer_buffer[i].repeat = repeat;
				soft_timer_obj->soft_timer_buffer[i].delete_on_completion = delete_on_completion;
				soft_timer_obj->soft_timer_buffer[i].instance = i;

				soft_timer_obj->soft_timer_count++;

				return i;
			}
		}
	}
	return -1;
}


bool remove_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance)
{
	if(soft_timer_obj->soft_timer_buffer[instance].instance != -1)
	{
		memset(&soft_timer_obj->soft_timer_buffer[instance],0,sizeof(soft_timer_handler_struct));
		soft_timer_obj->soft_timer_buffer[instance].instance = -1;
		soft_timer_obj->soft_timer_count--;
		return true;
	}
	return false;
}

bool stop_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance)
{
	if(soft_timer_obj->soft_timer_buffer[instance].instance != -1)
	{
		
		soft_timer_obj->soft_timer_buffer[instance].run = 0;

		return true;
	}
	return false;
}


bool start_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance, uint32_t time_ms)
{

	if(soft_timer_obj->init_struct->isTimerEnable)
	{
		if(!soft_timer_obj->init_struct->isTimerEnable())
		{
			if(soft_timer_obj->init_struct->StartTickTimer)
			{
				soft_timer_obj->init_struct->StartTickTimer();
			}
		}
	}

	uint32_t global_time_ms = soft_timer_obj->init_struct->GetTick();

	soft_timer_obj->soft_timer_buffer[instance].time_ms = time_ms;
	soft_timer_obj->soft_timer_buffer[instance].start_time_ms = global_time_ms;

	if(global_time_ms + time_ms <= global_time_ms)
		soft_timer_obj->soft_timer_buffer[instance].overflow = true;
	else
		soft_timer_obj->soft_timer_buffer[instance].overflow = false;

	soft_timer_obj->soft_timer_buffer[instance].run = 1;

	return true;
}


bool soft_timer_is_run(soft_timer_class_struct* soft_timer_obj, int instance)
{
	return soft_timer_obj->soft_timer_buffer[instance].run;
}




void soft_timer_tick_exec(soft_timer_class_struct* soft_timer_obj)
{
	if(soft_timer_obj->soft_timer_count == 0)
	{
		if(soft_timer_obj->init_struct->isTimerEnable)
		{
			if(soft_timer_obj->init_struct->isTimerEnable())
			{
				if(soft_timer_obj->init_struct->StopTickTimer)
									soft_timer_obj->init_struct->StopTickTimer();
			}
		}

		return;
	}
	uint8_t lcnt = 0;
	uint32_t global_time_ms = soft_timer_obj->init_struct->GetTick();

	for(uint8_t i = 0; i<soft_timer_obj->init_struct->timers_size; i++)
	{
		if(soft_timer_obj->soft_timer_buffer[i].instance != -1)
		{
			lcnt++;

			if(soft_timer_obj->soft_timer_buffer[i].run)
			{
				if((global_time_ms >= (soft_timer_obj->soft_timer_buffer[i].time_ms + soft_timer_obj->soft_timer_buffer[i].start_time_ms))&&(soft_timer_obj->soft_timer_buffer[i].overflow == false))
				{

					if(soft_timer_obj->soft_timer_buffer[i].mode == SOFT_TIMER_EXEC_IRQ_MODE)
					{

						soft_timer_handler_struct temp_timer_str = soft_timer_obj->soft_timer_buffer[i];

						if(soft_timer_obj->soft_timer_buffer[i].repeat)
							soft_timer_obj->soft_timer_buffer[i].start_time_ms += soft_timer_obj->soft_timer_buffer[i].time_ms;
						else
						{
							if(soft_timer_obj->soft_timer_buffer[i].instance != -1)
							{
								soft_timer_obj->soft_timer_buffer[i].run = 0;
								if(soft_timer_obj->soft_timer_buffer[i].delete_on_completion)
								{
									remove_soft_timer(soft_timer_obj, i);
									lcnt--;
								}
							}
						}

						temp_timer_str.handler(soft_timer_obj, i,temp_timer_str.p);
					//	soft_timer_obj->soft_timer_buffer[i].handler(soft_timer_obj, i,temp_timer_str.p);

					}else if(soft_timer_obj->soft_timer_buffer[i].mode == SOFT_TIMER_EXEC_WHILE_MODE)
					{
						//while_sheduller_add_func(soft_timer_buffer_data[i].handler, soft_timer_buffer_data[i].p, false);

						soft_timer_handler_struct temp_timer_str = soft_timer_obj->soft_timer_buffer[i];

						if(soft_timer_obj->soft_timer_buffer[i].repeat)
							soft_timer_obj->soft_timer_buffer[i].start_time_ms += soft_timer_obj->soft_timer_buffer[i].time_ms;
						else
						{
							if(soft_timer_obj->soft_timer_buffer[i].instance != -1)
							{
								soft_timer_obj->soft_timer_buffer[i].run = 0;
								if(soft_timer_obj->soft_timer_buffer[i].delete_on_completion)
								{
									remove_soft_timer(soft_timer_obj, i);
									lcnt--;
								}
							}
						}
					}
				}
			}
		}
		if(lcnt >= soft_timer_obj->soft_timer_count) break;
	}
}

