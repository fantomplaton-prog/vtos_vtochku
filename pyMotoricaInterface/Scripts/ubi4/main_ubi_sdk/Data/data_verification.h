/*
  ******************************************************************************
  * @file    data_verification.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef INC_DATA_VERIFICATION_H_
#define INC_DATA_VERIFICATION_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

uint8_t getCRC(uint8_t *data, uint32_t count);

#ifdef __cplusplus
}
#endif


#endif /* INC_DATA_VERIFICATION_H_ */
