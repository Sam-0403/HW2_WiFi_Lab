import socket
import numpy as np
import json
import time
import random
import ast # string to dict
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from matplotlib.ticker import FuncFormatter
from multiprocessing import Process, Queue

def Animate(i):  
    new_data = q.get()
    print('Animate: ', new_data)
    for i, line in enumerate(lines):
        data[i].append(new_data[i])
        line.set_ydata(data[i])
    return lines

def init():
    for i, line in enumerate(lines):
        line.set_ydata([np.nan] * len(x))
    return lines

def SensorDataParser(q):
    # Socket config, change the HOST IP and PORT corresponding to testing-client or mbed
    HOST_ENV = {"dev": "192.168.50.197", "local_test": "127.0.0.1"}
    HOST = HOST_ENV["dev"] # Standard loopback interface address
    PORT = 65431 # Port to listen on (use ports > 1023)

    # Run server and data renderer
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(5) # why 5: https://ask.csdn.net/questions/684321
        conn, addr = s.accept()

        with conn:
            print('Connected by', addr)
            while True:
                sensor_data = conn.recv(1024).decode('utf8')
                if not sensor_data: # client disconnects
                    break
                print('Received from socket server : ', sensor_data)
                    
                # print received data
                try:
                    sensor_data = ast.literal_eval(sensor_data)
                except:
                    print("Fail to parse data")
                
                # Add next value
                print(sensor_data)
                q.put(list(sensor_data.values()))
            
            conn.close()
            print('client closed connection')

if __name__ == '__main__':
    # Socket as child process asymchronously catching data
    q = Queue()
    p = Process(target=SensorDataParser, args=(q,))
    p.start()

    # Matpolib config, i from 0 to 3 allows x y z sensor data input.
    max_sample_num = 10
    x = np.arange(0, max_sample_num)

    data = []
    for i in range(3):
        data.append(deque(np.zeros(max_sample_num), maxlen=max_sample_num))  # hold the last 10 values

    ylim_low = [-500, -500, 0]
    ylim_up = [500, 500, 1500]
    lines = []
    fig, axes = plt.subplots(3)
    for i, ax in enumerate(axes):
        ax.set_ylim(ylim_low[i], ylim_up[i])
        ax.set_xlim(0, max_sample_num)
        l, = ax.plot(x, np.zeros(max_sample_num))
        lines.append(l)
        ax.xaxis.set_major_formatter(FuncFormatter(lambda x, pos: '{:.0f}s'.format(max_sample_num - x - 1))) #axis format
    plt.xlabel('Seconds ago')
    
    # List of Animation objects for tracking
    ani = animation.FuncAnimation(fig, Animate, init_func=init, interval=50, blit=True, save_count=10)
    plt.show()

    p.join() # block and wait for child process end
    print("server end. close")