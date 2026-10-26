import serial
import threading
from collections import deque
import time 
import numpy as np
import logging
from .crc import get_crc
import datetime
import pandas as pd

class MotoricaSerialInterface:
    """
    A class to read serial data from a specified port and parse it into packs.

    Attributes:
        port (str): The port to read from.
        baudrate (int): The baudrate for the serial communication. Default is 115200.
        timeout (float): The timeout for the serial communication. Default is 0.05. In seconds
        debug (bool): A flag to enable debug mode. Default is False.
        crc_table (list): A table for CRC check. Default is None. If None, then CRC is taken from production
    """

    def __init__(self, port, baudrate=115200, timeout=0.05, debug=False, crc_table=None, logging_enabled=False):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_open()

        self.rx_pack = deque(maxlen=1000)  # Updated to limit to 1000 elements
        self.read_thread = threading.Thread(target=self.parse)
        self.running = False
        self.thread_start_time = None  # Added property to store thread start time

        self.STATUS = 'IDLE'
        self.DEBUG = debug
        self.NEWPACK = False
        self.PACK_TIMEOUT_MS = 2  # milliseconds, when to reset the pack status to idle
        self.crc_table = crc_table

        # Set up logging
        self.logging_enabled = logging_enabled
        if self.logging_enabled:
            import os
            log_path = os.path.join(os.path.dirname(__file__), f'log/{time.strftime("%Y-%m-%d_%H-%M-%S")}.log')
            logging.basicConfig(filename=log_path, 
                                level=logging.INFO)

    def start(self):
        """
        Starts the serial reader in a dedicated thread
        """
        self.running = True
        self.thread_start_time = time.time()  # Store the thread start time
        self.read_thread.start()

    def stop(self):
        """
        Stops the serial reader, closes thread
        """
        self.running = False
        self.read_thread.join()

    def serial_close(self):
        """
        Closes serial connection
        """
        self.serial.close()

    def serial_open(self):
        """
        Opens a serial connection with the specified port, baudrate, and timeout.

        Raises:
            SerialException: If the serial connection cannot be established.
        """
        try:
            self.serial = serial.Serial(self.port, baudrate=self.baudrate, timeout=self.timeout)
        except serial.SerialException as e:
            print(f"Failed to open serial connection on port {self.port} with baudrate {self.baudrate}. Error: {e}")
            raise

    def flush_serial(self):
        """
        Flushes the serial buffer.
        """
        iteration = 0
        while self.serial.in_waiting > 50:
            self.serial.read_all()
            iteration += 1
            time.sleep(0.001)
            if self.DEBUG:
                print(f'Flushing {self.port} {iteration}', end=' ' * 20 + '\r')

    def flush_rx_packs(self):
        """
        Clears the received packets buffer.
        """
        self.rx_pack.clear()
        while self.rx_pack:
            self.rx_pack.popleft()

    def parse(self):
        """
        Parses the received data into packs
        """
        RX_IDX = 0
        PACK_START_TS = 0
        RX = np.zeros(16, dtype='uint8') # ensure small packages only (16 bytes), so it would fail otherwise
        
        self.flush_serial()

        logging.info(f'{datetime.datetime.now():%c}:\tStart Serial Reading w/\t{time.time()}')
        print('Start Serial Reading')

        while self.running:

            if self.serial.in_waiting:
                b = self.serial.read(1)
            else:
                time.sleep(0.01)
                continue

            ch = int.from_bytes(b, byteorder='big')

            # execute pack timeout
            if (time.time() - PACK_START_TS) > self.PACK_TIMEOUT_MS / 1000:
                self.STATUS = 'PACK_IDLE'      
                RX *= 0

            # go through state machine
            if self.STATUS == 'PACK_IDLE':
                PACK_START_TS = time.time()
                RX *= 0
                RX_IDX *= 0
                RX[0] = ch
                self.STATUS = 'PACK_DEVICE'

            elif self.STATUS == 'PACK_DEVICE':
                RX[1] = ch
                self.STATUS = 'PACK_REGISTER'

            elif self.STATUS == 'PACK_REGISTER':
                RX[2] = ch
                RX_IDX = 0
                self.STATUS = 'PACK_LEN'

            elif self.STATUS == 'PACK_LEN':
                if RX_IDX>16:
                    msg = 'failed to read pack within 16 bytes'
                    logging.info(f'{datetime.datetime.now():%c}:\t{msg}\t{time.time()}:\t{RX[:1 + 1 + 1 + RX_IDX]}')
                    self.STATUS = 'PACK_IDLE'
                    continue

                RX[1 + 1 + 1 + RX_IDX] = ch
                RX_IDX += 1
                if (RX_IDX > RX[2]):
                    if get_crc(RX, 1 + 1 + 1 + RX[2]) == RX[1 + 1 + 1 + RX_IDX - 1]:
                        self.NEWPACK = True
                        self.rx_pack.append((time.time(), RX[:1 + 1 + 1 + RX[2] + 1].copy()))

                        if self.logging_enabled:
                            logging.info(f'{datetime.datetime.now():%c}:\tsuccessful w/\t{time.time()}:\t{RX[:1 + 1 + 1 + RX_IDX]}')
                    else:
                        if self.logging_enabled:
                            logging.info(f'{datetime.datetime.now():%c}:\tunsuccessful w/\t{time.time()}:\t{RX[:1 + 1 + 1 + RX_IDX]}')
                    self.STATUS = 'PACK_IDLE'

            if self.DEBUG:
                print(f'{ch}\t{time.time():.3f}ts\t{self.STATUS}', end=' ' * 10 + '\r')

    def get_data(self, size=None):
        """
        Gets the received data.

        Parameters:
            size (int): The number of packets to get. If None, gets all packets. Default is None.

        Returns:
            list: A list of received packets. Each element of list is tuple of (received timestamp, pack)
        """
        if size is None:
            size = len(self.rx_pack)  
        data = []
        for _ in range(size):
            if self.rx_pack:
                data.append(self.rx_pack.popleft())
            else:
                break
        return data
    

class IndySerialInterface(MotoricaSerialInterface):
    """
    A class to manage specific serial operations for Indy devices using the MotoricaSerialInterface as a base.
    """
    
    def __init__(self, port, baudrate=115200, timeout=0.05, debug=False, crc_table=None, logging_enabled=False):
        """
        Initializes the IndySerialInterface with a given serial port configuration.

        Parameters:
            port (str): The serial port to use.
            baudrate (int): The baud rate for the serial communication. Default is 115200.
            timeout (float): The timeout for the serial communication. Default is 0.05.
            debug (bool): Whether to enable debug mode. Default is False.
            crc_table (list): The CRC table to use. Default is None.
            logging_enabled (bool): Whether to enable logging. Default is False.
        """
        super().__init__(port, baudrate, timeout, debug, crc_table, logging_enabled)

    def prot_send(self, pack, repeats=3):
        """
        Sends a command to the motor.

        Parameters:
            pack (bytearray): The command to send.
            repeats (int): The number of times to repeat the command. Default is 3.
        """
        print(pack)
        for _ in range(repeats):
            self.serial.write(bytearray(pack))
            time.sleep(0.01)
            
    def prot_stop(self):
        """
        Stops the motor by sending the appropriate command.
        """
        pack = np.array([0x01, 0x13, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
        time.sleep(0.05)
        
    def prot_close(self):
        """
        Closes the motor by sending the appropriate command.
        """
        pack = np.array([0x01, 0x10, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
        
    def prot_open(self):
        """
        Opens the motor by sending the appropriate command.
        """
        pack = np.array([0x01, 0x0F, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)


    def prot_close_duty(self, percent: np.uint8):
        """
        Closes the motor with a specified duty cycle.

        Parameters:
            percent (np.uint8): The duty cycle percentage to close the motor. 
                                Value should be between 0 and 100.

        Comment:
        Андрей Оводов, [23 Sep 2024 at 10:55:29]:
        Привет, я залез в код по поводу отправки PWM в инди, как мы обсуждали. А он уже умеет это
        typedef enum
        {
        GDC_StatisticsCommand  = SC_Last,
        GDC_DriveOpen      = 0x0F,
        GDC_DriveClose      = 0x10,
        GDC_DriveOpenDuty    = 0x11,
        GDC_DriveCloseDuty    = 0x12,
        GDC_DriveBreak      = 0x13,
        GDC_DriveDisable,
        GDC_SetSensorsPair    = 0x20,
        GDC_OpticalSensor_1    = 0x30,
        GDC_OpticalSensor_2,
        GDC_SerialDataFlow    = 0x40,
        GDC_GetVersion      = 0x80,
        GDC_SetAutocalibrationState = 0x82,
        GDC_SetGestureField    = 0x83
        } GripDriverCommands;

        Вот есть Drive open duty и close duty
        отправлять от 0 до 100 первым параметром
        """
        if percent > 100:
            percent = 100
        if percent < 0:
            percent = 0

        pack = np.array([0x01, 0x12, 0x01, percent, 0], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
        
    def prot_open_duty(self, percent: np.uint8):
        """
        Opens the motor with a specified duty cycle.

        Parameters:
            percent (np.uint8): The duty cycle percentage to open the motor. 
                                Value should be between 0 and 100.

        Comment:
        Андрей Оводов, [23 Sep 2024 at 10:55:29]:
        Привет, я залез в код по поводу отправки PWM в инди, как мы обсуждали. А он уже умеет это
        typedef enum
        {
        GDC_StatisticsCommand  = SC_Last,
        GDC_DriveOpen      = 0x0F,
        GDC_DriveClose      = 0x10,
        GDC_DriveOpenDuty    = 0x11,
        GDC_DriveCloseDuty    = 0x12,
        GDC_DriveBreak      = 0x13,
        GDC_DriveDisable,
        GDC_SetSensorsPair    = 0x20,
        GDC_OpticalSensor_1    = 0x30,
        GDC_OpticalSensor_2,
        GDC_SerialDataFlow    = 0x40,
        GDC_GetVersion      = 0x80,
        GDC_SetAutocalibrationState = 0x82,
        GDC_SetGestureField    = 0x83
        } GripDriverCommands;

        Вот есть Drive open duty и close duty
        отправлять от 0 до 100 первым параметром
        """
        if percent > 100:
            percent = 100
        if percent < 0:
            percent = 0
            
        pack = np.array([0x01, 0x11, 0x01, percent, 0], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
    
class FestSerialInterface(MotoricaSerialInterface):
    """
    A subclass of MotoricaSerialInterface specifically tailored for FEST.
    """

    def __init__(self, port, baudrate=115200, timeout=0.05, debug=False, crc_table=None, logging_enabled=False):
        super().__init__(port, baudrate, timeout, debug, crc_table, logging_enabled)
        
        self.df_gestures = pd.DataFrame([], columns = ['thumb', 'index', 'middle', 'ring', 'pinky', 'thumb_rot'])
        self.df_gestures.loc['grab'] = [100, 100, 100, 100, 100, 100]
        self.df_gestures.loc['open'] = [0, 0, 0, 0, 0, 0]
        self.df_gestures.loc['neutral'] = [25, 25, 25, 25, 25, 25]
        self.df_gestures.loc['indication'] = [0, 0, 100, 100, 100, 0]
        self.df_gestures.loc['pinch'] = [50, 50, 0, 0, 0, 100]
        self.gesture_names = list(self.df_gestures.index.values)

        pass

    def prot_send(self, pack, repeats=3):
        """
        Sends a command to the motor.

        Parameters:
            pack (bytearray): The command to send.
            repeats (int): The number of times to repeat the command. Default is 3.
        """
        print(pack)
        for _ in range(repeats):
            self.serial.write(bytearray(pack))
            time.sleep(0.01)
            
    def prot_stop(self):
        """
        Stops the motor by sending the appropriate command.
        """
        pack = np.array([0x01, 0x13, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
        time.sleep(0.05)
        
    def prot_close(self):
        """
        Closes the motor by sending the appropriate command.
        """
        self.prot_stop()
        pack = np.array([0x01, 0x10, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)
        
    def prot_open(self):
        """
        Opens the motor by sending the appropriate command.
        """
        self.prot_stop()
        pack = np.array([0x01, 0x0F, 0x00, 0x00], dtype='uint8')
        pack[-1] = get_crc(pack, len(pack) - 1)
        self.prot_send(pack)

    def prot_abs_state(self, thumb = 0, index = 0, middle = 0, ring = 0, pinky = 0, thumb_rot = 0):
        """
        Sets the absolute state of fingers
        0 — fully open
        100 - fully closed

        Parameters:
            thumb (int): The position of the thumb. Default is 0.
            index (int): The position of the index finger. Default is 0.
            middle (int): The position of the middle finger. Default is 0.
            ring (int): The position of the ring finger. Default is 0.
            pinky (int): The position of the pinky finger. Default is 0.
            thumb_rot (int): The rotation of the thumb. Default is 0.
        """
        pack = np.array([0x01, 0xBB, 6, index, middle, ring, pinky, thumb, thumb_rot, 0], dtype='uint8')
        pack[3:] = pack[3:].clip(0, 100)
        pack[-1] = get_crc(pack, len(pack)-1)
        self.prot_send(pack)

    def prot_rel_move(self, thumb:int = 0, index:int = 0, middle:int = 0, ring:int = 0, pinky:int = 0, thumb_rot:int = 0):
        """
        Moves the fingers and thumb relatively to their current positions.
        The movement is specified as a value between -100 and 100, where:
        -100 represents the maximum movement in the open direction,
        100 represents the maximum movement in the closed direction,
        and 0 represents no movement.

        Parameters:
            thumb (int): The relative movement of the thumb. Default is 0.
            index (int): The relative movement of the index finger. Default is 0.
            middle (int): The relative movement of the middle finger. Default is 0.
            ring (int): The relative movement of the ring finger. Default is 0.
            pinky (int): The relative movement of the pinky finger. Default is 0.
            thumb_rot (int): The relative rotation of the thumb. Default is 0.
        """
        thumb_cmd = (127 + np.clip(thumb/100, -1, 1)*127).astype('uint8')
        index_cmd = (127 + np.clip(index/100, -1, 1)*127).astype('uint8')
        middle_cmd = (127 + np.clip(middle/100, -1, 1)*127).astype('uint8')
        ring_cmd = (127 + np.clip(ring/100, -1, 1)*127).astype('uint8')
        pinky_cmd = (127 + np.clip(pinky/100, -1, 1)*127).astype('uint8')
        thumb_rot_cmd = (127 + np.clip(thumb_rot/100, -1, 1)*127).astype('uint8')

        pack = [0x01, 0xAA, 6, index_cmd, middle_cmd, ring_cmd, pinky_cmd, thumb_cmd, thumb_rot_cmd, 0]
        pack[-1] = get_crc(pack, len(pack)-1)
        self.prot_send(pack)

    def prot_exec_gesture(self, gesture):
        assert gesture in self.gesture_names, f'{gesture} not in gestures: {self.gesture_names}'
        self.prot_abs_state(**(dict(self.df_gestures.loc[gesture])))


class SerialInterface_2EMG(MotoricaSerialInterface):
    
    """
    A subclass of MotoricaSerialInterface specifically tailored for 2EMG data acquisition.

    typedef enum {
        SC_Gain = 0x01,
        SC_StreamMode,
        SC_SelectData,
        SC_GetResult,
        SC_Address,
        SC_Version                  = 0x80,
        SC_EnableDFU				= 0x81,
        SC_Protocol                 = 0x82,
    } SensorCommands;

    Attributes:
        Inherits all attributes from MotoricaSerialInterface.
    """

    def __init__(self, port, baudrate=115200, timeout=0.05, debug=False, crc_table=None, logging_enabled=False):
        super().__init__(port, baudrate, timeout, debug, crc_table, logging_enabled)
        self.ADDRESS = 0x01
        self.SensorCommandsRegisters = dict(
            SC_Gain = 0x01,
            SC_StreamMode = 0x02,
            SC_SelectData = 0x03,
            SC_GetResult = 0x04,
            SC_Address = 0x05,
            SC_Version = 0x80,
            SC_EnableDFU = 0x81,
            SC_Protocol = 0x82
        )

        self.SensorCommandsNames = self.SensorCommandsRegisters.keys()

        self.gain_table=[0.001000, 0.006151, 0.024606, 0.055363, 0.098424, 0.153787, 0.221453, 0.301423, 0.393695,
            0.498270, 0.615148, 0.744329, 0.885813, 1.039600, 1.205690, 1.384083, 1.574779, 1.777778,
            1.993080, 2.220685, 2.460592, 2.712803, 2.977316, 3.254133, 3.543253, 3.844675, 4.158401,
            4.484429, 4.822761, 5.173395, 5.536333, 5.911573, 6.299116, 6.698962, 7.111112, 7.535563,
            7.972319, 8.421377, 8.882738, 9.356401, 9.842369, 10.340638, 10.851212, 11.374087, 11.909266,
            12.456748, 13.016532, 13.588620, 14.173011, 14.769704, 15.378700, 16.000000, 16.633604, 17.279509,
            17.937717, 18.608227, 19.291044, 19.986158, 20.693581, 21.413301, 22.145330, 22.889658, 23.646292,
            24.415224, 25.196465, 25.990004, 26.795849, 27.613995, 28.444447, 29.287197, 30.142254, 31.009611,
            31.889275, 32.781239, 33.685509, 34.602074, 35.530952, 36.472126, 37.425606, 38.391388, 39.369476,
            40.359859, 41.362553, 42.377544, 43.404846, 44.444443, 45.496349, 46.560551, 47.637062, 48.725872,
            49.826992, 50.940403, 52.066128, 53.204151, 54.354481, 55.517105, 56.692043, 57.879272, 59.078815,
            60.290653, 61.514801, 62.751247, 64.000000, 65.261047, 66.534416, 67.820076, 69.118034, 70.428291,
            71.750870, 73.085739, 74.432907, 75.792381, 77.164177, 78.548256, 79.944633, 81.353317, 82.774323,
            84.207619, 85.653206, 87.111107, 88.581322, 90.063828, 91.558632, 93.065735, 94.585167, 96.116882,
            97.660896, 99.217216, 100.785858, 102.366783, 103.960014, 105.565544, 107.183395, 108.813538, 110.455978,
            112.110718, 113.777786, 115.457130, 117.148788, 118.852737, 120.569016, 122.297577, 124.038445, 125.791611,
            127.557098, 129.334869, 131.124954, 132.927322, 134.742035, 136.569016, 138.408295, 140.259888, 142.123810,
            144.000000, 145.888504, 147.789322, 149.702423, 151.627838, 153.565552, 155.515579, 157.477905, 159.452515,
            161.439438, 163.438690, 165.450211, 167.474045, 169.510178, 171.558640, 173.619385, 175.692429, 177.777771,
            179.875443, 181.985397, 184.107651, 186.242203, 188.389084, 190.548248, 192.719727, 194.903488, 197.099594,
            199.307968, 201.528641, 203.761612, 206.006927, 208.264511, 210.534409, 212.816605, 215.111115, 217.417923,
            219.737015, 222.068420, 224.412155, 226.768173, 229.136475, 231.517090, 233.910049, 236.315262, 238.732788,
            241.162613, 243.604782, 246.059204, 248.525940, 251.004990, 253.496353, 256.000000, 258.515991, 261.044189,
            263.584778, 266.137665, 268.702789, 271.280304, 273.870026, 276.472137, 279.086517, 281.713165, 284.352173,
            287.003479, 289.667053, 292.342957, 295.031128, 297.731628, 300.444458, 303.169525, 305.906952, 308.656708,
            311.418671, 314.193024, 316.979584, 319.778534, 322.589783, 325.413269, 328.249146, 331.097290, 333.957703,
            336.830475, 339.715454, 342.612823, 345.522522, 348.444427, 351.378693, 354.325287, 357.284119, 360.255310,
            363.238708, 366.234528, 369.242615, 372.262939, 375.295654, 378.340668, 381.397919, 384.467529, 387.549377,
            390.643585, 393.750122, 396.868866, 400.000000]
        
    def ReadGain(self, channel:np.uint8, timeout:float=1.0) -> np.float32:
        """
        Reads the gain for the specified channel.

        Parameters:
            channel (np.uint8): The channel to read from. Valid values are 0x01 or 0x02.
            timeout (float): The timeout for the serial communication, in seconds. Default is 1.0.

        Returns:
            np.float32: The gain for the specified channel, or None if the read operation times out.
        """

        assert (channel in [0x01, 0x02]), 'channel either 0x01 or 0x02'

        pack = bytearray([self.ADDRESS | (1 << 7), self.SensorCommandsRegisters['SC_Gain'], 0x02, 0x00, channel, 0x00])
        pack[-1] = get_crc(pack, len(pack)-1)
        # print(f'Sending {pack}')
        self.serial.write(pack)

        time_start = time.time()
        while (time.time() - time_start < timeout):
            if len(self.rx_pack) > 0:
                temp_pack = self.rx_pack.popleft()[1]
                if (temp_pack[1] == self.SensorCommandsRegisters['SC_Gain']):
                    # print('temp_pack', temp_pack)
                    assert(len(temp_pack) == 8), 'pack length for gain should be 8 bytes'
                    return np.frombuffer(temp_pack[3:7], dtype=np.float32)[0]
            else:
                time.sleep(0.01)
            
        return None
    

    def WriteGain(self, channel:np.uint8, value: np.uint8):
        pack = bytearray([self.ADDRESS, self.SensorCommandsRegisters['SC_Gain'], 0x02, value, channel, 0x00])
        pack[-1] = get_crc(pack, len(pack)-1)
        self.serial.write(pack)
        pass
    

    def ReadSelectResultType(self, channel:np.uint8, timeout:float=1.0):

        '''
        Reads the selected result type for the specified channel.
        
        Parameters:
            channel (np.uint8): The channel to read from. Either 0x01 or 0x02.
            timeout (float): The timeout for the serial communication. Default is 1.0. In seconds
        
        Returns:
            np.uint8: The selected result type for the specified channel if successful, None otherwise.

        Currently interpreted as:
        	 typedef enum {
                SAMPLE_E = 0,
                LOWPASS_E,
                FILTRED_E,
                ENV_E,
                SCALE_ENV_E,
                DIF_ENV_E,
                AVR_E,
                ENV_MIKE, // просто добавь еще один ENV_SUB_AVR
                ENV_SUB_AVR_E,
                SOURCE, // activations
                CH_DELTA_E,
                AVR_CH_DELTA_E,
                ENV_DELTA_KALMAN_E,
                NUM_RESULT
	        }result_type_enum;
        '''

        assert (channel in [0x01,0x02]), 'channel either 0x01 or 0x02'

        pack = bytearray([self.ADDRESS | (1 << 7), self.SensorCommandsRegisters['SC_SelectData'], 0x01, channel, 0x00])
        pack[-1] = get_crc(pack, len(pack)-1)
        # print(f'Sending {pack}')
        self.serial.write(pack)

        time_start = time.time()
        while (time.time() - time_start < timeout):
            if len(self.rx_pack) > 0:
                pack = self.rx_pack.popleft()[1]
                if pack[1] == self.SensorCommandsRegisters['SC_SelectData']:
                    return pack[3]
                
            else:
                time.sleep(0.01)
            
        return None
    
    def ReadVersion(self, timeout:float=1.0):
        '''
        Reads the firmware version from the device.

        Parameters:
            timeout (float): The timeout for the serial communication in seconds. Default is 1.0.

        Returns:
            int: The firmware version number if successful, None otherwise.
        '''
        pack = bytearray([self.ADDRESS | (1 << 7), self.SensorCommandsRegisters['SC_Version'], 0x00, 0x00])
        pack[-1] = get_crc(pack, len(pack)-1)
        # print(f'Sending {pack}')
        self.serial.write(pack)

        time_start = time.time()
        while (time.time() - time_start < timeout):
            if len(self.rx_pack) > 0:
                pack = self.rx_pack.popleft()[1]
                if pack[1] == self.SensorCommandsRegisters['SC_Version']:
                    return pack[3]
            else:
                time.sleep(0.01)
            
        return None
    

    def WriteProtocolFlag(self, flag:np.uint8):
        pack = bytearray([self.ADDRESS, self.SensorCommandsRegisters['SC_Protocol'], 0x01, flag, 0x00])
        pack[-1] = get_crc(pack, len(pack)-1)
        self.serial.write(pack)
        pass
    
    


# Example usage
if __name__ == "__main__":
    port = "/dev/ttyUSB0"  # Change this to your serial port
    baudrate = 115200
    reader = MotoricaSerialInterface(port, baudrate)
    reader.start()
