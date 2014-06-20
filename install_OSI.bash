#!/bin/bash
#  Install need libraries and compile the code and install
#  OpenSkyImager (OSI)

#  Install libraries needed for compiling.

echo
echo "Checking for needed libraries and installing if needed"
echo

libs="libgtk-3-0 libgtk-3-dev libgtk2.0-0 libgtk2.0-dev libglib2.0-0 libglib2.0-dev libcfitsio3-dev cmake" #libusb-1.0-0-dev
if [ -e `which apt-get` ]; then
	sudo apt-get install $libs
elif [ -e `which yum` ]; then
	sudo yum install $libs
elif [ -e `which pacman` ]; then
	sudo pacman -S $libs
else
	echo "Suitable package installer not found!"
	exit 1	
fi
if [ $? -gt 0 ]; then
    exit 1
fi

# make a build folder
if [ ! -d "./build" ]; then
	mkdir ./build
fi
cd ./build

# configuring the environment
cmake ..
if [ $? -gt 0 ]; then
    exit 1
fi
# actual compile / install
sudo make install
if [ $? -gt 0 ]; then
    exit 1
fi

