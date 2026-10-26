/*
  ******************************************************************************
  * @file    static_conteiner.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef UTILS_STATIC_CONTEINER_H_
#define UTILS_STATIC_CONTEINER_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct
{

	uint16_t ArraySize;
	uint16_t DataSize;
	void (*addDataCallback)(void* conteiner_obj, void* conteiner_p, void* inputData, uint16_t ID);
	void (*removeDataCallback)(void* conteiner_obj, void* conteiner_p, uint16_t ID);

}static_conteiner_init_struct;



typedef struct
{
	uint8_t* Array;
	uint16_t Counter;
	static_conteiner_init_struct* init_struct;
	void* owner;
}static_conteiner_struct;


void init_static_conteiner(static_conteiner_init_struct* initStruct, static_conteiner_struct** conteiner_obj);

void* StaticConteiner_addData(static_conteiner_struct* conteiner_obj, void* Data);
void* StaticConteiner_getSegment(static_conteiner_struct* conteiner_obj, uint16_t DataID);
void* StaticConteiner_getFirstSegment(static_conteiner_struct* conteiner_obj);
void* StaticConteiner_getNextSegment(static_conteiner_struct* conteiner_obj, uint16_t cur_seg_id);
void* StaticConteiner_getData(static_conteiner_struct* conteiner_obj, uint16_t DataID);
bool  StaticConteiner_removeData(static_conteiner_struct* conteiner_obj, uint16_t DataID);
void  StaticConteiner_clear(static_conteiner_struct* conteiner_obj);
void StaticConteiner_ForeachStart(static_conteiner_struct* conteiner_obj,void*owner, void (*sc_callback_func)(static_conteiner_struct*, void*, void*, uint16_t));




#endif /* UTILS_STATIC_CONTEINER_H_ */
