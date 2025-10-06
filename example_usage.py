#!/usr/bin/env python3
"""
Example usage of EMS Microcontroller Driver
Demonstrates how to use the EMSDriver class for controlling EMS device
"""

from EMSMcuDriver import EMSDriver, WaveformType
import time


def simple_touch_example():
    """Simple touch stimulation example"""
    print("=== Simple Touch Example ===")
    
    # Initialize driver (adjust port as needed)
    with EMSDriver('COM5') as driver:  # Change to your actual port
        if not driver.is_connected:
            print("Failed to connect to microcontroller")
            return
        
        print("Connected to microcontroller")
        
        # Configure basic waveform for channel 0
        print("Configuring basic waveform...")
        success = driver.configure_basic_waveform(
            channel=0,
            rest_time=50,
            silent_time=50,
            half_wave_period=50,
            current_level=0x3  # Moderate current level
        )
        
        if success:
            print("Basic waveform configured successfully")
            
            # Start stimulation
            print("Starting stimulation...")
            driver.start_channel(0)
            
            # Stimulate for 2 seconds
            time.sleep(2)
            
            # Stop stimulation
            print("Stopping stimulation...")
            driver.stop_channel(0)
            print("Stimulation completed")
        else:
            print("Failed to configure basic waveform")


def ems_training_example():
    """EMS training session example"""
    print("\n=== EMS Training Example ===")
    
    with EMSDriver('COM5') as driver:  # Change to your actual port
        if not driver.is_connected:
            print("Failed to connect to microcontroller")
            return
        
        print("Connected to microcontroller")
        
        # Configure EMS waveform for muscle training
        print("Configuring EMS training waveform...")
        success = driver.configure_ems_waveform(
            channel=0,
            total_time=3000,      # 3 seconds total stimulation
            interval_time=500,    # 500ms intervals between bursts
            pulse_quantity=8,     # 8 pulses per burst
            time_of_stay_at_high=1500,  # 1.5 seconds at high intensity
            rising_time=300,      # 300ms rising time
            down_time=300         # 300ms down time
        )
        
        if success:
            print("EMS training waveform configured")
            
            # Start EMS training
            print("Starting EMS training session...")
            driver.start_channel(0)
            
            # Training session for 10 seconds
            time.sleep(10)
            
            # Stop training
            print("Stopping EMS training...")
            driver.stop_channel(0)
            print("EMS training session completed")
        else:
            print("Failed to configure EMS waveform")


def multi_channel_example():
    """Multi-channel stimulation example"""
    print("\n=== Multi-Channel Example ===")
    
    with EMSDriver('COM5') as driver:  # Change to your actual port
        if not driver.is_connected:
            print("Failed to connect to microcontroller")
            return
        
        print("Connected to microcontroller")
        
        # Configure different waveforms for different channels
        channels_config = [
            (0, WaveformType.REPEAT_WAVE, "Channel 0: Repeat Wave"),
            (1, WaveformType.INTERRUPT_WAVE, "Channel 1: Interrupt Wave"),
            (2, WaveformType.VARIED_WAVE, "Channel 2: EMS Wave"),
            (3, WaveformType.DEFAULT_WAVE, "Channel 3: Default Wave")
        ]
        
        # Configure each channel
        for channel, waveform_type, description in channels_config:
            print(f"Configuring {description}")
            
            # Set waveform type
            driver.set_waveform_type(channel, waveform_type)
            
            # Configure basic parameters
            driver.configure_basic_waveform(
                channel=channel,
                rest_time=50,
                silent_time=50,
                half_wave_period=50,
                current_level=0x2 + channel  # Different current levels
            )
        
        print("All channels configured")
        
        # Start all channels sequentially
        for channel, _, description in channels_config:
            print(f"Starting {description}")
            driver.start_channel(channel)
            time.sleep(1)  # 1 second per channel
        
        # Stop all channels
        print("Stopping all channels...")
        for channel in range(4):
            driver.stop_channel(channel)
        
        print("Multi-channel stimulation completed")


def version_query_example():
    """Query microcontroller version"""
    print("\n=== Version Query Example ===")
    
    with EMSDriver('COM5') as driver:  # Change to your actual port
        if not driver.is_connected:
            print("Failed to connect to microcontroller")
            return
        
        print("Connected to microcontroller")
        
        # Query version
        version = driver.query_version()
        if version:
            print(f"Microcontroller {version}")
        else:
            print("Failed to query version")


if __name__ == "__main__":
    print("EMS Microcontroller Driver Examples")
    print("====================================")
    
    # Note: Change 'COM5' to your actual serial port
    # Common ports: 'COM3', 'COM5' (Windows), '/dev/ttyUSB0', '/dev/ttyACM0' (Linux/Mac)
    
    try:
        # Run examples
        version_query_example()
        simple_touch_example()
        ems_training_example()
        multi_channel_example()
        
    except KeyboardInterrupt:
        print("\nExamples interrupted by user")
    except Exception as e:
        print(f"Error running examples: {e}")
    
    print("\nExamples completed")
