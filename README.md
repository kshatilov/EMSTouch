# EMS Microcontroller Driver

A Python driver for communicating with EMS (Electrical Muscle Stimulation) microcontrollers. This driver implements the communication protocol based on analysis of the microcontroller source code.

## Features

- **Complete Protocol Implementation**: Implements the 9-byte packet protocol with CRC8-MAXIM checksum
- **Multiple Waveform Types**: Support for default, repeat, interrupt, EMS (varied), and self-defined waveforms
- **Multi-Channel Support**: Control up to 4 channels independently
- **EMS Training Modes**: Specialized functions for EMS muscle training with configurable parameters
- **Error Handling**: Robust error handling with logging
- **Context Manager Support**: Easy resource management with `with` statements

## Installation

### Prerequisites

- Python 3.7+
- pyserial library

```bash
pip install pyserial
```

### Hardware Requirements

- EMS microcontroller device
- Serial connection (USB-to-Serial adapter or direct USB connection)
- Compatible electrodes and cables

## Quick Start

```python
from EMSMcuDriver import EMSDriver, WaveformType

# Simple usage
with EMSDriver('COM5') as driver:  # Change to your port
    if driver.is_connected:
        # Configure basic waveform
        driver.configure_basic_waveform(channel=0)
        
        # Start stimulation
        driver.start_channel(0)
        
        # Wait 2 seconds
        time.sleep(2)
        
        # Stop stimulation
        driver.stop_channel(0)
```

## Communication Protocol

The driver implements a 9-byte packet protocol:

| Byte | Description | Value |
|------|-------------|-------|
| 0-1  | Header      | 0x55, 0x55 |
| 2    | Command     | Command + Sub-command |
| 3    | Channel     | Channel number (0-3) |
| 4-7  | Data        | 32-bit payload (little-endian) |
| 8    | CRC         | CRC8-MAXIM checksum |

### Commands

- `0x1x`: Version query
- `0x2x`: Start/Stop control
- `0x4x`: Waveform type configuration
- `0x5x`: Register configuration
- `0x6x`: Waveform timing configuration

## API Reference

### EMSDriver Class

#### Constructor
```python
EMSDriver(port: str, baudrate: int = 115200, timeout: float = 1.0)
```

#### Connection Methods
- `connect() -> bool`: Connect to microcontroller
- `disconnect()`: Disconnect from microcontroller

#### Basic Control
- `start_channel(channel: int) -> bool`: Start stimulation on channel
- `stop_channel(channel: int) -> bool`: Stop stimulation on channel
- `query_version() -> Optional[str]`: Query microcontroller version

#### Waveform Configuration
- `set_waveform_type(channel: int, waveform_type: WaveformType) -> bool`
- `configure_basic_waveform(channel: int, **params) -> bool`
- `configure_ems_waveform(channel: int, **params) -> bool`

#### Advanced Configuration
- `configure_register(channel: int, register: Register, value: int) -> bool`
- `configure_timing(channel: int, timing_param: TimingConfig, value: int) -> bool`

### Enums

#### WaveformType
- `DEFAULT_WAVE = 0x00`: Default waveform
- `REPEAT_WAVE = 0x01`: Repeat waveform
- `INTERRUPT_WAVE = 0x02`: Interrupt repeat waveform
- `VARIED_WAVE = 0x03`: Frequency sweep waveform (EMS)
- `SELF_DEFINED_WAVE = 0x04`: Self-defined waveform

#### Register
- `CONFIG_REG = 0x0`: Configuration register
- `CTRL_REG = 0x1`: Control register
- `REST_T_REG = 0x2`: Rest time register
- `SILENT_T_REG = 0x3`: Silent time register
- `HLF_WAVE_PRD_REG = 0x4`: Half wave period register
- `NEG_HLF_WAVE_PRD_REG = 0x5`: Negative half wave period register
- `CLK_FREQ_REG = 0x6`: Clock frequency register
- `IN_WAVE_REG = 0x8`: Input wave register
- `ALT_LIM_REG = 0x9`: Altitude limit register
- `ALT_SILENT_LIM_REG = 0xA`: Altitude silent limit register
- `DELAY_LIM_REG = 0xB`: Delay limit register
- `NEG_SCALE_REG = 0xC`: Negative scale register
- `NEG_OFFSET_REG = 0xD`: Negative offset register
- `INT_REG = 0xE`: Interrupt register
- `ISEL_REG = 0xF`: Current selection register

## Examples

### Basic Touch Stimulation
```python
with EMSDriver('COM5') as driver:
    driver.configure_basic_waveform(channel=0, current_level=0x3)
    driver.start_channel(0)
    time.sleep(2)
    driver.stop_channel(0)
```

### EMS Training Session
```python
with EMSDriver('COM5') as driver:
    driver.configure_ems_waveform(
        channel=0,
        total_time=3000,      # 3 seconds
        interval_time=500,    # 500ms intervals
        pulse_quantity=8,     # 8 pulses per burst
        time_of_stay_at_high=1500,  # 1.5s at high intensity
        rising_time=300,      # 300ms rising
        down_time=300         # 300ms down
    )
    driver.start_channel(0)
    time.sleep(10)
    driver.stop_channel(0)
```

### Multi-Channel Control
```python
with EMSDriver('COM5') as driver:
    # Configure different waveforms for each channel
    for channel in range(4):
        driver.set_waveform_type(channel, WaveformType.REPEAT_WAVE)
        driver.configure_basic_waveform(channel, current_level=0x2 + channel)
    
    # Start all channels
    for channel in range(4):
        driver.start_channel(channel)
        time.sleep(1)
    
    # Stop all channels
    for channel in range(4):
        driver.stop_channel(channel)
```

## Port Configuration

### Windows
- Common ports: `COM3`, `COM5`, `COM7`
- Check Device Manager for available ports

### Linux/Mac
- Common ports: `/dev/ttyUSB0`, `/dev/ttyACM0`, `/dev/tty.usbserial-*`
- Use `ls /dev/tty*` to list available ports

## Safety Considerations

⚠️ **Important Safety Notes:**

1. **Start with low current levels** and gradually increase
2. **Never exceed recommended current limits** for your electrodes
3. **Test on a small area first** before full muscle stimulation
4. **Follow electrode placement guidelines** for your specific application
5. **Stop immediately** if you experience discomfort or pain
6. **Consult medical professionals** for therapeutic applications

## Troubleshooting

### Connection Issues
- Verify the correct serial port
- Check baudrate (default: 115200)
- Ensure device is properly connected
- Try different USB ports

### Communication Errors
- Check CRC errors in logs
- Verify packet format
- Ensure microcontroller is responding
- Check for interference on serial line

### Performance Issues
- Increase timeout values for slow responses
- Check serial buffer sizes
- Verify microcontroller firmware version

## License

This driver is provided as-is for educational and development purposes. Please ensure compliance with local regulations and safety standards when using EMS devices.

## Contributing

Contributions are welcome! Please ensure any changes maintain compatibility with the existing protocol and include appropriate tests.
