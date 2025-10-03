# ATMEGA Power Hat docs

## Introduction
The *ATMEGA Power Hat* is an RTC enabled Raspberry Pi compatible HAT *(Hardware-Attached-on-Top)* that can automatically wakeup and shutdown the pi on a given schedule.

## Setup Instructions

#### Run `configure_i2c_rtc.sh`
- Run [configure_i2c_rtc.sh](./configure_i2c_rtc.sh) with sudo. This script configures the RTC on the hat as a device recognizable by the Pi as a clock source by adding a dtoverlay. Now, the PI can keep its system time even if it powers off and does not get a chance to sync over NTP.

#### Flash the ATMEGA
- Use the provided .hex files under `build/` to flash the firmware onto the ATEMGA using your ISP and programs such as Microchip Studio or AVRDude. 
- FUSE BITS (Low High Ext): `E2 D9 FF`
- ATMEGA should come shipped with the correct High and Extended fuse bits. Change the low fuse bit to `0xE2` to enable internal clock running at 8 MHz

And thats it! Your hat should be ready to use

## Firmware Description
The source code of the firmware is available in `power.cpp`. It was developed using the Arduino v2.3.6 libararies.

The code does the following when powered ON:
- Sets up the ATMEGA as an I2C receiver on address `0x20`.
- Tells the power controller to send power to the PI by default
- Starts a watchdog that will restart the PIs power if it does not receive communication from the PI

While waiting for successful boot
- **It is the PI's responsibility to tell the ATMEGA of it's successful boot via the `Boot Success` I2C command

After receiving the `Boot Success` signal
- The ATMEGA waits for the `Shutdown` command

When the `Shutdown` command is received
- The ATMEGA waits n seconds as specified by the command. Then, it tells the power chip to cut power. It enters deep sleep mode until the RTC alarm wakes it. Upon awakening, it turns back on the power to the PI, waiting for the `Boot Success` command to be sent from the PI. 

The ATMEGA does not program the RTC over i2c.

## Supported I2C Transactions
The following section describes the transaction format recognized by the ATMEGA. The first byte denotes the command type, and the second byte denotes the value if applicable.

The Base Address of the ATMEGA is `0x20`

| Command | First Byte (txn_type) | Second Byte (data) | Description | Action |
|------|----------------------|-------------------|-------------|---------|
| Shutdown | `0x00` | 0x00-0xFF | Shutdown command | Sets `shutdown_in` variable to data value |
| Boot Success | `0x01` | Any value | Boot success signal | Sets `successful_boot` flag to 1 |

#### Note on max number size
Since any data is encoded in a single byte, the max delay from receiving the i2c signal to cutting power is 2^8=256 seconds.

## Hardware
The CAD files can be found in the [ATMegaPowerCircuit](./ATMegaPowerCircuit/) folder.

![ATMEGA Hardware Connections](./assets/ATMEGA%20Pinout.png)

## Compiling the firmwre
Select the Aruduino UNO as your device. Paste [power.cpp](./power.cpp) into your Arduino IDE and click `Sketch > Export Compiled Binary`

## Additional Reference Information
**RTC lib docs**: https://adafruit.github.io/RTClib/html/index.html (We are using the DS3231 RTC chip)