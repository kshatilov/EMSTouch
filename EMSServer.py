import socket
import serial

import EMSMcuDriver


class EMSServer:
    HOST = "0.0.0.0"
    PORT = 1488
    SERIAL = 'COM4'
    BAUD_RATE = 115200
    # EMS_START_CMD = b'start_ems'
    EMS_CONF_CMD = b'FE 7 2 20 250'
    # EMS_STOP_CMD = b'stop_ems'

    EMS_START_CMD = EMSMcuDriver.EMSDriver("1").create_packet(EMSMcuDriver.Command.START_STOP_CONTROL, 0, 1)
    EMS_STOP_CMD = EMSMcuDriver.EMSDriver("1").create_packet(EMSMcuDriver.Command.START_STOP_CONTROL, 0, 0)


    def __init__(self, host=HOST, port=PORT, serial_port=SERIAL, baud_rate=BAUD_RATE):
        self.host = host
        self.port = port
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.ser = None
        self.sock = None
        self.setup_serial()
        self.setup_socket()

        self.start_cmd = EMSServer.EMS_START_CMD
        self.conf_cmd = EMSServer.EMS_CONF_CMD
        self.stop_cmd = EMSServer.EMS_STOP_CMD

    def setup_serial(self):
        try:
            self.ser = serial.Serial(self.serial_port, baudrate=self.baud_rate, timeout=1)
            print(f"Serial connection established on {self.serial_port} at {self.baud_rate} baud.")
        except serial.SerialException as e:
            print(f"Error opening serial port {self.serial_port}: {e}")
            self.ser = None

    def setup_socket(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.bind((self.host, self.port))
            self.sock.listen()
            print(f"Server listening on {self.host}:{self.port}")
        except socket.error as e:
            print(f"Socket error: {e}")
            self.sock = None

    def run(self):
        while True:
            try:
                conn, addr = self.sock.accept()
                with conn:
                    print(f"Connected by {addr}")
                    while True:
                        data = conn.recv(1024)
                        if not data:
                            break
                        received = data.decode()
                        print("Received:", received)
                        if received == "touch":
                            self.ser.write(self.start_cmd)
                        if received == "end":
                            self.ser.write(self.stop_cmd)
            except AttributeError as e:
                print(f"Components not initialized: {e}")
            except KeyboardInterrupt as e:
                print(f"Keyboard interrupt received, shutting down.: {e}")

if __name__ == "__main__":
    server = EMSServer()
    server.run()


