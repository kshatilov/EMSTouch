import random
import socket
import threading
import time
import struct

import numpy as np

import EMSnewMcuDriver
from EMSServer import EMSServer


class HSServer:
    HOST = "0.0.0.0"
    PORT = 1488
    SERIAL = 'COM5'
    BAUD_RATE = 115200

    def __init__(self):
        self.sock = None
        self.isPlaying = False
        self.serial_port = HSServer.SERIAL
        self.baud_rate = HSServer.BAUD_RATE
        self.ser = None
        self.driver = None
        self.channels = [0]
        self.trajectory = []
        self.file_name = "data/hs_trajectory_" + str(random.randint(1, 10000)) + "_"
        self.shakes = 0
        self.ems = []
        self.soften = 1
        self.animation = []

    def load_ems(self):
        with open("ems.txt", "r") as f:
            for line in f:
                self.ems.append(float(line.strip()))

    def load_animation(self):
        with open("animation.txt", "r") as f:
            for line in f:
                value = float(line.strip().split(" ")[1])
                self.animation.append(value)
        self.animation = np.array(self.animation)
        self.animation = np.diff(self.animation)

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((HSServer.HOST, HSServer.PORT))
        self.sock.listen()
        self.load_ems()
        self.load_animation()

        try:
            self.driver = EMSnewMcuDriver.WaveformDriver(HSServer.SERIAL, HSServer.BAUD_RATE)
            self.driver.connect()
            for channel in self.channels:
                self.driver.configure_channel(
                    channel=channel,
                    stimulation_period_us=16000,
                    on_time_us=400,
                    pos_neg_gap_us=50,
                    strength_level=160,
                    paired_switch_config=self.driver.STIMU_16
                )
        except Exception as e:
            print(f"Error initializing EMS driver: {e}")
            self.driver = None

    def adjust(self):
        a = np.diff(np.array(self.trajectory)[:, 1])
        b = self.animation
        if len(a) < 80: return
        min_len = min(len(a), len(b))
        a = a[:min_len]
        b = b[:min_len]
        a = a[15:]
        b = b[15:]
        self.soften *= max(b) / max(a)
        self.soften = min(1.0, self.soften)
        self.soften = max(0.6, self.soften)
        print(f"Adjusting intensity: soften={self.soften}")

    def run(self):

        def move(start, stop, up):
            ch = 0 if up else 1
            self.driver.start(ch)
            self.driver.set_current(channel=ch, current_mA=EMSServer.EMS_MIN_CURRENT)
            time.sleep((stop - start))
            self.driver.stop(ch)

        def gentle_move(start, stop, up):
            ch = 0 if up else 1
            step = 0.005
            steps = int((stop - start) / step / 2)
            min = 2
            max = 8
            self.driver.start(ch)
            for i in range(steps):
                curr = min + (max - min) * (i / steps)
                self.driver.set_current(channel=ch, current_mA=curr)
                time.sleep(step)
            for i in range(steps):
                curr = max - (max - min) * (i / steps)
                self.driver.set_current(channel=ch, current_mA=curr)
                time.sleep(step)
            self.driver.stop(ch)

        def const_move(start, stop, up, scale):
            ch = 0 if up else 1
            step = 0.01
            steps = int((stop - start) / step / 2)
            min = 5
            _max = 9
            max = _max * scale
            max = _max if max > _max else max
            curr = 0

            self.driver.start(ch)
            for i in range(steps):
                if not self.isPlaying: return
                curr = min + (max - min) * (i / steps)
                curr *= self.soften
                self.driver.set_current(channel=ch, current_mA=curr)
                time.sleep(step)
            for i in range(steps):
                if not self.isPlaying: return
                curr = max - (max - min) * (i / steps)
                curr *= self.soften
                self.driver.set_current(channel=ch, current_mA=curr)
                time.sleep(step)
            self.driver.stop(ch)

        def new_shaking():
            k = 2
            # time.sleep(10 / 60 * k)
            self.driver.start(2)
            self.driver.set_current(channel=2, current_mA=EMSServer.EMS_MIN_CURRENT)
            ch = 0
            for current in self.ems:
                if not self.isPlaying:
                    break
                up = current > 0
                curr = abs(current) * self.soften
                ch = 0 if up else 1
                self.driver.start(ch)
                self.driver.set_current(channel=ch, current_mA=curr)
                # print(ch, curr)
                time.sleep(1 / 60 * k)
                self.driver.stop(ch)

            for ch in [0, 1, 2]:
                self.driver.stop(ch)

        def shaking():
            k = 2
            # time.sleep(10 / 60 * k)
            direction = False  # down

            timestamps = [
                [5, 15, 0.9],
                [15, 26, 0.9],
                [26, 34, 0.4],
                [34, 41, 0.4],
                [41, 48, 0.3],
                [48, 53, 0.3],
                [53, 62, 0.3],
                [62, 67, 0.3],
                [67, 90, 0.3],
            ]
            i = 0

            # grab
            self.driver.start(2)
            self.driver.set_current(channel=2, current_mA=EMSServer.EMS_MIN_CURRENT / 2)

            while self.isPlaying:

                const_move(timestamps[i][0] / 60 * k, timestamps[i][1] / 60 * k, direction, timestamps[i][2])
                direction = not direction
                i += 1
                if i >= len(timestamps): break

            for ch in [0, 1, 2]:
                self.driver.stop(ch)

        t = threading.Thread()
        timer_thread = threading.Thread()

        while True:
            conn, addr = self.sock.accept()
            with conn:
                while True:
                    data = conn.recv(15)
                    if not data:
                        break

                    if "POS" == data[:3].decode():
                        def unpack(value):
                            return struct.unpack("<f", value)[0]

                        data = data[3:]
                        x = unpack(data[0:4])
                        y = unpack(data[4:8])
                        z = unpack(data[8:12])
                        # print(f"Position received: x={x}, y={y}, z={z}")
                        if timer_thread.is_alive():
                            self.trajectory.append((x, y, z))

                    else:
                        data = data.decode()
                        print("Received:", data)

                        if "EMS_beg_hnd_" in data:
                            if self.isPlaying or t.is_alive():
                                continue
                            self.isPlaying = True
                            self.trajectory = []
                            t = threading.Thread(target=shaking)
                            # t = threading.Thread(target=new_shaking)
                            t.start()

                            def timer():
                                slept = 0
                                while True:
                                    if not self.isPlaying:
                                        break
                                    if slept >= 5:
                                        break
                                    slept += 1
                                    time.sleep(1)

                            timer_thread = threading.Thread(target=timer)
                            timer_thread.start()

                        elif data == EMSServer.EMS_NET_STOP_CMD:
                            self.isPlaying = False
                            for channel in [0, 1, 2]:
                                self.driver.stop(channel)
                            if len(self.trajectory) == 0:
                                continue
                            self.shakes += 1
                            with open(self.file_name + str(self.shakes) + "_" + str(self.soften) + ".txt", "w") as f:
                                for line in self.trajectory:
                                    for value in line:
                                        f.write(str(value) + " ")
                                    f.write("\n")

                            # self.adjust()

                        elif "EMS_beg_tst_00" in data:
                            channel = int(data[len(data) - 1])
                            self.driver.start(channel)
                            for current_mA in range(20, 70, 5):
                                self.driver.set_current(channel=channel, current_mA=current_mA / 10 * self.soften)
                                time.sleep(0.1)

                            for current_mA in range(70, 20, -5):
                                self.driver.set_current(channel=channel, current_mA=current_mA / 10 * self.soften)
                                time.sleep(0.1)

                            self.driver.stop(channel)


if __name__ == "__main__":
    server = HSServer()
    server.connect()
    server.run()
