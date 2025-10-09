#!/usr/bin/env python3
"""
EMS Microcontroller Driver
Implements communication protocol for EMS (Electrical Muscle Stimulation) device
Based on analysis of microcontroller source code
"""

import serial
import time
import struct
from typing import Optional, Tuple, List, Dict, Any
from enum import IntEnum
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class WaveformType(IntEnum):
    """Waveform types supported by the microcontroller"""
    DEFAULT_WAVE = 0x00      # Default waveform
    REPEAT_WAVE = 0x01       # Repeat waveform
    INTERRUPT_WAVE = 0x02    # Interrupt repeat waveform
    VARIED_WAVE = 0x03       # Frequency sweep waveform (EMS)
    SELF_DEFINED_WAVE = 0x04 # Self-defined waveform


class Command(IntEnum):
    """Command types"""
    VERSION_QUERY = 0x10
    START_STOP_CONTROL = 0x20
    WAVEFORM_TYPE_CONFIG = 0x40
    REGISTER_CONFIG = 0x50
    WAVEFORM_TIMING_CONFIG = 0x60


class Register(IntEnum):
    """Register addresses for configuration"""
    CONFIG_REG = 0x0
    CTRL_REG = 0x1
    REST_T_REG = 0x2
    SILENT_T_REG = 0x3
    HLF_WAVE_PRD_REG = 0x4
    NEG_HLF_WAVE_PRD_REG = 0x5
    CLK_FREQ_REG = 0x6
    IN_WAVE_REG = 0x8
    ALT_LIM_REG = 0x9
    ALT_SILENT_LIM_REG = 0xA
    DELAY_LIM_REG = 0xB
    NEG_SCALE_REG = 0xC
    NEG_OFFSET_REG = 0xD
    INT_REG = 0xE
    ISEL_REG = 0xF


class TimingConfig(IntEnum):
    """Timing configuration parameters"""
    TOTAL_TIME = 0x0
    INTERVAL_TIME = 0x1
    PULSE_QUANTITY = 0x2
    TIME_OF_STAY_AT_HIGH = 0x3
    RISING_TIME = 0x4
    DOWN_TIME = 0x5


class EMSDriver:
    """Driver for EMS microcontroller communication"""
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0):
        """
        Initialize the EMS driver
        
        Args:
            port: Serial port (e.g., 'COM5', '/dev/ttyUSB0')
            baudrate: Communication baudrate
            timeout: Serial timeout in seconds
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_conn: Optional[serial.Serial] = None
        self.is_connected = False
        
    def connect(self) -> bool:
        """Connect to the microcontroller"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            self.is_connected = True
            logger.info(f"Connected to {self.port} at {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            logger.error(f"Failed to connect to {self.port}: {e}")
            self.is_connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the microcontroller"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            self.is_connected = False
            logger.info("Disconnected from microcontroller")
    
    def _crc8_maxim(self, data: bytes) -> int:
        """
        Calculate CRC8-MAXIM checksum
        
        Args:
            data: Data bytes to calculate CRC for
            
        Returns:
            CRC8-MAXIM checksum
        """
        crc = 0
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0x8C  # 0x8C = reverse 0x31
                else:
                    crc >>= 1
        return crc & 0xFF
    
    def create_packet(self, command: int, channel: int, data: int = 0) -> bytes:
        """
        Create a communication packet
        
        Args:
            command: Command byte
            channel: Channel number (0-3)
            data: 32-bit data payload
            
        Returns:
            Complete packet with CRC
        """
        packet = bytearray(9)
        packet[0] = 0x55  # Header byte 1
        packet[1] = 0x55  # Header byte 2
        packet[2] = command
        packet[3] = channel
        
        # Pack 32-bit data into bytes 4-7 (little-endian)
        packet[4:8] = struct.pack('<I', data)
        
        # Calculate CRC for first 8 bytes
        packet[8] = self._crc8_maxim(packet[:8])
        
        return bytes(packet)
    
    def _send_packet(self, packet: bytes) -> bool:
        """
        Send packet to microcontroller
        
        Args:
            packet: Packet to send
            
        Returns:
            True if sent successfully
        """
        if not self.is_connected or not self.serial_conn:
            logger.error("Not connected to microcontroller")
            return False
        
        try:
            self.serial_conn.write(packet)
            # Print sent packet for debugging
            print(f"[SENT] {packet.hex().upper()}")
            logger.debug(f"Sent packet: {packet.hex().upper()}")
            return True
        except serial.SerialException as e:
            logger.error(f"Failed to send packet: {e}")
            return False
    
    def _receive_response(self, timeout: float = 1.0) -> Optional[bytes]:
        """
        Receive response from microcontroller
        
        Args:
            timeout: Timeout in seconds
            
        Returns:
            Response packet or None if timeout/error
        """
        if not self.is_connected or not self.serial_conn:
            return None
        
        try:
            # Read 9 bytes (full packet)
            response = self.serial_conn.read(9)
            if len(response) == 9:
                # Print received packet for debugging
                print(f"[RECV] {response.hex().upper()}")
                logger.debug(f"Received response: {response.hex().upper()}")
                return response
            else:
                if len(response) > 0:
                    print(f"[RECV] {response.hex().upper()} (incomplete: {len(response)}/9 bytes)")
                logger.warning(f"Incomplete response: {len(response)} bytes")
                return None
        except serial.SerialException as e:
            logger.error(f"Failed to receive response: {e}")
            return None
    
    def _verify_response(self, response: bytes, expected_command: int, expected_channel: int) -> bool:
        """
        Verify response packet
        
        Args:
            response: Response packet
            expected_command: Expected command (without 0x80 bit)
            expected_channel: Expected channel
            
        Returns:
            True if response is valid
        """
        if len(response) != 9:
            return False
        
        # Check header
        if response[0] != 0x55 or response[1] != 0x55:
            return False
        
        # Check command (should have 0x80 bit set for acknowledgment)
        if (response[2] & 0x7F) != expected_command:
            return False
        
        # Check channel
        if response[3] != expected_channel:
            return False
        
        # Verify CRC
        calculated_crc = self._crc8_maxim(response[:8])
        if response[8] != calculated_crc:
            logger.warning("CRC mismatch in response")
            return False
        
        return True
    
    def query_version(self) -> Optional[str]:
        """Query microcontroller version"""
        packet = self.create_packet(Command.VERSION_QUERY, 0, 1)
        if self._send_packet(packet):
            response = self._receive_response()
            if response and self._verify_response(response, Command.VERSION_QUERY, 0):
                # Extract version from response data
                version_data = struct.unpack('<I', response[4:8])[0]
                return f"Version: {version_data}"
        return None
    
    def start_channel(self, channel: int) -> bool:
        """
        Start stimulation on specified channel
        
        Args:
            channel: Channel number (0-3)
            
        Returns:
            True if command sent successfully
        """
        packet = self.create_packet(Command.START_STOP_CONTROL, channel, 1)  # 1 = ENABLE
        return self._send_packet(packet)
    
    def stop_channel(self, channel: int) -> bool:
        """
        Stop stimulation on specified channel
        
        Args:
            channel: Channel number (0-3)
            
        Returns:
            True if command sent successfully
        """
        packet = self.create_packet(Command.START_STOP_CONTROL, channel, 0)  # 0 = DISABLE
        return self._send_packet(packet)
    
    def set_waveform_type(self, channel: int, waveform_type: WaveformType) -> bool:
        """
        Set waveform type for channel
        
        Args:
            channel: Channel number (0-3)
            waveform_type: Type of waveform
            
        Returns:
            True if command sent successfully
        """
        packet = self.create_packet(Command.WAVEFORM_TYPE_CONFIG, channel, waveform_type)
        return self._send_packet(packet)
    
    def configure_register(self, channel: int, register: Register, value: int) -> bool:
        """
        Configure register for channel
        
        Args:
            channel: Channel number (0-3)
            register: Register to configure
            value: Value to set
            
        Returns:
            True if command sent successfully
        """
        command = Command.REGISTER_CONFIG | register
        packet = self.create_packet(command, channel, value)
        time.sleep(0.1)
        return self._send_packet(packet)
        
    
    def configure_timing(self, channel: int, timing_param: TimingConfig, value: int) -> bool:
        """
        Configure timing parameters for channel
        
        Args:
            channel: Channel number (0-3)
            timing_param: Timing parameter to configure
            value: Value to set
            
        Returns:
            True if command sent successfully
        """
        command = Command.WAVEFORM_TIMING_CONFIG | timing_param
        packet = self.create_packet(command, channel, value)
        return self._send_packet(packet)
    
    def configure_ems_waveform(self, channel: int, 
                             total_time: int = 1000,
                             interval_time: int = 100,
                             pulse_quantity: int = 10,
                             time_of_stay_at_high: int = 500,
                             rising_time: int = 100,
                             down_time: int = 100) -> bool:
        """
        Configure EMS waveform parameters
        
        Args:
            channel: Channel number (0-3)
            total_time: Total stimulation time in ms
            interval_time: Interval between pulses in ms
            pulse_quantity: Number of pulses per burst
            time_of_stay_at_high: Time to stay at high intensity in ms
            rising_time: Rising time in ms
            down_time: Down time in ms
            
        Returns:
            True if all commands sent successfully
        """
        success = True
        
        # Set waveform type to EMS
        success &= self.set_waveform_type(channel, WaveformType.VARIED_WAVE)
        
        # Configure timing parameters
        success &= self.configure_timing(channel, TimingConfig.TOTAL_TIME, total_time)
        success &= self.configure_timing(channel, TimingConfig.INTERVAL_TIME, interval_time)
        success &= self.configure_timing(channel, TimingConfig.PULSE_QUANTITY, pulse_quantity)
        success &= self.configure_timing(channel, TimingConfig.TIME_OF_STAY_AT_HIGH, time_of_stay_at_high)
        success &= self.configure_timing(channel, TimingConfig.RISING_TIME, rising_time)
        success &= self.configure_timing(channel, TimingConfig.DOWN_TIME, down_time)
        
        return success
    
    def configure_basic_waveform(self, channel: int,
                               rest_time: int = 1,
                               silent_time: int = 1,
                               half_wave_period: int = 100,
                               neg_half_wave_period: int = 100,
                               clock_freq: int = 32,
                               current_level: int = 50) -> bool:
        """
        Configure basic waveform parameters
        
        Args:
            channel: Channel number (0-3)
            rest_time: Rest time in microseconds
            silent_time: Silent time in microseconds
            half_wave_period: Half wave period in microseconds
            neg_half_wave_period: Negative half wave period in microseconds
            clock_freq: Clock frequency register value
            current_level: Current level (ISEL register)
            
        Returns:
            True if all commands sent successfully
        """
        success = True
        
        # Configure basic registers
        success &= self.configure_register(channel, Register.CONFIG_REG, 127)
        success &= self.configure_register(channel, Register.REST_T_REG, rest_time)
        success &= self.configure_register(channel, Register.SILENT_T_REG, silent_time)
        success &= self.configure_register(channel, Register.HLF_WAVE_PRD_REG, half_wave_period)
        success &= self.configure_register(channel, Register.NEG_HLF_WAVE_PRD_REG, neg_half_wave_period)
        success &= self.configure_register(channel, Register.CLK_FREQ_REG, clock_freq)
        success &= self.configure_register(channel, Register.ISEL_REG, current_level)
        success &= self.configure_register(channel, Register.NEG_SCALE_REG, 2)
        success &= self.configure_register(channel, Register.NEG_OFFSET_REG, 0)
        
        return success
    
    def __enter__(self):
        """Context manager entry"""
        self.connect()
        return self
    
    def monitor_serial(self, duration: float = 10.0):
        """
        Monitor serial communication for debugging
        
        Args:
            duration: How long to monitor in seconds
        """
        if not self.is_connected or not self.serial_conn:
            print("Not connected to microcontroller")
            return
        
        print(f"Monitoring serial communication for {duration} seconds...")
        print("Press Ctrl+C to stop early")
        print("=" * 50)
        
        start_time = time.time()
        try:
            while time.time() - start_time < duration:
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    print(f"[RAW] {data.hex().upper()}")
                time.sleep(0.01)  # Small delay to prevent excessive CPU usage
        except KeyboardInterrupt:
            print("\nMonitoring stopped by user")
        
        print("=" * 50)
        print("Monitoring completed")
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.disconnect()


# Example usage and testing functions
def test_basic_communication():
    """Test basic communication with microcontroller"""
    with EMSDriver('COM5') as driver:
        if driver.is_connected:
            # Query version
            version = driver.query_version()
            print(f"Version: {version}")
            
            # Configure basic waveform for channel 0
            # success = driver.configure_basic_waveform(0)
            # print(f"Basic waveform configuration: {'Success' if success else 'Failed'}")
            
            # Start stimulation
            success = driver.start_channel(0)
            print(f"Start channel 0: {'Success' if success else 'Failed'}")
            
            # Wait for 2 seconds
            time.sleep(2)
            
            # Stop stimulation
            success = driver.stop_channel(0)
            print(f"Stop channel 0: {'Success' if success else 'Failed'}")

            time.sleep(1)

            success = driver.start_channel(1)
            print(f"Start channel 1: {'Success' if success else 'Failed'}")
            
            # Wait for 2 seconds
            time.sleep(2)
            
            # Stop stimulation
            success = driver.stop_channel(1)
            print(f"Stop channel 1: {'Success' if success else 'Failed'}")


def test_ems_waveform():
    """Test EMS waveform configuration"""
    with EMSDriver('/dev/tty.usbserial-110') as driver:
        if driver.is_connected:
            # Configure EMS waveform
            success = driver.configure_ems_waveform(
                channel=0,
                total_time=2000,      # 2 seconds total
                interval_time=200,     # 200ms intervals
                pulse_quantity=5,      # 5 pulses per burst
                time_of_stay_at_high=1000,  # 1 second at high intensity
                rising_time=200,      # 200ms rising
                down_time=200        # 200ms down
            )
            print(f"EMS waveform configuration: {'Success' if success else 'Failed'}")
            
            if success:
                # Start EMS stimulation
                driver.start_channel(0)
                print("EMS stimulation started")
                
                # Wait for 5 seconds
                time.sleep(5)
                
                # Stop stimulation
                driver.stop_channel(0)
                print("EMS stimulation stopped")


def test_debug_communication():
    """Test communication with full debugging output"""
    print("=== Debug Communication Test ===")
    
    with EMSDriver('/dev/tty.usbserial-110') as driver:
        if not driver.is_connected:
            print("Failed to connect to microcontroller")
            return
        
        print("Connected to microcontroller")
        print("Sending test commands with debug output...")
        print("=" * 60)
        
        # Test version query
        print("\n1. Testing version query:")
        version = driver.query_version()
        print(f"Version result: {version}")
        
        # Test basic configuration
        print("\n2. Testing basic waveform configuration:")
        success = driver.configure_basic_waveform(0)
        print(f"Configuration result: {'Success' if success else 'Failed'}")
        
        # Test start/stop
        print("\n3. Testing start channel:")
        success = driver.start_channel(0)
        print(f"Start result: {'Success' if success else 'Failed'}")
        
        print("\n4. Waiting 2 seconds...")
        time.sleep(2)
        
        print("\n5. Testing stop channel:")
        success = driver.stop_channel(0)
        print(f"Stop result: {'Success' if success else 'Failed'}")
        
        print("=" * 60)
        print("Debug test completed")


if __name__ == "__main__":
    # Run debug test
    print("Running debug communication test...")
    # test_debug_communication()
    
    # Uncomment these to run other tests
    print("\nTesting basic communication...")
    test_basic_communication()


    # print("\nTesting EMS waveform...")
    # test_ems_waveform()
