import numpy as np
from dataclasses import dataclass
import struct
from .ubi4_p2p_high_level_port_pack import \
    split_p2p_high_level_port_pack, \
    parse_p2p_high_level_port_pack, \
    P2P_PACK_SIZE_MISMATCH_ERROR, \
    P2P_CRC_CHECK_ERROR, \
    check_p2p_pack_consistency
from .ubi4_p2p_high_level_port_header import *
from .ubi4_p2p_high_level_port_header_byte import *

@dataclass
class full_inicialize_connection_struct:
    """
    typedef struct
    {
        char DeviceName[32];
        uint8_t DeviceVersion;
        uint8_t DeviceSubVersion;
        char DeviceLabel[16];
        uint8_t DeviceType;
        uint8_t DeviceCode;

        uint8_t DeviceAddress;

        char DeviceUUID_Prefix[16];
        uint32_t DeviceUUID;

        uint8_t ParametersNum;
        uint8_t SubDeviceNum;
        uint8_t ProgramType;
        uint8_t DefaultPort;

    }__attribute__((__packed__))full_inicialize_connection_struct;
    """
    DeviceName: str
    DeviceVersion: int
    DeviceSubVersion: int
    DeviceLabel: str
    DeviceType: int
    DeviceCode: int
    DeviceAddress: int
    DeviceUUID_Prefix: str
    DeviceUUID: int
    ParametersNum: int
    SubDeviceNum: int
    ProgramType: int
    DefaultPort: int

class FullInitializeConnectionStruct:
    """
    typedef struct
    {
        char DeviceName[32];
        uint8_t DeviceVersion;
        uint8_t DeviceSubVersion;
        char DeviceLabel[16];
        uint8_t DeviceType;
        uint8_t DeviceCode;

        uint8_t DeviceAddress;

        char DeviceUUID_Prefix[16];
        uint32_t DeviceUUID;

        uint8_t ParametrsNum;
        uint8_t SubDeviceNum;
        uint8_t ProgramType;
        uint8_t DefaultPort;

    }__attribute__((__packed__))full_inicialize_connection_struct;
    """
    
    def __init__(self, data=None):
        if data is None:
            pass
        else:
            assert(len(data) == 77)
            unpacked_data = struct.unpack('<32s B B 16s B B B 16s I B B B B', data)
            self.DeviceName = unpacked_data[0].decode('utf-8').strip('\x00')
            self.DeviceVersion = unpacked_data[1]
            self.DeviceSubVersion = unpacked_data[2]
            self.DeviceLabel = unpacked_data[3].decode('utf-8').strip('\x00')
            self.DeviceType = unpacked_data[4]
            self.DeviceCode = unpacked_data[5]
            self.DeviceAddress = unpacked_data[6]
            self.DeviceUUID_Prefix = unpacked_data[7].decode('utf-8').strip('\x00')
            self.DeviceUUID = unpacked_data[8]
            self.ParametrsNum = unpacked_data[9]
            self.SubDeviceNum = unpacked_data[10]
            self.ProgramType = unpacked_data[11]
            self.DefaultPort = unpacked_data[12]
            print('FullInitializeConnectionStruct loaded')

    def __repr__(self):
        return(f"""DeviceName: {self.DeviceName}
DeviceVersion: {self.DeviceVersion}
DeviceSubVersion: {self.DeviceSubVersion}
DeviceLabel: {self.DeviceLabel}
DeviceType: {self.DeviceType}
DeviceCode: {self.DeviceCode}
DeviceAddress: {self.DeviceAddress}
DeviceUUID_Prefix: {self.DeviceUUID_Prefix}
DeviceUUID: {self.DeviceUUID}
ParametrsNum: {self.ParametrsNum}
SubDeviceNum: {self.SubDeviceNum}
ProgramType: {self.ProgramType}
DefaultPort: {self.DefaultPort}""")
    


@dataclass
class BaseParameterInfoStruct:
    """
    typedef struct {

        uint8_t ID;
        uint8_t BroadcastID;
        uint8_t DataCode;
        uint8_t DataInstance;
        uint16_t parameter_size;
        uint8_t flag_shift;

        uint8_t optimisation_transfer:	1;
        uint8_t optimisation_stream:	7;

        uint8_t value_limit;

        uint8_t init_read:				1;
        uint8_t init_write:				1;
        uint8_t synch_type:				3;
        uint8_t synch_direction:		3;
        uint8_t synch_period;


        uint8_t type;
        uint8_t save_in_master:			1;
        uint8_t save_in_slave:			1;
        uint8_t main_consumer:			1;
        uint8_t link_type:				5;

        uint8_t additional_info_size;

        uint8_t related_parameter_ID;
        uint8_t related_DataCode;

    } __attribute__((__packed__)) base_parameter_info_struct;
    """

    ID: int
    BroadcastID: int
    DataCode: int
    DataInstance: int
    parameter_size: int
    flag_shift: int
    optimisation_transfer: int
    optimisation_stream: int
    value_limit: int
    init_read: int
    init_write: int
    synch_type: int
    synch_direction: int
    synch_period: int
    type: int
    save_in_master: int
    save_in_slave: int
    main_consumer: int
    link_type: int
    additional_info_size: int
    related_parameter_ID: int
    related_DataCode: int
    __original_data__ : bytearray

    def __repr__(self):
        return f"""BaseParameterInfoStruct
ID={self.ID}
BroadcastID={self.BroadcastID}
DataCode={self.DataCode}
DataInstance={self.DataInstance}
parameter_size={self.parameter_size}
flag_shift={self.flag_shift}
optimisation_transfer={self.optimisation_transfer}
optimisation_stream={self.optimisation_stream}
value_limit={self.value_limit}
init_read={self.init_read}
init_write={self.init_write}
synch_type={self.synch_type}
synch_direction={self.synch_direction}
synch_period={self.synch_period}
type={self.type}
save_in_master={self.save_in_master}
save_in_slave={self.save_in_slave}
main_consumer={self.main_consumer}
link_type={self.link_type}
additional_info_size={self.additional_info_size}
related_parameter_ID={self.related_parameter_ID}
related_DataCode={self.related_DataCode}"""

def parse_base_parameter_info_struct(data: bytearray) -> BaseParameterInfoStruct:

    assert(len(data) % 16 == 0)

    parameters = []

    for i_start in range(0, len(data), 16):
        data_chunk = data[i_start:i_start+16]
        unpacked_data = struct.unpack('<BBBBHBBBBBBBBBB', data_chunk)
        ID, BroadcastID, DataCode, DataInstance, parameter_size, flag_shift, \
        optimisation, value_limit, IR_IW_ST_SD, \
        synch_period, type, SIM_SIS_MC_LT, \
        additional_info_size, related_parameter_ID, related_DataCode = unpacked_data

        optimisation_transfer = (optimisation >> 0) & 0x1
        optimisation_stream = (optimisation >> 1) & 0x7F

        init_read = (IR_IW_ST_SD >> 0) & 0x1
        init_write = (IR_IW_ST_SD >> 1) & 0x1
        synch_type = (IR_IW_ST_SD >> 2) & 0x7
        synch_direction = (IR_IW_ST_SD >> 5) & 0x7

        save_in_master = (SIM_SIS_MC_LT >> 0) & 0x1
        save_in_slave = (SIM_SIS_MC_LT >> 1) & 0x1
        main_consumer = (SIM_SIS_MC_LT >> 2) & 0x1
        link_type = (SIM_SIS_MC_LT >> 3) & 0x1F

        parameters.append(BaseParameterInfoStruct(
            ID=ID,
            BroadcastID=BroadcastID,
            DataCode=DataCode,
            DataInstance=DataInstance,
            parameter_size=parameter_size,
            flag_shift=flag_shift,
            optimisation_transfer=optimisation_transfer,
            optimisation_stream=optimisation_stream,
            value_limit=value_limit,
            init_read=init_read,
            init_write=init_write,
            synch_type=synch_type,
            synch_direction=synch_direction,
            synch_period=synch_period,
            type=type,
            save_in_master=save_in_master,
            save_in_slave=save_in_slave,
            main_consumer=main_consumer,
            link_type=link_type,
            additional_info_size=additional_info_size,
            related_parameter_ID=related_parameter_ID,
            related_DataCode=related_DataCode,
            __original_data__ = data_chunk
        )
    )   

    return parameters