/*
 * h_brige_motor_driver.c
 *
 *  Created on: Aug 16, 2024
 *      Author: Andrey_work
 */

#include "h_brige_motor_driver.h"



#ifdef UBI_SDK_PWM_ENABLED

#include "main.h"


void mot_driver_init(mot_driver_init_struct* init_str)
{
    HAL_GPIO_WritePin(init_str->sleep_GPIO_port, init_str->sleep_pin, GPIO_PIN_SET);
    *init_str->fwd_ccr_pin = PWM_CCR_MIN;
    *init_str->rev_ccr_pin = PWM_CCR_MIN;
}

bool mot_drv_active(mot_driver_init_struct* init_str)
{
    return (init_str->fwd_ccr_pin != PWM_CCR_MIN || init_str->rev_ccr_pin != PWM_CCR_MIN);
}

bool mot_drv_is_move(mot_driver_init_struct* init_str)
{
    uint8_t fwd = *init_str->fwd_ccr_pin;
    uint8_t rev = *init_str->rev_ccr_pin;
    return ((fwd != PWM_CCR_MIN && rev == PWM_CCR_MIN)||
            (fwd == PWM_CCR_MIN && rev != PWM_CCR_MIN));
}

bool mot_drv_is_break(mot_driver_init_struct* init_str)
{
    return (*init_str->fwd_ccr_pin == PWM_CCR_MAX && *init_str->rev_ccr_pin == PWM_CCR_MAX);
}

bool mot_drv_is_idle(mot_driver_init_struct* init_str)
{
    return (init_str->fwd_ccr_pin == PWM_CCR_MIN && init_str->rev_ccr_pin == PWM_CCR_MIN);
}

void mot_drv_idle(mot_driver_init_struct* init_str)
{
    *init_str->fwd_ccr_pin = PWM_CCR_MIN;
    *init_str->rev_ccr_pin = PWM_CCR_MIN;
}

void mot_drv_brake(mot_driver_init_struct* init_str)
{
    *init_str->fwd_ccr_pin = PWM_CCR_MAX;
    *init_str->rev_ccr_pin = PWM_CCR_MAX;
}

void mot_drv_start_fwd(mot_driver_init_struct* init_str, uint8_t speed)
{
    *init_str->fwd_ccr_pin = PWM_CCR_MAX * speed / 100;
    *init_str->rev_ccr_pin = PWM_CCR_MIN;
}

void mot_drv_start_rev(mot_driver_init_struct* init_str, uint8_t speed)
{
    *init_str->fwd_ccr_pin = PWM_CCR_MIN;
    *init_str->rev_ccr_pin = PWM_CCR_MAX * speed / 100;
}

#else



#endif


#ifdef UBI_SDK_FAMALY_NRF52


void mot_driver_init(mot_driver_init_struct* init_str)
{

}

bool mot_drv_active(mot_driver_init_struct* init_str)
{
   return false;
}

bool mot_drv_is_move(mot_driver_init_struct* init_str)
{
    return false;
}

bool mot_drv_is_break(mot_driver_init_struct* init_str)
{
   return false;
}

bool mot_drv_is_idle(mot_driver_init_struct* init_str)
{
    return false;
}

void mot_drv_idle(mot_driver_init_struct* init_str)
{

}

void mot_drv_brake(mot_driver_init_struct* init_str)
{

}

void mot_drv_start_fwd(mot_driver_init_struct* init_str, uint8_t speed)
{

}

void mot_drv_start_rev(mot_driver_init_struct* init_str, uint8_t speed)
{

}

#endif
