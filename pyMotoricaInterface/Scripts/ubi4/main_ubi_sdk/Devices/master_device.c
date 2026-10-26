/*
 * master_device_info.c
 *
 *  Created on: 15.07.24
 *      Author: PC
 */



#include "Devices/master_device.h"
#include "Data/parameter_widgets.h"

#include "string.h"

#include "UBI_SDK.h"


//#define UBI_SDK_SYSTEM_ROLE_MASTER

#ifdef UBI_SDK_SYSTEM_ROLE_MASTER
#include "request_scheduler.h"


extern device_manager_struct* device_manager;


void StopParameterTransfer(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str);


void init_info_request_callback(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	StopParameterTransfer(deviceInfo, protocol_str, receive_str);
}




void init_master_device(init_device_information_struct* init_device_info,master_device_struct* device_info)
{
	init_base_device_info(init_device_info, device_info);
	init_device_info->ReadDeviceInformationCommandCallBack = init_info_request_callback;

}




void WriteParameterToSubDevice(device_parameters_struct* device, base_parameter_struct* param);

__attribute__((weak))
void master_device_CustomDeviceInformationCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	if(deviceInfo->init_device_info->CustomDeviceInformationCommand)
	{
		deviceInfo->init_device_info->CustomDeviceInformationCommand(deviceInfo,protocol_str,receive_str);
	}
}

void ReadSubDeviceInfo(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, 2 + sizeof(baseSubDeviceInfo_struct)*deviceInfo->device_manager->DeviceArrayCounter);

	if(!data_segment) return;

	data_segment->pData[0] = READ_SUB_DEVICE_INFO;
	data_segment->pData[1] = deviceInfo->device_manager->DeviceArrayCounter;



	for(uint8_t i = 0; i<deviceInfo->device_manager->DeviceArrayCounter; i++)
	{
		baseSubDeviceInfo_struct* subDeviceInfo = (baseSubDeviceInfo_struct*)(data_segment->pData + 2 + (sizeof(baseSubDeviceInfo_struct)*i));


		memcpy(subDeviceInfo, deviceInfo->device_manager->DeviceArray[i]->Info, sizeof(baseSubDeviceInfo_struct));

		/*
		subDeviceInfo->DeviceAddress = deviceInfo->device_manager->DeviceArray[i]->Address;
		subDeviceInfo->DeviceCode = deviceInfo->device_manager->DeviceArray[i]->Info->DeviceCode;
		subDeviceInfo->DeviceType = deviceInfo->device_manager->DeviceArray[i]->Info->DeviceType;
		subDeviceInfo->DeviceRole = deviceInfo->device_manager->DeviceArray[i]->Info->DeviceRole;
		subDeviceInfo->DeviceVersion = deviceInfo->device_manager->DeviceArray[i]->Info->DeviceVersion;
		subDeviceInfo->DeviceSubVersion = deviceInfo->device_manager->DeviceArray[i]->Info->DeviceSubVersion;
		subDeviceInfo->ParametersNum = deviceInfo->device_manager->DeviceArray[i]->ParametersSize;
		subDeviceInfo->SubDeviceNum = 0;
		subDeviceInfo->DefaultPort =
		*/

	}

	if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
				protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);
}




void base_device_CustomDeviceInformationCommand(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	switch(receive_str.data_buf[0])
	{
		case READ_SUB_DEVICE_INFO:							ReadSubDeviceInfo((master_device_struct*)deviceInfo, protocol_str,receive_str);			break;
		case READ_SUB_DEVICE_PARAMETERS:					ReadSubDeviceParameters((master_device_struct*)deviceInfo, protocol_str,receive_str);			break;
		case READ_SUB_DEVICE_ADDITIONAL_PARAMETER:			ReadSubDeviceAdditionalParameters((master_device_struct*)deviceInfo, protocol_str,receive_str);	break;
		case SUB_DEVICE_PARAMETER_INIT_READ:				SubDeviceParameterInitRead((master_device_struct*)deviceInfo, protocol_str,receive_str);	break;
		case SUB_DEVICE_PARAMETER_INIT_WRITE:				SubDeviceParameterInitWrite((master_device_struct*)deviceInfo, protocol_str,receive_str);	break;


		default:
		{
			master_device_CustomDeviceInformationCommand(deviceInfo, protocol_str,receive_str);
			break;
		}
	}
}


void Complex_Parameter_Transfer(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{

}


void update_external_source_param(base_parameter_struct* Param, uint8_t* Data)
{
	memcpy(Param->pointers->parameter_data_p, Data, Param->info->parameter_size);
	if(Param->pointers->dependence_info)
	{

	}
}


void master_WriteParameter(base_parameter_struct* Param, uint8_t* Data)
{
	if(Param->pointers->enable_function.smart_parameter_write)
										UpdateStreamingParameter(Param, Data);

	if(Param->pointers->enable_function.update_external_source_param)
									update_external_source_param(Param, Data);


	if(Param->pointers->enable_function.smart_parameter_write == 0)
	{
		memcpy(Param->pointers->parameter_data_p, Data, Param->info->parameter_size);
	}


	if(Param->pointers->change_notification)
		Param->pointers->change_notification(Param, Data);

}

xTimerHandle TxExternalDeviceTimer = 0;

//TaskStatus_t* TaskStatus;

static master_device_struct* master_device = 0;

uint8_t plot_value[6] = {0,20,40,60,80,100};


uint8_t test_values[128];




void external_device_exec()
{
	if(master_device != 0)
	{

		uint8_t size = master_device->external_device_stream->array_counter;

		if(size == 0)  return;

		ubi4_bus_protocol_obj_struct* protocol = master_device->external_device_stream->protocol;


		if(protocol->transmit_data_segment_to_master == 0) return;

		uint16_t tx_size = 0;
		for(uint8_t i = 0; i<size; i++)
		{
			tx_size += 2 + master_device->external_device_stream->param_array[i]->info->parameter_size;

		}

		if(tx_size == 0) return;

		base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, tx_size);

		if(!data_segment) return;


		uint8_t* TxData = data_segment->pData;
		uint32_t pointer = 0;

		base_parameter_struct** param_array = master_device->external_device_stream->param_array;

		for(uint8_t i = 0; i<size; i++)
		{
			if(param_array[i]->pointers->device_owner)
				TxData[pointer] = ((device_parameters_struct*)param_array[i]->pointers->device_owner)->Address;
			else
				TxData[pointer] = 0;

			pointer++;

			TxData[pointer] = param_array[i]->info->ID;
			pointer++;

			memcpy(&TxData[pointer],  param_array[i]->pointers->parameter_data_p,  param_array[i]->info->parameter_size);
			pointer += param_array[i]->info->parameter_size;

		}

		 master_device->external_device_stream->array_counter = 0;

		protocol->transmit_data_segment_to_master(protocol, COMPLEX_PARAMETER_TRANSFER, data_segment);
	}
}



static void osProgTimer_tx_to_external_device_exec(void* argument)
{
	external_device_exec();
}

static void tx_to_external_device_exec_task(void *argument)
{
	while(1)
	{
		external_device_exec();
		vTaskDelay(30 / portTICK_PERIOD_MS);
	}
}



static bool parameterStreamEnable = false;
static bool parameterStreamInited = false;
TaskHandle_t parameter_transfer_task_handle;

void external_stream_add_param(base_parameter_struct* param, external_device_parameter_stream__struct* object)
{
	if(parameterStreamEnable)
	{
		if(object->array_size > object->array_counter)
		{
			if(object->filter_array_counter)
			{
				bool find_filter = false;
				for(uint8_t i = 0; i<object->filter_array_counter; i++)
				{
					if(object->filter_array[i].Command == param->info->ID)
					{
						if(object->filter_array[i].Address == ((device_parameters_struct*)param->pointers->device_owner)->Address)
						{
							find_filter = true;
							break;
						}
					}else
						continue;
				}
				if(!find_filter) return;
			}


			object->param_array[object->array_counter] = param;
			object->array_counter++;
		}
	}
}



void localStopParameterTransfer()
{
	if(parameterStreamEnable)
	{
		vTaskDelete(parameter_transfer_task_handle);
		parameterStreamEnable = false;
	}
}

void StopParameterTransfer(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	localStopParameterTransfer();
}




void StartParameterTransfer(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{


	uint16_t streamingCounter = 0;
	uint16_t oneShotWriteCounter = 0;

	uint16_t count = 0;
	master_device = deviceInfo;

	if(!parameterStreamInited)
	{

	deviceInfo->external_device_stream = (external_device_parameter_stream__struct*)malloc(sizeof(external_device_parameter_stream__struct));

	memset(deviceInfo->external_device_stream, 0, sizeof(external_device_parameter_stream__struct));

	deviceInfo->external_device_stream->filter_array_size = 3;
	deviceInfo->external_device_stream->filter_array_counter = 0;
	deviceInfo->external_device_stream->filter_array = (parameter_stream_filter_struct*)malloc(sizeof(parameter_stream_filter_struct)*deviceInfo->external_device_stream->filter_array_size);
	memset(deviceInfo->external_device_stream->filter_array, 0, sizeof(parameter_stream_filter_struct)*deviceInfo->external_device_stream->filter_array_size);



	deviceInfo->external_device_stream->external_stream_add_param_callback = external_stream_add_param;




	uint8_t this_device_param_num = deviceInfo->device_manager->this_device_param_num;
	base_parameter_struct* this_device_parameters = deviceInfo->device_manager->this_device_parameters;



	for(uint8_t i = 0; i<this_device_param_num; i++)
	{

		uint8_t add_info_size = this_device_parameters[i].info->additional_info_size;

		base_parameter_info_struct* ParametersInfo = this_device_parameters[i].info;
		base_parameter_pointers_struct* ParametersPointer = this_device_parameters[i].pointers;

		 if(ParametersInfo->synch_type == PSTE_STREAMING)
		 {
			 for(uint8_t k = 0; k<add_info_size; k++)
			{
				if(ParametersPointer->add_info_segments[k].info_type == PAITE_WIDGET && ParametersPointer->add_info_segments[k].info_size)
				{
					base_parameter_widget_struct* widget_obj = (base_parameter_widget_struct*)ParametersPointer->add_info_segments[k].info;

					if(widget_obj->widget_type != PWTE_COMMAND && widget_obj->widget_type != PWTE_UNKNOW)
					{

						ParametersPointer->enable_function.smart_parameter_write = 1;

						parameter_add_dependence(ParametersPointer,PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM,0,deviceInfo->external_device_stream, sizeof(external_device_parameter_stream__struct));

						count++;

					}
				}
			}
		 }else if(ParametersInfo->synch_type == PSTE_WRITE_ONLY || ParametersInfo->synch_type == PSTE_NOTIFICATION )
		 {
			 for(uint8_t k = 0; k<add_info_size; k++)
			{
				if(ParametersPointer->add_info_segments[k].info_type == PAITE_WIDGET && ParametersPointer->add_info_segments[k].info_size)
				{
					base_parameter_widget_struct* widget_obj = (base_parameter_widget_struct*)ParametersPointer->add_info_segments[k].info;

					if(widget_obj->widget_type != PWTE_COMMAND && widget_obj->widget_type != PWTE_UNKNOW)
					{

						ParametersPointer->enable_function.smart_parameter_write = 1;

						parameter_add_dependence(ParametersPointer, PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM,0,deviceInfo->external_device_stream, sizeof(external_device_parameter_stream__struct));

						count++;

					}
				}
			}
		 }


	}



	for(uint8_t i = 0; i<deviceInfo->device_manager->DeviceArrayCounter; i++)
	{
		device_parameters_struct* device = deviceInfo->device_manager->DeviceArray[i];

		for(uint8_t n = 0; n<device->ParametersSize; n++)
		{
			uint8_t add_info_size = device->ParametersInfo[n].additional_info_size;

			 if(device->ParametersInfo[n].synch_type == PSTE_STREAMING)
			 {
				 for(uint8_t k = 0; k<add_info_size; k++)
				{
					if(device->ParametersPointer[n].add_info_segments[k].info_type == PAITE_WIDGET && device->ParametersPointer[n].add_info_segments[k].info_size)
					{
						base_parameter_widget_struct* widget_obj = (base_parameter_widget_struct*)device->ParametersPointer[n].add_info_segments[k].info;

						if(widget_obj->widget_type != PWTE_COMMAND && widget_obj->widget_type != PWTE_UNKNOW)
						{

							device->ParametersPointer[n].enable_function.smart_parameter_write = 1;

							parameter_add_dependence(&device->ParametersPointer[n],PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM,0,deviceInfo->external_device_stream, sizeof(external_device_parameter_stream__struct));

							count++;

						}
					}
				}
			 }else if(device->ParametersInfo[n].synch_type == PSTE_WRITE_ONLY || device->ParametersInfo[n].synch_type == PSTE_NOTIFICATION )
			 {
				 for(uint8_t k = 0; k<add_info_size; k++)
				{
					if(device->ParametersPointer[n].add_info_segments[k].info_type == PAITE_WIDGET && device->ParametersPointer[n].add_info_segments[k].info_size)
					{
						base_parameter_widget_struct* widget_obj = (base_parameter_widget_struct*)device->ParametersPointer[n].add_info_segments[k].info;

						if(widget_obj->widget_type != PWTE_COMMAND && widget_obj->widget_type != PWTE_UNKNOW)
						{

							device->ParametersPointer[n].enable_function.smart_parameter_write = 1;

							parameter_add_dependence(&device->ParametersPointer[n],PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM,0,deviceInfo->external_device_stream, sizeof(external_device_parameter_stream__struct));

							count++;

						}
					}
				}
			 }
		}
	}


	deviceInfo->external_device_stream->array_counter = 0;
	deviceInfo->external_device_stream->array_size = count;
	deviceInfo->external_device_stream->protocol = protocol_str;
	deviceInfo->external_device_stream->param_array = (base_parameter_struct**)malloc(sizeof(base_parameter_struct*)*count);

	parameterStreamInited = true;

	}

	if(deviceInfo->external_device_stream->param_array && !parameterStreamEnable)
	{
		//TxExternalDeviceTimer = xTimerCreate( "TxExternalDeviceTimer", 30/portTICK_RATE_MS, pdTRUE , deviceInfo, osProgTimer_tx_to_external_device_exec);
		//xTimerStart(TxExternalDeviceTimer, portMAX_DELAY);

		xTaskCreate(tx_to_external_device_exec_task, "StreamTask", 2048, 0, 21, &parameter_transfer_task_handle);
		parameterStreamEnable = true;
	}

}


void AddParameterTransferFilter(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	if(parameterStreamInited)
	{
		if(deviceInfo->external_device_stream->filter_array_size > deviceInfo->external_device_stream->filter_array_counter)
		{
			parameter_stream_filter_struct* filter = (parameter_stream_filter_struct*)&receive_str.data_buf[1];

			deviceInfo->external_device_stream->filter_array[deviceInfo->external_device_stream->filter_array_counter].Address = filter->Address;
			deviceInfo->external_device_stream->filter_array[deviceInfo->external_device_stream->filter_array_counter].Command = filter->Command;
		}
	}
}



void Data_Transfer_Setting(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	switch(receive_str.data_buf[0])
	{
		case DTSE_START_PARAMETER_STREAM: 		StartParameterTransfer((master_device_struct*)deviceInfo, protocol_str,receive_str ); 		break;
		case DTSE_STOP_PARAMETER_STREAM: 		StopParameterTransfer((master_device_struct*)deviceInfo, protocol_str,receive_str ); 		break;
		case DTSE_ADD_PARAMETER_STREAM_FILTER: 	AddParameterTransferFilter((master_device_struct*)deviceInfo, protocol_str,receive_str ); 	break;
	}
}




void Remove_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}

void Get_All_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}

void Add_SubDevice(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}




void Sub_Device_Manager(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	switch(receive_str.data_buf[0])
	{
		case ADD_SUB_DEVICE: 			Add_SubDevice(deviceInfo, protocol_str,receive_str);  					break;
		case REMOVE_SUB_DEVICE: 		Remove_SubDevice(deviceInfo, protocol_str,receive_str); 				break;
		case GET_ALL_SUB_DEVICE:		Get_All_SubDevice(deviceInfo, protocol_str,receive_str); 				break;
	}
}




void ReadSubDeviceParameters(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{


	if(deviceInfo->device_manager == 0)
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
		return;
	}


	uint8_t device_count = deviceInfo->device_manager->DeviceArrayCounter;

	device_parameters_struct** DeviceArray = deviceInfo->device_manager->DeviceArray;

	device_parameters_struct* device = 0;

	uint8_t reqDeviceAddres = receive_str.data_buf[1];

	for(uint8_t i = 0; i<device_count; i++)
	{
		if(DeviceArray[i]->Address == reqDeviceAddres)
		{
			device = DeviceArray[i];
			break;
		}
	}


	if(device)
	{

		uint8_t param_start_index = receive_str.data_buf[2];
		uint8_t param_count = receive_str.data_buf[3];

		base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, 4 + sizeof(base_parameter_info_struct)*param_count);

		if(!data_segment)
		{
			protocol->remove_pack_info(protocol, receive_str.pack_info);
			return;
		}

		data_segment->pData[0] = READ_SUB_DEVICE_PARAMETERS;
		data_segment->pData[1] = reqDeviceAddres;
		data_segment->pData[2] = param_start_index;
		data_segment->pData[3] = param_count;

		memcpy(data_segment->pData + 4, &device->ParametersInfo[param_start_index], sizeof(base_parameter_info_struct)*param_count);

		if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
						protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);


	}else
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
	}
}



void UpdateDependence(base_parameter_struct* param)
{
	dependence_info_struct* dep_info_arr = param->pointers->dependence_info->array;
	uint8_t size = param->pointers->dependence_info->counter;
	for(uint8_t i = 0; i<size; i++)
	{
		if(dep_info_arr[i].info_type == PDTE_EXTERNAL_DEVICE_PARAMETER_STREAM)
		{
			external_device_parameter_stream__struct* device_param_stream = dep_info_arr[i].info;
			if(device_param_stream->external_stream_add_param_callback)
					device_param_stream->external_stream_add_param_callback(param, device_param_stream);

		}
	}
}

void UpdateConsumers(base_parameter_struct* param)
{
	for(uint8_t ik = 0; ik<param->pointers->parameters_consumers->counter; ik++)
	{
		if(param->pointers->parameters_consumers->array[ik].info->synch_type == PSTE_WRITE_ONLY)
		{
			base_parameter_struct* consumer_param = &param->pointers->parameters_consumers->array[ik];

			if(consumer_param->pointers->parameter_data_p != param->pointers->parameter_data_p)
				memcpy(consumer_param->pointers->parameter_data_p, param->pointers->parameter_data_p, param->info->parameter_size); // ??????

			if(consumer_param->pointers->device_owner)
			{
				WriteParameterToSubDevice(consumer_param->pointers->device_owner, consumer_param);
			}
		}
	}
}


void WriteParameterToSubDevice(device_parameters_struct* device, base_parameter_struct* param)
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


void UpdateStreamingParameter(base_parameter_struct* param, uint8_t* Data)
{

	for(uint16_t i = 0; i<param->info->parameter_size; i++)
	{
		if(param->pointers->parameter_data_p[i] != Data[i])
		{

			memcpy(param->pointers->parameter_data_p, Data, param->info->parameter_size);

			if(param->pointers->dependence_info)
									UpdateDependence(param);


			if(param->pointers->parameters_consumers)
									UpdateConsumers(param);

			return;
		}
	}
}





void WriteParameterInThisDevice(base_parameter_struct* param, uint8_t* Data)
{
	switch(param->info->type)
	{
		case PARTE_STRUCT_ARRAY_TYPE:
		{
			parameter_write_master_to_slave_struct_array( param, Data);
		}break;
		case PARTE_INT32_TYPE:
		{
			parameter_write_master_to_slave_simple( param, Data);
		}break;

		default:
		{
			parameter_write_master_to_slave_simple( param, Data);
		}break;

	}

	if(param->pointers->dependence_info)
									UpdateDependence(param);

	if(param->pointers->parameters_consumers)
									UpdateConsumers(param);

}

void WriteParameterInSubDevice(device_parameters_struct* device, base_parameter_struct* param, uint8_t* Data)
{
	switch(param->info->type)
	{
		case PARTE_STRUCT_ARRAY_TYPE:
		{
			parameter_write_master_to_slave_struct_array( param, Data);
		}break;
		case PARTE_INT32_TYPE:
		{
			parameter_write_master_to_slave_simple( param, Data);
		}break;

		default:
		{
			parameter_write_master_to_slave_simple( param, Data);
		}break;

	}


	switch(param->info->synch_direction)
	{
		case PSDE_MASTER_TO_SLAVE:
		{

			device_parameters_struct* device = (device_parameters_struct*)param->pointers->device_owner;
			if(device)
			{
				if(device->request_scheduller)
				{

					command_receive_struct command;
					command.DeviceAddress = device->Address;

					if(param->info->synch_type == PSTE_WRITE_ONLY || param->info->synch_type == PSTE_READ_WRITE)
					{
						command.command = param->info->ID | 0x80;
						command.dir = TX_ONLY;
					}


					command.request_type = BPRT_PARAMETERS;
					command.delete_data_after_move_to_tx_buffer = false;
					command.WriteData = param->pointers->parameter_data_p;
					command.WriteDataSize = param->info->parameter_size;
					command.flags = 0;

					AddToRequestQueue(device->request_scheduller->receiver_queue, command);
				}
			}
			break;
		}
		case PSDE_SLAVE_TO_MASTER:
		{
			break;
		}
		case PSDE_BIDIRECTIONAL:
		{

			device_parameters_struct* device = (device_parameters_struct*)param->pointers->device_owner;
			if(device)
			{
				if(device->request_scheduller)
				{

					command_receive_struct command;
					command.DeviceAddress = device->Address;

					if(param->info->synch_type == PSTE_WRITE_ONLY || param->info->synch_type == PSTE_READ_WRITE)
					{
						command.command = param->info->ID | 0x80;
						command.dir = TX_ONLY;
					}


					command.request_type = BPRT_PARAMETERS;
					command.delete_data_after_move_to_tx_buffer = false;
					command.WriteData = param->pointers->parameter_data_p;
					command.WriteDataSize = param->info->parameter_size;
					command.flags = 0;

					AddToRequestQueue(device->request_scheduller->receiver_queue, command);
				}
			}
			break;
		}

	}

	if(param->pointers->dependence_info)
										UpdateDependence(param);

	if(param->pointers->parameters_consumers)
										UpdateConsumers(param);
}


void StreamingParameterInThisDevice()
{

}


void SubDeviceParameterInitRead(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{
	if(deviceInfo->device_manager == 0)
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
		return;
	}

	int8_t device_count = deviceInfo->device_manager->DeviceArrayCounter;

	device_parameters_struct** DeviceArray = deviceInfo->device_manager->DeviceArray;

	device_parameters_struct* device = 0;

	uint8_t reqDeviceAddres = receive_str.data_buf[1];

	for(uint8_t i = 0; i<device_count; i++)
	{
		if(DeviceArray[i]->Address == reqDeviceAddres)
		{
			device = DeviceArray[i];
			break;
		}
	}


	if(device)
	{

		uint8_t paramID = receive_str.data_buf[2] & 0x7F;

		//device->ParametersPointer[paramID].parameter_data_p;
		uint16_t size = device->ParametersInfo[paramID].parameter_size;


		base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, size+3);

		if(!data_segment) return;

		data_segment->pData[0] = SUB_DEVICE_PARAMETER_INIT_READ;
		data_segment->pData[1] = reqDeviceAddres;
		data_segment->pData[2] = receive_str.data_buf[2];

		memcpy(data_segment->pData + 3, device->ParametersPointer[paramID].parameter_data_p, size);


		if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
			protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);

	}else
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
	}
}

void SubDeviceParameterInitWrite(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

}




void ParameterRequestHandler(base_device_information_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str)
{
	uint8_t status = 0;
	uint8_t Address = protocol_str->get_specific_byte_pack_info(protocol_str, receive_str.pack_info, PISE_ADDRESS, &status);


	bool write = receive_str.command & 0x80;
	uint8_t paramPosition = receive_str.command & 0x7F;



	base_parameter_info_struct* device_parameter_i = 0;
	base_parameter_pointers_struct* device_parameter_pointer = 0;
	device_parameters_struct* device = 0;

	if(status)
	{
		if(Address != 0)
		{
				//добавить обнавление параметра на ведомом устройстве

			device = found_device_from_address(device_manager, Address);

			if(device != 0)
			{
				if(paramPosition<device->ParametersSize)
				{

					device_parameter_i = &device->ParametersInfo[paramPosition];
					device_parameter_pointer = &device->ParametersPointer[paramPosition];
				}
			}
		}
	}


	if(!device)
	{
		uint8_t param_num = deviceInfo->init_device_info->param_num;

		if(paramPosition<param_num)
		{
			device_parameter_i = &deviceInfo->init_device_info->param_info_struct[paramPosition];
			device_parameter_pointer =  &deviceInfo->init_device_info->param_pointer_struct[paramPosition];
		}
	}


	if(device_parameter_i)
	{

		if(!device)
		{
			if(write)
			{
				switch(device_parameter_i->synch_type)
				{
					case PSTE_WRITE_ONLY:
					{
						base_parameter_struct param;
						param.info = device_parameter_i;
						param.pointers = device_parameter_pointer;

						WriteParameterInThisDevice(&param,receive_str.data_buf);

						protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);

						break;
					}
					case PSTE_READ_ONLY:
					{
						break;
					}
					case PSTE_STREAMING:
					{
						break;
					}
					case PSTE_NOTIFICATION:
					{
						break;
					}
					case PSTE_READ_WRITE:
					{
						base_parameter_struct param;
						param.info = device_parameter_i;
						param.pointers = device_parameter_pointer;

						WriteParameterInThisDevice(&param,receive_str.data_buf);

						protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);

						break;
					}
				}
			}else
			{
				base_protocol_data_segment_struct* data_segment = protocol_str->get_data_segment_func(protocol_str,device_parameter_i->parameter_size);

				if(!data_segment) return;

				memcpy(data_segment->pData, device_parameter_pointer->parameter_data_p,device_parameter_i->parameter_size);


				if(protocol_str->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
					protocol_str->protocol_answer_obj_data_func(protocol_str, receive_str.pack_info, data_segment);
			}
		}else
		{
			switch(device_parameter_i->synch_type)
			{
				case PSTE_WRITE_ONLY:
				{
					if(write)
					{
						base_parameter_struct param;
						param.info = device_parameter_i;
						param.pointers = device_parameter_pointer;

						WriteParameterInSubDevice(device, &param, receive_str.data_buf);
						protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);
					}
					break;
				}
				case PSTE_READ_ONLY:
				{
					break;
				}
				case PSTE_STREAMING:
				{
					break;
				}
				case PSTE_NOTIFICATION:
				{
					break;
				}
				case PSTE_READ_WRITE:
				{
					if(write)
					{
						base_parameter_struct param;
						param.info = device_parameter_i;
						param.pointers = device_parameter_pointer;

						WriteParameterInSubDevice(device, &param, receive_str.data_buf);
						protocol_str->remove_pack_info(protocol_str, receive_str.pack_info);
					}
					break;
				}
			}
		}
	}
}

void ReadSubDeviceAdditionalParameters(master_device_struct* deviceInfo, ubi4_bus_protocol_obj_struct* protocol, protocol_receive_struct receive_str)
{

	if(deviceInfo->device_manager == 0)
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
		return;
	}

	int8_t device_count = deviceInfo->device_manager->DeviceArrayCounter;

	device_parameters_struct** DeviceArray = deviceInfo->device_manager->DeviceArray;

	device_parameters_struct* device = 0;

	uint8_t reqDeviceAddres = receive_str.data_buf[1];

	for(uint8_t i = 0; i<device_count; i++)
	{
		if(DeviceArray[i]->Address == reqDeviceAddres)
		{
			device = DeviceArray[i];
			break;
		}
	}


	if(device)
	{

		uint8_t paramID = receive_str.data_buf[2] & 0x7F;

		uint8_t addInfoNum = device->ParametersInfo[paramID].additional_info_size;

		if(addInfoNum == 0)
		{
			protocol->remove_pack_info(protocol, receive_str.pack_info);
			return;
		}

		uint16_t allsize = 3 + (addInfoNum*sizeof(base_parameter_addinfoseg_struct));

		uint16_t h_size = allsize;

		for(uint8_t i = 0; i<addInfoNum; i++)
					allsize += device->ParametersPointer[paramID].add_info_segments[i].info_size;


		base_protocol_data_segment_struct* data_segment = protocol->get_data_segment_func(protocol, allsize);

		if(!data_segment)
		{
			protocol->remove_pack_info(protocol, receive_str.pack_info);
			return;
		}

		uint8_t* seg_p = (uint8_t*)device->ParametersPointer[paramID].add_info_segments;

		data_segment->pData[0] = READ_SUB_DEVICE_ADDITIONAL_PARAMETER;
		data_segment->pData[1] = reqDeviceAddres;
		data_segment->pData[2] = receive_str.data_buf[2];

		memcpy(data_segment->pData + 3, seg_p, h_size - 3);

		uint16_t data_counter = 0;
		for(uint8_t i = 0; i<addInfoNum; i++)
		{
			uint8_t* info_p = device->ParametersPointer[paramID].add_info_segments[i].info;
			uint16_t info_size = device->ParametersPointer[paramID].add_info_segments[i].info_size;

			memcpy(data_segment->pData + h_size + data_counter, info_p, info_size);

			data_counter+=info_size;
		}


		if(protocol->protocol_answer_obj_data_func && receive_str.pack_info && data_segment)
					protocol->protocol_answer_obj_data_func(protocol, receive_str.pack_info, data_segment);

	}else
	{
		protocol->remove_pack_info(protocol, receive_str.pack_info);
	}

}

#endif
