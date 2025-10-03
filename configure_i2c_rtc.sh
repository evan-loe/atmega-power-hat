#!/bin/bash

# this one time setup file must be run with sudo
# it enables i2c and configures the pi to use the D3231 RTC
# as its clock.
#  it also makes sure to disable the alarm signal when the pi boots.

CONFIG_FILE="/boot/firmware/config.txt"

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root." 
   exit 1
fi

# Add dtoverlay line if it doesn't already exist
if ! grep -q "^dtoverlay=i2c-rtc,ds3231,wakeup-source" "$CONFIG_FILE"; then
    echo "Adding 'dtoverlay=i2c-rtc,ds3231,wakeup-source' to $CONFIG_FILE"
    echo "dtoverlay=i2c-rtc,ds3231,wakeup-source" >> "$CONFIG_FILE"
else
    echo "'dtoverlay=i2c-rtc,ds3231,wakeup-source' is already present in $CONFIG_FILE"
fi

# Uncomment dtparam=i2c_arm=on if it exists but is commented out
if grep -q "^#dtparam=i2c_arm=on" "$CONFIG_FILE"; then
    echo "Uncommenting 'dtparam=i2c_arm=on' in $CONFIG_FILE"
    sed -i 's/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/' "$CONFIG_FILE"
else
    echo "'dtparam=i2c_arm=on' is already enabled or not present in $CONFIG_FILE"
fi

# Check if /etc/rc.local exists
if [ ! -f /etc/rc.local ]; then
    echo "Error: /etc/rc.local does not exist."
fi

# Add the line before 'exit 0' if not already present
# this will disable the alarm siganl when the Pi boots, since it assumed the alarm is triggering (thats what started the whole pi boot process)
sudo sed -i '$!b; /exit 0/i echo "0" | sudo tee /sys/class/rtc/rtc0/wakealarm' /etc/rc.local


echo "Alarm auto-shutoff added successfully to /etc/rc.local."


echo "Changes completed. Reboot your Raspberry Pi to apply the settings."
