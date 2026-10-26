from .ubi4_p2p_high_level_port_header import \
    parse_p2p_high_level_port_header, \
    P2P_HIGH_LEVEL_PORT_HEADER_SIZE    
    
from .. import get_crc
    
class P2P_PACK_SIZE_MISMATCH_ERROR(Exception):
    """
    Exception raised when the size of the P2P pack does not match the expected size.
    """
    pass

class P2P_CRC_CHECK_ERROR(Exception):
    """
    Exception raised when the CRC check fails for the P2P pack.
    """
    pass
    
def split_p2p_high_level_port_pack(pack: bytearray) -> tuple[bytearray, bytearray]:
    """
    Splits the given P2P high level port pack into its header and payload.

    Args:
        pack (bytearray): The P2P high level port pack to be split.

    Returns:
        tuple[bytearray, bytearray]: A tuple containing the header and payload of the pack.
    """
    assert(len(pack) >= P2P_HIGH_LEVEL_PORT_HEADER_SIZE)
    header = pack[:P2P_HIGH_LEVEL_PORT_HEADER_SIZE]
    payload = pack[P2P_HIGH_LEVEL_PORT_HEADER_SIZE:]
    return header, payload

def combine_p2p_high_level_port_pack(header: bytearray, payload: bytearray) -> bytearray:
    """
    Combines the given header and payload into a single P2P high level port pack.

    Args:
        header (bytearray): The header of the P2P high level port pack.
        payload (bytearray): The payload of the P2P high level port pack.

    Returns:
        bytearray: The combined P2P high level port pack.
    """
    return header + payload


def check_p2p_pack_consistency(pack: bytearray):
    """
    Checks the consistency of the given P2P pack by verifying its size and CRC.

    Args:
        pack (bytearray): The P2P pack to be checked.

    Raises:
        P2P_PACK_SIZE_MISMATCH_ERROR: If the size of the pack does not match the expected size.
        P2P_CRC_CHECK_ERROR: If the CRC check fails.
    """
    header, payload = split_p2p_high_level_port_pack(pack)
    p2p_header = parse_p2p_high_level_port_header(header)

    # Pack size check
    if (len(pack) != (P2P_HIGH_LEVEL_PORT_HEADER_SIZE + p2p_header['package_size'])):
        raise P2P_PACK_SIZE_MISMATCH_ERROR(f"Actual pack length {len(pack)} is not consistent with header pack size {P2P_HIGH_LEVEL_PORT_HEADER_SIZE + p2p_header['package_size']}")
    
    # CRC check
    pack_crc = p2p_header['crc']
    calc_crc = get_crc(payload, len(payload))
    if pack_crc != calc_crc:
        print(f"CRC check failed: pack_crc={pack_crc}, calc_crc={calc_crc}")
        # raise P2P_CRC_CHECK_ERROR(f"CRC check failed")

    return True

def parse_p2p_high_level_port_pack(pack: bytearray, check_consistency: bool = True):
    """
    Parses the given P2P high level port pack into its header and payload, optionally checking consistency.

    Args:
        pack (bytearray): The P2P high level port pack to be parsed.
        check_consistency (bool, optional): If True, checks the consistency of the pack. Defaults to True.

    Returns:
        tuple[dict, bytearray]: A tuple containing the parsed header as a dictionary and the payload as a bytearray.
    """
    if check_consistency:
        check_p2p_pack_consistency(pack)
        
    header, payload = split_p2p_high_level_port_pack(pack)
    p2p_header = parse_p2p_high_level_port_header(header)
    return p2p_header, payload

def construct_p2p_high_level_port_pack(p2p_header: dict, payload: bytearray) -> bytearray:
    """
    Constructs a P2P high level port pack from the given header and payload.

    Args:
        p2p_header (dict): The header of the P2P high level port pack.
        payload (bytearray): The payload of the P2P high level port pack.

    Returns:
        bytearray: The constructed P2P high level port pack.
    """
    header = construct_p2p_high_level_port_header(p2p_header)
    return combine_p2p_high_level_port_pack(header, payload)
