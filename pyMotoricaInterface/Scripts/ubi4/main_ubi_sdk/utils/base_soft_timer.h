/*
  ******************************************************************************
  * @file    base_soft_timer.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef __BASE_SOFT_TIMER_H
#define __BASE_SOFT_TIMER_H


#ifdef __cplusplus
 extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"


typedef enum
{
	SOFT_TIMER_EXEC_WHILE_MODE,
	SOFT_TIMER_EXEC_IRQ_MODE
}soft_timer_mode_enum;


typedef void (* soft_timer_event_handler_t)(void* soft_timer_obj, int instance,void* p);
typedef void (* soft_timer_finish_handler_t)(int instance);



typedef struct
{
	int instance;
	soft_timer_event_handler_t handler;
	uint32_t start_time_ms;
	uint16_t time_ms;
	void* p;
	uint8_t mode: 1;
	uint8_t repeat: 1;
	uint8_t overflow: 1;
	uint8_t delete_on_completion: 1;
	uint8_t run: 1;

}soft_timer_handler_struct;


typedef struct
{
	uint8_t timers_size;
	uint32_t tick_step_us;

	uint32_t (*GetTick)(void);
	void (*StartTickTimer)(void);
	void (*StopTickTimer)(void);
	uint8_t (*isTimerEnable)(void);

}soft_timer_init_struct;

typedef struct
{
	uint8_t soft_timer_count;
	soft_timer_handler_struct* soft_timer_buffer;
	soft_timer_init_struct* init_struct;
}soft_timer_class_struct;



void soft_timer_init(soft_timer_init_struct* init_struct, soft_timer_class_struct** soft_timer_obj);

int create_soft_timer(soft_timer_class_struct* soft_timer_obj ,void (*handler)(void* timer_obj, int instance, void* p),void* p, bool repeat,bool delete_on_completion,soft_timer_mode_enum handler_exec_mode);
bool remove_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance);
bool stop_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance);

bool start_soft_timer(soft_timer_class_struct* soft_timer_obj, int instance, uint32_t time_ms);

void soft_timer_tick_exec(soft_timer_class_struct* soft_timer_obj);

void timer_owerflow(soft_timer_class_struct* soft_timer_obj);

bool soft_timer_is_run(soft_timer_class_struct* soft_timer_obj, int instance);

//void start_soft_while_timer(uint16_t ms, void (*callback)(void* ptr));

//bool start_soft_timer(void (*handler)(int instance, void* p), uint32_t time_ms,void* p, bool repeat,soft_timer_mode_enum handler_exec_mode);
//bool restart_soft_timer(void (*handler)(void* p), uint32_t time_ms,void* p, bool repeat,soft_timer_mode_enum handler_exec_mode);

//bool stop_soft_timer(void (*handler)(void* p));


//void soft_timer_ms_tick_exec();

void soft_timer_overflow_reset();



#ifdef __cplusplus
}
#endif



#endif //__BASE_SOFT_TIMER_H
