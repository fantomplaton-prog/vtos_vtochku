import serial
import time
import struct


def cast_bytes_to_float_array(bytes_data):
    float_array = []
    for i in range(0, len(bytes_data), 4):
        float_value = struct.unpack('<f', bytes_data[i:i+4])[0]
        float_array.append(float_value)
    return float_array


def main():
    ser = serial.Serial(
        port='/dev/cu.usbmodem1234561',
        baudrate=115200,
        timeout=1
    )

    cmd = [
        [0x20, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01],
        [0x60, 0x80, 0x00, 0x01, 0x00, 0x00, 0x0A, 0x02],
        # [0xc0, 0x86, 0x00, 0x01, 0x00, 0x00, 0x0A, 0x02],
    ]

    for i in range(len(cmd)):
        data_to_send = bytes(cmd[i])
        ser.write(data_to_send)
        print("Data sent:", data_to_send)
        time.sleep(1)
        response = ser.read(ser.in_waiting)
        print("Response received:", [hex(i) for i in response])
        time.sleep(1)
    while 1:
        response = ser.read(ser.in_waiting)
        print("Response received:", [hex(i) for i in response])
        time.sleep(1)


if __name__ == "__main__":
    main()
