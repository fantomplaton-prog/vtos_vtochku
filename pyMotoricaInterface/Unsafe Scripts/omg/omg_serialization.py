import struct
import asn1tools
import serial
import time
import binascii
from typing import List


def read_serial_packets(port='/dev/cu.usbmodem2082369D46321', baudrate=115200, timeout_ms=30, collect_time=1.0) -> List[bytes]:
    ser = serial.Serial(port, baudrate, timeout=timeout_ms / 1000)
    print(f"Connected to {port} at {baudrate} baud")

    packets = []
    current_packet = bytearray()
    last_read_time = time.perf_counter() * 1000
    start_time = time.perf_counter()

    try:
        while (time.perf_counter() - start_time) < collect_time:
            if ser.in_waiting:
                byte = ser.read()
                current_time = time.perf_counter() * 1000

                # Если прошло больше timeout_ms с момента последнего чтения
                if current_packet and (current_time - last_read_time) > timeout_ms:
                    packets.append(bytes(current_packet))
                    current_packet = bytearray()

                current_packet.extend(byte)
                last_read_time = current_time

            elif current_packet:
                # Проверяем таймаут для последнего пакета
                current_time = time.perf_counter() * 1000
                if (current_time - last_read_time) > timeout_ms:
                    packets.append(bytes(current_packet))
                    current_packet = bytearray()

            else:
                time.sleep(0.001)

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        ser.close()
        print("Port closed")

    print(f"Collected {len(packets)} packets in {collect_time:.1f} seconds")
    return packets


def decode_float32_array(data, count):
    return [struct.unpack('<f', data[i:i + 4])[0] for i in range(0, count * 4, 4)]


def decode_packet(packet):
    decoded = {
        'ts': struct.unpack('<f', packet['ts']['ts-byte-array'])[0],
        'td': struct.unpack('<f', packet['td']['td-byte-array'])[0],
        'omg': decode_float32_array(packet['omg']['omg-byte-array'], 16),
        'emg': decode_float32_array(packet['emg']['emg-byte-array'], 8),
        'bno_rpy': decode_float32_array(packet['bno-rpy']['bno-rpy-byte-array'], 3),
        'bno_acc': decode_float32_array(packet['bno-acc']['bno-acc-byte-array'], 3),
        'prb': decode_float32_array(packet['prb']['prb-byte-array'], 10),
        'argmax': struct.unpack('<f', packet['argmax']['argmax-byte-array'])[0],
        'denoize': struct.unpack('<f', packet['denoize']['denoize-byte-array'])[0]
    }
    return decoded


def print_decoded_pack(decoded_data):
    print("ts:", decoded_data['ts'])
    print("td:", decoded_data['td'])
    print("omg:", decoded_data['omg'])
    print("emg:", decoded_data['emg'])
    print("bno_rpy:", decoded_data['bno_rpy'])
    print("bno_acc:", decoded_data['bno_acc'])
    print("prb:", decoded_data['prb'])
    print("argmax:", decoded_data['argmax'])
    print("denoize:", decoded_data['denoize'])
    print()


def main():
    packets = read_serial_packets()  # '/dev/cu.usbmodem103'
    MMODULE = asn1tools.compile_files('omg_protocol.asn', codec='uper')
    for i, packet in enumerate(packets):
        hex_str = binascii.hexlify(packet).decode()
        print(f"\nPacket {i + 1} ({len(packet)} bytes): {hex_str}")
        try:
            decoded = MMODULE.decode('Omg-State', packet)
            print(decoded)
            print_decoded_pack(decode_packet(decoded))
        except Exception as e:
            print(e)


if __name__ == "__main__":
    main()
