/*
  ******************************************************************************
  * @file    a_protocol_struct.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef INC_A_PROTOCOL_STRUCT_H_
#define INC_A_PROTOCOL_STRUCT_H_



#include "stdint.h"
#include "stdbool.h"


typedef struct
{
	void* pack_info;
	uint8_t command;
	bool request_type;
//	bool wait_answer;  // [OPTIMIZATION POSSIBLE] объеденить флаги в один байт
	uint8_t* data_buf;
	uint16_t size;
}protocol_receive_struct;


#endif /* INC_A_PROTOCOL_STRUCT_H_ */
