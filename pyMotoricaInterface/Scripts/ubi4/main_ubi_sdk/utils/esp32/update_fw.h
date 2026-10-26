/*
 * update_fw.h
 *
 *  Created on: 19 мар. 2023 г.
 *      Author: PC
 */

#ifndef INC_UPDATE_FW_H_
#define INC_UPDATE_FW_H_


#include "UBI_SDK.h"

#ifdef UBI_SDK_TARGET_ESP32
#include "TransportLevel/main_receiver.h"

typedef enum {
       GET_RUN_PROGRAM_TYPE = 1,
	   JUMP_TO_PROGRAM,
       CHECK_NEW_FW,
       INITIALIZE_FLASH,
       IDLE_LOADING_FW,
       LOAD_NEW_FW,
       HOLD_NEXT_PART,
       COMPLETE_UPDATE
 }update_fw_state_enum;

 typedef enum {
	 	BOOTLOADER = 1,
        MAIN_PROGRAM,
  }update_fw_program_type_enum;





 void Update_FW_Receive(command_selector_struct command);

#endif
#endif /* INC_UPDATE_FW_H_ */
