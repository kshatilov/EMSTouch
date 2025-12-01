import threading
import time

from pyomyo import Myo, emg_mode
import matplotlib.pyplot as plt
import numpy as np

import EMSnewMcuDriver


class MirrorServer:
    SERIAL = 'COM5'
    BAUD_RATE = 115200

    def __init__(self):
        self.myo = Myo(mode=emg_mode.PREPROCESSED)
        self.isRunning = False
        self.emg = []
        self.imu = []
        self.driver = None

    def connect(self):
        self.myo.add_emg_handler(self.emg_handler)
        self.myo.add_imu_handler(self.imu_handler)
        self.myo.connect()

        try:
            self.driver = EMSnewMcuDriver.WaveformDriver(MirrorServer.SERIAL, MirrorServer.BAUD_RATE)
            self.driver.connect()
            for channel in [0]:
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

    def emg_handler(self, emg, moving, times=[]):
        self.emg.append(np.sum(emg[1:]))

        def grip_control():
            _emg = np.sum(emg[1:])
            _min = 1500
            if _emg > _min:
                a = 8
                b = 2
                _max = 3000
                intensity = (a - b) * (_emg - _min) / (_max - _min) + b
                intensity = min(a, intensity)
                self.driver.set_current(channel=2, current_mA=intensity)
                self.driver.start(channel=2)
                time.sleep(0.05)
                self.driver.stop(channel=2)

        threading.Thread(target=grip_control).start()

    def imu_handler(self, quat, acc, gyro):
        def move_control():
            last = self.imu[-1] if len(self.imu) > 0 else 0
            if last != 0 and self.driver is not None:
                diff = np.sum(quat) - last
                _min = 50
                _max = 1000
                a = 8
                b = 2
                if diff > _min:
                    intensity = (a - b) * (diff - _min) / (_max - _min) + b
                    intensity = min(a, intensity)
                    print(intensity)
                    self.driver.set_current(channel=0, current_mA=intensity)
                    self.driver.start(channel=0)
                    time.sleep(0.05)
                    self.driver.stop(channel=0)
                if diff < -_min:
                    intensity = (a - b) * (-diff - _min) / (_max - _min) + b
                    intensity = min(a, intensity)
                    print(intensity)
                    self.driver.set_current(channel=1, current_mA=intensity)
                    self.driver.start(channel=1)
                    time.sleep(0.05)
                    self.driver.stop(channel=1)

        threading.Thread(target=move_control).start()
        self.imu.append(np.sum(quat))

    def run(self):
        t = threading.Thread(target=self.run_myo)
        t.start()
        # time.sleep(20)
        # self.isRunning = False

    def run_myo(self):
        self.isRunning = True
        while self.isRunning:
            self.myo.run()

    def plot_data(self):
        fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(10, 4))

        ax1.set_title('EMG Data')
        ax1.plot(self.emg)

        ax2.set_title('IMU')
        ax2.plot(self.imu)

        der = np.diff(self.imu)
        ax3.set_title('IMU Derivative')
        ax3.plot(der)

        plt.tight_layout()
        plt.show()


if __name__ == '__main__':
    server = MirrorServer()
    server.connect()
    server.run()
    # server.plot_data()
