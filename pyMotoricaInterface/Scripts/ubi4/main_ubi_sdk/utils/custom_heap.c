/*
  ******************************************************************************
  * @file    custom_heap.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "utils/custom_heap.h"
#include <string.h>
#include <stdlib.h>



custom_heap_obj_struct* create_heap(uint8_t* heapBuf, uint32_t heapSize, uint16_t NumHeapSlots)
{
	custom_heap_obj_struct* tHeap = malloc(sizeof(custom_heap_obj_struct));

	if(tHeap)
	{
		tHeap->heap = heapBuf;
		tHeap->heapSize = heapSize;
		tHeap->currentHeapSize = 0;
		tHeap->slots_array = malloc(sizeof(custom_heap_slot_struct)*NumHeapSlots);
		tHeap->slots_array_array_count = 0;
		tHeap->slots_array_array_size = NumHeapSlots;
		memset(tHeap->slots_array,0,sizeof(custom_heap_slot_struct)*NumHeapSlots);
		return tHeap;
	}
	return 0;
}

custom_heap_slot_struct* get_heap_buffer(custom_heap_obj_struct* heap, uint32_t BufferSize)
{
	int index = -1;

	if(heap == 0) return 0;

	if(heap->slots_array_array_count >= heap->slots_array_array_size) return 0;

		for(uint16_t i = heap->slots_array_array_count; i<heap->slots_array_array_size; i++)
		{
			if(heap->slots_array[i].p == 0)
			{
				index = i;
				break;
			}
		}


		if(index == -1)
		{
			for(uint16_t i = 0; i<heap->slots_array_array_count; i++)
			{
				if(heap->slots_array[i].p == 0)
				{
						index = i;
						break;
				}
			}
		}

		if(index != -1)
		{
			if(index>0)
			{
				if(heap->slots_array[index-1].p != 0)
				{
					if((heap->slots_array[index-1].p + heap->slots_array[index-1].size + BufferSize) < (heap->heap + heap->heapSize))
					{
						heap->slots_array[index].p = heap->slots_array[index-1].p + heap->slots_array[index-1].size;
						heap->slots_array[index].size = BufferSize;
					}else
						return 0;
				}else
					return 0;
			}else if(index == 0)
			{
				if(((uint32_t)heap->slots_array[index].p + BufferSize) < ((uint32_t)heap->heap + heap->heapSize))
				{
					heap->slots_array[index].p = heap->heap;
					heap->slots_array[index].size = BufferSize;
				}else
					return 0;
			}


			heap->currentHeapSize += heap->slots_array[index].size;
			heap->slots_array_array_count++;

			return &heap->slots_array[index];
		}
		return 0 ;
}

void free_heap_buffer(custom_heap_obj_struct* heap, custom_heap_slot_struct* heap_slot_str)
{
	if(heap)
	{
		if(heap_slot_str)
		{
			if(heap->slots_array_array_count)
			{
				heap->currentHeapSize -= heap_slot_str->size;
				heap_slot_str->p = 0;
				heap_slot_str->size = 0;
				heap->slots_array_array_count--;

			}
		}
	}

}

void free_heap(custom_heap_obj_struct* heap)
{
	if(!heap) return;

	//memset(heap->slots_array,0,sizeof(custom_heap_slot_struct)*heap->slots_array_array_size);
	heap->currentHeapSize = 0;
	heap->slots_array_array_count = 0;
}
