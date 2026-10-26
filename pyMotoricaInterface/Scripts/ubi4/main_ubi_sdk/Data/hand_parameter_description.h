/*
  ******************************************************************************
  * @file    hand_parameter_description.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef DATA_HAND_PARAMETER_DESCRIPTION_H_
#define DATA_HAND_PARAMETER_DESCRIPTION_H_


#include "stdint.h"
#include "stdbool.h"




typedef enum {

		PDCE_SIMPLE_COMMAND = 0,

		PDCE_ACTION_REQUEST = 30,


//Global settings
		PDCE_SELECT_GESTURE = 1,					// uint8_t select_gesture;
		PDCE_SELECT_PROFILE = 2,					// uint8_t select_profile;
		PDCE_GLOBAL_FORCE = 3,						// uint8_t global_force;
		PDCE_GLOBAL_SENSITIVITY = 4,				// uint8_t global_sensitivity;
		PDCE_GLOBAL_THRESHOLD = 5,					// uint8_t global_threshold;



		PDCE_UNIVERSAL_CONTROL_INPUT = 6,
		PDCE_OPEN_CLOSE_SIGNAL = 7,                 // uint8_t open uint8_t close signal


		PDCE_EMG_CH_1_3_VAL = 8,
		PDCE_EMG_CH_4_6_VAL = 9,
		PDCE_EMG_CH_7_9_VAL = 10,

		PDCE_EMG_CH_1_3_GAIN = 11,
		PDCE_EMG_CH_4_6_GAIN = 12,
		PDCE_EMG_CH_7_9_GAIN = 13,



////////////////////////////////


// Drive control set group
		PDCE_MOVE_DRIVE_PERCENT = 14,					//int8_t move_drive[DRIVE_NUM]; // drive forward: 1-100(speed percent) | drive stop 0 | drive reverse: (-1)-(-100)(speed percent)
		PDCE_TARGET_DRIVE_POSITION_PERCENT = 15,		//int8_t drive_pos[DRIVE_NUM]; // drive forward: 0-100(pos percent) 0 - open, 100-close
		PDCE_TARGET_DRIVE_SPEED_PERCENT = 16,			//
		PDCE_TARGET_DRIVE_FORCE_PERCENT = 17,			//

// Drive control get group
		PDCE_CURRENT_DRIVE_POSITION = 18,			//
		PDCE_CURRENT_DRIVE_CURRENT_UINT8 = 19,		//
		PDCE_CURRENT_DRIVE_CURRENT_UINT16 = 20,		//
		PDCE_CURRENT_DRIVE_FORCE_UINT8 = 21,		//
		PDCE_CURRENT_DRIVE_FORCE_UINT16 = 22,		//



		PDCE_GESTURES_CHANGE_SETTINGS = 23,				//
		PDCE_CONTROL_MODE_SETTINGS = 24,				//
		PDCE_DRIVE_SETTINGS = 25,						//
		PDCE_OPEN_CLOSE_THRESHOLD = 26,					// open_close_threshold_param_struct


		PDCE_CALIB_STATUS = 27,							//
		PDCE_CURRENT_LIMITS = 28,						//
        PDCE_BMS_STATUS_COMBINED_PARAM = 29,             //

		PDCE_GESTURE_SETTINGS = 31,

		PDCE_GESTURE_GROUP = 32,

		PDCE_OPTIC_LEARNING_DATA = 33,

		PDCE_EMG_ENV_E_VAL = 34,

		PDCE_DMS_OUTPUT = 35,

		PDCE_DATE_AND_TIME = 36,

		PDCE_INTERFECE_ERROR_COUNTER = 37,
		PDCE_CALIBRATION_CURRENT_PERCENT = 38,

		///////////toggles
		PDCE_ENERGY_SAVE_MODE = 39,

		PDCE_LEFT_RIGHT_HAND_MODE = 40,
		PDCE_SWITCH_SENSORS = 41,
		
		PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER = 42,


////////////////////////////////


}parameter_data_code_enum;



typedef struct {
    uint8_t bat_level;
    uint8_t charge_status;
    uint16_t charge_current;
} __attribute__((__packed__)) bms_status_combined_parameter_struct;

typedef struct {
    uint8_t open_signal;
    uint8_t close_signal;
}__attribute__((__packed__)) open_close_signal_param_struct;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t week_day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} __attribute__((__packed__)) rtc_date_time_param_struct;


typedef enum {
    RATE_NO_CHANGE,
    RATE_INCREASE,
    RATE_DECREASE,
    RATE_IN_DE_CREASE,
    RATE_SET_VALUE,
    RATE_REQUEST_VALUE,
	RATE_ENABLE_TRANSFER
}request_action_type_enum;

typedef struct
{
	uint8_t data_code;
	uint8_t data_instance;
	uint8_t action_priority;
	uint8_t action_type;
	uint8_t action_args[2];
}__attribute__((__packed__))action_request_param_struct;

typedef struct
{
	uint8_t open_upper;
	uint8_t open_lower;
    uint8_t close_upper;
	uint8_t close_lower;
}__attribute__((__packed__))open_close_threshold_param_struct;

typedef struct
{
	uint8_t gesture_id;
	uint8_t drive_num;

	uint8_t OpenPosition[6];
	uint8_t ClosePosition[6];
	uint8_t OpenToCloseTimeShift[6];
	uint8_t CloseToOpenTimeShift[6];

}__attribute__((__packed__))editable_gesture_param_struct;


typedef struct
{
	uint8_t gesture_id;
	uint8_t gesture_image_code;
}__attribute__((__packed__))gesture_group_param_item_struct;


#define GESTURE_GROUP_SIZE 8

typedef struct
{

	gesture_group_param_item_struct gestures_description[GESTURE_GROUP_SIZE];

}__attribute__((__packed__))gesture_group_param_struct;




#endif /* DATA_BASE_PARAMETER_H_ */
