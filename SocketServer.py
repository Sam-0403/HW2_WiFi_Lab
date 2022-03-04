import socket
import numpy as np
import json
import time
import random
HOST = '192.168.2.103'
PORT = 65431
# IP address
# Port to listen on (use ports > 1023)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(1024).decode('utf-8')
            print('Received from socket server : ')
            data = data.split()
            print(data)