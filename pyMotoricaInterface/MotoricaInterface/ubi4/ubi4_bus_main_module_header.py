from typing import Dict

"""
typedef struct
{
	uint8_t last_package: 					1;
	uint8_t waiting_answer: 				1;
	uint8_t request_type: 					1;
	uint8_t size_type: 						2;
	uint8_t exist_uid_or_param_com_size: 	1; 
	uint8_t brige:							1;
	uint8_t request: 						1;
	union u_device_address addr;
	uint8_t package_code;
	union u_bus_res_payload Data;
	uint8_t crc;
}__attribute__((__packed__)) bus_main_module_header;
"""


bus_protocol_request_type_enum = {0x00: "BPRT_COMMAND", 
                                  0x01: "BPRT_PARAMETRS"}

bus_protocol_package_type_enum = {0x00: "BPPT_FIX_LEN_PACKAGE", 
                                  0x01: "BPPT_FLOAT_LEN_PACKAGE", 
                                  0x02: "BPPT_RES1", 
                                  0x03: "BPPT_RES2"}

def construct_ubi4_header_byte(last_package: int,
                               waiting_answer: int, 
                               request_type: int, 
                               size_type: int, 
                               exist_uid_or_param_com_size: int, 
                               brige: int, 
                               request: int) -> int:
    """
    Constructs a header byte from keyword arguments.

    Keyword Args:
    last_package (int): 1-bit value.
    waiting_answer (int): 1-bit value.
    request_type (int): 1-bit value.
    size_type (int): 2-bit value.
    exist_uid_or_param_com_size (int): 1-bit value.
    brige (int): 1-bit value.
    request (int): 1-bit value.

    Returns:
    int: The constructed header byte.
    """

    header = 0
    header |= (last_package & 0x1) << 0
    header |= (brige & 0x1) << 1
    header |= (exist_uid_or_param_com_size & 0x1) << 2
    header |= (size_type & 0x3) << 3
    header |= (request_type & 0x1) << 5
    header |= (waiting_answer & 0x1) << 6
    header |= (request & 0x1) << 7
    return int(header)

def parse_ubi4_header_byte(header: int) -> Dict[str, int]:
    """
    Parse header byte into a dictionary.

    uint8_t last_package: 					1;
    uint8_t waiting_answer: 				1;
    uint8_t request_type: 					1;
    uint8_t size_type: 						2;
    uint8_t exist_uid_or_param_com_size: 	1; 
    uint8_t brige:							1;
    uint8_t request: 						1;
    """
    
    request = (header >> 0) & 0x1
    brige = (header >> 1) & 0x1
    exist_uid_or_param_com_size = (header >> 2) & 0x1
    size_type = (header >> 3) & 0x3
    request_type = (header >> 5) & 0x1
    waiting_answer = (header >> 6) & 0x1
    last_package = (header >> 7) & 0x1
    return {"request": request, 
            "brige": brige, 
            "exist_uid_or_param_com_size": exist_uid_or_param_com_size, 
            "size_type": size_type, 
            "request_type": request_type, 
            "waiting_answer": waiting_answer, 
            "last_package": last_package}


def bus_main_module_header(pack):
    """
    ```c
    typedef struct
    {
        uint8_t last_package: 					1;
        uint8_t waiting_answer: 				1;
        uint8_t request_type: 					1;
        uint8_t size_type: 						2;
        uint8_t exist_uid_or_param_com_size: 	1; 
        uint8_t brige:							1;
        uint8_t request: 						1;
        union u_device_address addr;
        uint8_t package_code;
        union u_bus_res_payload Data;
        uint8_t crc;
    }__attribute__((__packed__)) bus_main_module_header;
    ```
    """

    # if len(pack) != 10:
    #     return None
    assert len(pack) == 10
    header = pack[0]
    address = pack[1]
    code_id = pack[2]
    payload = pack[3:9]
    crc = pack[9]
    return {"header":           header, 
            "addr":             address, 
            "package_code":     code_id, 
            "Data":             payload, 
            "crc":              crc}