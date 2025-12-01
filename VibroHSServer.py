import multiprocessing
import socket
import threading
import time

from pyomyo import Myo, emg_mode

from EMSServer import EMSServer


class VibroHSServer:
    HOST = "0.0.0.0"
    PORT = 1488

    def __init__(self):
        self.myo = Myo()
        self.sock = None
        self.isRunning = False
        self.detach = True

    def connect(self):
        self.myo.connect()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((VibroHSServer.HOST, VibroHSServer.PORT))
        self.sock.listen()

    def run(self):

        def handshake():
            timestamps = [
                [5, 15, 1],
                [15, 26, 1],

                [26, 34, 0.5],
                [34, 41, 0.5],

                [41, 48, 0.4],
                [48, 53, 0.4],

                [53, 62, 0.35],
                [62, 67, 0.35],
            ]

            k = 2
            time.sleep(10 / 60)
            vibro = {
                "1": 2,
                "0.5": 1,
                "0.4": 1,
                "0.35": 1
            }
            i = 0
            while True:
                if i > len(timestamps) - 1: break
                t1 = timestamps[i]
                t2 = timestamps[i + 1]
                if not self.isRunning:
                    break
                v = vibro[str(t1[2])]
                self.myo.vibrate(v)
                interval = (t2[1] - t1[0]) / 60 * k
                time.sleep(interval)
                i += 2

        t = threading.Thread()

        while True:
            conn, addr = self.sock.accept()
            with conn:
                while True:
                    data = conn.recv(len(EMSServer.EMS_NET_START_CMD.encode()))
                    if not data:
                        break
                    print("Received:", data.decode())
                    data = data.decode()
                    if data == EMSServer.EMS_NET_STOP_CMD:
                        self.isRunning = False
                        self.detach = True
                        t.join()
                    if t.is_alive():
                        print("Skipping command, previous still running")
                        continue
                    if EMSServer.EMS_NET_DST_CMD in data:
                        if self.detach:
                            t = threading.Thread(target=handshake)
                            self.isRunning = True
                            self.detach = False
                            t.start()


if __name__ == "__main__":
    server = VibroHSServer()
    server.connect()
    server.run()
