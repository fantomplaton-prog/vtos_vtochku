/*
  ******************************************************************************
  * @file    h_bridge_inc_enc.c
  * @author  A.Alborov
  ******************************************************************************
 */


#include "stdlib.h"
#include "string.h"
#include "Controllers/Drive/h_brige_inc_enc.h"

#include "utils/base_soft_timer.h"
#include "device_info.h"
#include "Drivers/Motor/h_brige_motor_driver.h"

#ifdef UBI_MOTOR_DRIVER_ENABLED

#include "hand_driver.h"

extern soft_timer_class_struct* ms_timer_obj;
extern soft_timer_class_struct* us_timer_obj;
extern drive_driver_setting_struct *motor_settings;
extern hand_struct hand;

void timeout_protect_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive);
void brake_timeout_finish_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive);
void startup_timer_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive);
void start_delay_timer_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive);

int32_t FingerCurrent_adc_value_to_current_uA_convert(int32_t adc_value);
int32_t FingerCurrent_ma_to_adc_value_convert(int32_t current_uA);

int32_t calibration_current_buffer[1000]; 

void DriveInit_HBIE(hbie_drive_init_struct* init_str, hbie_drive_struct** drive_obj)
{
	hbie_drive_struct* _drive_str = (hbie_drive_struct*)malloc(sizeof(hbie_drive_struct));

	memset(_drive_str, 0, sizeof(hbie_drive_struct));
	
	if(_drive_str)
	{
		_drive_str->init_str = init_str;
		
		_drive_str->saved_parameters = &(motor_settings[init_str->ch_number]);
		_drive_str->mode = FME_NORMAL;
		
		_drive_str->movment_info.move_mode = 0;
		
		mot_driver_init(&init_str->mot_driver_init);

		_drive_str->startup_timer_instance = create_soft_timer(ms_timer_obj, startup_timer_event,_drive_str, false, false, SOFT_TIMER_EXEC_IRQ_MODE);
		_drive_str->timeout_timer_instance = create_soft_timer(ms_timer_obj, timeout_protect_event, _drive_str, false, false, SOFT_TIMER_EXEC_IRQ_MODE);
		_drive_str->brake_temer_instance = create_soft_timer(ms_timer_obj, brake_timeout_finish_event, _drive_str, false, false, SOFT_TIMER_EXEC_IRQ_MODE);
		_drive_str->start_delay_timer_instance =  create_soft_timer(ms_timer_obj, start_delay_timer_event, _drive_str, false, false, SOFT_TIMER_EXEC_IRQ_MODE);
		*drive_obj = _drive_str;
	}
}

#ifdef UBI_SDK_PWM_ENABLED
void DriveTimerAddStep_HBIE(hbie_drive_struct* drive)
{
    //uint16_t speed_points = 15.3**drive->init_str->mot_driver_init.fwd_ccr_pin - 0.085**drive->init_str->mot_driver_init.fwd_ccr_pin*drive->movment_info.target_speed+152;

    if(drive->mode == FME_NORMAL)
    {
        if (drive->movment_info.current_enc_pos < drive->saved_parameters->calib_steps)
            drive->movment_info.current_enc_pos += *drive->init_str->mot_driver_init.fwd_ccr_pin / 1000;
        if (drive->movment_info.current_enc_pos > 0)
            drive->movment_info.current_enc_pos -= *drive->init_str->mot_driver_init.rev_ccr_pin / 1000;

    } else if (drive->mode == FME_CALIBRATING)
    {
        drive->movment_info.current_enc_pos += *drive->init_str->mot_driver_init.fwd_ccr_pin / 1000;
        drive->movment_info.current_enc_pos -= *drive->init_str->mot_driver_init.rev_ccr_pin / 1000;
    }
}
#endif


int32_t FingerCurrent_adc_value_to_current_uA_convert(int32_t adc_value)
{
	return 0;
}




void DriveEncodersAddStep_HBIE(hbie_drive_struct* drive)
{
	
	//Increase/Decrease encoder position according to direction
	if (drive->movment_info.current_direction == FDE_OPEN) {
		//Decrease
		if(drive->movment_info.current_enc_pos > 0)
			drive->movment_info.current_enc_pos--;
	} else {
		//Increase
		drive->movment_info.current_enc_pos++;
	}	
	
	if(drive->mode == FME_CALIBRATING)
	{
		if(drive->calib_struct->calib_state == FCS_CALIBRATING_CLOSE)
		{
			drive->calib_struct->close_encoder_counter++;
		}else if(drive->calib_struct->calib_state == FCS_CALIBRATING_OPEN)
		{
			drive->calib_struct->open_encoder_counter++;
		}
	}
}

void DriveUpdateCurrentValue_HBIE(hbie_drive_struct* drive, int32_t newValue)
{
	if(drive->movment_info.motor_state == MSE_MOVE)
	{
		if( ms_timer_obj->init_struct->GetTick() - drive->movment_info.start_time > 50)
		{
			int32_t new_uA = FingerCurrent_adc_value_to_current_uA_convert(newValue);
			drive->movment_info.last_current = (drive->movment_info.last_current*9 + new_uA)/10;
			
			if(drive->mode == FME_CALIBRATING && drive->calib_struct->calib_state == FCS_CALIBRATING_CLOSE)
			{
				drive->calib_struct->current_buffer[drive->calib_struct->current_buffer_counter] = drive->movment_info.last_current;
			}

			drive->movment_info.new_current_exist = true;
			
		}
	}
	
}

void startup_timer_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive)
{
    if (drive->movment_info.target_direction == MOT_FWD)
    {
        mot_drv_start_fwd(&drive->init_str->mot_driver_init, drive->movment_info.target_speed);

    } else
    {
        mot_drv_start_rev(&drive->init_str->mot_driver_init, drive->movment_info.target_speed);
    }
    stop_soft_timer(soft_timer_obj, drive->startup_timer_instance);
}

void timeout_protect_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive)
{
	DriveStop_HBIE(drive, MOT_TIMEOUT_STOP);
}

void brake_timeout_finish_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive)
{

	mot_drv_idle(&drive->init_str->mot_driver_init);
	drive->movment_info.motor_state = MSE_IDLE;
	
	if(drive->stop_callback)
							drive->stop_callback(drive, drive->movment_info.last_stop_reason);
}

void start_delay_timer_event(soft_timer_class_struct* soft_timer_obj, int instance, hbie_drive_struct* drive)
{
    stop_soft_timer(ms_timer_obj, drive->start_delay_timer_instance);
    MoveFingerStart(drive);
}

/*
	Function	Convert current in [mA] to ADC value according to multiplier and divisor defined in header
	Arguments	(uint32_t) current_mA - current in mA
	Return 		(uint32_t) converted adc_value
*/
int32_t DriveCurrent_ma_to_adc_value_convert_HBIE(int32_t current_uA)
{
	//Multiply
	uint32_t res = current_uA/ADC_TO_CURRENT_UA_COEF;
	
	//Return converted value
	return res;
}

/*
	Function	Convert current in [mA] to ADC value according to multiplier and divisor defined in header
	Arguments	(uint32_t) current_mA - current in mA
	Return 		(uint32_t) converted adc_value
*/
int32_t DriveCurrent_adc_value_to_current_uA_convert_HBIE(int32_t adc_value)
{
	
	int32_t uA = adc_value *ADC_TO_CURRENT_UA_COEF;    
	
	//Return converted value
	return uA;
}


/*
	Function	Calculate encoder ticks according to position 
	Arguments	(uint8_t) drive 	- drive object
						(uint8_t) pos 						- Position (0..100)
	Return		Encoder position (0..TOTAL_ENCODER_STEPS)
*/
static uint32_t calculate_encoder_pos(hbie_drive_struct* drive, uint8_t pos)
{
	if ( pos > ENCODER_POS_MAX )
		return drive->saved_parameters->calib_steps; //FULL_CLOSE_OPEN_STEPS;
	
	uint32_t result = (pos * drive->saved_parameters->calib_steps) / ENCODER_POS_MAX ;
	
	return result;
}


motor_state_enum GetDriveMotorState_HBIE(hbie_drive_struct* drive)
{
	return drive->movment_info.motor_state;
}

void MoveFingerStart(hbie_drive_struct* drive)
{
    drive->movment_info.current_direction = drive->movment_info.target_direction;
	if(drive->movment_info.target_direction == MOT_FWD)
	{
		DriveMoveForward_HBIE(drive, drive->movment_info.target_speed);
		
	}else if(drive->movment_info.target_direction == MOT_REV)
	{
		DriveMoveRevers_HBIE(drive, drive->movment_info.target_speed);
	}
}

void DriveMoveToPos_HBIE(hbie_drive_struct* drive, uint8_t pos, uint8_t start_delay ,uint8_t power, uint8_t speed)
{
	
	//if(GetFingerMotorState(drive) != MSE_MOVE)
	//{
		//DriveStop_HBIE(drive);
	
		uint32_t finish_enc_pos = calculate_encoder_pos(drive, pos);
		//MOT_DIR dir;
		uint32_t delta = 0;
		if(finish_enc_pos >  drive->movment_info.current_enc_pos)
		{
			
			delta = ((finish_enc_pos - drive->movment_info.current_enc_pos)*ENCODER_POS_MAX*ENCODER_ACCURACY_MULTIPLIER)/ drive->saved_parameters->calib_steps;
			if(delta < (ENCODER_PROTECT_DELTA*ENCODER_ACCURACY_MULTIPLIER)) return;
			
			drive->movment_info.target_enc_pos = finish_enc_pos;
			drive->movment_info.target_speed = speed;
			drive->movment_info.target_max_current = 800000; //drive->saved->current_limit_ma;
			drive->movment_info.target_direction = MOT_FWD;
			drive->movment_info.force = power;
			
			drive->movment_info.move_mode |= FMME_ENCODER_CONTROL;
			
			if(start_delay == 0)
			{
				DriveMoveForward_HBIE(drive, speed);
			} else
            {
                start_soft_timer(ms_timer_obj, drive->start_delay_timer_instance, start_delay*10);
            }
			
			
		}else
		{
			
			delta = ((drive->movment_info.current_enc_pos - finish_enc_pos)*ENCODER_POS_MAX*ENCODER_ACCURACY_MULTIPLIER)/ drive->saved_parameters->calib_steps;
			if(delta < (ENCODER_PROTECT_DELTA*ENCODER_ACCURACY_MULTIPLIER)) return;
			
			drive->movment_info.target_enc_pos = finish_enc_pos;
			drive->movment_info.target_speed = speed;
			drive->movment_info.target_max_current = 800000; //drive->saved->current_limit_ma;
			drive->movment_info.target_direction = MOT_REV;
            drive->movment_info.force = power;
			
			drive->movment_info.move_mode |= FMME_ENCODER_CONTROL;
			
			if(start_delay == 0)
			{
				DriveMoveRevers_HBIE(drive, speed);
			} else
			{
			    start_soft_timer(ms_timer_obj, drive->start_delay_timer_instance, start_delay*10);
			}
			
		}
//	}else
//	{
		
//	}
}

	 
void DriveMoveForward_HBIE(hbie_drive_struct* drive, uint32_t speed)
{
	drive->movment_info.move_mode |= FMME_CURRENT_CONTROL | FMME_TIME_CONTROL;
	
	//drive->movment_info.current_enc_pos
	
	drive->movment_info.target_direction = MOT_FWD;
	drive->movment_info.current_direction = MOT_FWD;
	drive->movment_info.motor_state = MSE_MOVE;
	drive->movment_info.last_current = 0;
	drive->movment_info.overcurrent_counter = 0;
	drive->movment_info.currentArrayCounter = 0;
	drive->movment_info.new_current_exist = false;
	drive->movment_info.start_time = ms_timer_obj->init_struct->GetTick();
	drive->movment_info.target_speed = speed;
	
	mot_drv_start_fwd(&drive->init_str->mot_driver_init, 100);

    start_soft_timer(ms_timer_obj, drive->startup_timer_instance, STARTUP_BURST_LENGHT_MS);
	start_soft_timer(ms_timer_obj, drive->timeout_timer_instance, MAX_MOT_ACTIVE_TIME);
}

void DriveMoveRevers_HBIE(hbie_drive_struct* drive, uint32_t speed)
{
	drive->movment_info.move_mode |= FMME_CURRENT_CONTROL | FMME_TIME_CONTROL;
	
	
	drive->movment_info.target_direction = MOT_REV;
	drive->movment_info.current_direction = MOT_REV;
	drive->movment_info.motor_state = MSE_MOVE;
	drive->movment_info.last_current = 0;
	drive->movment_info.overcurrent_counter = 0;
	drive->movment_info.currentArrayCounter = 0;
	drive->movment_info.new_current_exist = false;
	drive->movment_info.start_time = ms_timer_obj->init_struct->GetTick();
    drive->movment_info.target_speed = speed;

	mot_drv_start_rev(&drive->init_str->mot_driver_init, 100);
	
	start_soft_timer(ms_timer_obj, drive->startup_timer_instance, STARTUP_BURST_LENGHT_MS);
	start_soft_timer(ms_timer_obj, drive->timeout_timer_instance, MAX_MOT_ACTIVE_TIME);
}

void DriveStop_HBIE(hbie_drive_struct* drive, mot_stop_reason_enum reason)
{

	mot_drv_brake(&drive->init_str->mot_driver_init);
	drive->movment_info.move_mode = 0;
	drive->movment_info.target_direction = MOT_STOP;
    drive->movment_info.current_direction = MOT_STOP;
	drive->movment_info.motor_state = MSE_BREAK;
	drive->movment_info.last_stop_reason = reason;
	
	start_soft_timer(ms_timer_obj, drive->brake_temer_instance,20);
	stop_soft_timer(ms_timer_obj, drive->timeout_timer_instance);
	
}

void DriveCurrentHandler(hbie_drive_struct* drive)
{
    if(drive->movment_info.new_current_exist && drive->movment_info.motor_state == MSE_MOVE)
    {
        drive->movment_info.new_current_exist = false;

        if(drive->mode == FME_CALIBRATING)
        {

            if(drive->movment_info.last_current > hand.hand_settings->max_current)
            {
                    drive->movment_info.overcurrent_counter++;

                    if(drive->movment_info.overcurrent_counter > 50)
                    {

                        //mot_drv_idle(drive->init_str->fwd_pin, drive->init_str->rev_pin);
                        DriveStop_HBIE(drive, MOT_OVERCURENT_STOP);
                        drive->movment_info.overcurrent_counter = 0;
                        drive->calib_struct->small_current_counter = 0;
                        drive->movment_info.last_current = 0;
                    }
            }else if(drive->movment_info.last_current > 1000 && drive->movment_info.last_current <= hand.hand_settings->max_current*2){

                drive->movment_info.overcurrent_counter = 0;
                drive->calib_struct->small_current_counter = 0;

            }else if(drive->movment_info.last_current <= hand.hand_settings->max_current / 4)
            {
                drive->calib_struct->small_current_counter++;

                if(drive->calib_struct->small_current_counter > 100)
                {
                    DriveStop_HBIE(drive, MOT_SMALL_CURENT_STOP);
                    drive->movment_info.overcurrent_counter = 0;
                    drive->movment_info.last_current = 0;
                    drive->calib_struct->small_current_counter = 0;
                }
            }

        }else if(drive->mode == FME_NORMAL)
        {

            if(drive->movment_info.last_current > hand.hand_settings->max_current * hand.hand_settings->global_force / 100)
            {
                    drive->movment_info.overcurrent_counter++;

                    if(drive->movment_info.overcurrent_counter > 50)
                    {

                        if(drive->movment_info.current_enc_pos < 0 ||
                          (drive->movment_info.current_direction == MOT_REV &&
                           drive->movment_info.current_enc_pos > 0 &&
                           drive->saved_parameters->calib_steps/drive->movment_info.current_enc_pos > 20)) //current pos in 5% or less
                        {
                            drive->movment_info.current_enc_pos = 0;
                        }
                    //mot_drv_idle(drive->init_str->fwd_pin, drive->init_str->rev_pin);
                        DriveStop_HBIE(drive, MOT_OVERCURENT_STOP);
                        drive->movment_info.overcurrent_counter = 0;
                        drive->movment_info.last_current = 0;

                    }
            }else{

                drive->movment_info.overcurrent_counter = 0;
            }
        }
    }
}

void DriveEncoderHandler(hbie_drive_struct* drive)
{
	if(drive->mode == FME_NORMAL && (drive->movment_info.move_mode & FMME_ENCODER_CONTROL))
	{
		if(drive->movment_info.current_direction == MOT_FWD)
		{
			if(drive->movment_info.target_enc_pos < drive->saved_parameters->calib_steps && //dont stop by position if going to 100%
			   drive->movment_info.current_enc_pos >= drive->movment_info.target_enc_pos)
			{
					DriveStop_HBIE(drive, MOT_ENCODER_TARGET_POS_FINISH);
			}
		}else if(drive->movment_info.current_direction == MOT_REV)
		{
			if(drive->movment_info.target_enc_pos > 0 && //dont stop by position if going to 0%
			   drive->movment_info.current_enc_pos <= drive->movment_info.target_enc_pos)
			{
					DriveStop_HBIE(drive, MOT_ENCODER_TARGET_POS_FINISH);
			}
		}
	}
}

void FingerCalibrationFinish(hbie_drive_struct* drive)
{
	
	void (*temp_drive_calib_finish_callback)(void* drive) = drive->calib_struct->drive_calib_finish_callback;
	
	free(drive->calib_struct);
	
	drive->mode = FME_NORMAL;
	drive->stop_callback = 0;
	
	if(temp_drive_calib_finish_callback)
	    temp_drive_calib_finish_callback(drive);
}

void DriveCalibrationStopEvent(void* _drive, mot_stop_reason_enum reason)
{
	hbie_drive_struct* drive = (hbie_drive_struct*)_drive;

	if(reason == MOT_OVERCURENT_STOP)
	{
		switch(drive->calib_struct->calib_state)
		{
			case FCS_FIRST_OPEN:
			{
				
				drive->calib_struct->calib_state = FCS_CALIBRATING_CLOSE;
				drive->calib_struct->small_current_counter = 0;
				drive->movment_info.current_enc_pos = 0;
				DriveMoveForward_HBIE(drive, 100);
				
			}break;
			
			case FCS_CALIBRATING_CLOSE:
			{
				
				drive->calib_struct->calib_state = FCS_CALIBRATING_OPEN;
				drive->calib_struct->small_current_counter = 0;
				drive->saved_parameters->calib_steps = drive->movment_info.current_enc_pos;
				DriveMoveRevers_HBIE(drive, 100);
				
			}break;
			
			case FCS_CALIBRATING_OPEN:
			{
				//drive->saved->calib_steps = (drive->calib_struct->open_encoder_counter + drive->calib_struct->close_encoder_counter)/2;
				
				FingerCalibrationFinish(drive);
			}break;
		}
	}else if(reason ==  MOT_SMALL_CURENT_STOP)
	{
		FingerCalibrationFinish(drive);
	}
}


void FingerCalibrateStart(hbie_drive_struct* drive, void (*finish_callback)(void* drive))
{
	drive->mode = FME_CALIBRATING;
	drive->stop_callback = DriveCalibrationStopEvent;
	
	drive->calib_struct = (drive_calibrating_struct*)malloc(sizeof(drive_calibrating_struct));
	
	
	//drive->calib_struct->current_buffer = calibration_current_buffer;
	
	drive->calib_struct->drive_calib_finish_callback = finish_callback;
	drive->calib_struct->calib_state = FCS_FIRST_OPEN;
	drive->calib_struct->open_encoder_counter = 0;
	drive->calib_struct->close_encoder_counter = 0;
	drive->calib_struct->small_current_counter = 0;
	
	DriveMoveRevers_HBIE(drive, 100);
	
}

void FingerExec(hbie_drive_struct* drive)
{
    DriveCurrentHandler(drive);
    DriveEncoderHandler(drive);
}


#endif
