import time
from datetime import datetime

import serial

import MotoricaInterface as mi


def main():
    port = list(mi.utils.list_devices()['device'])[-1]
    ser = serial.Serial(
        port=port,
        baudrate=115200,
        timeout=1
    )

    command = "M0\r\n"
    ser.write(command.encode())
    time.sleep(0.1)

    filename = "log.txt"
    start_time = time.time()
    duration = 30
    ser.flush()
    with open(filename, 'w') as file:
        while (time.time() - start_time) < duration:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8')
                file.write(f"{line}")
                print(f"{line}")
            time.sleep(0.001)


if __name__ == '__main__':
    main()
