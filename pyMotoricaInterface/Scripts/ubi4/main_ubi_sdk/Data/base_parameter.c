/*
  ******************************************************************************
  * @file    base_parameter.c
  * @author  A.Alborov
  ******************************************************************************
*/


#include "string.h"
#include "Data/base_parameter.h"
#include "UBI_SDK.h"
#include "stdlib.h"

#ifdef UBI_SDK_SYSTEM_ROLE_MASTER
#include "request_scheduler.h"
#include "device_manager.h"
#endif


void init_parameter_manager(parameter_manager_struct** param_manager, base_parameter_struct* parameters, uint8_t param_num)
{
	parameter_manager_struct* t_param_manager = (parameter_manager_struct*)malloc(sizeof(parameter_manager_struct));

	t_param_manager->param_num = param_num;
	t_param_manager->parameters = parameters;


	static_conteiner_init_struct* conteiner_init = (static_conteiner_init_struct*)malloc(sizeof(static_conteiner_init_struct));

	conteiner_init->ArraySize = 20;
	conteiner_init->DataSize = sizeof(base_parameter_struct);
	conteiner_init->addDataCallback = 0;
	conteiner_init->removeDataCallback = 0;

	init_static_conteiner(conteiner_init, &t_param_manager->change_parameter_conteiner);

	*param_manager = t_param_manager;
}

void parameter_write_data(parameter_manager_struct* param_manager, uint8_t data_code, uint8_t instance, void* data)
{
	
}


#ifdef UBI_SDK_SYSTEM_ROLE_MASTER

void parameter_write_data_obj(parameter_manager_struct* param_manager, base_parameter_struct* param, uint8_t* data)
{
	//memcpy(param->pointers->parameter_data_p, data, param->info->parameter_size);

	for(uint16_t i = 0; i<param->info->parameter_size; i++)
	{
		param->pointers->parameter_data_p[i] = data[i];
	}

	if(param->info->synch_type == PSTE_NOTIFICATION)
	{

	}


	device_parameters_struct* device = ((device_parameters_struct*)param->pointers->device_owner);

	if(device)
	{
		if(device->request_scheduller)
		{

			command_receive_struct command;
			command.DeviceAddress = device->Address;

			if(param->info->synch_direction == PSDE_MASTER_TO_SLAVE)
			{
				if(param->info->synch_type == PSTE_WRITE_ONLY)
				{
					command.command = param->info->ID | 0x80;
					command.dir = TX_ONLY;
				}
			}

			command.request_type = BPRT_PARAMETERS;
			command.delete_data_after_move_to_tx_buffer = false;
			command.WriteData = param->pointers->parameter_data_p;
			command.WriteDataSize = param->info->parameter_size;
			command.flags = 0;

			AddToRequestQueue(device->request_scheduller->receiver_queue, command);
		}
	}



}


void parameter_write_array_element_obj(parameter_manager_struct* param_manager, base_parameter_struct* param, uint16_t index, uint8_t* data)
{
	//memcpy(param->pointers->parameter_data_p, data, param->info->parameter_size);


	if(param->info->synch_type == PARTE_STRUCT_ARRAY_TYPE)
	{
		StructArrayAddInfo_struct* struct_arr_info =  (StructArrayAddInfo_struct*)param->pointers->parameter_data_p;

		uint8_t* ep = param->pointers->parameter_data_p + sizeof(StructArrayAddInfo_struct) + (index*struct_arr_info->ItemSize);
		memcpy(ep,data,struct_arr_info->ItemSize);

		if(param->info->synch_type == PSTE_NOTIFICATION)
		{

		}

		device_parameters_struct* device = ((device_parameters_struct*)param->pointers->device_owner);

		if(device)
		{
			if(device->request_scheduller)
			{

				command_receive_struct command;
				command.DeviceAddress = device->Address;

				if(param->info->synch_direction == PSDE_MASTER_TO_SLAVE)
				{
					if(param->info->synch_type == PSTE_WRITE_ONLY)
					{
						command.command = param->info->ID | 0x80;
						command.dir = TX_ONLY;
					}
				}

				command.request_type = BPRT_PARAMETERS;
				command.delete_data_after_move_to_tx_buffer = true;

				uint8_t* txdata = malloc(struct_arr_info->ItemSize + sizeof(StructArray_write_item_header_struct) - 1);

				StructArray_write_item_header_struct* header = txdata;


				header->write_type = SAWCE_ITEM_WRITE;
				header->Index = index;


				memcpy(header->Data, data, struct_arr_info->ItemSize);

				command.WriteData = txdata;
				command.WriteDataSize = struct_arr_info->ItemSize + sizeof(StructArray_write_item_header_struct)-1;
				command.flags = 0;


				AddToRequestQueue(device->request_scheduller->receiver_queue, command);
			}
		}
	}
}
#endif

#ifdef UBI_SDK_SYSTEM_ROLE_SLAVE
void parameter_write_data_obj(parameter_manager_struct* param_manager, base_parameter_struct* param, void* data)
{

}
#endif


void parameter_write_master_to_slave_simple(base_parameter_struct* param, uint8_t* Data)
{
	if(param->pointers->change_notification)
	{
		param->pointers->change_notification(&param, Data);
	}else
	{
		if(param->pointers->parameter_data_p != Data)
			memcpy(param->pointers->parameter_data_p, Data,param->info->parameter_size);
	}
}

void parameter_write_master_to_slave_data_transfer(base_parameter_struct* param, uint8_t* Data, uint16_t size)
{
	if(param->pointers->change_notification)
	{
		((pre_read_data_transfer_callback_t)param->pointers->change_notification)(&param, Data, size);
	}else
	{
		if(param->pointers->parameter_data_p != Data)
			memcpy(param->pointers->parameter_data_p, Data,param->info->parameter_size);
	}
}

void parameter_write_master_to_slave_struct_array(base_parameter_struct* param, uint8_t* Data)
{
	StructArray_write_item_header_struct* header = (StructArray_write_item_header_struct*)Data;
	
	StructArrayAddInfo_struct* struct_arr_info =  (StructArrayAddInfo_struct*)param->pointers->parameter_data_p;
	uint8_t* ep = param->pointers->parameter_data_p + sizeof(StructArrayAddInfo_struct) + (header->Index*struct_arr_info->ItemSize);
	
	if(param->pointers->change_notification)
	{
		param->pointers->change_notification(&param, Data);
	}else
	{
		memcpy(ep, header->Data, struct_arr_info->ItemSize);
	}
}

void create_parameter_consumer_array(base_parameter_pointers_struct *param_pointers, uint8_t size)
{
	if(param_pointers->parameters_consumers == 0)
	{
		param_pointers->parameters_consumers = malloc(sizeof(struct parameter_consumer_array_struct));

		if(param_pointers->parameters_consumers)
		{
			param_pointers->parameters_consumers->counter = 0;
			param_pointers->parameters_consumers->size = size;
			param_pointers->parameters_consumers->array = malloc(sizeof(base_parameter_struct)*param_pointers->parameters_consumers->size);
			memset(param_pointers->parameters_consumers->array, 0, sizeof(base_parameter_struct)*param_pointers->parameters_consumers->size);

		}
	}
}


void create_dependence_info_array(base_parameter_pointers_struct *param_pointers, uint8_t size)
{
	if(param_pointers->dependence_info == 0)
	{
		param_pointers->dependence_info = malloc(sizeof(struct dependence_info_array_struct));

		if(param_pointers->dependence_info)
		{
			param_pointers->dependence_info->counter = 0;
			param_pointers->dependence_info->size = size;
			param_pointers->dependence_info->array = malloc(sizeof(dependence_info_struct)*param_pointers->dependence_info->size);
			memset(param_pointers->dependence_info->array, 0, sizeof(dependence_info_struct)*param_pointers->dependence_info->size);
		}
	}
}





void parameter_add_consumer(base_parameter_pointers_struct *source_param_pointers, base_parameter_struct param)
{
	if(source_param_pointers->parameters_consumers == 0)
	{
		create_parameter_consumer_array(source_param_pointers, 4);
	}

	if(source_param_pointers->parameters_consumers)
	{

		for(uint8_t i = 0; i<source_param_pointers->parameters_consumers->counter; i++)
		{
			if(param.info == source_param_pointers->parameters_consumers->array[i].info)
				return;
		}


		if(source_param_pointers->parameters_consumers->size > source_param_pointers->parameters_consumers->counter)
		{
			source_param_pointers->parameters_consumers->array[source_param_pointers->parameters_consumers->counter].info = param.info;
			source_param_pointers->parameters_consumers->array[source_param_pointers->parameters_consumers->counter].pointers = param.pointers;
			source_param_pointers->parameters_consumers->counter++;
		}else
		{
			struct parameter_consumer_array_struct* old_parameters_consumers = source_param_pointers->parameters_consumers;

			create_parameter_consumer_array(source_param_pointers, source_param_pointers->parameters_consumers->size + 2);
			memcpy(source_param_pointers->parameters_consumers->array, old_parameters_consumers->array, old_parameters_consumers->counter*sizeof(base_parameter_struct));
			source_param_pointers->parameters_consumers->counter = old_parameters_consumers->counter;
			free(old_parameters_consumers);

			 parameter_add_consumer(source_param_pointers, param);
		}
	}
}


bool parameter_add_dependence(base_parameter_pointers_struct *param_pointers, uint8_t type, uint8_t instance, void* dependence_pointer, uint32_t dependence_size)
{


	if(param_pointers->dependence_info == 0)
	{
		create_dependence_info_array(param_pointers, 1);
	}

	if(param_pointers->dependence_info)
	{

		for(uint8_t i = 0; i<param_pointers->dependence_info->counter; i++)
		{

			if(param_pointers->dependence_info->array[i].info_type == type)
			{
				if(param_pointers->dependence_info->array[i].info_instance == instance)
					return false;
			}
		}


		if(param_pointers->dependence_info->size > param_pointers->dependence_info->counter)
		{
			param_pointers->dependence_info->array[param_pointers->dependence_info->counter].info = dependence_pointer;
			param_pointers->dependence_info->array[param_pointers->dependence_info->counter].info_size = dependence_size;
			param_pointers->dependence_info->array[param_pointers->dependence_info->counter].info_type = type;
			param_pointers->dependence_info->array[param_pointers->dependence_info->counter].info_instance = instance;
			param_pointers->dependence_info->counter++;
			return true;

		}else
		{
			struct dependence_info_array_struct* old_parameter_dependences = param_pointers->dependence_info;

			create_dependence_info_array(param_pointers, param_pointers->dependence_info->size + 1);
			memcpy(param_pointers->dependence_info->array, old_parameter_dependences->array, old_parameter_dependences->counter*sizeof(base_parameter_struct));
			param_pointers->dependence_info->counter = old_parameter_dependences->counter;
			free(old_parameter_dependences);

			return parameter_add_dependence(param_pointers, type, instance, dependence_pointer, dependence_size);
		}
	}

	return false;
}



void parameter_write_data_info(parameter_manager_struct* param_manager, base_parameter_info_struct* param_info, void* data)
{

}
/*
void parameter_write_data(base_parameter_struct* param, void* val, uint32_t size)
{

	memcpy(param->parameter_data_p,val, size);

	if(param->flag_buffer)
	{
		*param->flag_buffer |= 1<<param->flag_shift;
	}
	if(param->change_notification)
	{
		param->change_notification(param);
	}
}

void* parameter_get_data_pointer(base_parameter_struct* param)
{
	return 0;
}
*/
