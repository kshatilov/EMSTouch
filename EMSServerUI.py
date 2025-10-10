import threading
import time
import tkinter as tk
from tkinter import scrolledtext

from tornado.gen import sleep

from EMSServer import EMSServer


class EMSServerUI:
    def __init__(self):

        self.server = None

        self.root = tk.Tk()
        self.root.title("EMS Server Control")

        # Main frame with horizontal padding
        self.main_frame = tk.Frame(self.root)
        self.main_frame.grid(row=0, column=0, padx=16, pady=16, sticky="nsew")

        # Status areas
        tk.Label(self.main_frame, text="Server Status:").grid(row=3, column=0, sticky="w")
        self.server_status = tk.Text(self.main_frame, width=25, height=1)
        self.server_status.grid(row=3, column=1, padx=5, pady=2)

        tk.Label(self.main_frame, text="Serial Connection:").grid(row=4, column=0, sticky="w")
        self.serial_status = tk.Text(self.main_frame, width=25, height=1)
        self.serial_status.grid(row=4, column=1, padx=5, pady=2)
        self.serial_status.bind("<<Modified>>", lambda e: self.update_serial())

        tk.Label(self.main_frame, text="EMS cmd:").grid(row=5, column=0, sticky="w")
        self.main_cmd = tk.Text(self.main_frame, width=25, height=1)
        self.main_cmd.grid(row=5, column=1, padx=5, pady=2)
        self.main_cmd.bind("<<Modified>>", lambda e: self.update_cmd())

        tk.Button(self.main_frame, text="Test EMS", command=self.test_cmd).grid(row=6, column=0, columnspan=2, pady=10)

        self.start_server()

    def test_cmd(self):
        pass
        if self.server and self.server.ser.is_open:
            with threading.Lock():
                self.server.ser.write(self.server.EMS_START_CMD)
                time.sleep(0.6)
                self.server.ser.write(self.server.EMS_STOP_CMD)
                self.server.ser.flush()

    def update_cmd(self):
        cmd = self.main_cmd.get("1.0", tk.END).strip()
        if cmd:
            self.server.start_cmd = bytes.fromhex(cmd)
        self.main_cmd.edit_modified(False)

    def update_serial(self):
        serial = self.serial_status.get("1.0", tk.END).split("@")
        self.server.serial_port = serial[0]
        self.server.baud_rate = int(serial[1])
        if self.server.ser:
            self.server.ser.close()
        self.server.setup_serial()
        self.serial_status.edit_modified(False)

    def start_server(self):
        self.server = EMSServer()
        self.server_status.insert(tk.END, self.server.host + ":" + str(self.server.port))
        self.serial_status.insert(tk.END, self.server.serial_port + "@" + str(self.server.baud_rate))
        self.main_cmd.insert(tk.END, self.server.start_cmd.hex())
        thread = threading.Thread(target=self.server.run, daemon=True)
        thread.start()

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    ui = EMSServerUI()
    ui.run()