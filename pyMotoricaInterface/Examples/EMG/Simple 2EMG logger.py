import MotoricaInterface as mi; assert(mi.__version__=="0.0.8") # last time launched
from MotoricaInterface import SerialInterface_2EMG
import numpy as np
import time
import pandas as pd
import click
import socket

# Plotting to teleplot
teleplotAddr = ("127.0.0.1",47269)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
def sendTelemetry(name, value):
    now = time.time() * 1000
    msg = name+":"+str(now)+":"+str(value)+"|g"
    sock.sendto(msg.encode(), teleplotAddr)
# / Plotting to teleplot    

@click.command()
@click.option('--port', required=True, help='Serial port to connect to the EMG device')
def main(port):
    # Initialize the SerialInterface_2EMG with the provided port

    print(f"Connecting to EMG device on port {port}...")
    ser = SerialInterface_2EMG(port, 
                            debug=False, 
                            logging_enabled=False, 
                            timeout=0.5)
    
    ser.start()
    time.sleep(1)


    ser.DEBUG = False
    pack_counter = 0;
    t_start = time.time()
    t_last_pack = 0;
    while_True = True

    ser.flush_rx_packs()

    states = ["Neutral", 'RIGHT', 'LEFT']
    while while_True:
        try:
            if (len(ser.rx_pack)):
                now = time.time()
                ts, RX = ser.rx_pack.pop()
                t_last_pack = now
                pack_counter+=1

                # emg data
                if (RX[0] == 0x01) & (RX[1] == 0x20):
                    print(f'N={pack_counter}\tt={time.time() - t_start:.2f}s'
                            f'\tRX: {len(ser.rx_pack)}'
                            f'\tEMG_A {RX[3]:03d}\tEMG_B {RX[4]:03d}')
                    sendTelemetry('EMG_A', RX[3])
                    sendTelemetry('EMG_B', RX[4])

                # emg floats
                if (RX[0] == 0x01) & (RX[1] == 0x22):
                    [emg_A, emg_B] = np.frombuffer(RX[3:(3+8)], dtype=np.float32).round(3)

                    print(f'N={pack_counter}\tt={time.time() - t_start:.2f}s'
                            f'\tRX: {len(ser.rx_pack)}'
                            f'\tEMG_A {emg_A:.03f}\tEMG_B {emg_B:.03f}')
                    sendTelemetry('EMG_A', emg_A)
                    sendTelemetry('EMG_B', emg_B)

                # emg state decoded
                if (RX[0] == 0x01) & (RX[1] == 0x83):
                    print(f'{pack_counter}\t{ time.time() - t_start:.2f}\tEMG STATE {RX[3]:01d} {states[RX[3]]}')
                    sendTelemetry('STATE', RX[3])

            else:
                time.sleep(0.01)

        except Exception as e:
            print(f"Error: {e}")
            break
    
    ser.stop()
    
    

if __name__ == '__main__':
    main()
