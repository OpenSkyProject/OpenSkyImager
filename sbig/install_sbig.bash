#!/bin/bash
#Collect parameters in named vars for ease of use
basedir=$1
cpu=`uname -p`
bit=`getconf LONG_BIT`
if [[ $cpu == *arm* ]]
then
	sudo cp $basedir/arm-32/libsbigudrv.so /usr/lib/
else
	sudo cp $basedir/x86-$bit/libsbigudrv.so /usr/lib/
fi
sudo cp $basedir/sbigudrv.h /usr/include/
sudo cp $basedir/51-sbig-debian.rules /etc/udev/rules.d/99-sbig.rules
sudo cp $basedir/sbigfcam.hex /lib/firmware/
sudo cp $basedir/sbiglcam.hex /lib/firmware/
sudo cp $basedir/sbigpcam.hex /lib/firmware/
sudo cp $basedir/sbigucam.hex /lib/firmware/
sudo cp $basedir/stfga.bin /lib/firmware/
sudo ldconfig
echo "Supporting files for SBIG USB devices installed."

