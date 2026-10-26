import numpy as np
from .ubi4_p2p_high_level_port_header_byte import \
    parse_p2p_high_level_port_header_byte, \
    construct_p2p_high_level_port_header_byte
    
P2P_HIGH_LEVEL_PORT_HEADER_SIZE = 7

"""
typedef struct
{
    uint8_t port:5;
    uint8_t waiting_answer: 	1;
    uint8_t request_type:		1;
    uint8_t bridge: 				1;
    uint8_t package_code;
    uint8_t package_id;
    uint16_t package_size;
    uint8_t crc;
    uint8_t address;
}__attribute__((__packed__)) p2p_high_level_port_header;
"""

def parse_p2p_high_level_port_header(p2p_header: bytearray):
    assert len(p2p_header) == P2P_HIGH_LEVEL_PORT_HEADER_SIZE

    header_byte = p2p_header[0]
    header_info = parse_p2p_high_level_port_header_byte(header_byte)
    # print(header_info)

    header_info['package_code'] = p2p_header[1]
    header_info['package_id'] = p2p_header[2]
    header_info['package_size'] = int(np.frombuffer(p2p_header[3:5], dtype=np.uint16)[0])
    header_info['crc'] = p2p_header[5]
    header_info['address'] = p2p_header[6]

    return header_info

def construct_p2p_high_level_port_header(port: np.uint8,
                                         waiting_answer: np.uint8,
                                         request_type: np.uint8,
                                         bridge: np.uint8,
                                         package_code: np.uint8, 
                                         package_id: np.uint8, 
                                         package_size: np.uint16, 
                                         crc: np.uint8, 
                                         address: np.uint8) -> bytearray:
    
    header_byte = construct_p2p_high_level_port_header_byte(port=port, 
                                                            waiting_answer=waiting_answer, 
                                                            request_type=request_type, 
                                                            bridge=bridge)
    
    return bytearray([header_byte, 
            package_code, 
            package_id, 
            *package_size.to_bytes(2, byteorder='little'), 
            crc, 
            address])