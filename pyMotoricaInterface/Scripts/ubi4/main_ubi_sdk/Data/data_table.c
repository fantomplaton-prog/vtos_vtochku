/*
  ******************************************************************************
  * @file    data_table.c
  * @author  A.Alborov
  ******************************************************************************
*/



#include "Data/data_table.h"
#include "Data/data_verification.h"
#include "string.h"
#include "stdbool.h"
#include "stdint.h"




///////////////////////////////////////////////////////// COMMAND_NEW_VERSION  ///////////////////////////////////////////////////////////



uint32_t Get_Slot_Data_Start_Address_From_Index(data_table_struct* data_table, uint8_t index, uint8_t* result)
{

	if(data_table == 0)
	{
		*result = 0;
		return 0;
	}

	if(data_table->slots_num == 0)
	{
		*result = 0;
		return 0;
	}

	if(index >= data_table->slots_num)
	{
		*result = 0;
		return 0;
	}


	*result = 1;

	uint32_t SlotDataStartAddress = data_table->DataTableStartAddres + sizeof(data_table_info_struct) +
			   	   	   	   	   	   	data_table->slots_num*sizeof(data_table_slot_struct)+
									data_table->slots[index].StartAddressShift;


	return SlotDataStartAddress;
}







void WriteDataInSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	data_table_slot_struct* dt_slots_arr = data_table->slots;

	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	uint8_t slot = receive_str.data_buf[1];

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		if(dt_slots_arr[i].DataCode == slot)
		{


			if(data_table->ram_image_map_size > i)
			{
				if(data_table->ram_image_map[i].DataCode == slot && data_table->ram_image_map[i].ram_image_init)
				{
					memcpy(data_table->ram_image_map[i].ram_image, &receive_str.data_buf[2], dt_slots_arr[i].DataSize);
					data_table->ram_image_map[i].exist_change = 1;

					protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);
					return;
				}
			}


			uint8_t res = 0;
			uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);


			memory_driver_status_enum fd_status = 0;

			memory_driver->smart_write(memory_driver, SlotDataStartAddress, &receive_str.data_buf[2], dt_slots_arr[i].DataSize,&fd_status);

			switch(fd_status)
			{
				case MDSE_OK:
					{


					}break;
				case MDSE_OWERWRITING_NOT_REQUERED:
					{


					}break;
				case MDSE_DATA_CONTAINE_IN_RAM_BUFFER:
					{

						memory_driver->finish_used_flash(memory_driver);

					}break;
				case MDSE_FLASH_BUFFER_BUSY:
					{


					}break;
				case MDSE_ERROR:
					{


					}break;
			}

			uint8_t out_status = 1;

		//	MainAnswer(command.header_id, &out_status, 1,0,0);
			return;
		}
	}
}


// Вернуть ID слота данных??
void ReadDataInSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	data_table_read_slot_status_enum slot_status = 0;
	data_table_slot_struct* _slot = Data_Table_ReadSlot(data_table, receive_str.data_buf[1], &slot_status);

	if(slot_status == DTRS_READ_SLOT_OK)
	{

		data_table_read_data_status_enum data_status = 0;
		uint8_t* read_data = Data_Table_ReadData(data_table, receive_str.data_buf[1],&data_status);

		if(data_status == DTRD_READ_RAM_IMAGE)
		{
			base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, 1+_slot->DataSize);

			if(!data_segment) return;

			memcpy(data_segment->pData+1,read_data,_slot->DataSize);

			data_segment->pData[0] = READ_DATA;

			if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
												protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);

		}
	}

}

void ReadAvailableSlot_v2(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, 1+sizeof(data_table_slot_struct)*data_table->slots_num);

	if(!data_segment) return;

	data_table_slot_struct* dt_slots_arr = data_table->slots;

	memcpy(data_segment->pData+1,dt_slots_arr, sizeof(data_table_slot_struct)*data_table->slots_num);

	data_segment->pData[0] = READ_AVAILABLE_SLOTS;

	if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
												protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);

}

void ReadSlot(data_table_struct* data_table, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

	data_table_read_slot_status_enum slot_status = 0;
	data_table_slot_struct* _slot = Data_Table_ReadSlot(data_table, receive_str.data_buf[1], &slot_status);

	if(slot_status == DTRS_READ_SLOT_OK)
	{

		base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str, 2+sizeof(data_table_slot_struct));

		if(!data_segment) return;


		memcpy(data_segment->pData+2,_slot, sizeof(data_table_slot_struct));

		data_segment->pData[0] = READ_SLOT;
		data_segment->pData[1] = receive_str.data_buf[1];

		if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
													protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);

	}
}


///////////////////////////////////////////////////////// COMMAND_OLD_VERSION  ///////////////////////////////////////////////////////////

void WriteDataInSlot(data_table_struct* data_table, command_selector_struct command)
{

	data_table_slot_struct* dt_slots_arr = data_table->slots;
	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	uint8_t slot = command.data_buf[0];



		for(uint8_t i = 0; i<data_table->slots_num; i++)
		{
			if(data_table->slots[i].DataCode == slot)
			{

				uint8_t res = 0;
				uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);

				memory_driver_status_enum fd_status = 0;

			//	SmartWriteToFlash(SlotDataStartAddress, &command.data_buf[1], dt_slots_arr[i].DataSize,&fd_status);

				memory_driver->smart_write(memory_driver, SlotDataStartAddress, &command.data_buf[1], dt_slots_arr[i].DataSize,&fd_status);

				switch(fd_status)
				{
					case MDSE_OK:
						{


						}break;
					case MDSE_OWERWRITING_NOT_REQUERED:
						{


						}break;
					case MDSE_DATA_CONTAINE_IN_RAM_BUFFER:
						{

							memory_driver->finish_used_flash(memory_driver);
						//	finish_used_flash();


						}break;
					case MDSE_FLASH_BUFFER_BUSY:
						{


						}break;
					case MDSE_ERROR:
						{


						}break;
				}

				uint8_t out_status = 1;

			//	MainAnswer(command.header_id, &out_status, 1,0,0);
				return;
			}
		}
}

void ReadDataInSlot(data_table_struct* data_table, command_selector_struct command)
{
	data_table_read_slot_status_enum slot_status = 0;

	data_table_slot_struct* _slot = Data_Table_ReadSlot(data_table,command.data_buf[0], &slot_status);

	data_table_read_data_status_enum data_status = 0;
	uint8_t* read_data = Data_Table_ReadData(data_table, command.data_buf[0],&data_status);

//	MainAnswer(command.header_id, read_data,_slot->DataSize,0,0);

}

void ReadAvailableSlot(data_table_struct* data_table, command_selector_struct command)
{

	data_table_slot_struct* dt_slots_arr = data_table->slots;

	//MainAnswer(command.header_id, (uint8_t*)dt_slots_arr, Data_Table_Info->data_table_size*sizeof(data_table_slot_struct),0,0);
}

void DataTableEnableVirtualPage()
{

}

///////////////////////////////////////////////////////// COMMAND_OLD_VERSION_END  ///////////////////////////////////////////////////////////





void DataTable_Clear_RAM_Image(data_table_struct* data_table)
{
	if(data_table == 0) return;

	// Clear ram_image
	if(data_table->ram_image_map_size != 0)
	{
		if(data_table->ram_image_map)
		{
			for(uint8_t i = 0; i<data_table->slots_num; i++)
			{
				if(data_table->ram_image_map[i].ram_image)
				{

					memset(data_table->ram_image_map[i].ram_image, 0, data_table->slots[i].DataSize);
					free(data_table->ram_image_map[i].ram_image);

					memset(&data_table->ram_image_map[i], 0, sizeof(data_table_ram_image_pointer_struct));
				}
			}
			free(data_table->ram_image_map);
			data_table->ram_image_map = 0;
			data_table->ram_image_map_size = 0;
		}
	}
}

void DataTable_Clear_RAM_Slots(data_table_struct* data_table)
{
	if(data_table == 0) return;

	if(data_table->slots_num != 0)
	{
		if(data_table->slots)
		{
			memset(data_table->slots, 0, sizeof(data_table_slot_struct)*data_table->slots_num);
			free(data_table->slots);
			data_table->slots = 0;
			data_table->slots_num = 0;
		}
	}
}

void DataTable_Clear_RAM_Info(data_table_struct* data_table)
{
	if(data_table == 0) return;

	if(data_table->info)
	{
		memset(data_table->info, 0, sizeof(data_table_info_struct));
		free(data_table->info);
		data_table->info = 0;
	}
}

void DataTable_Clear_RAM(data_table_struct* data_table)
{
	DataTable_Clear_RAM_Image(data_table);
	DataTable_Clear_RAM_Slots(data_table);
	DataTable_Clear_RAM_Info(data_table);
}







// Под вопросом, возможно не нужно
void DataTable_Init_Default_RAM(data_table_struct* data_table)
{


	data_table->ram_image_map = (data_table_ram_image_pointer_struct*)malloc(data_table->slots_num*sizeof(data_table_ram_image_pointer_struct));
	memset(data_table->ram_image_map, 0, data_table->slots_num*sizeof(data_table_ram_image_pointer_struct));
	data_table->ram_image_map_size = data_table->slots_num;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		data_table->ram_image_map[i].DataCode = data_table->slots[i].DataCode;
		data_table->ram_image_map[i].exist_change = false;
		data_table->ram_image_map[i].ram_image_init = true;
		data_table->ram_image_map[i].ram_image = (void*)malloc(data_table->slots[i].DataSize);


		memcpy(data_table->ram_image_map[i].ram_image, data_table->default_data_table.data_pointer_buffer[i], data_table->slots[i].DataSize);

	}
}







data_table_read_data_result_enum DataTable_Init_RAM_Image(data_table_struct* data_table)
{

	if(data_table == 0)return DTRDRE_ERROR;

	//if(data_table->info->init_code == DATA_TABLE_INIT_CODE) return;

	data_table->ram_image_map_size = data_table->slots_num;

	data_table->ram_image_map = (data_table_ram_image_pointer_struct*)malloc(data_table->slots_num*sizeof(data_table_ram_image_pointer_struct));
	memset(data_table->ram_image_map, 0, data_table->slots_num*sizeof(data_table_ram_image_pointer_struct));

	uint8_t crc_error = 0;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{

		uint8_t res = 0;
		uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);

		data_table->ram_image_map[i].ram_image = (uint8_t*)malloc(data_table->slots[i].DataSize);
		memset(data_table->ram_image_map[i].ram_image, 0, data_table->slots[i].DataSize);

		data_table->ram_image_map[i].DataCode = data_table->slots[i].DataCode;

		memory_driver_status_enum status = 0;

		data_table->memory_driver->readData(data_table->memory_driver,SlotDataStartAddress, data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize, &status);

		if(status != MDSE_OK)
				return DTSTE_ERROR;

		uint8_t read_data_crc = getCRC(data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize);


		if(read_data_crc != data_table->slots[i].crc)
		{

			crc_error = 1;

			//data_table->ram_image_map[i].exist_change = 0;
		}

			data_table->ram_image_map[i].ram_image_init = 1;
	}

	if(crc_error)
	{
		return DTRDRE_CRC_ERROR;
	}

	return DTRDRE_OPEN;
}


void DataTable_Write_Default_in_ROM(data_table_struct* data_table)
{

	data_table_info_struct info;
	info.data_table_size = data_table->default_data_table.slot_num;
	info.data_table_version = DATA_TABLE_VERSION;
	info.init_code = DATA_TABLE_INIT_CODE;
	info.reserv = 0;

	data_table_slot_struct* slots = (data_table_slot_struct*)malloc(sizeof(data_table_slot_struct) * data_table->default_data_table.slot_num);

	if(slots == 0) return;

	memcpy((uint8_t*)slots, (uint8_t*)data_table->default_data_table.slots, sizeof(data_table_slot_struct) * data_table->default_data_table.slot_num);



	for(uint8_t i = 0; i<data_table->default_data_table.slot_num; i++)
		slots[i].crc = getCRC(data_table->default_data_table.data_pointer_buffer[i], data_table->default_data_table.slots[i].DataSize);

	info.data_table_crc = getCRC((uint8_t*)slots, info.data_table_size * sizeof(data_table_slot_struct));

	if(!data_table->memory_driver->write_bytes(data_table->memory_driver, data_table->DataTableStartAddres, (uint8_t*)&info, sizeof(data_table_info_struct)))
	{
		free(slots);
		return;
	}

	uint32_t StartAddress = data_table->DataTableStartAddres + sizeof(data_table_info_struct);
	uint32_t Size = data_table->default_data_table.slot_num * sizeof(data_table_slot_struct);

	if(!data_table->memory_driver->write_bytes(data_table->memory_driver,StartAddress,(uint8_t*)slots,Size))
	{
		free(slots);
		return;
	}

	free(slots);

	data_table->slots_num = data_table->default_data_table.slot_num;

	uint8_t* Data = 0;
	uint32_t SlotStartAddress = 0;
	Size = 0;
	uint8_t result = 0;


	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{

		SlotStartAddress = data_table->DataTableStartAddres + sizeof(data_table_info_struct) +
						   data_table->default_data_table.slot_num *sizeof(data_table_slot_struct)+
						   data_table->default_data_table.slots[i].StartAddressShift;

		Data = data_table->default_data_table.data_pointer_buffer[i];
		Size = data_table->default_data_table.slots[i].DataSize;

		data_table->memory_driver->write_bytes(data_table->memory_driver, SlotStartAddress, Data, Size);
	}
}

void Data_Table_Write_Default(data_table_struct* data_table);

void Data_Table_Reset(data_table_struct* data_table)
{
	//data_table->memory_driver->clear_sector(data_table->memory_driver, 0);
	//DataTableInitInfo(data_table);
	//DataTable_InitDefaultData(data_table);
	//data_table->memory_driver->clear_sector(data_table->memory_driver, 0);

	Data_Table_Write_Default(data_table);

	DataTable_Clear_RAM(data_table);

	DataTable_Open(data_table);

	//DataTable_Save(data_table);


}

void Data_Table_Write_Default(data_table_struct* data_table)
{
	//[PROBLEM] Добавить отчистку всех страниц памяти
	data_table->memory_driver->clear_sector(data_table->memory_driver, data_table->DataTableStartAddres);
/*
	uint8_t* Data = malloc(data_table->slots[0].DataSize);
	memset(Data, 0, data_table->slots[0].DataSize);
	memory_driver_status_enum status = 0;
	data_table->memory_driver->readData(data_table->memory_driver, 80, Data, data_table->slots[0].DataSize, &status);
	free(Data);
*/
	DataTable_Write_Default_in_ROM(data_table);
	data_table->memory_driver->finish_used_flash(data_table->memory_driver);

}



data_table_initialize_status_enum DataTableInit(data_table_struct* data_table, base_memory_driver_struct* memory_driver)
{

	data_table->memory_driver = memory_driver;

	//Data_Table_Write_Default(data_table);

	data_table_open_status_enum status = DataTable_Open(data_table);

	switch(status)
	{
		case DTSTE_OPEN:
		{
			return DTRD_ALREADY_INITIALIZED;
			break;
		}
		case DTSTE_CRC_OPEN_PROBLEM:
		{
			Data_Table_Write_Default(data_table);

			if(DataTable_Open(data_table) == DTSTE_OPEN)
				return DTRD_FIRST_INITIALIZE;

			break;
		}
		case DTSTE_ALREADY_OPEN:
					break;
		case DTSTE_DATA_CODE_ERROR:
		{
			Data_Table_Write_Default(data_table);

			if(DataTable_Open(data_table) == DTSTE_OPEN)
				return DTRD_FIRST_INITIALIZE;

			break;
		}
		case DTSTE_ERROR:
		{
			return DTRD_INITIALIZE_ERROR;
			break;
		}
	}

	return DTRD_INITIALIZE_ERROR;
}

/*
//проводить запись флеша при старте. (((Под вопросом)))
		uint8_t res = 0;
		uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);

		memory_driver_status_enum fd_status = 0;

		data_table->memory_driver->smart_write(data_table->memory_driver,SlotDataStartAddress, data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize,&fd_status);
//==========================================================
*/



void DataTable_Write_ROM_From_RAM(data_table_struct* data_table)
{

	if(data_table == 0) return;


//==================Calculate CRC===========================================
	for(uint8_t i = 0; i<data_table->slots_num; i++)
		data_table->slots[i].crc = getCRC(data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize);


	data_table->info->data_table_crc = getCRC((uint8_t*)data_table->slots, data_table->slots_num * sizeof(data_table_slot_struct));
	data_table->info->data_table_size = data_table->slots_num;
	data_table->info->data_table_version = DATA_TABLE_VERSION;
	data_table->info->init_code = DATA_TABLE_INIT_CODE;
    data_table->info->reserv = 0;
//==========================================================================



	if(!data_table->memory_driver->write_bytes(data_table->memory_driver,data_table->DataTableStartAddres,(uint8_t*)data_table->info, sizeof(data_table_info_struct)))
		return;

	uint32_t DT_slots_address = data_table->DataTableStartAddres+sizeof(data_table_info_struct);

	if(!data_table->memory_driver->write_bytes(data_table->memory_driver,DT_slots_address,(uint8_t*)data_table->slots, sizeof(data_table_slot_struct)*data_table->slots_num))
		return;

	memory_driver_status_enum status = 0;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		uint32_t data_address = data_table->DataTableStartAddres + data_table->slots[i].StartAddressShift + sizeof(data_table_info_struct) + data_table->slots_num*sizeof(data_table_slot_struct);
		uint32_t data_size = data_table->slots[i].DataSize;

		//uint8_t* Data = malloc(data_table->slots[i].DataSize);
	//	memset(Data, 0, data_table->slots[i].DataSize);

		//data_table->memory_driver->readData(data_table->memory_driver,data_address, Data, data_table->slots[i].DataSize, &status);


		if(!data_table->memory_driver->write_bytes(data_table->memory_driver, data_address, data_table->ram_image_map[i].ram_image, data_size)) return;
		//free(Data);

		//data_table->memory_driver->readData(data_table->memory_driver,data_address,data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize, &status);
		data_table->ram_image_map[i].exist_change = 0;
	}
}



//ADD CRC
data_table_open_status_enum DataTable_Open(data_table_struct* data_table)
{
	if(data_table == 0) return DTSTE_ERROR;


	if(data_table->info)
		if(data_table->info->init_code == DATA_TABLE_INIT_CODE)
		    return DTSTE_ALREADY_OPEN;

	data_table->info = (data_table_info_struct*)malloc(sizeof(data_table_info_struct));
	memset(data_table->info, 0, sizeof(data_table_info_struct));

	memory_driver_status_enum status;

	data_table->memory_driver->readData(data_table->memory_driver, data_table->DataTableStartAddres, (uint8_t*)data_table->info,sizeof(data_table_info_struct),&status);

	if(data_table->info->init_code != DATA_TABLE_INIT_CODE)
	    return DTSTE_DATA_CODE_ERROR;

	data_table->slots_num = data_table->info->data_table_size;
	data_table->slots = (data_table_slot_struct*)malloc(sizeof(data_table_slot_struct) * data_table->slots_num);
	memset(data_table->slots, 0, sizeof(data_table_slot_struct) * data_table->slots_num);

	uint32_t slots_address = data_table->DataTableStartAddres + sizeof(data_table_info_struct);
	uint32_t slots_size = data_table->info->data_table_size*sizeof(data_table_slot_struct);
	data_table->slots_num = data_table->info->data_table_size;

	data_table->memory_driver->readData(data_table->memory_driver, slots_address, (uint8_t*)data_table->slots, slots_size, &status);

	if(status != MDSE_OK)
		return DTSTE_ERROR;

	uint8_t read_slots_crc = getCRC((uint8_t*)data_table->slots, slots_size);

	if(read_slots_crc != data_table->info->data_table_crc) return DTSTE_CRC_OPEN_PROBLEM;


	if(DataTable_Init_RAM_Image(data_table) == DTRDRE_OPEN)
		return DTSTE_OPEN;
	else
		return DTRDRE_CRC_ERROR;

	return DTSTE_ERROR;
}

//ADD CRC
void DataTable_Close(data_table_struct* data_table)
{

	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	uint32_t DataTable_Size = sizeof(data_table_info_struct) + data_table->slots_num*sizeof(data_table_slot_struct);

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
	  DataTable_Size += data_table->slots[i].DataSize;
	}

	data_table->info->data_table_size = data_table->slots_num;


	  //Calc CRC

	if(memory_driver->memory_region_is_free(memory_driver,data_table->DataTableStartAddres, DataTable_Size))
	{
	   DataTable_Write_ROM_From_RAM(data_table);
	}else
	{
		data_table->memory_driver->clear_sector(data_table->memory_driver, 0);
		DataTable_Write_ROM_From_RAM(data_table);
	}

	DataTable_Clear_RAM(data_table);

}




void DataTable_Calc_Default_CRC()
{

}

void DataTable_Calc_Image_CRC(data_table_struct* data_table)
{

	for(uint8_t i = 0; i<data_table->slots_num; i++)
			data_table->slots[i].crc = getCRC((uint8_t*)data_table->ram_image_map[i].ram_image, data_table->slots[i].DataSize);


	data_table->info->data_table_crc = getCRC((uint8_t*)data_table->slots,data_table->slots_num*sizeof(data_table_slot_struct));
}

void DataTable_Save(data_table_struct* data_table)
{
	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	uint32_t DataTable_Size = sizeof(data_table_info_struct) + data_table->slots_num*sizeof(data_table_slot_struct);

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
	  DataTable_Size += data_table->slots[i].DataSize;
	}

	data_table->info->data_table_size = data_table->slots_num;


	while(!memory_driver->memory_region_is_free(memory_driver,data_table->DataTableStartAddres, DataTable_Size))
	{
		data_table->memory_driver->clear_sector(data_table->memory_driver, data_table->DataTableStartAddres); // [PROBLEM] Отчищается только один сектор
	}

	DataTable_Write_ROM_From_RAM(data_table);

	/*
	if(memory_driver->memory_region_is_free(memory_driver,data_table->DataTableStartAddres, DataTable_Size))
	{
	   DataTable_Write_ROM_From_RAM(data_table);
	}else
	{
		data_table->memory_driver->clear_sector(data_table->memory_driver, 0);
		DataTable_Write_ROM_From_RAM(data_table);
	}
	*/
    data_table->memory_driver->finish_used_flash(data_table->memory_driver);

}

void DataTableSetRamImageFlag(data_table_struct* data_table, uint8_t slot)
{
    for (uint8_t i = 0; i < data_table->ram_image_map_size; i++)
    {
        if (data_table->ram_image_map[i].DataCode == slot)
        {
            data_table->ram_image_map[i].exist_change = true;
        }
    }
}

void DataTableInitSlots()
{

}



data_table_slot_struct* DataTable_WriteNewSlot(data_table_struct* data_table, data_table_slot_struct* Row, data_table_write_new_slot_status_enum* status)
{


	//data_table->memory_driver->get_pointer()


	data_table_slot_struct* dt_slots_arr = data_table->slots;

	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		if(dt_slots_arr[i].DataCode == Row->DataCode)
		{

			if(memory_driver->compare_memory_val(memory_driver,(uint32_t)&dt_slots_arr[i], (uint8_t*)Row, sizeof(data_table_slot_struct)))
			{
				*status = DTWS_OWERWRITING_NOT_REQUERED;
				return &dt_slots_arr[i];
			}else
			{
				memory_driver_status_enum fd_status = 0;



				void* fd_pointer = memory_driver->smart_write(memory_driver, (uint32_t)&dt_slots_arr[i], (uint8_t*)Row, sizeof(data_table_slot_struct),&fd_status);

				switch(fd_status)
				{
					case MDSE_OK:
					{

					}break;
					case MDSE_OWERWRITING_NOT_REQUERED:
					{
						*status = DTWS_OWERWRITING_NOT_REQUERED;
						return &dt_slots_arr[i];
					}break;
					case MDSE_DATA_CONTAINE_IN_RAM_BUFFER:
					{
						*status = DTWS_DATA_CONTAINE_IN_FLASH_BUFFER;
						return (data_table_slot_struct*)fd_pointer;
					}break;
					case MDSE_FLASH_BUFFER_BUSY:
					{

					}break;
					case MDSE_ERROR:
						{

						}break;
				}

			}

			return &dt_slots_arr[i];
		}
	}

	data_table_slot_struct* dt_slots = 0;

	for(uint32_t i = 0; i<data_table->info->data_table_size; i++)
	{
		if(dt_slots_arr[i].DataCode == 0xFF)
		{
			dt_slots = &dt_slots_arr[i];
			break;
		}
	}
	if(dt_slots == 0)
	{
		*status = DTWS_ALL_SLOT_BUSY;
		return 0;
	}

	if(memory_driver->memory_region_is_free(memory_driver, (uint32_t)dt_slots, sizeof(data_table_slot_struct)))
	{
		memory_driver->write_bytes(memory_driver, (uint32_t)dt_slots,(uint8_t*)Row, sizeof(data_table_slot_struct));
		//WriteBytesToFlash((uint32_t)dt_slots,(uint8_t*)Row, sizeof(data_table_slot_struct));
		*status = DTWS_WRITE_SLOT_OK;
		return dt_slots;
	}
	return 0;
}

void DataTable_RAM_Image_Init(data_table_struct* data_table)
{
	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	for(uint8_t i = 0; i<data_table->info->data_table_size; i++)
	{
		uint8_t res = 0;
		uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);

		data_table->ram_image_map = (data_table_ram_image_pointer_struct*)malloc(data_table->slots_num*sizeof(data_table_ram_image_pointer_struct));
		data_table->ram_image_map_size = data_table->slots_num;

		memory_driver_status_enum status = 0;

		memory_driver->readData(memory_driver,SlotDataStartAddress,(uint8_t*)data_table->ram_image_map[i].ram_image, sizeof(data_table_ram_image_pointer_struct), &status);
	}
}

void DataTable_Write_Data_ROM(data_table_struct* data_table, uint8_t slot,uint8_t* Data, data_table_write_data_status_enum* status)
{
	data_table_slot_struct* dt_slots_arr = data_table->slots;
	base_memory_driver_struct* memory_driver = data_table->memory_driver;


	for(uint8_t i = 0; i<data_table->info->data_table_size; i++)
	{
		if(dt_slots_arr[i].DataCode == slot)
		{

			uint8_t res = 0;
			uint32_t SlotDataStartAddress = Get_Slot_Data_Start_Address_From_Index(data_table, i, &res);

			memory_driver_status_enum fd_status = 0;

			memory_driver->smart_write(memory_driver, SlotDataStartAddress, Data, dt_slots_arr[i].DataSize,&fd_status);

		//	SmartWriteToFlash(SlotDataStartAddress, Data, dt_slots_arr[i].DataSize,&fd_status);

			switch(fd_status)
			{
				case MDSE_OK:
					{
						*status = DTWD_WRITE_DATA_OK;
						return;
					}break;
				case MDSE_OWERWRITING_NOT_REQUERED:
					{
						*status = DTWD_OWERWRITING_NOT_REQUERED;
						return;
					}break;
				case MDSE_DATA_CONTAINE_IN_RAM_BUFFER:
					{
						*status = DTWD_DATA_CONTAINE_IN_FLASH_BUFFER;
						return;
					}break;
				case MDSE_FLASH_BUFFER_BUSY:
					{
						*status = DTWD_FLASH_BUFFER_BUSY;
						return;
					}break;
				case MDSE_ERROR:
					{
						*status = DTWD_SLOT_ERROR;
						return;
					}break;
			}
		}
	}

	*status = DTWD_SLOT_NOT_FOUND;
	return;
}

void DataTableInitFinish(data_table_struct* data_table)
{
	base_memory_driver_struct* memory_driver = data_table->memory_driver;

	memory_driver->finish_used_flash(memory_driver);
}


void DataTable_WriteData_RAM(data_table_struct* data_table, uint8_t slot, uint8_t* Data, data_table_write_data_status_enum* status)
{

	for(uint8_t i = 0; i<10; i++)
	{

	}
}


void* DataTable_ReadData_RAM(data_table_struct* data_table, uint8_t slot, data_table_read_data_status_enum* status)
{
	return 0;
}


data_table_slot_struct* Data_Table_ReadSlot(data_table_struct* data_table, uint8_t slot, data_table_read_slot_status_enum* status)
{
	data_table_slot_struct* dt_slots_arr = data_table->slots;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		if(slot == dt_slots_arr[i].DataCode)
		{
			*status = DTRD_READ_DATA_OK;
			return &dt_slots_arr[i];
		}
	}

	*status = DTRD_SLOT_NOT_FOUND;
	return 0;
}


void* Data_Table_ReadData(data_table_struct* data_table, uint8_t slot, data_table_read_data_status_enum* status)
{

	data_table_slot_struct* dt_slots_arr = data_table->slots;

	for(uint8_t i = 0; i<data_table->slots_num; i++)
	{
		if(dt_slots_arr[i].DataCode == slot)
		{
			if(data_table->ram_image_map_size > i)
			{
				if(data_table->ram_image_map[i].DataCode == slot && data_table->ram_image_map[i].ram_image_init)
				{
					*status = DTRD_READ_RAM_IMAGE;
					return data_table->ram_image_map[i].ram_image;
				}
			}

			// Добавить логику если нет RAM_Image


		}
	}



	*status = DTRD_SLOT_NOT_FOUND;
	return 0;
}


