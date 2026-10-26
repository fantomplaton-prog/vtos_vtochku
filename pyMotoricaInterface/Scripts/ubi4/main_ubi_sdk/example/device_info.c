/*
  ******************************************************************************
  * @file    device_info.c
  * @author  A.Alborov
  ******************************************************************************
*/


/*

#ifdef example

#include "device_info.h"
#include "Drivers/stm32/flash_driver.h"
#include "Devices/enum_discription.h"
#include "Data/data_table.h"
#include "Data/base_parameter.h"
#include "Data/device_command_structs.h"
#include "Devices/enum_discription.h"
#include "Data/hand_parameter_description.h"
#include "Data/parameter_widgets.h"

#include "string.h"
#include "malloc.h"
#include "main.h"


base_memory_driver_struct* flash_memory_driver;
base_device_information_struct* device_info;

init_device_information_struct init_info;

base_parameter_pointers_struct device_parameters_pointer[PCE_NUM];


data_table_struct data_table = {
		.DataTableStartAddres =  0x0800C000,
		.slots_num = 6,

		.info = 0,
		.memory_driver=0,
		.slots = 0
};

uint32_t DataTableSize = 0;


data_table_slot_struct DataTableSlots[DSE_NUM] =
{
		{
				.DataCode = DTCE_BOOTLOADER_INFO_TYPE,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 0,
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(base_BootloaderInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DTCE_FW_INFO_TYPE,
				.DataType = DTE_STRUCT,
				.StartAddressShift = ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8),
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(base_FWInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DTCE_DEVICE_INFO_TYPE,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 	ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8) +
									 	 ROUND_MEMORY_CELL_SIZE(sizeof(base_FWInfo_struct),8),
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(baseDeviceInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DTCE_BOARD_INFO_TYPE,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 	ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_FWInfo_struct),8)+
										ROUND_MEMORY_CELL_SIZE(sizeof(baseDeviceInfo_struct),8),
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(base_BoardInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DTСE_PRODUCT_INFO_TYPE,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 	ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_FWInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(baseDeviceInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_BoardInfo_struct),8),
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(base_ProductInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DTСE_SERVICE_INFO,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 	ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_FWInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(baseDeviceInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_BoardInfo_struct),8)+
										ROUND_MEMORY_CELL_SIZE(sizeof(base_ProductInfo_struct),8),
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(base_ServiceInfo_struct),
				.crc = 0
		},
		{
				.DataCode = DSE_REO_SETTINGS,
				.DataType = DTE_STRUCT,
				.StartAddressShift = 	ROUND_MEMORY_CELL_SIZE(sizeof(base_BootloaderInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_FWInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(baseDeviceInfo_struct),8) +
										ROUND_MEMORY_CELL_SIZE(sizeof(base_BoardInfo_struct),8)+
										ROUND_MEMORY_CELL_SIZE(sizeof(base_ProductInfo_struct),8)+
										ROUND_MEMORY_CELL_SIZE(sizeof(base_ServiceInfo_struct),8)
				.DataTypeVersion = 1,
				.DataTypeSubVersion = 0,
				.DataSize = sizeof(ROM_REO_settings_struct),
				.crc = 0
		}
};


base_BootloaderInfo_struct BootloaderInfo =
{
		.BootloaderCode = BCE_BASE_BOOTLOADER,
		.BootloaderVersion = 1,
		.BootloaderSubVersion = 0,
		.ProtocolVersion = 1,
		.ProtocolSubVersion = 0,

		.BootloaderStartAddress = 0x08000000,
		.BootloaderSize = 0,
		.BootloaderCRC = 0,

		.BootloaderStart = 0,
		.BotloaderRegRes = 0,



		.BootloaderAdditionalInfoType = BAIT_EMPTY,
		.BootloaderAdditionalInfo = 0

};

base_FWInfo_struct FW_Info =
{
		.FWCode = 0,
		.FWType = 0,
		.FWVersion = 0,
		.FWSubVersion = 0,
		.ProtocolVersion = 0,
		.ProtocolSubVersion = 0,

		.FWStartAddress = 0,
		.FWSize = 0,
		.FWCRC = 0,

		.FWAdditionalInfoType = 0,
		.FWAdditionalInfo = 0

};

base_BoardInfo_struct Board_Info =
{
		.BoardName = "MultigripX",
		.BoardVersion = 2,
		.BoardSubVersion = 0,
		.BoardRev = 0,
		.BoardSubRev = 0,
		.BoardBuild = 1,
		.BoardType = 1,
		.BoardCode = BCE_EMG_SENSE_32BIT,

		.BoardAdditionalInfoType = 0,
		.BoardAdditionalInfo = 0
};

base_ProductInfo_struct ProductInfo =
{
		.ProductName = "FEST-H",
		.ProductVersion = 2,
		.ProductSubVersion = 4,
		.ProductLabel = "pilot",
		.ProductType = 1,
		.ProductCode = 2,

		.ProductUUID_Prefix = "ELBOW-H ",
		.ProductUUID = 123456,

		.ProductAdditionalInfoType = 0,
		.ProductAdditionalInfo = 0
};

baseDeviceInfo_struct DeviceInfo =
{
		.DeviceName = "FEST-H",
		.DeviceVersion = 1,
		.DeviceSubVersion = 0,
		.DeviceLabel = "",
		.DeviceType = DTE_MOTOR_DRIVER_MULTY_CH,
		.DeviceCode = DCE_FEST_H_AND_F,
		.DeviceRole = DRE_HAND_DRIVER,

		.DeviceAddress = 9,

		.DeviceUUID_Prefix = "FEST-H",
		.DeviceUUID = 1234567,

		.DeviceAdditionalInfoType = 0,
		.DeviceAdditionalInfo = 0
};

base_ServiceInfo_struct ServiceInfo =
{
		.ProductionDate_Day = 9,
		.ProductionDate_Month = 2,
		.ProductionDate_Year = 2023,

		.LastServiceDate_Day = 0,
		.LastServiceDate_Month = 0,
		.LastServiceDate_Year = 0
};

ROM_REO_settings_struct ROM_REO_settings =
{

};




const base_parameter_info_struct device_parameters[PCE_NUM] =
{
	[PCE_REO_SETTINGS] =
	{
			.flag_shift = 0,

			.parameter_size = sizeof(USER_REO_settings_struct),

			.type = PARTE_STRUCT_TYPE,
			.synch_type = PSTE_WRITE_ONLY,
			.synch_direction = PSDE_MASTER_TO_SLAVE,
			.synch_period = 1,
			.init_read = 1,

			.ID = PCE_REO_SETTINGS,
			.DataCode = PDCE_REO_SETTINGS

	},[PCE_REO_SIGNALS_0_5] =
	{
		.flag_shift = 0,

		.parameter_size = 6,

		.type = PARTE_UINT8_TYPE,
		.synch_type = PSTE_STREAMING,
		.synch_direction = PSDE_SLAVE_TO_MASTER,
		.synch_period = 2,
		.DataInstance = 0,

		.ID = PCE_REO_SIGNALS_0_5,
		.DataCode = PDCE_REO_SIGNALS
	},[PCE_REO_SIGNALS_6_11] =
	{
		.flag_shift = 0,

		.parameter_size = 6,

		.type = PARTE_UINT8_TYPE,
		.synch_type = PSTE_STREAMING,
		.synch_direction = PSDE_SLAVE_TO_MASTER,
		.synch_period = 2,
		.DataInstance = 1,

		.ID = PCE_REO_SIGNALS_0_5,
		.DataCode = PDCE_REO_SIGNALS
	},[PCE_REO_SIGNALS_12_17] =
	{
		.flag_shift = 0,

		.parameter_size = 6,

		.type = PARTE_UINT8_TYPE,
		.synch_type = PSTE_STREAMING,
		.synch_direction = PSDE_SLAVE_TO_MASTER,
		.synch_period = 2,
		.DataInstance = 2,

		.ID = PCE_REO_SIGNALS_0_5,
		.DataCode = PDCE_REO_SIGNALS
	},[PCE_REO_SIGNALS_18_23] =
	{
		.flag_shift = 0,

		.parameter_size = 6,

		.type = PARTE_UINT8_TYPE,
		.synch_type = PSTE_STREAMING,
		.synch_direction = PSDE_SLAVE_TO_MASTER,
		.synch_period = 2,
		.DataInstance = 3,

		.ID = PCE_REO_SIGNALS_0_5,
		.DataCode = PDCE_REO_SIGNALS
	}
};




uint8_t REO_Values[24];
USER_REO_settings_struct REO_user_settings;





base_parameter_pointers_struct device_parameters_pointer[PCE_NUM] =
{
		[PCE_REO_SETTINGS] =
		{
			.change_notification = 0,
			.flag_buffer = 0,
			.related_parameter_p = 0,
			.parameter_data_p = (uint8_t*)&REO_user_settings,
			.add_info_segments = 0,
			.tx_notification = 0
		}, [PCE_REO_SIGNALS_0_5] =
		{
			.change_notification = 0,
			.flag_buffer = 0,
			.related_parameter_p = 0,
			.parameter_data_p = &REO_Values[0],
			.tx_notification = 0

		}, [PCE_REO_SIGNALS_6_11] =
		{
			.change_notification = 0,
			.flag_buffer = 0,
			.related_parameter_p = 0,
			.parameter_data_p = &REO_Values[6],
			.tx_notification = 0

		}, [PCE_REO_SIGNALS_12_17] =
		{
			.change_notification = 0,
			.flag_buffer = 0,
			.related_parameter_p = 0,
			.parameter_data_p = &REO_Values[12],
			.tx_notification = 0

		}, [PCE_REO_SIGNALS_18_23] =
		{
			.change_notification = 0,
			.flag_buffer = 0,
			.related_parameter_p = 0,
			.parameter_data_p = &REO_Values[18],
			.tx_notification = 0

		}
};




uint8_t* Info_p_buf[DSE_NUM] = {(uint8_t*)&BootloaderInfo,(uint8_t*)&FW_Info,(uint8_t*)&DeviceInfo,(uint8_t*)&Board_Info,(uint8_t*)&ProductInfo,(uint8_t*)&ServiceInfo,(uint8_t*)&ROM_REO_settings};




void init_device_info()
{

	init_info.DataTableStartAddres = 0x08010000;

	init_info.default_data_table.slots = DataTableSlots;
	init_info.default_data_table.data_pointer_buffer = Info_p_buf;
	init_info.default_data_table.slot_num = DSE_NUM;


	init_info.param_info_struct = device_parameters;
	init_info.param_pointer_struct = device_parameters_pointer;
	init_info.param_num = PCE_NUM;

	init_flash_driver(&flash_memory_driver);

	init_info.memory_driver = flash_memory_driver;

	device_info = (base_device_information_struct*)malloc(sizeof(base_device_information_struct));
	memset(device_info, 0, sizeof(base_device_information_struct));
	init_base_device_info(&init_info, device_info);

}




#endif

*/
