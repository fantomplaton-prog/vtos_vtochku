/*
  ******************************************************************************
  * @file    array_function.h
  * @author  A.Alborov
  ******************************************************************************
*/



#ifndef __ARRAY_FUNCTION_H
#define __ARRAY_FUNCTION_H



#ifdef __cplusplus
 extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

uint32_t differentNumberInArray(uint8_t *array, uint32_t size);
void Histogram(uint8_t *array, uint32_t size, uint32_t* dif_num, uint8_t **num_arr, uint8_t **sum_arr, bool sorted);
uint8_t* sort(uint8_t *array, uint32_t size);


#ifdef __cplusplus
}
#endif

#endif
