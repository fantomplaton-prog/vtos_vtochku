/*
  ******************************************************************************
  * @file    while_sheduller.h
  * @author  A.Alborov
  ******************************************************************************
*/



#ifndef __WHILE_SHEDULLER_H
#define __WHILE_SHEDULLER_H



#ifdef __cplusplus
 extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

void sheduller_exece();

bool while_sheduller_add_func(void (*handler)(void* p),void* p, bool repeat);


#ifdef __cplusplus
}
#endif

#endif
