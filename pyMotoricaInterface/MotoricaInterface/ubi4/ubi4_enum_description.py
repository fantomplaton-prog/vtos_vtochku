from enum import Enum
from dataclasses import dataclass
from typing import List, Optional, Callable

########################################################
## FRESULT FatFs enum
########################################################

class FRESULT(Enum):
    """
    https://elm-chan.org/fsw/ff/doc/rc.html
    
    Original Path to enums:
    FatFs -> ff.h
    

    typedef enum {
        FR_OK = 0,				/* (0) Succeeded */
        FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
        FR_INT_ERR,				/* (2) Assertion failed */
        FR_NOT_READY,			/* (3) The physical drive cannot work */
        FR_NO_FILE,				/* (4) Could not find the file */
        FR_NO_PATH,				/* (5) Could not find the path */
        FR_INVALID_NAME,		/* (6) The path name format is invalid */
        FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
        FR_EXIST,				/* (8) Access denied due to prohibited access */
        FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
        FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
        FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
        FR_NOT_ENABLED,			/* (12) The volume has no work area */
        FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
        FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
        FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
        FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
        FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
        FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_LOCK */
        FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
    } FRESULT;
    """
    
    FR_OK = 0
    FR_DISK_ERR = 1
    FR_INT_ERR = 2
    FR_NOT_READY = 3
    FR_NO_FILE = 4
    FR_NO_PATH = 5
    FR_INVALID_NAME = 6
    FR_DENIED = 7
    FR_EXIST = 8
    FR_INVALID_OBJECT = 9
    FR_WRITE_PROTECTED = 10
    FR_INVALID_DRIVE = 11
    FR_NOT_ENABLED = 12
    FR_NO_FILESYSTEM = 13
    FR_MKFS_ABORTED = 14
    FR_TIMEOUT = 15
    FR_LOCKED = 16
    FR_NOT_ENOUGH_CORE = 17
    FR_TOO_MANY_OPEN_FILES = 18
    FR_INVALID_PARAMETER = 19
    





########################################################
## main_ubi_sdk/Data/hand_parameter_description.h
########################################################

class parameter_data_code_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Data/hand_parameter_description.h

    /**
    * @brief Enumeration of parameter data codes.
    */
    typedef enum {

        PDCE_SIMPLE_COMMAND = 0, /**< 0 Simple command */

        PDCE_ACTION_REQUEST = 30, /**< 30 Action request */

        /** Global settings */
        PDCE_SELECT_GESTURE = 1, /**< 1 Select gesture (uint8_t) */
        PDCE_SELECT_PROFILE = 2, /**< 2 Select profile (uint8_t) */
        PDCE_GLOBAL_FORCE = 3, /**< 3 Global force (uint8_t) */
        PDCE_GLOBAL_SENSITIVITY = 4, /**< 4 Global sensitivity (uint8_t) */
        PDCE_GLOBAL_THRESHOLD = 5, /**< 5 Global threshold (uint8_t) */

        PDCE_UNIVERSAL_CONTROL_INPUT = 6, /**< 6 Universal control input */
        PDCE_OPEN_CLOSE_SIGNAL = 7, /**< 7 Open/close signal (uint8_t open, uint8_t close) */

        PDCE_EMG_CH_1_3_VAL = 8, /**< 8 EMG channel 1-3 value */
        PDCE_EMG_CH_4_6_VAL = 9, /**< 9 EMG channel 4-6 value */
        PDCE_EMG_CH_7_9_VAL = 10, /**< 10 EMG channel 7-9 value */

        PDCE_EMG_CH_1_3_GAIN = 11, /**< 11 EMG channel 1-3 gain */
        PDCE_EMG_CH_4_6_GAIN = 12, /**< 12 EMG channel 4-6 gain */
        PDCE_EMG_CH_7_9_GAIN = 13, /**< 13 EMG channel 7-9 gain */

        /** Drive control set group */
        PDCE_MOVE_DRIVE_PERCENT = 14, /**< 14 Drive movement percentage (int8_t[DRIVE_NUM]) */
        PDCE_TARGET_DRIVE_POSITION_PERCENT = 15, /**< 15 Target drive position percentage (int8_t[DRIVE_NUM]) */
        PDCE_TARGET_DRIVE_SPEED_PERCENT = 16, /**< 16 Target drive speed percentage */
        PDCE_TARGET_DRIVE_FORCE_PERCENT = 17, /**< 17 Target drive force percentage */

        /** Drive control get group */
        PDCE_CURRENT_DRIVE_POSITION = 18, /**< 18 Current drive position */
        PDCE_CURRENT_DRIVE_CURRENT_UINT8 = 19, /**< 19 Current drive current (uint8_t) */
        PDCE_CURRENT_DRIVE_CURRENT_UINT16 = 20, /**< 20 Current drive current (uint16_t) */
        PDCE_CURRENT_DRIVE_FORCE_UINT8 = 21, /**< 21 Current drive force (uint8_t) */
        PDCE_CURRENT_DRIVE_FORCE_UINT16 = 22, /**< 22 Current drive force (uint16_t) */

        PDCE_GESTURES_CHANGE_SETTINGS = 23, /**< 23 Gesture change settings */
        PDCE_CONTROL_MODE_SETTINGS = 24, /**< 24 Control mode settings */
        PDCE_DRIVE_SETTINGS = 25, /**< 25 Drive settings */
        PDCE_OPEN_CLOSE_THRESHOLD = 26, /**< 26 Open/close threshold parameters */

        PDCE_CALIB_STATUS = 27, /**< 27 Calibration status */
        PDCE_CURRENT_LIMITS = 28, /**< 28 Current limits */
        PDCE_BMS_STATUS_COMBINED_PARAM = 29, /**< 29 Battery Management System combined status parameter */

        PDCE_GESTURE_SETTINGS = 31, /**< 31 Gesture settings */
        PDCE_GESTURE_GROUP = 32, /**< 32 Gesture group */
        PDCE_OPTIC_LEARNING_DATA = 33, /**< 33 Optic learning data */
        PDCE_EMG_ENV_E_VAL = 34, /**< 34 EMG environment E value */
        PDCE_DMS_OUTPUT = 35, /**< 35 DMS output */
        PDCE_DATE_AND_TIME = 36, /**< 36 Date and time */
        PDCE_INTERFECE_ERROR_COUNTER = 37, /**< 37 Interface error counter */
        PDCE_CALIBRATION_CURRENT_PERCENT = 38, /**< 38 Calibration current percentage */

        /** Toggles */
        PDCE_ENERGY_SAVE_MODE = 39, /**< 39 Energy save mode */
        PDCE_LEFT_RIGHT_HAND_MODE = 40, /**< 40 Left/right hand mode */
        PDCE_SWITCH_SENSORS = 41, /**< 41 Switch sensors */

        PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER = 42, /**< 42 Optic checkpoint data receiver */
        PDCE_OPTIC_BINDING_DATA = 43, /**< 43 Optic binding data */
        PDCE_OPTIC_ENABLE = 44, /**< 44 Enable optic feature */

        PDCE_OPTIC_SELECT_GESTURE_TIMEOUT = 45, /**< 45 Optic select gesture timeout */
        PDCE_DISABLE_MOTORS_FLAG = 46, /**< 46 Disable motors flag */
        PDCE_OPTIC_SELECT_GESTURE_STATE = 47, /**< OPTIC_SELECT_GESTURE_STATE */
        PDCE_VIBRO_MOTOR_ACTION = 48,
        PDCE_VIBRO_MOTOR_CPNTROL = 49,

        /** Generic parameters */
        PDCE_GENERIC_9 = 246, /**< 246 Generic parameter 9 */
        PDCE_GENERIC_8 = 247, /**< 247 Generic parameter 8 */
        PDCE_GENERIC_7 = 248, /**< 248 Generic parameter 7 */
        PDCE_GENERIC_6 = 249, /**< 249 Generic parameter 6 */
        PDCE_GENERIC_5 = 250, /**< 250 Generic parameter 5 */
        PDCE_GENERIC_4 = 251, /**< 251 Generic parameter 4 */
        PDCE_GENERIC_3 = 252, /**< 252 Generic parameter 3 */
        PDCE_GENERIC_2 = 253, /**< 253 Generic parameter 2 */
        PDCE_GENERIC_1 = 254, /**< 254 Generic parameter 1 */
        PDCE_NO_CODE   = 255  /**< 255 Generic parameter 0 */

    } parameter_data_code_enum;
    """    

    PDCE_SIMPLE_COMMAND = 0
    PDCE_SELECT_GESTURE = 1
    PDCE_SELECT_PROFILE = 2
    PDCE_GLOBAL_FORCE = 3
    PDCE_GLOBAL_SENSITIVITY = 4
    PDCE_GLOBAL_THRESHOLD = 5
    PDCE_UNIVERSAL_CONTROL_INPUT = 6
    PDCE_OPEN_CLOSE_SIGNAL = 7
    PDCE_EMG_CH_1_3_VAL = 8
    PDCE_EMG_CH_4_6_VAL = 9
    PDCE_EMG_CH_7_9_VAL = 10
    PDCE_EMG_CH_1_3_GAIN = 11
    PDCE_EMG_CH_4_6_GAIN = 12
    PDCE_EMG_CH_7_9_GAIN = 13
    PDCE_MOVE_DRIVE_PERCENT = 14
    PDCE_TARGET_DRIVE_POSITION_PERCENT = 15
    PDCE_TARGET_DRIVE_SPEED_PERCENT = 16
    PDCE_TARGET_DRIVE_FORCE_PERCENT = 17
    PDCE_CURRENT_DRIVE_POSITION = 18
    PDCE_CURRENT_DRIVE_CURRENT_UINT8 = 19
    PDCE_CURRENT_DRIVE_CURRENT_UINT16 = 20
    PDCE_CURRENT_DRIVE_FORCE_UINT8 = 21
    PDCE_CURRENT_DRIVE_FORCE_UINT16 = 22
    PDCE_GESTURES_CHANGE_SETTINGS = 23
    PDCE_CONTROL_MODE_SETTINGS = 24
    PDCE_DRIVE_SETTINGS = 25
    PDCE_OPEN_CLOSE_THRESHOLD = 26
    PDCE_CALIB_STATUS = 27
    PDCE_CURRENT_LIMITS = 28
    PDCE_BMS_STATUS_COMBINED_PARAM = 29
    PDCE_ACTION_REQUEST = 30
    PDCE_GESTURE_SETTINGS = 31
    PDCE_GESTURE_GROUP = 32
    PDCE_OPTIC_LEARNING_DATA = 33
    PDCE_EMG_ENV_E_VAL = 34
    PDCE_DMS_OUTPUT = 35
    PDCE_DATE_AND_TIME = 36
    PDCE_INTERFECE_ERROR_COUNTER = 37
    PDCE_CALIBRATION_CURRENT_PERCENT = 38
    PDCE_ENERGY_SAVE_MODE = 39
    PDCE_LEFT_RIGHT_HAND_MODE = 40
    PDCE_SWITCH_SENSORS = 41
    PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER = 42
    PDCE_OPTIC_BINDING_DATA = 43
    PDCE_OPTIC_ENABLE = 44
    PDCE_OPTIC_SELECT_GESTURE_TIMEOUT = 45
    PDCE_DISABLE_MOTORS_FLAG = 46
    PDCE_OPTIC_SELECT_GESTURE_STATE = 47
    PDCE_VIBRO_MOTOR_ACTION = 48
    PDCE_VIBRO_MOTOR_CPNTROL = 49
    PDCE_GENERIC_9 = 246
    PDCE_GENERIC_8 = 247
    PDCE_GENERIC_7 = 248
    PDCE_GENERIC_6 = 249
    PDCE_GENERIC_5 = 250
    PDCE_GENERIC_4 = 251
    PDCE_GENERIC_3 = 252
    PDCE_GENERIC_2 = 253
    PDCE_GENERIC_1 = 254
    PDCE_NO_CODE = 255
    

class request_action_type_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Data/hand_parameter_description.h

    typedef enum {
        RATE_NO_CHANGE,
        RATE_INCREASE,
        RATE_DECREASE,
        RATE_IN_DE_CREASE,
        RATE_SET_VALUE,
        RATE_REQUEST_VALUE,
        RATE_ENABLE_TRANSFER
    }request_action_type_enum;
    """
    
    RATE_NO_CHANGE = 0
    RATE_INCREASE = 1
    RATE_DECREASE = 2
    RATE_IN_DE_CREASE = 3
    RATE_SET_VALUE = 4
    RATE_REQUEST_VALUE = 5
    RATE_ENABLE_TRANSFER = 6

GESTURE_GROUP_SIZE = 8
OPTIC_BINDING_DATA_GROUP_SIZE = 13

@dataclass
class bms_status_combined_parameter_struct:
    bat_level: int  # uint8_t
    charge_status: int  # uint8_t
    charge_current: int  # uint16_t

@dataclass
class open_close_signal_param_struct:
    open_signal: int  # uint8_t
    close_signal: int  # uint8_t

@dataclass
class rtc_date_time_param_struct:
    year: int  # uint16_t
    month: int  # uint8_t
    date: int  # uint8_t
    week_day: int  # uint8_t
    hours: int  # uint8_t
    minutes: int  # uint8_t
    seconds: int  # uint8_t

@dataclass
class action_request_param_struct:
    data_code: int  # uint8_t
    data_instance: int  # uint8_t
    action_priority: int  # uint8_t
    action_type: int  # uint8_t
    action_args: List[int]  # uint8_t[2]

@dataclass
class open_close_threshold_param_struct:
    open_upper: int  # uint8_t
    open_lower: int  # uint8_t
    close_upper: int  # uint8_t
    close_lower: int  # uint8_t

@dataclass
class editable_gesture_param_struct:
    gesture_id: int  # uint8_t
    drive_num: int  # uint8_t
    OpenPosition: List[int]  # uint8_t[6]
    ClosePosition: List[int]  # uint8_t[6]
    OpenToCloseTimeShift: List[int]  # uint8_t[6]
    CloseToOpenTimeShift: List[int]  # uint8_t[6]

@dataclass
class gesture_group_param_item_struct:
    gesture_id: int  # uint8_t
    gesture_image_code: int  # uint8_t

@dataclass
class optic_binding_data_item_struct:
    dms_state: int  # uint8_t
    gesture_id: int  # uint8_t

@dataclass
class gesture_group_param_struct:
    gestures_description: List[gesture_group_param_item_struct]  # gesture_group_param_item_struct[GESTURE_GROUP_SIZE]

@dataclass
class optic_binding_data_param_struct:
    sate_bind: List[optic_binding_data_item_struct]  # optic_binding_data_item_struct[OPTIC_BINDING_DATA_GROUP_SIZE]



########################################################
## main_ubi_sdk/Devices/enum_discription.h
########################################################

# Constants from C header
CHECK_BOARD_NAME_MASK = 0x01
CHECK_BOARD_VERSION_MASK = 0x02
CHECK_BOARD_SUB_VERSION_MASK = 0x04
CHECK_BOARD_REVISION_MASK = 0x08
CHECK_BOARD_SUB_REVISION_MASK = 0x10
CHECK_BOARD_BUILD_MASK = 0x20
CHECK_BOARD_TYPE_MASK = 0x40
CHECK_BOARD_CODE_MASK = 0x80

PTD_BOOTLOADER = 1
PTD_MAIN_PROGRAM = 2

class device_roles_enum(Enum):

    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

    typedef enum {
        DRE_UNKNOWN = 0,
        DRE_MAIN_CONTROLLER,
        DRE_DISPLAY,
        DRE_TOUCH_DISPLAY,
        DRE_BUTTON,
        DRE_CONTROL_PANEL,
        DRE_BMS,

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
        DRE_ROTATION_THUMB_FINGER_DRIVER

    }device_roles_enum;
    """

    DRE_UNKNOWN = 0
    DRE_MAIN_CONTROLLER = 1
    DRE_DISPLAY = 2
    DRE_TOUCH_DISPLAY = 3
    DRE_BUTTON = 4
    DRE_CONTROL_PANEL = 5
    DRE_BMS = 6
    DRE_HAND_OPEN_SENSOR = 7
    DRE_HAND_CLOSE_SENSOR = 8
    DRE_HAND_OPEN_CLOSE_SENSOR = 9
    DRE_HAND_ARRAY_SENSOR = 10
    DRE_WRIST_ARRAY_SENSOR = 11
    DRE_WRIST_ROTATION_OPEN_SENSOR = 12
    DRE_WRIST_ROTATION_CLOSE_SENSOR = 13
    DRE_WRIST_ROTATION_OPEN_CLOSE_SENSOR = 14
    DRE_WRIST_FLEXION_OPEN_SENSOR = 15
    DRE_WRIST_FLEXION_CLOSE_SENSOR = 16
    DRE_WRIST_FLEXION_OPEN_CLOSE_SENSOR = 17
    DRE_WRIST_OPPOSITION_OPEN_SENSOR = 18
    DRE_WRIST_OPPOSITION_CLOSE_SENSOR = 19
    DRE_WRIST_OPPOSITION_OPEN_CLOSE_SENSOR = 20
    DRE_ELBOW_ARRAY_SENSOR = 21
    DRE_ELBOW_FLEXION_OPEN_SENSOR = 22
    DRE_ELBOW_FLEXION_CLOSE_SENSOR = 23
    DRE_ELBOW_FLEXION_OPEN_CLOSE_SENSOR = 24
    DRE_ELBOW_ROTATION_OPEN_SENSOR = 25
    DRE_ELBOW_ROTATION_CLOSE_SENSOR = 26
    DRE_ELBOW_ROTATION_OPEN_CLOSE_SENSOR = 27
    DRE_GESTURE_CONTROL_SENSOR = 28
    DRE_HAND_DRIVER = 29
    DRE_WRIST_DRIVER = 30
    DRE_WRIST_ROTATION_DRIVER = 31
    DRE_WRIST_FLEXION_DRIVER = 32
    DRE_WRIST_OPOSITION_DRIVER = 33
    DRE_WRIST_ROTATION_FLEXION_DRIVER = 34
    DRE_ELBOW_DRIVER = 35
    DRE_ELBOW_FLEXION_DRIVER = 36
    DRE_ELBOW_OPOSITION_DRIVER = 37
    DRE_INDEX_FINGER_DRIVER = 38
    DRE_MIDDLE_FINGER_DRIVER = 39
    DRE_RING_FINGER_DRIVER = 40
    DRE_PINKY_FINGER_DRIVER = 41
    DRE_THUMB_FINGER_DRIVER = 42
    DRE_ROTATION_FINGER_DRIVER = 43
    DRE_ROTATION_THUMB_FINGER_DRIVER = 44
    
    
class device_codes_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

    typedef enum {
        DCE_CPU_MODULE = 1,
        DCE_FEST_H_AND_F,
        DCE_INDY,
        DCE_EMG_SENSE,
        DCE_BMS,
        DCE_GUI,
        DCE_OMG_MODULE,
        DCE_FINGERS_DC_DRIVER,
        DCE_BLDC_FINGER_DRIVER,
        DCE_DC_FINGER_DRIVER

        }device_codes_enum;
    """
    DCE_CPU_MODULE = 1
    DCE_FEST_H_AND_F = 2
    DCE_INDY = 3
    DCE_EMG_SENSE = 4
    DCE_BMS = 5
    DCE_GUI = 6
    DCE_OMG_MODULE = 7
    DCE_FINGERS_DC_DRIVER = 8
    DCE_BLDC_FINGER_DRIVER = 9
    DCE_DC_FINGER_DRIVER = 10
    

class device_types_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

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
    """

    DTE_UNKNOW = 0x00
    DTE_CPU = 0x01
    DTE_SUB_CPU = 0x02
    DTE_DRIVER = 0x10
    DTE_MOTOR_DRIVER = 0x11
    DTE_MOTOR_DRIVER_MULTY_CH = 0x12
    DTE_SENSOR = 0x20
    DTE_EMG_1CH = 0x21
    DTE_EMG_2CH = 0x22
    DTE_EMG_MULTY_CH = 0x23
    DTE_OMG_SEGMENT = 0x24
    DTE_OMG_MULTY_CH = 0x25
    DTE_OMG_EMG_SEGMENT = 0x26
    DTE_OMG_EMG_MULTY_CH = 0x27
    DTE_STIMULATOR = 0x30
    DTE_HMI = 0x40
    DTE_DISPLAY = 0x41
    DTE_BUTTONS = 0x42
    DTE_SOUNDS = 0x43
    DTE_DISPLAY_BUTTONS = 0x44
    DTE_DISPLAY_SOUNDS = 0x45
    DTE_BUTTONS_SOUNDS = 0x46
    DTE_DISPLAY_BUTTONS_SOUNDS = 0x47
    DTE_TOUCH_DISPLAY = 0x48
    DTE_TOUCH_DISPLAY_SOUNDS = 0x49
    DTE_POWER = 0x50
    DTE_BMS = 0x51
    DTE_ACB = 0x52
    DTE_POWER_CONVERTER = 0x53
    DTE_POWER_ISOLATOR = 0x54
    DTE_POWER_COMBO = 0x55
    DTE_COMBO = 0x60
    DTE_OTHER = 0x70
    
    
class board_types_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

    typedef enum {
        BTE_MULTI_MODULE_PERIPHERAL_BOARD_TYPE = 1,
        BTE_MULTI_MODULE_CPU_BOARD_TYPE,
        BTE_CPU_BOARD_TYPE,
        BTE_BMS_BOARD_TYPE,
        BTE_DRIVER_BOARD_TYPE,
        BTE_SENSORE_BOARD_TYPE,
        BTE_UI_BOARD_TYPE

    }board_types_enum;
    """
    BTE_MULTI_MODULE_PERIPHERAL_BOARD_TYPE = 1
    BTE_MULTI_MODULE_CPU_BOARD_TYPE = 2
    BTE_CPU_BOARD_TYPE = 3
    BTE_BMS_BOARD_TYPE = 4
    BTE_DRIVER_BOARD_TYPE = 5
    BTE_SENSORE_BOARD_TYPE = 6
    BTE_UI_BOARD_TYPE = 7
    
    
class board_codes_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

    typedef enum {
        BCE_CPU_MODULE = 1,
        BCE_FEST_X_DRIVER,
        BCE_SINGLE_GRIP_DRIVER,
        BCE_EMG_SENSE_32BIT,
        BCE_CHARGER_2S_USB_ROUND,
        BCE_GUI_MODULE,
        BCE_MULTIGRIP_F,
        BCE_OMG_MODULE
    }board_codes_enum;
    """
    BCE_CPU_MODULE = 1
    BCE_FEST_X_DRIVER = 2
    BCE_SINGLE_GRIP_DRIVER = 3
    BCE_EMG_SENSE_32BIT = 4
    BCE_CHARGER_2S_USB_ROUND = 5
    BCE_GUI_MODULE = 6
    BCE_MULTIGRIP_F = 7
    BCE_OMG_MODULE = 8
    
    
class Program_type_code_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

        typedef enum {
                PTE_UNKNOW = 0,
                PTE_BOOTLOADER,
                PTE_MAIN_PROGRAM
            }Program_type_code_enum;
    """
    PTE_UNKNOW = 0
    PTE_BOOTLOADER = 1
    PTE_MAIN_PROGRAM = 2


class bootloader_code_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h

        typedef enum {
                BCE_UNKNOW = 0,
                BCE_BASE_BOOTLOADER,
                BCE_TEST_STM32F4_BOOTLOADER,
                BCE_CPU_BOOTLOADER,
                BCE_BMS_BOOTLOADER,
                BCE_EMG_2CH_BOOTLOADER,
                BCE_GUI_BOOTLOADER,
                BCE_FINGERS_DRIVER_BOOTLOADER,
                BCE_OMG_BOOTLOADER
            }bootloader_code_enum;
    """
    BCE_UNKNOW = 0
    BCE_BASE_BOOTLOADER = 1
    BCE_TEST_STM32F4_BOOTLOADER = 2
    BCE_CPU_BOOTLOADER = 3
    BCE_BMS_BOOTLOADER = 4
    BCE_EMG_2CH_BOOTLOADER = 5
    BCE_GUI_BOOTLOADER = 6
    BCE_FINGERS_DRIVER_BOOTLOADER = 7
    BCE_OMG_BOOTLOADER = 8

class bootloader_additional_info_type_enum(Enum):
    """	
    Original Path to enums:
    main_ubi_sdk/Devices/enum_discription.h
    typedef enum {
                BAIT_EMPTY = 0,
            }bootloader_additional_info_type_enum;"""

    BAIT_EMPTY = 0
    
    



########################################################
## main_ubi_sdk/TransportLevel/main_receiver.h
########################################################
# Constants
COLLECT_HEADER_SIZE = 100

class Command_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Device Access */
    typedef enum {
        DEVICE_INFORMATION           = 1, /**< 1 Запрос базовой информации девайса */
        DATA_MANAGER                 = 2, /**< 2 Работа с Data Table */
        WRITE_FW_COMMAND             = 3, /**< 3 Запись прошивки */
        DEVICE_ACCESS_COMMAND        = 4, /**< 4 Доступ к устройству */
        ECHO_COMMAND                 = 5, /**< 5 Эхо-команда для проверки связи */
        SUB_DEVICE_MANAGER           = 6, /**< 6 Управление подустройствами */
        GET_DEVICE_STATUS            = 7, /**< 7 Запрос статуса устройства */
        DATA_TRANSFER_SETTINGS       = 8, /**< 8 Настройки передачи данных */
        COMPLEX_PARAMETER_TRANSFER   = 9, /**< 9 Передача сложных параметров */
        POWER_CONTROL                = 10, /**< 10 Управление питанием */
        PROTOCOL_PING                = 11  /**< 11 Проверка соединения по протоколу */
    } Command_enum;
    """
    DEVICE_INFORMATION = 1  # Запрос базовой информации девайса
    DATA_MANAGER = 2  # Работа с Data Table
    WRITE_FW_COMMAND = 3  # Запись прошивки
    DEVICE_ACCESS_COMMAND = 4  # Доступ к устройству
    ECHO_COMMAND = 5  # Эхо-команда для проверки связи
    SUB_DEVICE_MANAGER = 6  # Управление подустройствами
    GET_DEVICE_STATUS = 7  # Запрос статуса устройства
    DATA_TRANSFER_SETTINGS = 8  # Настройки передачи данных
    COMPLEX_PARAMETER_TRANSFER = 9  # Передача сложных параметров
    POWER_CONTROL = 10  # Управление питанием
    PROTOCOL_PING = 11  # Проверка соединения по протоколу

class DevicesManagerCommand_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Sub Device Manager */
    typedef enum {
        GET_ALL_SUB_DEVICE            = 1, /**< 1 Получить список всех подустройств */
        ADD_SUB_DEVICE                = 2, /**< 2 Добавить подустройство */
        REMOVE_SUB_DEVICE             = 3  /**< 3 Удалить подустройство */
    } DevicesManagerCommand_enum;
    """
    GET_ALL_SUB_DEVICE = 1  # Получить список всех подустройств
    ADD_SUB_DEVICE = 2  # Добавить подустройство
    REMOVE_SUB_DEVICE = 3  # Удалить подустройство

class DataManagerCommand_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Data Manager */
    typedef enum {
        READ_AVAILABLE_SLOTS          = 1, /**< 1 Прочитать info слотов */
        WRITE_SLOT                    = 2, /**< 2 NOT IMPLEMENTED */
        READ_DATA                     = 3, /**< 3 Прочитать данные из слота по DataCode */
        WRITE_DATA                    = 4, /**< 4 Записать данные в слот по DataCode */
        RESET_TO_FACTORY              = 5, /**< 5 Сброс к заводским настройкам */
        SAVE_DATA                     = 6, /**< 6 Сохранить данные в память */
        READ_SLOT                     = 7, /**< 7 Прочитать info по DataCode */
        WRITE_DATA_PART               = 8,
        READ_DATA_PART                = 9
    } DataManagerCommand_enum;
    """
    READ_AVAILABLE_SLOTS = 1  # Прочитать info слотов
    WRITE_SLOT = 2  # NOT IMPLEMENTED
    READ_DATA = 3  # Прочитать данные из слота по DataCode
    WRITE_DATA = 4  # Записать данные в слот по DataCode
    RESET_TO_FACTORY = 5  # Сброс к заводским настройкам
    SAVE_DATA = 6  # Сохранить данные в память
    READ_SLOT = 7  # Прочитать info по DataCode
    WRITE_DATA_PART = 8
    READ_DATA_PART = 9

class DeviceInformationCommand_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Device Information */
    typedef enum {
        INICIALIZE_INFORMATION                       = 1,  /**< 1 Инициализация информации */
        READ_DEVICE_PARAMETERS                       = 2,  /**< 2 Чтение параметров устройства */
        READ_DEVICE_ADDITIONAL_PARAMETER             = 3,  /**< 3 Чтение дополнительных параметров устройства */
        READ_DEVICE_ADDITIONAL_PARAMETER_SIZE        = 16, /**< 16 Размер дополнительных параметров устройства */
        READ_SUB_DEVICES_FIRST_INFO                  = 4,  /**< 4 Чтение первой информации о подустройствах */
        READ_SUB_DEVICE_INFO                         = 5,  /**< 5 Чтение информации о подустройстве */
        READ_SUB_DEVICE_PARAMETERS                   = 6,  /**< 6 Чтение параметров подустройства */
        READ_SUB_DEVICE_ADDITIONAL_PARAMETER         = 7,  /**< 7 Чтение дополнительных параметров подустройства */
        READ_SUB_DEVICE_ADDITIONAL_PARAMETER_SIZE    = 17, /**< 17 Размер дополнительных параметров подустройства */
        SUB_DEVICE_PARAMETER_INIT_READ               = 8,  /**< 8 Инициализация чтения параметров подустройства */
        SUB_DEVICE_PARAMETER_INIT_WRITE              = 9,  /**< 9 Инициализация записи параметров подустройства */
        GET_SERIAL_NUMBER                            = 10, /**< 10 Получить серийный номер */
        SET_SERIAL_NUMBER                            = 11, /**< 11 Установить серийный номер */
        GET_DEVICE_NAME                              = 12, /**< 12 Получить имя устройства */
        SET_DEVICE_NAME                              = 13, /**< 13 Установить имя устройства */
        GET_DEVICE_ROLE                              = 14, /**< 14 Получить роль устройства */
        SET_DEVICE_ROLE                              = 15  /**< 15 Установить роль устройства */
    } DeviceInformationCommand_enum;
    """
    INICIALIZE_INFORMATION = 1  # Инициализация информации
    READ_DEVICE_PARAMETERS = 2  # Чтение параметров устройства
    READ_DEVICE_ADDITIONAL_PARAMETER = 3  # Чтение дополнительных параметров устройства
    READ_DEVICE_ADDITIONAL_PARAMETER_SIZE = 16  # Размер дополнительных параметров устройства
    READ_SUB_DEVICES_FIRST_INFO = 4  # Чтение первой информации о подустройствах
    READ_SUB_DEVICE_INFO = 5  # Чтение информации о подустройстве
    READ_SUB_DEVICE_PARAMETERS = 6  # Чтение параметров подустройства
    READ_SUB_DEVICE_ADDITIONAL_PARAMETER = 7  # Чтение дополнительных параметров подустройства
    READ_SUB_DEVICE_ADDITIONAL_PARAMETER_SIZE = 17  # Размер дополнительных параметров подустройства
    SUB_DEVICE_PARAMETER_INIT_READ = 8  # Инициализация чтения параметров подустройства
    SUB_DEVICE_PARAMETER_INIT_WRITE = 9  # Инициализация записи параметров подустройства
    GET_SERIAL_NUMBER = 10  # Получить серийный номер
    SET_SERIAL_NUMBER = 11  # Установить серийный номер
    GET_DEVICE_NAME = 12  # Получить имя устройства
    SET_DEVICE_NAME = 13  # Установить имя устройства
    GET_DEVICE_ROLE = 14  # Получить роль устройства
    SET_DEVICE_ROLE = 15  # Установить роль устройства

class DataTransferSetting_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Data Transfer Settings */
    typedef enum {
        DTSE_START_PARAMETER_STREAM      = 1, /**< 1 Запуск потока параметров */
        DTSE_STOP_PARAMETER_STREAM       = 2, /**< 2 Остановка потока параметров */
        DTSE_PROTOCOL_PREAMBLE           = 3, /**< 3 Преамбула протокола */
        DTSE_ADD_PARAMETER_STREAM_FILTER = 4,  /**< 4 Добавление фильтра потока параметров */
        DTSE_ADD_PARAMETER_IN_EXTERNAL_STREAM = 5,
    } DataTransferSetting_enum;
    """
    DTSE_START_PARAMETER_STREAM = 1  # Запуск потока параметров
    DTSE_STOP_PARAMETER_STREAM = 2  # Остановка потока параметров
    DTSE_PROTOCOL_PREAMBLE = 3  # Преамбула протокола
    DTSE_ADD_PARAMETER_STREAM_FILTER = 4  # Добавление фильтра потока параметров
    DTSE_ADD_PARAMETER_IN_EXTERNAL_STREAM = 5

class DataTransferSettingsCommand_enum(Enum):
    """
    Original Path to enums:
    main_ubi_sdk/TransportLevel/main_receiver.h

    /** @brief Список команд Data Transfer Settings Command */
    typedef enum {
        SET_BRIGE_MODE = 1 /**< 1 Установить режим моста */
    } DataTransferSettingsCommand_enum;
    """
    SET_BRIGE_MODE = 1  # Установить режим моста

@dataclass
class command_selector_struct:
    """
    Original Path to struct:
    main_ubi_sdk/TransportLevel/main_receiver.h

    typedef struct {
        Command_enum command;
        uint8_t* data_buf;
        uint16_t data_size;
        ports_enum port;
        uint16_t header_id;
    } command_selector_struct;
    """
    command: Command_enum
    data_buf: bytes  # uint8_t*
    data_size: int  # uint16_t
    port: int  # ports_enum
    header_id: int  # uint16_t

@dataclass
class collect_header_struct:
    """
    Original Path to struct:
    main_ubi_sdk/TransportLevel/main_receiver.h

    typedef struct {
        uint8_t res:6;
        uint8_t request_answer:1;
        uint8_t brige:1;
        uint8_t transmitter_address;
        uint8_t package_id;
        uint8_t header_id;
        uint8_t command;
        ports_enum port;
    } __attribute__((__packed__)) collect_header_struct;
    """
    res: int  # uint8_t:6
    request_answer: bool  # uint8_t:1
    brige: bool  # uint8_t:1
    transmitter_address: int  # uint8_t
    package_id: int  # uint8_t
    header_id: int  # uint8_t
    command: int  # uint8_t
    port: int  # ports_enum

# Type definitions for callbacks
custom_receiver_callback = Callable[[bytes, int, int], bool]  # (uint8_t* Buf, uint16_t size, ports_enum port) -> bool
custom_command_selector_callback = Callable[[command_selector_struct], bool]  # (command_selector_struct command) -> bool
device_custom_receiver_callback = Callable[[object, object], None]  # (ubi4_bus_protocol_obj_struct* protocol_str, protocol_receive_struct receive_str) -> void
