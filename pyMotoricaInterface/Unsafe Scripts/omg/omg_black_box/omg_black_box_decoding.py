import asn1tools
from Scripts.omg.omg_serialization import print_decoded_pack, decode_packet


def main():
    data = open('002_chunk.txt', 'rb').read().split(b'\r\n')
    data.pop(-1)
    MMODULE = asn1tools.compile_files('../omg_protocol.asn', codec='uper')
    for line in data:
        decoded = MMODULE.decode('Omg-State', line)
        print_decoded_pack(decode_packet(decoded))


if __name__ == '__main__':
    main()
