/*
  ******************************************************************************
  * @file    parameter_widgets.h
  * @author  A.Alborov
  ******************************************************************************
*/


#ifndef DATA_PARAMETER_WIDGETS_H_
#define DATA_PARAMETER_WIDGETS_H_


#include "stdint.h"
#include "stdbool.h"


typedef enum {
	PWTE_UNKNOW = 0,

	PWTE_COMMAND,

	PWTE_COMBOBOX_ENUM,
	PWTE_COMBOBOX_STRING,

	PWTE_ONE_CHANNEL_PLOT,
	PWTE_MULTY_CHANNEL_PLOT,
	PWTE_ONE_CHANNEL_PLOT_LEGEND,
	PWTE_MULTY_CHANNEL_PLOT_LEGEND,


	PWTE_EMG_GESTURE_CHANGE_SETTINGS,
	PWTE_GESTURE_SETTINGS,
	PWTE_CALIB_STATUS,
	PWTE_CONTROL_MODE,
	PWTE_OPEN_CLOSE_THRESHOLD,
	PWTE_SCALAR,

	PWTE_GESTURES_WINDOW,
	PWTE_SCALAR_ARRAY



	}parameter_widget_type_enum;




typedef enum {
	PWCE_UNKNOW = 0,
	PWCE_BUTTON,
	PWCE_SWITCH,
	PWCE_COMBOBOX,
	PWCE_SLIDER,
	PWCE_PLOT,
	PWCE_SPINBOX,

	PWCE_EMG_GESTURE_CHANGE_SETTINGS,
	PWCE_GESTURE_SETTINGS,
	PWCE_CALIB_STATUS,
	PWCE_CONTROL_MODE,
	PWCE_OPEN_CLOSE_THRESHOLD,
	PWCE_PLOT_AND_1_THRESHOLD,
	PWCE_PLOT_AND_2_THRESHOLD,

	PWCE_GESTURES_WINDOW,
	PWCE_OPTIC_LERNING_WIDGET

}parameter_widget_code_enum;


typedef enum {
	PWLE_UNKNOW = 0,
	PWLE_OPEN,
	PWLE_CLOSE,
	PWLE_CALIBRATE,
	PWLE_RESET,
	PWLE_CONTROL_SETTINGS,
	PWLE_OPEN_CLOSE_THRESHOLD,
	PWLE_SELECT_GESTURE,
	PWLE_SELECT_PROFILE,
	PWLE_GLOBAL_FORCE,
	PWLE_PLOT,
	PWLE_OMG_LEARNING
}parameter_widget_label_enum;


typedef enum {
	PWDCE_UNKNOW = 0,
	PWDCE_MAIN_DISPLAY,
	PWDCE_SERVICE_DISPLAY,


}parameter_widget_display_code_enum;



typedef enum {
    PWLTE_CODE_LABEL = 0,
    PWLTE_STRING_LABEL
}parameter_widget_label_type_enum;


typedef struct {
	uint8_t device_id;
	uint8_t widget_id;
} __attribute__((__packed__)) widget_id_struct;



union widget_id_union {
	uint16_t full_id;
	widget_id_struct id;
} __attribute__((__packed__));

typedef struct {

	uint8_t widget_type: 7;
	uint8_t widget_label_type: 1;
	uint8_t widget_code;
	uint8_t display;
	uint8_t widget_position;
	union widget_id_union widget_id;
	uint8_t data_offset;
	uint8_t data_size;
} __attribute__((__packed__)) base_parameter_widget_struct;



typedef struct {
	base_parameter_widget_struct Base;
	uint8_t label_code;
} __attribute__((__packed__)) base_parameter_widget_e_struct;

typedef struct {
	base_parameter_widget_struct Base;
	char 	label[32];
} __attribute__((__packed__)) base_parameter_widget_s_struct;

typedef struct {
    base_parameter_widget_e_struct Base;


} __attribute__((__packed__)) gesture_settings_widget_e_struct;

typedef struct {
	base_parameter_widget_e_struct Base;
	uint8_t click_command;
	uint8_t pressed_command;
	uint8_t released_command;

} __attribute__((__packed__)) command_parameter_widget_e_struct;

typedef struct {
	base_parameter_widget_s_struct Base;
	uint8_t click_command;
	uint8_t pressed_command;
	uint8_t released_command;

} __attribute__((__packed__)) command_parameter_widget_s_struct;


typedef struct {
	base_parameter_widget_e_struct Base;
	uint8_t control_type_enum_size;
} __attribute__((__packed__)) control_mode_parameter_widget_e_struct;


typedef struct {
	base_parameter_widget_e_struct Base;
	uint8_t color;
	int16_t max_size;
	int16_t min_size;

} __attribute__((__packed__)) plot_parameter_widget_e_struct;

typedef struct {
	base_parameter_widget_s_struct Base;
	uint8_t color;
	int16_t max_size;
	int16_t min_size;

} __attribute__((__packed__)) plot_parameter_widget_s_struct;


typedef struct {
	base_parameter_widget_e_struct Base;

	uint8_t start_learning_button_id;
	uint8_t click_command;

} __attribute__((__packed__)) optic_learning_widget_e_struct;




#endif /* DATA_PARAMETER_WIDGETS_H_ */
