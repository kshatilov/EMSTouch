import socket
import threading
import time

import EMSnewMcuDriver


class EMSServer:
    HOST = "0.0.0.0"
    PORT = 1488
    SERIAL = 'COM4'
    BAUD_RATE = 115200
    # EMS_START_CMD = b'start_ems'
    EMS_CONF_CMD = b'FE 7 2 20 250'
    # EMS_STOP_CMD = b'stop_ems'

    # EMS_START_CMD = EMSMcuDriver.EMSDriver("1").create_packet(EMSMcuDriver.Command.START_STOP_CONTROL, 0, 1)
    # EMS_STOP_CMD = EMSMcuDriver.EMSDriver("1").create_packet(EMSMcuDriver.Command.START_STOP_CONTROL, 0, 0)
    EMS_NET_START_CMD = "EMS_beg_def_000"
    EMS_NET_STOP_CMD = "EMS_end_def_000"

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
        # self.setup_serial()
        self.setup_socket()

    def setup_driver(self):
        try:
            self.driver = EMSnewMcuDriver.WaveformDriver('COM4', 115200)
            self.driver.connect()  # Open the serial port connection
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
                        data = conn.recv(len(self.EMS_NET_START_CMD.encode()))
                        if not data:
                            break
                        received = data.decode()
                        print("Received:", received)
                        if received == EMSServer.EMS_NET_START_CMD:
                            for channel in self.channels:
                                if self.driver:
                                    self.driver.start_channel(channel)
                                    for current_mA in range(30, 80, 5):
                                        print(f"Adjusting strength to {current_mA}...")
                                        self.driver.set_current(channel=0, current_mA=current_mA / 10)
                                        time.sleep(0.2)

                        if received == EMSServer.EMS_NET_STOP_CMD:
                            if self.driver:
                                self.driver.stop_all()
            except AttributeError as e:
                print(f"Components not initialized: {e}")
            except KeyboardInterrupt as e:
                print(f"Keyboard interrupt received, shutting down.: {e}")

if __name__ == "__main__":
    # server = EMSServer()
    # server.run()
    driver = EMSnewMcuDriver.WaveformDriver('/dev/tty.usbserial-MQZXDN', 115200)
    driver.connect()  # Open the serial port connection
    driver.configure_channel(
        channel=0,
        stimulation_period_us=16000,
        on_time_us=400,
        pos_neg_gap_us=50,
        strength_level=160,
        paired_switch_config=driver.STIMU_16
    )
    driver.start(0)

