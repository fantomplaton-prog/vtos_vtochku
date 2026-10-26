/*
  ******************************************************************************
  * @file    custom_heap.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef UTILS_CUSTOM_HEAP_H_
#define UTILS_CUSTOM_HEAP_H_


#include "stdint.h"

typedef enum
{
	CHSE_FREE			= 0x00,
	CHSE_BUSY

}custom_heap_slot_status_enum;



typedef struct
{
	uint8_t* p;
	uint16_t size;
}custom_heap_slot_struct;


typedef struct
{
	uint8_t* heap;
	uint32_t heapSize;
	uint32_t currentHeapSize;

	uint16_t slots_array_array_size;
	uint16_t slots_array_array_count;
	custom_heap_slot_struct* slots_array;

}custom_heap_obj_struct;


custom_heap_obj_struct* create_heap(uint8_t* heapBuf, uint32_t heapSize, uint16_t NumHeapSlots);
custom_heap_slot_struct* get_heap_buffer(custom_heap_obj_struct* heap, uint32_t BufferSize);
void free_heap_buffer(custom_heap_obj_struct* heap, custom_heap_slot_struct* heap_slot_str);
void free_heap(custom_heap_obj_struct* heap);

#endif /* UTILS_CUSTOM_HEAP_H_ */
