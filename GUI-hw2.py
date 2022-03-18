import socket
import json
import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from multiprocessing import Process, Array
import sys
import time

def sensor_data_receiver(lis_of_sensor_data):
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
                sensor_data = conn.recv(200).decode('utf-8')
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

if __name__ == '__main__':
    lis_of_sensor_data = Array('f', 6)
    p = Process(target=sensor_data_receiver, args=([lis_of_sensor_data]))
    p.start()
    
    value_of_time, val_of_acceleration_of_x, val_of_acceleration_of_y, val_of_acceleration_of_z, val_of_gyro_x, val_of_gyro_y, val_of_gyro_z = deque(maxlen=20), deque(maxlen=20), deque(maxlen=20), deque(maxlen=20), deque(maxlen=20), deque(maxlen=20), deque(maxlen=20)
    
    fig, axs = plt.subplots(2, 3)
    plt.tight_layout()

    def dynamic_plotting(i, lis_of_sensor_data):

        for s in range(0,6):
            if s == 0:
                val_of_acceleration_of_x.append(lis_of_sensor_data[s])
            if s == 1:
                val_of_acceleration_of_y.append(lis_of_sensor_data[s])
            if s == 2:
                val_of_acceleration_of_z.append(lis_of_sensor_data[s])
            if s == 3:
                val_of_gyro_x.append(lis_of_sensor_data[s])
            if s == 4:
                val_of_gyro_y.append(lis_of_sensor_data[s])
            if s == 5:
                val_of_gyro_z.append(lis_of_sensor_data[s])

        value_of_time = [dt.datetime.now() + dt.timedelta(seconds=i) for i in range(len(val_of_acceleration_of_x))]
        
        mapping_of_title = dict()
        mapping_of_title[(0,0)] = 'Acceleration of x'
        mapping_of_title[(0,1)] = 'Acceleration of y'
        mapping_of_title[(0,2)] = 'Acceleration of z'
        mapping_of_title[(1,0)] = 'Gyro-x'
        mapping_of_title[(1,1)] = 'Gyro-y'
        mapping_of_title[(1,2)] = 'Gyro-z'
        
        mapping_of_those_list_of_values = dict()
        mapping_of_those_list_of_values[(0,0)] = val_of_acceleration_of_x
        mapping_of_those_list_of_values[(0,1)] = val_of_acceleration_of_y
        mapping_of_those_list_of_values[(0,2)] = val_of_acceleration_of_z
        mapping_of_those_list_of_values[(1,0)] = val_of_gyro_x
        mapping_of_those_list_of_values[(1,1)] = val_of_gyro_y
        mapping_of_those_list_of_values[(1,2)] = val_of_gyro_z
        
        mapping_of_colors = dict()
        mapping_of_colors[(0,0)] = 'b'
        mapping_of_colors[(0,1)] = 'g'
        mapping_of_colors[(0,2)] = 'r'
        mapping_of_colors[(1,0)] = 'c'
        mapping_of_colors[(1,1)] = 'y'
        mapping_of_colors[(1,2)] = 'k'     

        
        for index in range(0,2):
            for j in range(0,3):
                axs[index,j].cla()
                axs[index,j].set_title(mapping_of_title[(index,j)])
                axs[index,j].grid(True, linewidth=0.5, linestyle=':')
        for index in range(0,2):
            for j in range(0,3):
                axs[index,j].plot(value_of_time, mapping_of_those_list_of_values[(index,j)], mapping_of_colors[(index,j)])
        
        plt.pause(0.1)

    ani = animation.FuncAnimation(fig, dynamic_plotting, fargs=([lis_of_sensor_data]))
    plt.show()
    p.join()
