#!/bin/bash
rm /dev/ttyEVK
if [ "$(ls /dev/ttyUSB* | wc -l)" -eq "4" ]; then
    echo "Found new Rev EVK with four UARTs."
    ln -s /dev/ttyUSB2 /dev/ttyEVK
elif [ "$(ls /dev/ttyUSB* | wc -l)" -eq "2" ]; then
    echo "Found old Rev EVK with two UARTs."
    ln -s /dev/ttyUSB0 /dev/ttyEVK
else
    echo "No ttyUSB ports found!!"
fi
