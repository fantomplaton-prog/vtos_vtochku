/*
 * slave_device.c
 *
 *  Created on: 15.07.24
 *      Author: PC
 */


#include "Devices/slave_device.h"

#include "string.h"

#include "UBI_SDK.h"

#ifdef UBI_SDK_SYSTEM_ROLE_MASTER
#include "request_scheduler.h"
#endif


