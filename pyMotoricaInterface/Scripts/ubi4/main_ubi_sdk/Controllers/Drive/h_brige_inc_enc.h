/*
  ******************************************************************************
  * @file    h_bridge_inc_enc.h
  * @author  A.Alborov
  ******************************************************************************
 */

#ifndef DRIVERS_FINGER_DRIVER_H_
#define DRIVERS_FINGER_DRIVER_H_


#include "stdint.h"
#include "stdbool.h"
#include "UBI_SDK.h"

#include "Drivers/Motor/h_brige_motor_driver.h"

#define STARTUP_BURST_LENGHT_MS                 2
#define MAX_MOT_ACTIVE_TIME 					3000
#define ENCODER_POS_MAX 						100
#define ENCODER_ACCURACY_MULTIPLIER				100
#define ENCODER_PROTECT_DELTA 					2

#define ADC_TO_CURRENT_UA_COEF  				336 // MotCurrent_uA = (adc_value * adc_max_V* 1000000) / (adc_max_val * op_amp_gain * R_cursens)

typedef enum {
	MOT_ENCODER_TARGET_POS_FINISH = 0,
	MOT_SMALL_CURENT_STOP,
	MOT_OVERCURENT_STOP,
	MOT_MANUAL_STOP,
	MOT_TIMEOUT_STOP
}mot_stop_reason_enum;

typedef enum {
	MOT_MANUAL_START = 0,
	MOT_SERVICE_START
}mot_start_reason_enum;

typedef struct
{
	uint8_t exist_motor: 1;
	uint8_t exist_encoder: 1;
}motor_status_struct;

typedef enum {
    FME_NORMAL = 0,
    FME_CALIBRATING,
    FME_ERROR
}drive_mode_enum;

typedef enum {
    FCS_FIRST_OPEN = 0,
    FCS_CALIBRATING_CLOSE,
    FCS_CALIBRATING_OPEN
}drive_calibrating_state_enum;

typedef enum {
	FDE_CLOSE   = MOT_FWD,
	FDE_OPEN    = MOT_REV
}FINGER_DIR_ENUM;

typedef enum {
	MSE_IDLE = 0,
	MSE_BREAK,
	MSE_MOVE
}motor_state_enum;

typedef enum {
	FMME_CURRENT_CONTROL = 	1<<0,
	FMME_ENCODER_CONTROL = 	1<<1,
	FMME_TIME_CONTROL 	 = 	1<<2
}drive_move_mode_enum;

typedef struct
{
    uint32_t calib_steps;
    uint16_t calib_current; //холостой ход
    uint8_t calib_status; // 1

    uint16_t current_limit_ma; //
    uint8_t  current_limit_percent;

    uint16_t overcurrent_counter_max;

    uint32_t  motor_cycles_100ms;
    uint32_t motor_driver_cycles_100ms;

}__attribute__((__packed__))drive_driver_setting_struct;
 
typedef struct
{
	mot_driver_init_struct mot_driver_init;
	uint32_t cur_sensor_pin;
	uint32_t encoder_pin;
	uint8_t  ch_number;

}hbie_drive_init_struct;



typedef struct
{
	motor_state_enum	                    motor_state;
	
	MOT_DIR									target_direction;						// 	Direction (fvd or rev)
	drive_move_mode_enum 	                move_mode;
	uint32_t								target_enc_pos;
	uint32_t								target_max_current;
	uint32_t								target_speed;
	
	MOT_DIR									current_direction;
	int32_t 								current_enc_pos; 		//	0..TOTAL_ENCODER_STEPS
	uint8_t                                 force;
	int32_t 								last_current;
	uint32_t								current_speed;
	uint16_t								overcurrent_counter;
	
	uint32_t  				                start_time;
	
	mot_stop_reason_enum                    last_stop_reason;

	uint16_t                                stall_current;
	uint32_t                                currentArray[20];
	uint32_t                                currentArrayCounter;
	
	bool                                    new_current_exist;
}movment_info_struct;

typedef struct
{
	drive_calibrating_state_enum calib_state;
	void (*drive_calib_finish_callback)(void* drive);
	
	uint16_t small_current_counter;
	uint32_t open_encoder_counter;
	uint32_t close_encoder_counter;
	
	int32_t current_buffer[1000];
	uint16_t current_buffer_counter;
	
}drive_calibrating_struct;

typedef struct
{
	 drive_mode_enum mode;
	 drive_driver_setting_struct* saved_parameters;
	 hbie_drive_init_struct* init_str;
	 movment_info_struct movment_info;
	
	 void (*stop_callback)(void* drive, mot_stop_reason_enum reason);
	
	 drive_calibrating_struct* calib_struct;
		
	
	int startup_timer_instance;
	int timeout_timer_instance;
	int brake_temer_instance;
	int start_delay_timer_instance;


}hbie_drive_struct;


void DriveInit_HBIE(hbie_drive_init_struct* init_str, hbie_drive_struct** drive_obj);
 
motor_state_enum GetDriveState_HBIE(hbie_drive_struct* drive);

void DriveUpdateCurrentValue_HBIE(hbie_drive_struct* drive, int32_t newValue);
void DriveEncodersAddStep_HBIE(hbie_drive_struct* drive);
void DriveTimerAddStep_HBIE(hbie_drive_struct* drive);

void DriveMoveToPos_HBIE(hbie_drive_struct* drive, uint8_t pos, uint8_t start_delay, uint8_t power, uint8_t speed);
void MoveFingerStart(hbie_drive_struct* drive);
void DriveMoveForward_HBIE(hbie_drive_struct* drive, uint32_t speed);
void DriveMoveRevers_HBIE(hbie_drive_struct* drive, uint32_t speed);
void DriveStop_HBIE(hbie_drive_struct* drive, mot_stop_reason_enum reason);

void DriveCalibrateStart_HBIE(hbie_drive_struct* drive, void (*finish_callback)(void* drive));
void DriveCalibrate_HBIE(hbie_drive_struct* drive);
void DriveExec_HBIE(hbie_drive_struct* drive);
void FingerExec(hbie_drive_struct* drive);
void FingerCalibrateStart(hbie_drive_struct *drive,
        void (*finish_callback)(void *drive));

#endif /* DRIVERS_FINGER_DRIVER_H_ */
