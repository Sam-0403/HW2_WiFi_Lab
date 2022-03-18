import socket
import numpy as np
import json
import datetime
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
from multiprocessing import Process, Array
import sys

def SensorDataParser(lis_of_sensor_data):
    HOST = "192.168.50.197"
    PORT = 65431

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen() 
        conn, addr = s.accept()

        with conn:
            print('Connected by', addr)
            while True:
                sensor_data = conn.recv(1024).decode('utf-8')
                print(sys.getsizeof(sensor_data))                 
                print('Received from socket server : ', sensor_data)
    
                try:
                    sensor_data = json.loads(sensor_data)
                    tst = dict()
                    if (type(sensor_data) != type(tst)):
                        print("Parsing error: sensor_data now should be a dictionary.")
                        break

                    lis_of_sensor_data[0] = float(sensor_data['x_a'])
                    lis_of_sensor_data[1] = float(sensor_data['y_a'])
                    lis_of_sensor_data[2] = float(sensor_data['z_a'])
                    lis_of_sensor_data[3] = float(sensor_data['x_g'])
                    lis_of_sensor_data[4] = float(sensor_data['y_g'])
                    lis_of_sensor_data[5] = float(sensor_data['z_g'])
                except:
                    print("Fail to Parse Data")
            # conn.close()
            # print('client closed connection')

if __name__ == '__main__':
    lis_of_sensor_data = Array('f', 6)
    p = Process(target=SensorDataParser, args=([lis_of_sensor_data]))
    p.start()

    HISTORY_SIZE = 20

    # Deque for X-Axis (time)
    x_vals = deque(maxlen=HISTORY_SIZE)

    # Deque for Y-Axis (accelerometer readings)
    accel_x = deque(maxlen=HISTORY_SIZE)
    accel_y = deque(maxlen=HISTORY_SIZE)
    accel_z = deque(maxlen=HISTORY_SIZE)

    gyro_x = deque(maxlen=HISTORY_SIZE)
    gyro_y = deque(maxlen=HISTORY_SIZE)
    gyro_z = deque(maxlen=HISTORY_SIZE)

    # Create 3 side-by-side subplots
    fig, ((ax1, ax2, ax3), (gx1, gx2, gx3)) = plt.subplots(2,3)

    # Automatically adjust subplot parameters for nicer padding between plots
    plt.tight_layout()


    def animate(i, lis_of_sensor_data):
        accel_data = lis_of_sensor_data[:3]
        gyro_data = lis_of_sensor_data[3:6]

        # Add the X/Y/Z values to the accel arrays
        accel_x.append(accel_data[0])
        accel_y.append(accel_data[1])
        accel_z.append(accel_data[2])
        
        # Add the X/Y/Z values to the gyro arrays
        gyro_x.append(gyro_data[0])
        gyro_y.append(gyro_data[1])
        gyro_z.append(gyro_data[2])

        # Grab the datetime, auto-range based on length of accel_x array
        x_vals = [datetime.datetime.now() + datetime.timedelta(seconds=i) for i in range(len(accel_x))]
        print(x_vals)

        # Clear all axis
        ax1.cla()
        ax2.cla()
        ax3.cla()

        gx1.cla()
        gx2.cla()
        gx3.cla()

        # Set grid titles
        ax1.set_title('acc X', fontsize=10)
        ax2.set_title('acc Y', fontsize=10)
        ax3.set_title('acc Z', fontsize=10)

        gx1.set_title('gyro X', fontsize=10)
        gx2.set_title('gyro Y', fontsize=10)
        gx3.set_title('gyro Z', fontsize=10)

        # Enable subplot grid lines
        ax1.grid(True, linewidth=0.5, linestyle=':')
        ax2.grid(True, linewidth=0.5, linestyle=':')   
        ax3.grid(True, linewidth=0.5, linestyle=':')

        gx1.grid(True, linewidth=0.5, linestyle=':')
        gx2.grid(True, linewidth=0.5, linestyle=':')   
        gx3.grid(True, linewidth=0.5, linestyle=':')

        # Display the sub-plots
        ax1.plot(x_vals, accel_x, color='r')
        ax2.plot(x_vals, accel_y, color='g')
        ax3.plot(x_vals, accel_z, color='b')

        gx1.plot(x_vals, gyro_x, color='r')
        gx2.plot(x_vals, gyro_y, color='g')
        gx3.plot(x_vals, gyro_z, color='b')

        # Pause the plot for INTERVAL seconds 
        plt.pause(0.1)

    ani = FuncAnimation(fig, animate, fargs=([lis_of_sensor_data]))
    plt.show()
    p.join()