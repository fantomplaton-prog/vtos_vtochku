/*
  ******************************************************************************
  * @file    parameter_modifire.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifndef DATA_PARAMETER_MODIFIRE_H_
#define DATA_PARAMETER_MODIFIRE_H_

#include "stdint.h"
#include "stdbool.h"
#include "Data/base_parameter.h"
#include "utils/static_conteiner.h"

typedef enum {
    MODTE_UNKNOW = 0,
    MODTE_COMBINER,
    MODTE_NUM

} modifire_type_enum;

typedef enum {
    MODCE_UNKNOW = 0,
   // MODCE_
    MODCE_NUM

} modifire_code_enum;



typedef struct {

    uint8_t ModifireCode;
    uint8_t ModifireType;

} __attribute__((__packed__)) modifire_info_struct;



typedef struct {

    uint8_t source_parameter_code;
    uint8_t source_parameter_instance;
    uint8_t device_role;
    uint8_t device_code;

} __attribute__((__packed__)) combiner_source_array_struct;


typedef struct {

    uint8_t consumer_parameter_code;
    uint8_t consumer_parameter_instance;
    uint8_t sources_number;

    
    
} modifire_combiner_parameters_filter_struct;


typedef struct {

    base_parameter_struct* consumer_parameter;
    uint8_t source_parameters_size;
    base_parameter_struct* source_parameters;


} modifire_combiner_pointers_struct;


/*
typedef struct {

    uint8_t *parameter_data_p;

    void (*change_notification)(void *param, uint8_t *newVal);

    void *device_owner;

    void **parameters_consumers;

    base_parameter_addinfoseg_struct *add_info_segments;

} base_parameter_pointers_struct;
*/



#endif /* DATA_PARAMETER_MODIFIRE_H_ */
