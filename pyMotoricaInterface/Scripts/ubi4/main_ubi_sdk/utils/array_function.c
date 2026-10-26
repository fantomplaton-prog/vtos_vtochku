/*
  ******************************************************************************
  * @file    array_function.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "utils/array_function.h"

#include <stdlib.h>
#include <malloc.h>

uint32_t differentNumberInArray(uint8_t *array, uint32_t size)
{
    uint32_t differentNumber = 0;
    for (uint32_t i = 0; i < size-1; i++)
        if(array[i]^array[i+1])
            differentNumber++;

    return differentNumber+1;
}

uint8_t* sort(uint8_t *array, uint32_t size)
{

     bool isSorted;

     for (uint32_t i = 0; i < size; i++) {

       isSorted = true;
       for (uint32_t j = 0; j < size - i - 1; j++) {

         if (array[j] > array[j + 1]) {

           uint8_t tmp = array[j];

           array[j] = array[j + 1];

           array[j + 1] = tmp;

           isSorted = false;

         }

       }

       if (isSorted) return array;
     }

     return array;
}

void Histogram(uint8_t *array, uint32_t size, uint32_t* dif_num, uint8_t **num_arr, uint8_t **sum_arr, bool sorted)
{
    if(!sorted)
      sort(array,size);

    uint32_t differentNumber = 0;
    for (uint32_t i = 0; i < size-1; i++)
        if(array[i]^array[i+1])
            differentNumber++;

    differentNumber+=1;

    *dif_num = differentNumber;

    uint8_t* MatchesArray = (uint8_t*)malloc(differentNumber);
    uint8_t* NumMatchesArray = (uint8_t*)malloc(differentNumber);

    uint8_t n = 0;
    for(uint8_t i = 0; i<differentNumber; i++)
    {

        MatchesArray[i] = array[n];
        NumMatchesArray[i] = 1;
        n++;

        while(MatchesArray[i] == array[n] && n<size)
        {
            NumMatchesArray[i]++;
            n++;
        }
    }

    *num_arr = MatchesArray;
    *sum_arr = NumMatchesArray;

}
