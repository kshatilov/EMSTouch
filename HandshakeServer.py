import socket
import threading
import time

import EMSnewMcuDriver
from EMSServer import EMSServer


class HSServer:
    HOST = "0.0.0.0"
    PORT = 1488
    SERIAL = 'COM4'
    BAUD_RATE = 115200

    def __init__(self):
        self.sock = None
        self.isPlaying = False
        self.serial_port = HSServer.SERIAL
        self.baud_rate = HSServer.BAUD_RATE
        self.ser = None
        self.driver = None
        self.channels = [0]

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.bind((HSServer.HOST, HSServer.PORT))
        self.sock.listen()

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

    def run(self):

        def move(start, stop, up):
            ch = 0 if up else 1
            self.driver.start(ch)
            self.driver.set_current(channel=ch, current_mA=EMSServer.EMS_MAX_CURRENT)
            time.sleep((stop - start))
            self.driver.stop(ch)

        def gentle_move(start, stop, up):
            ch = 0 if up else 1
            self.driver.start(ch)
            self.driver.set_current(channel=ch, current_mA=EMSServer.EMS_MIN_CURRENT)
            time.sleep((stop - start) / 2)
            self.driver.set_current(channel=ch, current_mA=EMSServer.EMS_MAX_CURRENT)
            time.sleep((stop - start) / 2)
            self.driver.stop(ch)

        def shaking():
            time.sleep(5 / 60)
            direction = False # down

            timestamps = [
                [5, 18],
                [18, 28],
                [28, 38],
                [38, 43],
                [43, 50],
                [50, 58],
                [58, 65],
                [65, 69],
                [69, 80]
            ]
            i = 0

            # grab
            self.driver.start(2)
            self.driver.set_current(channel=2, current_mA=EMSServer.EMS_MIN_CURRENT)

            while self.isPlaying:
                # move(timestamps[i][0] / 60, timestamps[i][1] / 60, direction)
                gentle_move(timestamps[i][0] / 60, timestamps[i][1] / 60, direction)
                direction = not direction
                i += 1
                if i >= len(timestamps): break

            for ch in [0, 1, 2]:
                self.driver.stop(ch)

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

                    if EMSServer.EMS_NET_DST_CMD in data:
                        if self.isPlaying or t.is_alive():
                            continue
                        self.isPlaying = True
                        t = threading.Thread(target=shaking())
                        t.start()

                    elif data == EMSServer.EMS_NET_STOP_CMD:
                        self.isPlaying = False
                        for channel in [0, 1, 2]:
                            self.driver.stop(channel)


if __name__ == "__main__":
    server = HSServer()
    server.connect()
    server.run()