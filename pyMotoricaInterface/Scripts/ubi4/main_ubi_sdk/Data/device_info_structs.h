/*
 ******************************************************************************
 * @file    device_info_structs.h
 * @author  A.Alborov
 ******************************************************************************
 */

#ifndef INC_DEVICE_INFO_STRUCTS_H_
#define INC_DEVICE_INFO_STRUCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "UBI_SDK.h"
//#include "binding_system.h"

typedef enum {
    DTCE_UNKNOW = 0,
    DTCE_BOOTLOADER_INFO_TYPE,
    DTCE_FW_INFO_TYPE,
    DTCE_DEVICE_INFO_TYPE,
    DTCE_BOARD_INFO_TYPE,
    DTCE_PRODUCT_INFO_TYPE,
    DTCE_SERVICE_INFO,

    DTCE_SYSTEM_DEVICES,

    DTCE_DRIVE_INFO,

    DTCE_GESTURE_COLLECTION,
    DTCE_USER_GESTURES,
    DTCE_DRIVE_SETTINGS,

    DTCE_EMG_SETTINGS,
    DTCE_INDY_SETTINGS,
    DTCE_BMS_SETTING,
    DTCE_FEST_X_SETTINGS,
    DTCE_MOTOR_SETTINGS,
    DTCE_STATIC_BINDING_STRUCTS,
    DTCE_DYNAMIC_BINDING_STRUCTS,
    DTCE_UPPER_LIMB_PROSTHESIS_SETTINGS,

    DCTE_GESTURES_KEY_DESCRIPTION,
    DCTE_GESTURES_STRING_DESCRIPTION,

    DTCE_FREE_SLOT = 0xFF,
} data_table_slots_code_enum;




typedef struct {
    char DeviceName[32];
    uint8_t DeviceVersion;
    uint8_t DeviceSubVersion;
    char DeviceLabel[16];
    uint8_t DeviceType;
    uint8_t DeviceCode;
    uint8_t DeviceRole;

    uint8_t DeviceAddress;

    char DeviceUUID_Prefix[16];
    uint32_t DeviceUUID;

    uint8_t DeviceAdditionalInfoType;
    uint32_t DeviceAdditionalInfo;
} __attribute__((__packed__)) baseDeviceInfo_struct;

typedef enum {
    PTE_PROSTETIC = 0,
    PTE_DIAGNOSTIC_EQUIPMENT,
} product_type_enum;

typedef enum {
    PPCE_LEFT_HAND = 0,
    PPCE_RIGHT_HAND,
} product_code_prostetic_enum;

typedef enum {
    PCDEE_MYOBOX  = 0b00000001,
    PCDEE_OPTOBOX = 0b00000010,
    PCDEE_REOBOX  = 0b00000100,
} product_code_diagnistic_equipment_enum;

typedef struct {
    char ProductName[32];
    uint8_t ProductVersion;
    uint8_t ProductSubVersion;
    char ProductLabel[16];
    uint8_t ProductType;
    uint8_t ProductCode;

    char ProductUUID_Prefix[16];
    uint32_t ProductUUID;

    uint8_t ProductAdditionalInfoType;
    uint32_t ProductAdditionalInfo;

} __attribute__((__packed__)) base_ProductInfo_struct;

typedef struct {
    char BoardName[32];
    uint8_t BoardVersion;
    uint8_t BoardSubVersion;
    uint8_t BoardRev;
    uint8_t BoardSubRev;
    uint16_t BoardBuild;
    uint8_t BoardType;
    uint8_t BoardCode;

    uint8_t BoardAdditionalInfoType;
    char *BoardAdditionalInfo;

} __attribute__((__packed__)) base_BoardInfo_struct;

typedef struct {
    char FWName[32];
    uint8_t FWVersion;
    uint8_t FWSubVersion;
    char FWLabel[16];
    uint8_t FWType;
    uint8_t FWCode;

    uint32_t FWStartAddress;
    uint32_t FWSize;
    uint8_t FWCRC;

    uint8_t ProtocolVersion;
    uint8_t ProtocolSubVersion;

    uint8_t FWAdditionalInfoType;
    uint32_t FWAdditionalInfo;

} __attribute__((__packed__)) base_FWInfo_struct;

typedef struct {
    uint8_t BootloaderCode;
    uint8_t BootloaderVersion;
    uint8_t BootloaderSubVersion;

    uint8_t ProtocolVersion;
    uint8_t ProtocolSubVersion;

    uint8_t BootloaderAdditionalInfoType;

    uint8_t BootloaderCRC;

    uint8_t BootloaderStart :1;
    uint8_t BotloaderRegRes :7;

    uint32_t BootloaderStartAddress;
    uint32_t BootloaderSize;

    uint32_t BootloaderAdditionalInfo;

} __attribute__((__packed__)) base_BootloaderInfo_struct;

typedef struct {
    uint8_t ProductionDate_Day;
    uint8_t ProductionDate_Month;
    uint16_t ProductionDate_Year;

    uint8_t LastServiceDate_Day;
    uint8_t LastServiceDate_Month;
    uint16_t LastServiceDate_Year;

} __attribute__((__packed__)) base_ServiceInfo_struct;

#ifdef MAX_SUB_DEVICES

typedef struct {
    uint8_t DeviceType;
    uint8_t DeviceCode;
    uint8_t DeviceRole;

    uint8_t DeviceVersion;
    uint8_t DeviceSubVersion;

    uint8_t DeviceAddress;

    uint8_t ParametersNum;

    uint8_t SubDeviceNum;

    uint8_t DefaultPort;

} __attribute__((__packed__)) baseSubDeviceInfo_struct;

typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    baseSubDeviceInfo_struct DevicesArray[MAX_SUB_DEVICES];

} __attribute__((__packed__)) baseSubDevicesArrayInfo_struct;

#endif

#ifdef DRIVE_NUM

typedef struct {
    uint32_t calib_steps;
    uint16_t calib_current;
    uint8_t calib_status;

    uint16_t current_limit_ma;
    uint8_t current_limit_percent;
    uint16_t overcurrent_counter_max;

    uint32_t motor_cycles_100ms;
    uint32_t motor_driver_cycles_100ms;

} __attribute__((__packed__)) Drive_struct;

typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    Drive_struct GesturesArray[DRIVE_NUM];

} __attribute__((__packed__)) DriveSettingsInfo_struct;

#endif

#ifdef DRIVE_NUM

typedef enum {
    OCBS_OPEN = 0x00,
    OCBS_CLOSE = 0x01,
    OCBS_OPEN_DELAY = 0x80,
    OCBS_CLOSE_DELAY = 0x81,
} OPEN_CLOSE_BUTTON_STATE_ENUM;

typedef struct {
    uint8_t id;
    uint8_t open_position[DRIVE_NUM];
    uint8_t close_position[DRIVE_NUM];
    uint8_t open_to_close_time_shift[DRIVE_NUM];
    uint8_t close_to_open_time_shift[DRIVE_NUM];
} __attribute__((__packed__)) GestureInfo_struct;

typedef struct {
    GestureInfo_struct gesture_info;
    OPEN_CLOSE_BUTTON_STATE_ENUM open_close_button_state;
}__attribute__((__packed__)) GestureInfoTransfer_struct;

#ifdef GESTURE_COLLECTION_SIZE

typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    uint8_t NumDrive;
    GestureInfo_struct GesturesArray[GESTURE_COLLECTION_SIZE];

} __attribute__((__packed__)) GesturesCollectionInfo_struct;

typedef struct {
    uint8_t slot;
    uint8_t pos;
    uint8_t ID;
    uint8_t ImageKey;
    uint8_t Code;

} __attribute__((__packed__)) GesturesKeyDescriptionItem_struct;

typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    GesturesKeyDescriptionItem_struct GesturesDiscriptionArray[GESTURE_COLLECTION_SIZE];

} __attribute__((__packed__)) GesturesKeyDescription_struct;

#endif

#ifdef USER_GESTURES_NUM
typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    uint8_t NumDrive;
    GestureInfo_struct GesturesArray[USER_GESTURES_NUM];

} __attribute__((__packed__)) UserGesturesInfo_struct;

typedef struct {
    uint8_t slot;
    uint8_t pos;
    uint8_t ID;
    uint8_t ImageKey;
    uint8_t Code;
    char name[16];

} __attribute__((__packed__)) GesturesStringDescriptionItem_struct;

typedef struct {
    uint8_t Size;
    uint8_t Count;
    uint16_t ItemSize;
    GesturesStringDescriptionItem_struct GesturesDiscriptionArray[USER_GESTURES_NUM];

} __attribute__((__packed__)) GesturesStringDescription_struct;

typedef struct {
    uint32_t encoder_steps[6];
    int16_t current[6];
    uint8_t status[6];
} __attribute__((packed)) FingerCalib_struct;

#endif

#endif

/* EXAMPLE PARAMETERS SLOT STRUCT in DataTable
 * Need copy in device_info.h
 typedef struct
 {
 uint8_t Size;		// = DEVICE_PARAMETERS_SIZE;
 uint8_t Count		// = DEVICE_PARAMETERS_SIZE;
 uint16_t ItemSize;	// = sizeof(base_parameter_info_struct);
 base_parameter_info_struct ParametersInfo[DEVICE_PARAMETERS_SIZE];

 }__attribute__((__packed__))ParametersSlotInfo_struct;
 */

#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_INFO_STRUCTS_H_*/
