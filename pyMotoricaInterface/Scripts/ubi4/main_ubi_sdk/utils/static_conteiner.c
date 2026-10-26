/*
  ******************************************************************************
  * @file    static_conteiner.c
  * @author  A.Alborov
  ******************************************************************************
*/

#include "utils/static_conteiner.h"
#include "string.h"

void init_static_conteiner(static_conteiner_init_struct* initStruct, static_conteiner_struct** conteiner_obj)
{
	static_conteiner_struct* t_static_conteiner = (static_conteiner_struct*)malloc(sizeof(static_conteiner_struct));

	t_static_conteiner->Counter = 0;
	t_static_conteiner->init_struct = initStruct;
	t_static_conteiner->Array = (void*)malloc((initStruct->DataSize)*initStruct->ArraySize);
	memset(t_static_conteiner->Array, 0,(initStruct->DataSize)*initStruct->ArraySize);

	*conteiner_obj = t_static_conteiner;
}

void* StaticConteiner_addData(static_conteiner_struct* conteiner_obj, void* Data)
{
	int index = -1;
	uint16_t* pID = 0;

	for(uint16_t i = conteiner_obj->Counter; i<conteiner_obj->init_struct->ArraySize; i++)
	{
		pID = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);
		if(*pID == 0)
		{
			index = i;
			break;
		}
	}

	if(index == -1)
	{
		for(uint16_t i = 0; i<conteiner_obj->Counter; i++)
		{
			pID = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);
			if(*pID == 0)
			{
				index = i;
				break;
			}
		}
	}

	if(index == -1) return 0;

	pID = (uint16_t*)(conteiner_obj->Array + index*conteiner_obj->init_struct->DataSize);

	*pID = index + 1;

	conteiner_obj->Counter++;

	if(conteiner_obj->init_struct->addDataCallback)
	{
		conteiner_obj->init_struct->addDataCallback(conteiner_obj, pID, Data, *pID);
	}else
	{
		memcpy((uint8_t*)pID+2, (uint8_t*)Data+2, conteiner_obj->init_struct->DataSize-2);
	}

	return (void*)pID;
}

void* StaticConteiner_getSegment(static_conteiner_struct* conteiner_obj, uint16_t DataID)
{
	if(DataID == 0)return 0;

	void* pSeg = conteiner_obj->Array + (DataID-1)*(conteiner_obj->init_struct->DataSize);

	return pSeg;

}

void* StaticConteiner_getFirstSegment(static_conteiner_struct* conteiner_obj)
{
	uint16_t* pSeg = 0;
	for(uint16_t i = 0; i<conteiner_obj->init_struct->ArraySize; i++)
	{
		pSeg = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);
		if(*pSeg >0)
			return pSeg;
	}

	return 0;
}


void StaticConteiner_ForeachStart(static_conteiner_struct* conteiner_obj,void*owner, void (*sc_callback_func)(static_conteiner_struct*, void*, void*, uint16_t))
{
	if(conteiner_obj->Counter == 0) return;

	uint16_t f_counter = 0;
	for(uint16_t i = 1; i<conteiner_obj->init_struct->ArraySize+1; i++)
	{
		uint16_t* pID = StaticConteiner_getSegment(conteiner_obj, i);

		if(*pID)
		{
			sc_callback_func(conteiner_obj, owner,(void*)(pID+2),*pID);
			f_counter++;
			if(f_counter >= conteiner_obj->Counter)return;
		}
	}
}



void* StaticConteiner_getNextSegment(static_conteiner_struct* conteiner_obj, uint16_t cur_seg_id)
{
	uint16_t* pSeg = 0;
	for(uint16_t i = cur_seg_id; i<conteiner_obj->init_struct->ArraySize; i++)
	{
		pSeg = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);

		if(*pSeg >0)
			return pSeg;
	}

	return 0;
}


void* StaticConteiner_getData(static_conteiner_struct* conteiner_obj, uint16_t DataID)
{
	if(DataID == 0)return 0;

	void* pSeg = conteiner_obj->Array + (DataID-1)*(conteiner_obj->init_struct->DataSize);

	return (uint8_t*)pSeg + 2;
}


bool StaticConteiner_removeData(static_conteiner_struct* conteiner_obj, uint16_t DataID)
{
	if(DataID == 0) return false;

	uint16_t* pID = (uint16_t*)(conteiner_obj->Array + (DataID-1)*(conteiner_obj->init_struct->DataSize));
	uint16_t tID = *pID;

	if(conteiner_obj->init_struct->removeDataCallback)
		conteiner_obj->init_struct->removeDataCallback(conteiner_obj,(void*)pID, tID);

	*pID = 0;

	conteiner_obj->Counter--;
	return true;

}

void StaticConteiner_clear(static_conteiner_struct* conteiner_obj)
{

	uint16_t t_counter = conteiner_obj->Counter;
	uint16_t* pID = 0;

	for(int i = t_counter-1; i >= 0; i--)
	{
		pID = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);
		
		if(*pID != 0)
		{
			StaticConteiner_removeData(conteiner_obj, *pID);
		}
	}

	if(conteiner_obj->Counter)
	{
		for(uint16_t i = t_counter; i<conteiner_obj->init_struct->ArraySize; i++)
		{
			pID = (uint16_t*)(conteiner_obj->Array + i*conteiner_obj->init_struct->DataSize);
			if(*pID != 0)
			{
				StaticConteiner_removeData(conteiner_obj, *pID);
			}
		}
	}

}
