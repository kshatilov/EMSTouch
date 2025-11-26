import multiprocessing
import socket
import threading
import time

from pyomyo import Myo, emg_mode

from EMSServer import EMSServer


class EMGServer:
    HOST = "0.0.0.0"
    PORT = 1488
    def __init__(self):
        self.myo = Myo()
        self.sock = None

    def connect(self):
        self.myo.connect()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((EMGServer.HOST, EMGServer.PORT))
        self.sock.listen()

    def run(self):
        def sleeper():
            time.sleep(0.5)

        t = threading.Thread()

        while True:
            conn, addr = self.sock.accept()
            with conn:
                while True:
                    data = conn.recv(len(EMSServer.EMS_NET_START_CMD.encode()))
                    if t.is_alive():
                        print("Skipping command, previous still running")
                        continue
                    if not data:
                        break
                    print("Received:", data.decode())
                    data = data.decode()
                    if EMSServer.EMS_NET_DST_CMD in data:
                        dst = int(data.replace(EMSServer.EMS_NET_DST_CMD, "").replace("E", "0"))

                    if 0 <= dst <= 500:
                        self.myo.vibrate(1)
                        t = threading.Thread(target=sleeper)
                        t.start()

if __name__ == "__main__":
    server = EMGServer()
    server.connect()
    server.run()