#ifndef UBI4_ENUM_DISCRIPTION_H_
#define UBI4_ENUM_DISCRIPTION_H_

#ifdef __cplusplus
 extern "C" {
#endif


typedef enum {
	DCE_CPU_MODULE = 1,
	DCE_FEST_H_AND_F,
	DCE_INDY,
	DCE_EMG_SENSE,
	DCE_BMS,
	DCE_GUI,
	DCE_OMG_MODULE

	}device_codes_enum;

typedef enum {

	DTE_UNKNOW						= 		0x00,				//0x0  	0
	DTE_CPU 						= 		0x01, 				//0x0  	1
	DTE_SUB_CPU						=		0x02, 				//0x0  	2

	DTE_DRIVER 						= 		0x10,               //0x1 	0
	DTE_MOTOR_DRIVER 				= 		0x11, 				//0x1 	1
	DTE_MOTOR_DRIVER_MULTY_CH 		= 		0x12,				//0x1 	2
	
	DTE_SENSOR 						= 		0x20, 				//0x2 	0
	DTE_EMG_1CH 					=		0x21, 				//0x2 	1
	DTE_EMG_2CH 					=		0x22, 				//0x2 	2
	DTE_EMG_MULTY_CH 				=		0x23, 				//0x2 	3
	DTE_OMG_SEGMENT 				=		0x24, 				//0x2 	4
	DTE_OMG_MULTY_CH 				=		0x25, 				//0x2 	5
	DTE_OMG_EMG_SEGMENT 			=		0x26, 				//0x2 	6
	DTE_OMG_EMG_MULTY_CH			=		0x27, 				//0x2 	7

	DTE_STIMULATOR 					=		0x30, 				//0x3 	0

	DTE_HMI 						= 		0x40, 				//0x4 	0
	DTE_DISPLAY						=		0x41,				//0x4 	1
	DTE_BUTTONS						=		0x42,				//0x4 	2
	DTE_SOUNDS						=		0x43,				//0x4 	3
	DTE_DISPLAY_BUTTONS				=		0x44,				//0x4 	4
	DTE_DISPLAY_SOUNDS				=		0x45,				//0x4 	5
	DTE_BUTTONS_SOUNDS				=		0x46,				//0x4 	6
	DTE_DISPLAY_BUTTONS_SOUNDS		=		0x47,				//0x4 	7
	DTE_TOUCH_DISPLAY				=		0x48,				//0x4 	8
	DTE_TOUCH_DISPLAY_SOUNDS		=		0x49,				//0x4 	9

	DTE_POWER						=		0x50,				//0x5 	0
	DTE_BMS 						= 		0x51, 				//0x5 	1
	DTE_ACB							=		0x52, 				//0x5 	2
	DTE_POWER_CONVERTER 			=		0x53, 				//0x5 	3
	DTE_POWER_ISOLATOR				=		0x54, 				//0x5 	4
	DTE_POWER_COMBO					=		0x55, 				//0x5 	5

	DTE_COMBO 						= 		0x60, 				//0x6	0

	DTE_OTHER 						= 		0x70, 				//0x7	0



	}device_types_enum;

typedef enum {
	DRE_UNKNOWN = 0,
    DRE_MAIN_CONTROLLER,
    DRE_DISPLAY,
    DRE_TOUCH_DISPLAY,
    DRE_BUTTON,
    DRE_CONTROL_PANEL,
    DRE_BMS,
	DRE_OMG,

    // SENSORS
        //HAND
    DRE_HAND_OPEN_SENSOR,
    DRE_HAND_CLOSE_SENSOR,
    DRE_HAND_OPEN_CLOSE_SENSOR,
    DRE_HAND_ARRAY_SENSOR,

        //WRIST
    DRE_WRIST_ARRAY_SENSOR,

            //ROTATION (ROLL)
    DRE_WRIST_ROTATION_OPEN_SENSOR,
    DRE_WRIST_ROTATION_CLOSE_SENSOR,
    DRE_WRIST_ROTATION_OPEN_CLOSE_SENSOR,

            //FLEXION (PITCH)
    DRE_WRIST_FLEXION_OPEN_SENSOR,
    DRE_WRIST_FLEXION_CLOSE_SENSOR,
    DRE_WRIST_FLEXION_OPEN_CLOSE_SENSOR,

            //OPPOSITION (YAW)
    DRE_WRIST_OPPOSITION_OPEN_SENSOR,
    DRE_WRIST_OPPOSITION_CLOSE_SENSOR,
    DRE_WRIST_OPPOSITION_OPEN_CLOSE_SENSOR,

        //ELBOW
    DRE_ELBOW_ARRAY_SENSOR,

            //FLEXION
    DRE_ELBOW_FLEXION_OPEN_SENSOR,
    DRE_ELBOW_FLEXION_CLOSE_SENSOR,
    DRE_ELBOW_FLEXION_OPEN_CLOSE_SENSOR,

            //ROTATION
    DRE_ELBOW_ROTATION_OPEN_SENSOR,
    DRE_ELBOW_ROTATION_CLOSE_SENSOR,
    DRE_ELBOW_ROTATION_OPEN_CLOSE_SENSOR,

        //GESTURE CONTROL
	DRE_GESTURE_CONTROL_SENSOR,


    //DRIVERS
    DRE_HAND_DRIVER,

    DRE_WRIST_DRIVER,
    DRE_WRIST_ROTATION_DRIVER,
    DRE_WRIST_FLEXION_DRIVER,
    DRE_WRIST_OPOSITION_DRIVER,
    DRE_WRIST_ROTATION_FLEXION_DRIVER,

    DRE_ELBOW_DRIVER,
    DRE_ELBOW_FLEXION_DRIVER,
    DRE_ELBOW_OPOSITION_DRIVER,


    DRE_INDEX_FINGER_DRIVER,
    DRE_MIDDLE_FINGER_DRIVER,
    DRE_RING_FINGER_DRIVER,
    DRE_PINKY_FINGER_DRIVER,
    DRE_THUMB_FINGER_DRIVER,
    DRE_ROTATION_FINGER_DRIVER,
    DRE_ROTATION_THUMB_FINGER_DRIVER,


}device_roles_enum;






typedef enum {
BTE_FF = 1
}board_types_enum;

typedef enum {
	BCE_CPU_MODULE = 1,
	BCE_FEST_X_DRIVER,
	BCE_SINGLE_GRIP_DRIVER,
	BCE_EMG_SENSE_32BIT,
	BCE_CHARGER_2S_USB_ROUND,
	BCE_GUI_MODULE
}board_codes_enum;

/*
typedef enum {
		BCE_BASE_BOOTLOADER = 1
}bootloader_code_enum;
*/



	typedef enum {
			PTE_UNKNOW = 0,
			PTE_BOOTLOADER,
			PTE_MAIN_PROGRAM
		}Program_type_code_enum;



	typedef enum {
			BCE_UNKNOW = 0,
			BCE_BASE_BOOTLOADER,
			BCE_TEST_STM32F4_BOOTLOADER
		}bootloader_code_enum;

	typedef enum {
			BAIT_EMPTY = 0,
		}bootloader_additional_info_type_enum;

#ifdef __cplusplus
}
#endif

#endif /* UBI4_ENUM_DISCRIPTION_H_*/
