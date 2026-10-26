from collections import OrderedDict

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

def parse_p2p_high_level_port_header_byte(header_byte: int):

    port = (header_byte >> 0) & 0x1F
    waiting_answer = (header_byte >> 5) & 0x1
    request_type = (header_byte >> 6) & 0x1
    brige = (header_byte >> 7) & 0x1

    return OrderedDict([
        ("port", port),
        ("waiting_answer", waiting_answer),
        ("request_type", request_type),
        ("brige", brige)
    ])

def construct_p2p_high_level_port_header_byte(port: int, 
                                              waiting_answer: int, 
                                              request_type: int, 
                                              bridge: int) -> int:
    """
    Constructs a p2p_high_level_port_header from the given parameters.

    Returns:
        int: byte representing the p2p_high_level_port_header.
    """
    
    header = 0
    header |= (port & 0x1F) << 0
    header |= (waiting_answer & 0x1) << 5
    header |= (request_type & 0x1) << 6
    header |= (bridge & 0x1) << 7
    return header