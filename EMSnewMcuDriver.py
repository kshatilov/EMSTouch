#!/usr/bin/env python3
"""
UART Serial Driver for CMSDK Waveform Generator Control
========================================================

This driver implements the 9-byte message frame protocol for controlling
the microcontroller's waveform generator channels (STIMU 8-15).

Message Frame Format (9 bytes):
    Byte 0:     0x55        Header byte 1 (sync)
    Byte 1:     0x55        Header byte 2 (sync)
    Byte 2:     CMD         Command byte
    Byte 3:     CHANNEL     Channel select (0-7 for STIMU 8-15)
    Byte 4-7:   DATA[4]     32-bit data (little-endian)
    Byte 8:     CRC8        CRC8-MAXIM checksum of bytes 0-7

Command Groups:
    0x1x: Version inquiry
    0x2x: Start/Stop control
    0x5x: Register configuration
        0x50: STIMULATION_PERIOD_US (32 bits)
        0x51: ON_TIME_US (32 bits)
        0x52: POS_NEG_GAP_US (0-255)
        0x53: STRENGTH_LEVEL (0-255)
        0x54: PAIRED_SWITCH_CONFIG (8 bits, STIMU 16-23)

Usage Example:
    driver = WaveformDriver('COM3', baudrate=115200)
    driver.connect()
    
    # Configure channel 0 (STIMU 8)
    driver.set_stimulation_period(channel=0, period_us=20000)
    driver.set_on_time(channel=0, on_time_us=400)
    driver.set_pos_neg_gap(channel=0, gap_us=50)
    driver.set_strength_level(channel=0, level=160)
    driver.set_paired_switch(channel=0, config=0x01)
    
    # Start stimulation
    driver.start(channel=0)
    
    # Stop stimulation
    driver.stop(channel=0)
    
    driver.disconnect()
"""

import serial
import struct
import time
from typing import Optional


class WaveformDriver:
    """Serial driver for CMSDK waveform generator control."""
    
    # Frame constants
    FRAME_HEADER1 = 0x55
    FRAME_HEADER2 = 0x55
    FRAME_SIZE = 9
    
    # Command groups
    CMD_VERSION = 0x10
    CMD_START = 0x20
    CMD_STOP = 0x21
    CMD_STIMULATION_PERIOD = 0x50
    CMD_ON_TIME = 0x51
    CMD_POS_NEG_GAP = 0x52
    CMD_STRENGTH_LEVEL = 0x53
    CMD_PAIRED_SWITCH = 0x54
    
    # Channel definitions (0-7 maps to STIMU 8-15)
    MIN_CHANNEL = 0
    MAX_CHANNEL = 7
    
    # Paired switch bits (STIMU 16-23)
    STIMU_16 = 0x01
    STIMU_17 = 0x02
    STIMU_18 = 0x04
    STIMU_19 = 0x08
    STIMU_20 = 0x10
    STIMU_21 = 0x20
    STIMU_22 = 0x40
    STIMU_23 = 0x80
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0):
        """
        Initialize the waveform driver.
        
        Args:
            port: Serial port name (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
            baudrate: Baud rate for serial communication (default: 115200)
            timeout: Read timeout in seconds (default: 1.0)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None
        
    def connect(self):
        """Open the serial port connection."""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            time.sleep(0.1)  # Allow time for connection to stabilize
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            print(f"Connected to {self.port} at {self.baudrate} baud")
        except serial.SerialException as e:
            raise ConnectionError(f"Failed to open serial port {self.port}: {e}")
    
    def disconnect(self):
        """Close the serial port connection."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print(f"Disconnected from {self.port}")
    
    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
    
    @staticmethod
    def crc8_maxim(data: bytes) -> int:
        """
        Calculate CRC8-MAXIM checksum.
        
        Polynomial: 0x31 (x^8 + x^5 + x^4 + 1)
        Initial value: 0x00
        
        Args:
            data: Bytes to calculate CRC for
            
        Returns:
            CRC8 checksum
        """
        crc = 0x00
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x01:  # Check LSB (not MSB)
                    crc = (crc >> 1) ^ 0x8C  # Right shift with reversed poly
                else:
                    crc = crc >> 1
        return crc
    
    def _validate_channel(self, channel: int):
        """Validate channel number is in valid range."""
        if not (self.MIN_CHANNEL <= channel <= self.MAX_CHANNEL):
            raise ValueError(
                f"Channel must be between {self.MIN_CHANNEL} and {self.MAX_CHANNEL} "
                f"(STIMU 8-15), got {channel}"
            )
    
    def _send_frame(self, command: int, channel: int, data: int):
        """
        Send a message frame to the microcontroller.
        
        Args:
            command: Command byte
            channel: Channel number (0-7)
            data: 32-bit data value
        """
        if not self.serial or not self.serial.is_open:
            raise ConnectionError("Serial port is not connected")
        
        self._validate_channel(channel)
        
        # Build frame (bytes 0-7)
        frame = bytearray([
            self.FRAME_HEADER1,
            self.FRAME_HEADER2,
            command,
            channel,
        ])
        
        # Add 32-bit data in little-endian format (bytes 4-7)
        frame.extend(struct.pack('<I', data & 0xFFFFFFFF))
        
        # Calculate and append CRC8 (byte 8)
        crc = self.crc8_maxim(frame)
        frame.append(crc)
        
        # Send frame
        self.serial.write(frame)
        self.serial.flush()
        
        # Debug output
        frame_hex = ' '.join(f'{b:02X}' for b in frame)
        print(f"Sent: {frame_hex}")
        # time.sleep(0.1)
    
    # =========================================================================
    # Control Commands
    # =========================================================================
    
    def start(self, channel: int):
        """
        Start waveform generation on specified channel.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
        """
        self._send_frame(self.CMD_START, channel, 0x01)
        print(f"Started channel {channel} (STIMU {8+channel})")
    
    def stop(self, channel: int):
        """
        Stop waveform generation on specified channel.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
        """
        self._send_frame(self.CMD_STOP, channel, 0x00)
        print(f"Stopped channel {channel} (STIMU {8+channel})")
    
    def start_all(self):
        """Start all channels (STIMU 8-15)."""
        for ch in range(self.MIN_CHANNEL, self.MAX_CHANNEL + 1):
            self.start(ch)
            time.sleep(0.01)  # Small delay between commands
    
    def stop_all(self):
        """Stop all channels (STIMU 8-15)."""
        for ch in range(self.MIN_CHANNEL, self.MAX_CHANNEL + 1):
            self.stop(ch)
            time.sleep(0.01)  # Small delay between commands
    
    # =========================================================================
    # Configuration Commands
    # =========================================================================
    
    def set_stimulation_period(self, channel: int, period_us: int):
        """
        Set stimulation period in microseconds.
        
        The stimulation period is the total time for one complete stimulation cycle.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            period_us: Stimulation period in microseconds (typically 10000-30000)
        """
        if period_us < 0 or period_us > 0xFFFFFFFF:
            raise ValueError(f"Period must be between 0 and {0xFFFFFFFF} us")
        
        self._send_frame(self.CMD_STIMULATION_PERIOD, channel, period_us)
        print(f"Channel {channel}: Set stimulation period to {period_us} us")
    
    def set_on_time(self, channel: int, on_time_us: int):
        """
        Set pulse on-time (pulse width) in microseconds.
        
        This is the duration of both the positive and negative pulse phases.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            on_time_us: On-time in microseconds (typically 200-800)
        """
        if on_time_us < 0 or on_time_us > 0xFFFFFFFF:
            raise ValueError(f"On-time must be between 0 and {0xFFFFFFFF} us")
        
        self._send_frame(self.CMD_ON_TIME, channel, on_time_us)
        print(f"Channel {channel}: Set on-time to {on_time_us} us")
    
    def set_pos_neg_gap(self, channel: int, gap_us: int):
        """
        Set gap between positive and negative pulse in microseconds.
        
        This is the inter-phase gap (rest time between pos and neg pulses).
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            gap_us: Gap in microseconds (typically 20-100)
        """
        if gap_us < 0 or gap_us > 0xFFFFFFFF:
            raise ValueError(f"Gap must be between 0 and {0xFFFFFFFF} us")
        
        self._send_frame(self.CMD_POS_NEG_GAP, channel, gap_us)
        print(f"Channel {channel}: Set pos-neg gap to {gap_us} us")
    
    def set_strength_level(self, channel: int, level: int):
        """
        Set current strength level (0-255).
        
        The actual current is: 25.5 µA × current_range × (1 + level)
        Where current_range is typically 1 (set in firmware).
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            level: Strength level (0-255)
        """
        if level < 0 or level > 255:
            raise ValueError("Strength level must be between 0 and 255")
        
        self._send_frame(self.CMD_STRENGTH_LEVEL, channel, level)
        print(f"Channel {channel}: Set strength level to {level}")
    
    def set_current(self, channel: int, current_mA: float):
        """
        Set current in microamperes.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            current_mA: Current in milliamperes
        """
        self.set_strength_level(channel, int(current_mA / 0.051))
        print(f"Channel {channel}: Set current to {current_mA} mA")
    
    def set_paired_switch(self, channel: int, config: int):
        """
        Set paired switch configuration (STIMU 16-23 enable bits).
        
        Each bit enables one of the paired switches (STIMU 16-23):
            Bit 0 (0x01): STIMU 16
            Bit 1 (0x02): STIMU 17
            Bit 2 (0x04): STIMU 18
            Bit 3 (0x08): STIMU 19
            Bit 4 (0x10): STIMU 20
            Bit 5 (0x20): STIMU 21
            Bit 6 (0x40): STIMU 22
            Bit 7 (0x80): STIMU 23
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            config: 8-bit configuration (can be ORed, e.g., STIMU_16 | STIMU_17)
        """
        if config < 0 or config > 255:
            raise ValueError("Paired switch config must be between 0 and 255")
        
        self._send_frame(self.CMD_PAIRED_SWITCH, channel, config)
        
        # Print which switches are enabled
        enabled = []
        for i in range(8):
            if config & (1 << i):
                enabled.append(f"STIMU {16+i}")
        enabled_str = ", ".join(enabled) if enabled else "None"
        print(f"Channel {channel}: Set paired switches to 0x{config:02X} ({enabled_str})")
    
    # =========================================================================
    # Convenience Methods
    # =========================================================================
    
    def configure_channel(self, channel: int, stimulation_period_us: int = 20000,
                         on_time_us: int = 400, pos_neg_gap_us: int = 50,
                         strength_level: int = 160, paired_switch_config: int = 0x01):
        """
        Configure all parameters for a channel in one call.
        
        Args:
            channel: Channel number (0-7 for STIMU 8-15)
            stimulation_period_us: Total stimulation period (default: 20000)
            on_time_us: Pulse width (default: 400)
            pos_neg_gap_us: Inter-phase gap (default: 50)
            strength_level: Current level 0-255 (default: 160)
            paired_switch_config: Paired switch bits (default: 0x01)
        """
        print(f"\n=== Configuring Channel {channel} (STIMU {8+channel}) ===")
        self.set_stimulation_period(channel, stimulation_period_us)
        time.sleep(0.01)
        self.set_on_time(channel, on_time_us)
        time.sleep(0.01)
        self.set_pos_neg_gap(channel, pos_neg_gap_us)
        time.sleep(0.01)
        self.set_strength_level(channel, strength_level)
        time.sleep(0.01)
        self.set_paired_switch(channel, paired_switch_config)
        time.sleep(0.01)
        print(f"=== Channel {channel} configuration complete ===\n")
    
    def get_version(self, channel: int = 0):
        """
        Request version information (if implemented in firmware).
        
        Args:
            channel: Channel number (default: 0)
        """
        self._send_frame(self.CMD_VERSION, channel, 0x00)
        print("Version request sent (if firmware supports it)")


# =============================================================================
# Example Usage
# =============================================================================
def main():
    """Example usage of the WaveformDriver."""
    
    # Configuration
    PORT = '/dev/tty.usbserial-MQZXDN'  # Change to your serial port (e.g., '/dev/ttyUSB0' on Linux)
    BAUDRATE = 115200
    
    print("=" * 70)
    print("CMSDK Waveform Generator Serial Driver - Example")
    print("=" * 70)
    
    try:
        # Connect using context manager (auto-disconnects on exit)
        with WaveformDriver(PORT, BAUDRATE) as driver:
            
            # # Example 1: Configure and start a single channel
            # print("\n--- Example 1: Single Channel Control ---")
            driver.configure_channel(
                channel=0,
                stimulation_period_us=16000,
                on_time_us=400,
                pos_neg_gap_us=50,
                strength_level=160,
                paired_switch_config=driver.STIMU_16
            )
            # time.sleep(1)
            driver.configure_channel(
                channel=1,
                stimulation_period_us=16000,
                on_time_us=400,
                pos_neg_gap_us=50,
                strength_level=160,
                paired_switch_config=driver.STIMU_17
            )
            # time.sleep(1)
            # driver.start(channel=0)
            # time.sleep(10)  # Run for 2 seconds
            # driver.stop(channel=0)
            
            # Example 2: Configure multiple channels
            # print("\n--- Example 2: Multiple Channel Configuration ---")
            # for ch in range(4):  # Configure channels 0-3
            #     driver.configure_channel(
            #         channel=ch,
            #         stimulation_period_us=20000,
            #         on_time_us=400,
            #         pos_neg_gap_us=50,
            #         strength_level=100 + ch * 20,  # Varying strength
            #         paired_switch_config=(1 << ch)  # Each channel gets one paired switch
            #     )
            
            # # Start all configured channels
            # print("\n--- Starting all configured channels ---")
            # for ch in range(4):
            #     driver.start(ch)
            #     # time.sleep(0.05)
            
            # time.sleep(10)  # Run for 3 seconds
            
            # # Stop all channels
            # print("\n--- Stopping all channels ---")
            # driver.stop_all()
            
            # Example 3: Dynamic parameter adjustment
            print("\n--- Example 3: Dynamic Strength Adjustment ---")
            # driver.configure_channel(channel=0, strength_level=1)
            driver.set_current(channel=0, current_mA=3)
            driver.set_current(channel=1, current_mA=6)
            driver.start(channel=0)
            driver.start(channel=1)
            
            # for current_mA in range(10, 60, 1):
            #     print(f"Adjusting strength to {current_mA}...")
            #     driver.set_current(channel=0, current_mA=current_mA/10)
            #     time.sleep(0.5)
            
            # for current_mA in range(60, 10, -1):
            #     print(f"Adjusting strength to {current_mA}...")
            #     driver.set_current(channel=0, current_mA=current_mA/10)
            #     time.sleep(0.5)
            while True:
                
                # driver.start(channel=0)
                # time.sleep(1)
                # driver.stop(channel=0)
                # time.sleep(1)
                # driver.start(channel=1)
                # time.sleep(1)
                # driver.stop(channel=1)
                # time.sleep(3)

                for current_mA in range(5, 90, 3):
                    print(f"Adjusting strength to {current_mA}...")
                    driver.set_current(channel=1, current_mA=current_mA/10)
                    time.sleep(0.2)
            
                for current_mA in range(90, 5, -3):
                    print(f"Adjusting strength to {current_mA}...")
                    driver.set_current(channel=1, current_mA=current_mA/10)
                    time.sleep(0.2)
            
            # driver.stop(channel=0)
            
            print("\n--- Examples complete ---")
    
    except ConnectionError as e:
        print(f"Connection error: {e}")
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
    
    print("\nDriver disconnected")


if __name__ == "__main__":
    main()

