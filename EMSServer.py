import socket
import threading
import time
import select

import EMSnewMcuDriver


class EMSServer:
    HOST = "0.0.0.0"
    PORT = 1488
    SERIAL = 'COM4'
    BAUD_RATE = 115200

    EMS_NET_START_CMD = "EMS_beg_def_000"
    EMS_NET_STOP_CMD = "EMS_end_def_000"
    EMS_NET_DST_CMD = "EMS_beg_dst_"

    EMS_MAX_DST = 1000
    EMS_MIN_CURRENT = 4
    EMS_MAX_CURRENT = 8

    def __init__(self, host=HOST, port=PORT, serial_port=SERIAL, baud_rate=BAUD_RATE):
        self.host = host
        self.port = port
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.ser = None
        self.sock = None
        self.driver = None
        self.channels = [0]
        self.setup_driver()
        self.setup_socket()

    def setup_driver(self):
        try:
            self.driver = EMSnewMcuDriver.WaveformDriver(EMSServer.SERIAL, 115200)
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

    def setup_socket(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.bind((self.host, self.port))
            self.sock.listen()
            # self.sock.setblocking(False)
            print(f"Server listening on {self.host}:{self.port}")
        except socket.error as e:
            print(f"Socket error: {e}")
            self.sock = None

    def run(self):
        while True:

            conn, addr = self.sock.accept()
            with conn:
                print(f"Connected by {addr}")
                while True:
                    try:
                        data = conn.recv(len(self.EMS_NET_START_CMD.encode()))
                        if not data:
                            break
                        received = data.decode()
                        print("Received:", received)

                        if received == EMSServer.EMS_NET_START_CMD:
                            for channel in self.channels:
                                self.driver.start(channel)
                                for current_mA in range(50, 80, 5):
                                    print(f"Adjusting strength to {current_mA}...")
                                    self.driver.set_current(channel=channel, current_mA=current_mA / 10)
                                    time.sleep(0.2)

                        elif EMSServer.EMS_NET_DST_CMD in received:
                            dst = int(received.replace(EMSServer.EMS_NET_DST_CMD, "").replace("E", "0"))
                            if 0 <= dst <= EMSServer.EMS_MAX_DST:
                                current_mA = EMSServer.EMS_MIN_CURRENT + (EMSServer.EMS_MAX_DST - dst) / EMSServer.EMS_MAX_DST * (EMSServer.EMS_MAX_CURRENT - EMSServer.EMS_MIN_CURRENT)
                                if EMSServer.EMS_MAX_CURRENT >= current_mA >= EMSServer.EMS_MIN_CURRENT:
                                    for channel in self.channels:
                                        self.driver.start(channel)
                                        self.driver.set_current(channel=channel, current_mA=current_mA)

                        elif received == EMSServer.EMS_NET_STOP_CMD:
                            for channel in self.channels:
                                self.driver.stop(channel)

                    except AttributeError as e:
                        print(f"Components not initialized: {e}")
                    except KeyboardInterrupt as e:
                        print(f"Keyboard interrupt received, shutting down.: {e}")
                    except Exception as e:
                        print(f"Unhandled exception: {e}")

if __name__ == "__main__":
    server = EMSServer()
    server.run()


