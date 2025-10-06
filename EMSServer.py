import socket
import serial
import time

try:
    ser = serial.Serial('COM5', baudrate=115200, timeout=1)
except serial.SerialException as e:
    print(e)
    ser = None
    

HOST = "0.0.0.0"   # Listen on all interfaces
PORT = 1488       # Port to listen on (use >1024)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))     # Bind to address and port
    s.listen()               # Start listening for connections
    print(f"Server listening on {HOST}:{PORT}")
    
    try:
        while True:
            conn, addr = s.accept()  # Accept a client connection
            with conn:
                print(f"Connected by {addr}")
                while True:
                    data = conn.recv(1024)   # Receive up to 1024 bytes
                    if not data:
                        break
                    received = data.decode()
                    print("Received:", received)
                    if received == "touch":
                        if not ser:
                            continue
                        ser.write(b'start_ems')
                        ser.write(b'FE 7 2 20 250') 
                        #time.sleep(0.6)
                        #ser.write(b'stop_ems')
                    if received == "end":
                        if not ser:
                            continue
                        ser.write(b'stop_ems')
                    
                    #conn.sendall(data) 
                    
    except KeyboardInterrupt:
        print("Exiting ..")
