
#include "Data/base_parameter.h"
#include "TransportLevel/a_protocol.h"


typedef struct
{

	uint8_t Code;

	uint8_t getCommand;
	uint8_t setCommand;

	uint8_t* pointer;
	uint32_t size;
}device_register_struct;





typedef struct
{
	uint8_t Code;
	base_parameter_info_struct* parameters;
	uint32_t parameters_size;
	uint8_t* Memory;
	uint32_t size;
}device_memory_segment_struct;


typedef struct
{
	uint8_t address;

	base_parameter_info_struct* parameters;
	uint32_t parameters_size;


	device_memory_segment_struct* Memory;
	uint32_t MemorySegNum;

}base_virtual_device_struct;


//void ReadDeviceParameters(ubi4_bus_protocol_obj_struct* protocol, void* pInfo, uint8_t command, uint8_t* Buf, uint16_t size);


