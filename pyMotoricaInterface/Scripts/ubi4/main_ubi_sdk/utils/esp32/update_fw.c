/*
 * update_fw.c
 *
 *  Created on: 19 мар. 2023 г.
 *      Author: PC
 */


#include "update_fw.h"

#ifdef UBI_SDK_TARGET_ESP32

uint32_t write_fw_pointer = 0;

void GetRunProgramType(command_selector_struct command)
{
	uint8_t AnswerBuf[2];
	AnswerBuf[0] = GET_RUN_PROGRAM_TYPE;
	AnswerBuf[1] = MAIN_PROGRAM;

//	MainAnswer(command.header_id, AnswerBuf, 2,0,0);
}

void JumpToProgramCommand(command_selector_struct command)
{

}


void InicializeFlash(command_selector_struct command)
{

}

void LoadNewPart(command_selector_struct command)
{

}



void Update_FW_Receive(command_selector_struct command)
{
	update_fw_state_enum fw_update_state = command.data_buf[0];
	switch(fw_update_state)
	{
		case GET_RUN_PROGRAM_TYPE: GetRunProgramType(command); break;
		case JUMP_TO_PROGRAM: JumpToProgramCommand(command); 	break;
		case CHECK_NEW_FW: break;
		case INITIALIZE_FLASH: break;
		case IDLE_LOADING_FW: break;
		case LOAD_NEW_FW: break;
		case HOLD_NEXT_PART: break;
		case COMPLETE_UPDATE: break;

	}

}

#endif
