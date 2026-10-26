from . import ubi4_p2p_high_level_port
from . import ubi4_enum_description
import time
import struct
from collections import OrderedDict
from dataclasses import dataclass
from typing import Optional
import serial
from collections import deque
import threading
from ..crc import get_crc
import numpy as np
from . import ubi4_p2p_high_level_port_header
from . import ubi4_p2p_high_level_port
from . import ubi4_p2p_high_level_port_pack

from tqdm.autonotebook import tqdm
import pandas as pd
import os

@dataclass
class UBI4Device:
    DeviceType: int
    DeviceCode: int
    DeviceRole: int
    DeviceVersion: int
    DeviceSubVersion: int
    DeviceAddress: int
    ParametersNum: int
    SubDeviceNum: int
    DefaultPort: int
    parsed_parameters: Optional[list[ubi4_p2p_high_level_port.BaseParameterInfoStruct]] = None
    __stack__ = None

    def __repr__(self):
        repr_string =  f"""DeviceType={self.DeviceType} : {ubi4_enum_description.device_types_enum(self.DeviceType).name}
DeviceCode={self.DeviceCode} : {ubi4_enum_description.device_codes_enum(self.DeviceCode).name}
DeviceRole={self.DeviceRole} : {ubi4_enum_description.device_roles_enum(self.DeviceRole).name}
DeviceVersion={self.DeviceVersion}
DeviceSubVersion={self.DeviceSubVersion}
DeviceAddress={self.DeviceAddress}
ParametersNum={self.ParametersNum}
SubDeviceNum={self.SubDeviceNum}
DefaultPort={self.DefaultPort}"""
        
        if self.parsed_parameters is not None:
            repr_string += '\nParameters:\n'
            for parameter in self.parsed_parameters:
                if parameter.DataCode in ubi4_enum_description.parameter_data_code_enum._value2member_map_:
                    repr_string += f'\t({ubi4_enum_description.parameter_data_code_enum(parameter.DataCode).name}:{parameter.parameter_size} bytes)\n'
                else:
                    repr_string += f'\t({parameter.DataCode}:{parameter.parameter_size} bytes)\n'        
        return repr_string
    
    def request_parameters(self):
        pack_payload = bytearray([ubi4_enum_description.DeviceInformationCommand_enum['READ_SUB_DEVICE_PARAMETERS'].value, 
                          self.DeviceAddress, 0x00, self.ParametersNum])


        pack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                               bridge = 0, 
                                                                               waiting_answer = 0, 
                                                                               request_type = 0,
                                                                               package_code=ubi4_enum_description.Command_enum["DEVICE_INFORMATION"].value, 
                                                                               package_id=0, 
                                                                               package_size=len(pack_payload), 
                                                                               crc=get_crc(pack_payload, len(pack_payload)), 
                                                                               address=0)

        pack = bytearray(pack_header) + bytearray(pack_payload)

        print(list(map(lambda x: f'0x{x:02x}', pack)))
        """['0x00', '0x01', '0x00', '0x04', '0x00', '0xbe', '0x00', '0x06', '0x0a', '0x00', '0x06']"""

        self.__stack__.ser.write(pack)
        time.sleep(self.__stack__.UBI4_STATIC_TIMOUT_MS/1000)
        pack_info = self.__stack__.ser.read(self.__stack__.ser.in_waiting)

        pack_header, pack_payload = ubi4_p2p_high_level_port.parse_p2p_high_level_port_pack(pack_info)
        # crc = get_crc(pack_payload, len(pack_payload))
        # if crc != pack_header['crc']:
        #     print(f'CRC mismatch: calculated crc {crc} != pack header crc {pack_header["crc"]}')


        self.parsed_parameters = ubi4_p2p_high_level_port.parse_base_parameter_info_struct(pack_payload[4:])

        return self.parsed_parameters

class Ubi4Stack:
    full_init_struct: Optional[ubi4_p2p_high_level_port.FullInitializeConnectionStruct] = None
    parsed_parameters: Optional[list[ubi4_p2p_high_level_port.BaseParameterInfoStruct]] = None
    sub_devices_array: Optional[list[UBI4Device]] = None
    __run_parsing_thread__: bool = False
    UBI4_STATIC_TIMOUT_MS = 100
    
    def __init__(self, ser_port: str, PACK_TIMEOUT_MS: int = 1):

        self.PACK_TIMEOUT_MS = PACK_TIMEOUT_MS
        self.pack_history = deque(maxlen=1000)
        self.__ubi4_state_history__ = deque(maxlen=1000)
        self.__pack_parsing_thread__ = None
        self.n_pack = 0

        ser = serial.Serial(
            port=ser_port,
            baudrate=115200,
            timeout=self.PACK_TIMEOUT_MS/1000
        )

        self.ser = ser
        self.stop_streaming()
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        self.flush_serial()
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        self.flush_serial()

    def __repr__(self):
        ans = f'UBI4Stack\nser={self.ser.port}'

        if self.full_init_struct is not None:
            ans += f'\n{self.full_init_struct}'
        
        if self.sub_devices_array is not None:
            ans += f'\n{len(self.sub_devices_array)} subdevices found'

        if self.parsed_parameters is not None:
            ans += '\nParameters:\n'
            for parameter in self.parsed_parameters:
                if parameter.DataCode in ubi4_enum_description.parameter_data_code_enum._value2member_map_:
                    ans += f'\t({ubi4_enum_description.parameter_data_code_enum(parameter.DataCode).name}:{parameter.parameter_size} bytes)\n'
                else:
                    ans += f'\t({parameter.DataCode}:{parameter.parameter_size} bytes)\n'  
        
        return ans
    
    def initialize_stack(self):
        self.request_first_info_blocked()
        self.request_device_parameters()
        self.request_sub_device_info()
        self.__device_addresses_map__ = {0x00: self}

        for device in self.sub_devices_array:
            device.request_parameters()
            self.__device_addresses_map__[device.DeviceAddress] = device
    
    def start_streaming(self):
        print("--------------------------------")
        print("Starting streaming")
        
        outpack_payload = bytearray([ubi4_enum_description.DataTransferSetting_enum.DTSE_START_PARAMETER_STREAM.value])

        outpack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                            bridge = 0, 
                                                                            waiting_answer = 1, 
                                                                            request_type = 0,
                                                                            package_code=ubi4_enum_description.Command_enum["DATA_TRANSFER_SETTINGS"].value, 
                                                                            package_id=0, 
                                                                            package_size=len(outpack_payload), 
                                                                            crc=get_crc(outpack_payload, len(outpack_payload)), 
                                                                            address=0)

        outpack = bytearray(outpack_header) + bytearray(outpack_payload)

        print('Sending:', list(map(lambda x: f'0x{x:02x}', outpack)))
        print('Should be:', '[0x20, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01]')

        self.ser.write(outpack)
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)

        print("Streaming started")
        print("********************************")
        return self.ser.read(self.ser.in_waiting)
    
    def stop_streaming(self):
        print("--------------------------------")
        print("Stopping streaming")
        outpack_payload = bytearray([ubi4_enum_description.DataTransferSetting_enum.DTSE_STOP_PARAMETER_STREAM.value])


        outpack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                            bridge = 0, 
                                                                            waiting_answer = 1, 
                                                                            request_type = 0,
                                                                            package_code=ubi4_enum_description.Command_enum["DATA_TRANSFER_SETTINGS"].value, 
                                                                            package_id=0, 
                                                                            package_size=len(outpack_payload), 
                                                                            crc=get_crc(outpack_payload, len(outpack_payload)), 
                                                                            address=0)

        outpack = bytearray(outpack_header) + bytearray(outpack_payload)

        print("Sending:", list(map(lambda x: f'0x{x:02x}', outpack)))
        print("Should be:", '[0x20, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02]')

        self.ser.write(outpack)
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        
        print("Streaming stopped")
        
        print("********************************")
        return self.ser.read(self.ser.in_waiting)
    
    def flush_serial(self):
        while self.ser.in_waiting > 0:
            self.ser.read(self.ser.in_waiting)
            time.sleep(0.001)
        print('Serial flushed')
        return True

    def start_parsing_thread(self):

        assert (self.__run_parsing_thread__ is False), f"Parsing thread is already running, you should stop it first by calling: `stop_parsing_thread()`"

        print("--------------------------------")
        print('Starting parsing thread')

        def parsing_thread():
            self.n_pack = 0

            self.START_PARSING_TIME = time.time()

            pack_parsing_struct = dict(
                IN_PACK = b'',
                IN_PACK_LEN = 0,
                IN_PACK_BYTES_TO_READ = -1,
                MAX_PACK_SIZE = 999
            )


            addresses_devices_map = {subdevice.DeviceAddress: subdevice 
                                    for subdevice in self.sub_devices_array}

            addresses_names_map = {a: ubi4_enum_description.device_codes_enum(d.DeviceCode).name 
                                    for a, d in addresses_devices_map.items()}

            addresses_devices_map[0x00] = self
            addresses_names_map[0x00] = ubi4_enum_description.device_codes_enum(1).name

            def add_pack_to_history(p_struct):

                def parse_state(p_struct):
                    try:
                        pack_header, pack_payload = ubi4_p2p_high_level_port.parse_p2p_high_level_port_pack(p_struct['IN_PACK'])
                        # crc = get_crc(pack_payload, len(pack_payload))
                        # if crc != pack_header['crc']:
                        #     print(f'CRC mismatch: calculated crc {crc} != pack header crc {pack_header["crc"]}')

                    except ubi4_p2p_high_level_port_pack.P2P_PACK_SIZE_MISMATCH_ERROR:
                        print(f'Pack size mismatch: {len(p_struct["IN_PACK"])}')
                        return
                    except Exception as e:
                        print(f'Error: {e}')
                        return
                
                    if ubi4_enum_description.Command_enum(pack_header['package_code']).name == 'COMPLEX_PARAMETER_TRANSFER':
                        ubi4_state = {}

                        while len(pack_payload)>0:
                            device_address = pack_payload[0]
                            if device_address not in addresses_devices_map:
                                print(f'Device address {device_address} not found')
                                pack_payload = pack_payload[1:]
                                continue

                            device_code_name = addresses_names_map[device_address]

                            device_parameter_id = pack_payload[1]
                            try:
                                device_parameter_name = ubi4_enum_description.parameter_data_code_enum(self.__device_addresses_map__[device_address].parsed_parameters[device_parameter_id].DataCode).name
                            except Exception as e:
                                device_parameter_name = str(self.__device_addresses_map__[device_address].parsed_parameters[device_parameter_id].DataCode)

                            vec_index = (device_code_name, device_parameter_name)

                            if not vec_index in ubi4_state:
                                ubi4_state[vec_index] = {}

                            device_parameter_width = self.__device_addresses_map__[device_address].parsed_parameters[device_parameter_id].parameter_size
                            payload_chunk = pack_payload[2:2+device_parameter_width]
                            pack_values = self.parse_parameter_values(payload_chunk, device_code_name, device_parameter_name)                            
                            ubi4_state[vec_index] = pack_values
                            pack_payload = pack_payload[2+device_parameter_width:]

                        self.__ubi4_state_history__.append((time.time(), ubi4_state.copy()))

                if len(p_struct['IN_PACK']) > 0:
                    self.pack_history.append((time.time(), p_struct['IN_PACK']))
                    self.n_pack += 1
                    parse_state(p_struct)

                p_struct['IN_PACK'] = b''
                p_struct['IN_PACK_LEN'] = 0
                p_struct['IN_PACK_BYTES_TO_READ'] = -1

            self.flush_serial() # ensure that we start with an empty buffer

            while self.__run_parsing_thread__:
                # trigger for new pack had been received
                if (pack_parsing_struct['IN_PACK_BYTES_TO_READ'] == 0):
                    add_pack_to_history(pack_parsing_struct)
                    continue

                # read one byte from the serial
                in_byte = self.ser.read(1)

                # trigger for timeout processing
                if in_byte == b'': 
                    add_pack_to_history(pack_parsing_struct)
                    continue

                pack_parsing_struct['IN_PACK'] += in_byte
                pack_parsing_struct['IN_PACK_LEN'] += 1

                if pack_parsing_struct['IN_PACK_BYTES_TO_READ'] > 0:
                    pack_parsing_struct['IN_PACK_BYTES_TO_READ'] -= 1 # decrease the number of bytes to read
                    continue
                elif pack_parsing_struct['IN_PACK_BYTES_TO_READ'] == -1:
                    pass # we are waiting for the header
                else:
                    raise Exception(f'Found IN_PACK_BYTES_TO_READ is {pack_parsing_struct["IN_PACK_BYTES_TO_READ"]}. Something went wrong')

                if pack_parsing_struct['IN_PACK_LEN'] == ubi4_p2p_high_level_port_header.P2P_HIGH_LEVEL_PORT_HEADER_SIZE:
                    pack_parsing_struct['IN_PACK_BYTES_TO_READ'] = ubi4_p2p_high_level_port.parse_p2p_high_level_port_header(pack_parsing_struct['IN_PACK'])['package_size']

                    if pack_parsing_struct['IN_PACK_BYTES_TO_READ'] > pack_parsing_struct['MAX_PACK_SIZE']:
                        pack_parsing_struct['IN_PACK_BYTES_TO_READ'] = pack_parsing_struct['MAX_PACK_SIZE'] # something went wrong


        self.__run_parsing_thread__ = True
        self.__pack_parsing_thread__ = threading.Thread(target=parsing_thread)
        self.__pack_parsing_thread__.start()
        print('Parsing thread started successfully')
        print("********************************")

    def stop_parsing_thread(self):
        print("--------------------------------")
        print("Stopping parsing thread")
        self.__run_parsing_thread__ = False
        if self.__pack_parsing_thread__ is not None:
            self.__pack_parsing_thread__.join()
            print('Parsing thread stopped successfully')
        else:
            print('Parsing thread had not been initialized')
        print("********************************")
        
    def parse_parameter_values(self, pack_payload, device_code_name, device_parameter_name):
        if device_code_name == 'DCE_OMG_MODULE':
            try:
                if device_parameter_name == 'PDCE_OPTIC_LEARNING_DATA':
                    if len(pack_payload) == 180:
                        
                        # typedef struct {
                        #     uint32_t ts; // 4
                        #     uint32_t td; // 4
                        #     float omg[MAX_OMG]; // 16*4
                        #     float emg[MAX_EMG]; // 8*4
                        #     float bno_rpy[MAX_BNO_RPY]; // 3*4
                        #     float bno_acc[MAX_BNO_ACC]; // 3*4
                        #     float prb[MAX_PRB]; // 11*4
                        #     float argmax; // 4
                        #     float denoize; // 4
                        # } RAW_STATE;
                        # total = 180 // master
                        
                        deserialized_tuple = struct.unpack('<'+ 1*"I" + 1*"I" + 16*'f' + 8*'f' + 3*'f' + 3*'f' + 11*'f' + 1*'f' + 1*'f', pack_payload)
                        col_names = ['ts', 'td', 'omg', 'emg', 'bno_rpy', 'bno_acc', 'prb', 'argmax', 'denoize']
                        col_idx_start = [0, 1, 2, 18, 26, 29, 32, 43, 44]
                        col_idx_end =      [1, 2, 18, 26, 29, 32, 43, 44, 45]
                        col_values = [deserialized_tuple[c_idx_start:c_idx_end] 
                                    for c_idx_start, c_idx_end in zip(col_idx_start, col_idx_end)]
                        deserialized_dict = dict(zip(col_names, col_values))
                        return deserialized_dict
                    elif len(pack_payload) == 156:

                        # typedef struct {
                        #     uint32_t ts; //4
                        #     uint32_t td; //4
                        #     float omg[MAX_OMG]; //16*4
                        #     float emg[MAX_EMG]; //8*4
                        #     float bno[MAX_BNO]; //3*4
                        #     float prb[MAX_PRB]; //8*4
                        #     float argmax; //4
                        #     float denoize; //4
                        # } RAW_STATE; 
                        # total = 4 + 4 + (16*4) + (8*4) + (3*4) + (8*4) + 4 + 4  # = 4+4+64+32+12+32+4+4 = 156 bytes // merge-branch
                        
                        deserialized_tuple = struct.unpack('<'+ 1*"I" + 1*"I" + 16*'f' + 8*'f' + 3*'f' + 8*'f' + 1*'f' + 1*'f', pack_payload)
                        col_names = ['ts', 'td', 'omg', 'emg', 'bno', 'prb', 'argmax', 'denoize']
                        col_idx_start = [0, 1, 2, 18, 26, 29, 37, 38]
                        col_idx_end =      [1, 2, 18, 26, 29, 37, 38, 39]
                        col_values = [deserialized_tuple[c_idx_start:c_idx_end] 
                                    for c_idx_start, c_idx_end in zip(col_idx_start, col_idx_end)]
                        deserialized_dict = dict(zip(col_names, col_values))
                        return deserialized_dict
                    else:
                        raise ValueError(f'Unexpected pack_payload length: {len(pack_payload)}')

                
                if device_parameter_name == 'PDCE_DMS_OUTPUT':
                    return {"DMS_OUTPUT": struct.unpack('<B', pack_payload)[0]}
                
                if device_parameter_name == 'PDCE_GENERIC_2':     
                    deserialized_tuple = struct.unpack('< B B H B B', pack_payload)
                    # print(f'deserialized_tuple: {deserialized_tuple}')
                    col_names = ['OPTIC_RECEIVER_HAS_NEW_FLAG', 'inOpticsPack-n_bytes', 'pack_index_in_series', 'sd-res', 'sd-bytes_written']
                    deserialized_dict = dict(zip(col_names, deserialized_tuple))
                    if deserialized_dict['sd-res'] in ubi4_enum_description.FRESULT._value2member_map_:
                        deserialized_dict['sd-res'] = ubi4_enum_description.FRESULT(deserialized_dict['sd-res']).name
                    else:
                        deserialized_dict['sd-res'] = str(deserialized_dict['sd-res'])
                    return deserialized_dict
                else:
                    return pack_payload
                
            except Exception as e:
                print(f'Error in parse_parameter_values {device_code_name} {device_parameter_name}: {e}')
                return pack_payload
            
        if device_code_name == 'DCE_EMG_SENSE':
            if device_parameter_name == 'PDCE_EMG_CH_1_3_VAL':
                return struct.unpack('<BB', pack_payload)
            else:
                return pack_payload
            
        if device_code_name == 'DCE_CPU_MODULE':
            if device_parameter_name == 'PDCE_INTERFECE_ERROR_COUNTER':
                return struct.unpack('<B', pack_payload)[0]
            else:
                return pack_payload
            
        if device_code_name == 'DCE_FEST_H_AND_F':
            if device_parameter_name == 'PDCE_SELECT_GESTURE':
                return struct.unpack('<B', pack_payload)[0]
            else:
                return pack_payload
            
        else:
            return pack_payload


    def request_first_info_blocked(self):

        """
        Request first info from the stack
        """
        print("--------------------------------")
        print('Requesting first info from the stack')
        
        pack_payload = bytearray([ubi4_enum_description.DeviceInformationCommand_enum['INICIALIZE_INFORMATION'].value, 0x02])

        pack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                        bridge = 0, 
                                                                        waiting_answer = 0, 
                                                                        request_type = 0,
                                                                        package_code=ubi4_enum_description.Command_enum["DEVICE_INFORMATION"].value, 
                                                                        package_id=0, 
                                                                        package_size=len(pack_payload), 
                                                                        crc=get_crc(pack_payload, len(pack_payload)), 
                                                                        address=0)

        pack = bytearray(pack_header) + bytearray(pack_payload)

        print('Sending:', list(map(lambda x: f'0x{x:02x}', pack)))

        # TODO: stream parsing
        self.ser.write(pack)
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        pack_info = self.ser.read(self.ser.in_waiting)

        pack_header, pack_payload = ubi4_p2p_high_level_port.parse_p2p_high_level_port_pack(pack_info)
        # crc = get_crc(pack_payload, len(pack_payload))
        # if crc != pack_header['crc']:
        #     print(f'CRC mismatch: calculated crc {crc} != pack header crc {pack_header["crc"]}')


        # Assuming pack_payload contains the binary data for the struct
        self.full_init_struct = ubi4_p2p_high_level_port.FullInitializeConnectionStruct(pack_payload[2:])
        
        print("Full initialize structure:")
        print(self.full_init_struct)
        print("********************************")

        return self.full_init_struct
    
    def request_device_parameters(self):
        print("--------------------------------")
        print("Requesting device parameters")
        if self.full_init_struct is None:
            raise Exception('!!!Full initialize structure is not initialized. You should initialize the stack first by calling: `request_first_info_blocked()`')

        pack_payload = bytearray([ubi4_enum_description.DeviceInformationCommand_enum['READ_DEVICE_PARAMETERS'].value, 
                                0x00, 10])
                                # self.full_init_struct.ParametrsNum])

        pack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                        bridge = 0, 
                                                                        waiting_answer = 0, 
                                                                        request_type = 0,
                                                                        package_code=ubi4_enum_description.Command_enum["DEVICE_INFORMATION"].value, 
                                                                        package_id=0, 
                                                                        package_size=len(pack_payload), 
                                                                        crc=get_crc(pack_payload, len(pack_payload)), 
                                                                        address=0)

        pack = bytearray(pack_header) + bytearray(pack_payload)

        print('Requesting device parameters:', list(map(lambda x: f'0x{x:02x}', pack)))

        # TODO: stream parsing
        self.ser.write(pack)
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        pack_info = self.ser.read(self.ser.in_waiting)

        pack_header, pack_payload = ubi4_p2p_high_level_port.parse_p2p_high_level_port_pack(pack_info)
        # crc = get_crc(pack_payload, len(pack_payload))
        # if crc != pack_header['crc']:
        #     print(f'CRC mismatch: calculated crc {crc} != pack header crc {pack_header["crc"]}')


        print(f'pack_header {pack_header}, pack_payload {pack_payload}, get_crc(pack_info, len(pack_info)) {get_crc(pack_info, len(pack_info))}')

        self.parsed_parameters = ubi4_p2p_high_level_port.parse_base_parameter_info_struct(pack_payload[3:])
        print("Parsed parameters:")
        print(self.parsed_parameters)
        print("********************************")
        
        return self.parsed_parameters
    
    def request_sub_device_info(self):
        print("--------------------------------")
        print("Requesting sub device info")

        pack_payload = bytearray([ubi4_enum_description.DeviceInformationCommand_enum['READ_SUB_DEVICE_INFO'].value])

        pack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                                    bridge = 0, 
                                                                                    waiting_answer = 0, 
                                                                                    request_type = 0,
                                                                                    package_code=ubi4_enum_description.Command_enum["DEVICE_INFORMATION"].value, 
                                                                                    package_id=0, 
                                                                                    package_size=len(pack_payload), 
                                                                                    crc=get_crc(pack_payload, len(pack_payload)), 
                                                                                    address=0)

        pack = bytearray(pack_header) + bytearray(pack_payload)

        print('Sending:', list(map(lambda x: f'0x{x:02x}', pack)))
        
        # TODO: stream parsing
        self.ser.write(pack)
        time.sleep(self.UBI4_STATIC_TIMOUT_MS/1000)
        pack_info = self.ser.read(self.ser.in_waiting)

        pack_header, pack_payload = ubi4_p2p_high_level_port.parse_p2p_high_level_port_pack(pack_info)
        # crc = get_crc(pack_payload, len(pack_payload))
        # if crc != pack_header['crc']:
        #     print(f'CRC mismatch: calculated crc {crc} != pack header crc {pack_header["crc"]}')


        print(f'pack_header {pack_header}, pack_payload {pack_payload}, get_crc(pack_info, len(pack_info)) {get_crc(pack_info, len(pack_info))}')

        _, MAX_SUB_DEVICES = struct.unpack('<BB', pack_payload[:2])
        self.MAX_SUB_DEVICES = MAX_SUB_DEVICES

        self.sub_devices_array = []
        for i in range(MAX_SUB_DEVICES):
            sub_device_data = struct.unpack('<BBBBBBBBB', pack_payload[2+i*9:2+i*9+9])
            sub_device = UBI4Device(
                DeviceType=sub_device_data[0],
                DeviceCode=sub_device_data[1],
                DeviceRole=sub_device_data[2],
                DeviceVersion=sub_device_data[3],
                DeviceSubVersion=sub_device_data[4],
                DeviceAddress=sub_device_data[5],
                ParametersNum=sub_device_data[6],
                SubDeviceNum=sub_device_data[7],
                DefaultPort=sub_device_data[8]
            )
            sub_device.__stack__ = self

            self.sub_devices_array.append(sub_device)
        
        print("Sub devices array:")
        print(self.sub_devices_array)
        print("********************************")
        return self.sub_devices_array
    
            
    def transfer_file_to_omg_sd(self, 
                                local_file_path: str, 
                                remote_file_path: str, 
                                CKPT_PACK_TIMEOUT_MS: int = 2000,
                                verbose: bool = False):
        
        if os.path.exists(local_file_path):
            with open(local_file_path, 'rb') as f:
                file_data = f.read()
        else:
            raise FileNotFoundError(f"File not found: {local_file_path}")
        
        all_params_names = [ubi4_enum_description.parameter_data_code_enum(p.DataCode).name 
                            if p.DataCode in ubi4_enum_description.parameter_data_code_enum._value2member_map_ 
                            else str(p.DataCode) for p in self.sub_devices_array[0].parsed_parameters]
        if 'PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER' in all_params_names:
            local_index = all_params_names.index('PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER')
            local_param = self.sub_devices_array[0].parsed_parameters[local_index]
            print('Found parameter:', local_param)
        else:
            raise ValueError("PDCE_OPTIC_CHECKPOINT_DATA_RECEIVER not found in UBI4 stack")

        chunk_size = local_param.parameter_size - 4  # size of each chunk in bytes
        print('Chunk size for transfer:', chunk_size)
        
        _order = []
            
        def construct_pack_for_transfer(pack_chunk: bytearray,
                                    command: int = 0,
                                    pack_index_in_series: int = 0):
            
            n_bytes = len(pack_chunk)
            assert n_bytes <= (local_param.parameter_size-4), f"pack_chunk size must be less than {local_param.parameter_size-4} bytes"

            header_bytes = struct.pack('<BBH', command, n_bytes, pack_index_in_series) 
            pack_payload = header_bytes + pack_chunk + bytearray([0x00]*(local_param.parameter_size - n_bytes - len(header_bytes)))
            
            _order.append({'command': command,
                        'n_bytes': n_bytes,
                        'pack_index_in_series': pack_index_in_series,
                        'pack_payload': pack_payload})

            pack_header = ubi4_p2p_high_level_port.construct_p2p_high_level_port_header(port = 0,
                                                                            bridge = 0, 
                                                                            waiting_answer = 1, 
                                                                            request_type = 1,
                                                                            package_code=local_index | 0x80, 
                                                                            package_id=0, 
                                                                            package_size=len(pack_payload), 
                                                                            crc=get_crc(pack_payload, len(pack_payload)), 
                                                                            address=0x0A)

            pack= bytearray(pack_header) + bytearray(pack_payload)

            return pack    

        packs = []
        n_packs = 0

        # Create remote file on SD card
        n_packs += 1
        packs.append(construct_pack_for_transfer(remote_file_path.encode('utf-8'), 
                                                1, 
                                                pack_index_in_series=n_packs))

        # Populate remote file with data
        for ipos, pos in tqdm(enumerate(range(0, len(file_data), chunk_size))):
            pack_chunk = file_data[pos:pos+chunk_size]

            n_packs += 1
            packs.append(construct_pack_for_transfer(pack_chunk, 
                                                    command=2, 
                                                    pack_index_in_series=n_packs))

        # Close remote file on SD card
        n_packs += 1    
        packs.append(construct_pack_for_transfer(''.encode('utf-8'), 
                                                command=3,
                                                pack_index_in_series=n_packs))
        
        ubi4_state_history = []
        for ipack, pack in tqdm(enumerate(packs), total = len(packs)):
            if verbose: 
                print('Sending Header:', bin(pack[0]), 'Payload:', list(map(lambda x: f'0x{x:02x}', pack)))

            # TODO: stream parsing
            self.__ubi4_state_history__.clear()
            self.pack_history.clear()
            self.ser.write(pack)
            
            ts_send = time.time()
            
            good_flag = 0
            while (time.time() - ts_send) < CKPT_PACK_TIMEOUT_MS/1000:
                if len(self.__ubi4_state_history__) > 0:
                    ts_state, ubi4_state = self.__ubi4_state_history__.popleft()
                    ubi4_state_history.append((ts_state, ubi4_state))
                    if ('DCE_OMG_MODULE', 'PDCE_GENERIC_2') in ubi4_state:
                        values = ubi4_state[('DCE_OMG_MODULE', 'PDCE_GENERIC_2')]
                        if verbose: 
                            print(values)
                        
                        if (values["OPTIC_RECEIVER_HAS_NEW_FLAG"] == 0) and \
                            (values["pack_index_in_series"] == ipack+1) and \
                            (values["sd-res"] == 'FR_OK'):
                            if verbose: 
                                print('next pack')
                            good_flag = 1
                            break
                else:
                    time.sleep(0.01)
            if good_flag == 0:
                raise Exception('Timeout')
            
        df_states = pd.DataFrame({ts: state for ts, state in ubi4_state_history}).T
        df_states.index -= df_states.index[0]
        df_PDCE_GENERIC_2 = pd.DataFrame(df_states[('DCE_OMG_MODULE', 'PDCE_GENERIC_2')].to_list())
        
        df_order = pd.DataFrame(_order)
        
        return df_order, df_PDCE_GENERIC_2