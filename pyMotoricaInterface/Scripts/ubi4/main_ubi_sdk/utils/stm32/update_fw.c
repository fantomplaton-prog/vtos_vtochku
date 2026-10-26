/*
 * update_fw.c
 *
 *  Created on: 19 мар. 2023 г.
 *      Author: PC
 */


#include "utils/stm32/update_fw.h"

#ifdef UBI_SDK_TARGET_STM32

#include "HAL_DRIVERS.h"

uint32_t write_fw_pointer = 0;

void GetRunProgramType(command_selector_struct command)
{
	uint8_t AnswerBuf[2];
	AnswerBuf[0] = GET_RUN_PROGRAM_TYPE;
	AnswerBuf[1] = MAIN_PROGRAM;

	MainAnswer(command.header_id, AnswerBuf, 2,0,0);
}

void JumpToProgramCommand(command_selector_struct command)
{

}


void InicializeFlash(command_selector_struct command)
{
	/*
	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_config;

	erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_config.Sector = MAIN_PROGRAM_START_SECTOR;
	erase_config.NbSectors = MAIN_PROGRAM_NUMBER_SECTORS;
	erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	uint32_t SectorError = 0;
	update_fw_status_enum status = 0;
	if(HAL_FLASHEx_Erase(&erase_config, &SectorError) != HAL_OK)
	{
		st_counter = IDLE;
		status = MEMORY_NOT_READY;
		USB_Answer(rx_command.header_id, &status, 1);
		HAL_FLASH_Lock();
		break;
	}else
	{
		status = READY_TO_UPDATE;
		write_fw_pointer = 0;
		MainAnswer(rx_command.header_id, &status, 1);
		st_counter = IDLE_LOADING_FW;
		HAL_FLASH_Lock();
	}
	*/
}

void LoadNewPart(command_selector_struct command)
{
	HAL_FLASH_Unlock();
	/*

	for(uint16_t i = 1; i<rx_command.data_size; i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, MAIN_PROGRAM_START_ADDRESS + write_fw_pointer, rx_command.data_buf[i]);
		write_fw_pointer++;
	}


	HAL_FLASH_Lock();

	update_fw_status_enum status = 0;
	if(write_fw_pointer<input_fw_size)
	{
		//status = PART_WRITEN;
		st_counter = HOLD_NEXT_PART;
	}
	else
	{
		//status = UPDATE_COMPLETE;
		st_counter = COMPLETE_UPDATE;
	}


	USB_Answer(rx_command.header_id, &status, 1);
	*/
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


	//USB_Answer(command.header_id, &status, 1);

}
#endif
