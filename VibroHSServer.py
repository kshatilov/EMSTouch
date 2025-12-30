import random
import socket
import struct
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
        self.trajectory = []
        self.file_name = "data/vibro_hs_trajectory_" + str(random.randint(1, 10000)) + "_"
        self.shakes = 0

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
        timer_thread = threading.Thread()

        while True:
            conn, addr = self.sock.accept()
            with conn:
                while True:
                    data = conn.recv(len(EMSServer.EMS_NET_START_CMD.encode()))
                    if not data:
                        break

                    # handshake trajectory recording
                    if "POS" == data[:3].decode():
                        def unpack(value):
                            return struct.unpack("<f", value)[0]

                        data = data[3:]
                        x = unpack(data[0:4])
                        y = unpack(data[4:8])
                        z = unpack(data[8:12])
                        if timer_thread.is_alive():
                            self.trajectory.append((x, y, z))

                    # stopping condition
                    else:
                        data = data.decode()
                        print("Command received:", data)
                        if data == EMSServer.EMS_NET_STOP_CMD:
                            self.isRunning = False
                            self.detach = True
                            t.join()
                            if len(self.trajectory) == 0:
                                continue
                            self.shakes += 1
                            with open(self.file_name + str(self.shakes) + ".txt", "w") as f:
                                for line in self.trajectory:
                                    for value in line:
                                        f.write(str(value) + " ")
                                    f.write("\n")

                        # skip
                        if t.is_alive():
                            print("Skipping command, previous still running")
                            continue

                        # starting condition
                        if "EMS_beg_hnd_" in data:
                            if self.detach:
                                t = threading.Thread(target=handshake)
                                self.isRunning = True
                                self.detach = False
                                t.start()

                                self.trajectory = []

                                def timer():
                                    slept = 0
                                    while True:
                                        if not self.isRunning:
                                            break
                                        if slept >= 4:
                                            break
                                        slept += 1
                                        time.sleep(1)

                                timer_thread = threading.Thread(target=timer)
                                timer_thread.start()


if __name__ == "__main__":
    server = VibroHSServer()
    server.connect()
    server.run()
