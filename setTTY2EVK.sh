#!/bin/bash
rm /dev/ttyEVK
if [ "$(ls /dev/ttyUSB* | wc -l)" -eq "4" ]; then
    echo "Found new Rev EVK with four UARTs."
    echo "ttyEVK ==> ttyUSB2"
    ln -s /dev/ttyUSB2 /dev/ttyEVK
elif [ "$(ls /dev/ttyUSB* | wc -l)" -eq "2" ]; then
    echo "Found old Rev EVK with two UARTs."
    echo "ttyEVK ==> ttyUSB0"
    ln -s /dev/ttyUSB0 /dev/ttyEVK
else
    echo "No ttyUSB ports found!!"
fi
