/*
  ******************************************************************************
  * @file    h_brige_motor_driver.h
  * @author  A.Alborov
  ******************************************************************************
*/



#ifndef INC_H_BRIGE_MOTOR_DRIVER_H_
#define INC_H_BRIGE_MOTOR_DRIVER_H_





#ifdef __cplusplus
 extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "UBI_SDK.h"





#ifdef UBI_SDK_FAMILY_STM32
#include "HAL_DRIVERS.h"

#ifdef UBI_SDK_PWM_ENABLED

typedef struct
{
     volatile uint32_t* fwd_ccr_pin;
     uint32_t fwd_timer_period;

     volatile uint32_t* rev_ccr_pin;
     uint32_t rev_timer_period;

     GPIO_TypeDef *sleep_GPIO_port;
     uint32_t sleep_pin;

 }mot_driver_init_struct;

#else

typedef struct
{
     GPIO_TypeDef  *fwd_GPIO_port;
     uint32_t fwd_pin;

     GPIO_TypeDef  *rev_GPIO_port;
     uint32_t rev_pin;

}mot_driver_init_struct;


#endif

#endif

#ifdef UBI_SDK_FAMALY_NRF52


typedef struct
 {
 	uint32_t fwd_pin;
 	uint32_t rev_pin;

 }mot_driver_init_struct;

#endif


typedef enum {
    MOT_STOP = 0,
    MOT_FWD,
    MOT_REV
}MOT_DIR;



void mot_driver_init(mot_driver_init_struct* init_str);

bool mot_drv_active(mot_driver_init_struct* init_str);

bool mot_drv_is_move(mot_driver_init_struct* init_str);
bool mot_drv_is_break(mot_driver_init_struct* init_str);
bool mot_drv_is_idle(mot_driver_init_struct* init_str);

void mot_drv_idle(mot_driver_init_struct* init_str);
void mot_drv_brake(mot_driver_init_struct* init_str);

void mot_drv_start_fwd(mot_driver_init_struct* init_str, uint8_t speed);
void mot_drv_start_rev(mot_driver_init_struct* init_str, uint8_t speed);





#ifdef __cplusplus
}
#endif

#endif /* INC_H_BRIGE_MOTOR_DRIVER_H_ */

