/*
 * control_type_parameter.h
 *
 *  Created on: 15 июл. 2024 г.
 *      Author: PC
 */

#ifndef DATA_CONTROL_TYPE_PARAMETER_H_
#define DATA_CONTROL_TYPE_PARAMETER_H_


#include "stdint.h"
#include "stdbool.h"


// Тип управления
typedef enum {
    CTE_GESTURE = 0,
    CTE_INDIVIDUAL,
    CTE_INERCIAL
} control_type_enum;
///////////////////////////


// Режим управления
//Тип управления: Жестовый
typedef enum {
	CMGE_TWO_CHANNEL = 0,
    CMGE_ONE_CHANNEL,
	CMGE_MULTY_CHANNEL,
	CMGE_RESERV
} control_mode_gesture_ch_num_enum; 

typedef enum {
	CMGOCE_ALWAYS_CLOSE = 0,
    CMGOCE_ALWAYS_OPEN,
    CMGOCE_CHANGE_DIRECTION,
    CMGOCE_RESERV
} control_mode_gesture_one_ch_enum;

typedef enum {
	CMGTCE_DIRECT = 0,
    CMGTCE_REVERS,
    CMGTCE_RESERV1,
    CMGTCE_RESERV2
} control_mode_gesture_two_ch_enum;

typedef enum {
	CMGMCE_DIRECT = 0,
    CMGMCE_REVERS,
    CMGMCE_RESERV1,
    CMGMCE_RESERV2
} control_mode_gesture_multy_ch_enum;



//Тип управления: Индивидуальный
typedef enum {
    CMIE_MOVE_MODE = 0,
    CMIE_POS_MODE
} control_mode_individual_enum;

//Тип управления: Инерциальный
typedef enum {
    CMIE_ADD_ANGLE = 0,
    CMIE_HOLD_ANGLE,
    CMIE_BLUETOOTH_IMU_DEVICE
} control_mode_inercial_enum;


typedef struct {
	uint8_t prop_speed: 1;
	uint8_t prop_force: 1;
	uint8_t prop_pos:   1;
	control_mode_gesture_one_ch_enum OneChMode: 3;
	control_mode_gesture_ch_num_enum ChannelNum: 2;
} __attribute__((__packed__)) control_type_gesture_one_ch_struct;

typedef struct {
	uint8_t prop_speed: 1;
	uint8_t prop_force: 1;
	uint8_t prop_pos:   1;
	control_mode_gesture_two_ch_enum TwoChMode: 3;
	control_mode_gesture_ch_num_enum ChannelNum: 2;
} __attribute__((__packed__)) control_type_gesture_two_ch_struct;


typedef struct {
	uint8_t prop_speed: 1;
	uint8_t prop_force: 1;
	uint8_t prop_pos:   1;
	control_mode_gesture_multy_ch_enum MultyChMode: 3;
	control_mode_gesture_ch_num_enum ChannelNum: 2;
} __attribute__((__packed__)) control_type_gesture_multy_ch_struct;


typedef struct {
	uint8_t setting;
} __attribute__((__packed__)) individual_mode_struct;

typedef struct {
	uint8_t setting;   
} __attribute__((__packed__)) inercial_mode_struct;



union control_mode_union {
    control_type_gesture_one_ch_struct   gesture_one_channel_mode;
	control_type_gesture_two_ch_struct   gesture_two_channel_mode;
	control_type_gesture_multy_ch_struct gesture_multy_channel_mode;

    individual_mode_struct individual_mode;
    inercial_mode_struct inercial_mode;
} __attribute__((__packed__));



typedef struct {
	uint8_t ControlType;

	union control_mode_union Control_mode;
	

} __attribute__((__packed__)) control_mode_parameter_struct;



#endif /* DATA_CONTROL_TYPE_PARAMETER_H_ */
