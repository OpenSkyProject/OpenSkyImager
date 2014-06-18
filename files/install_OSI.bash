#!/bin/bash
#  Install need libraries and compile the code and install
#  OpenSkyImager (OSI)
#  This bash script should compile both GTK2 and GTK3 versions.

#  Install libraries needed for compiling.

echo
echo "Checking for needed libraries and installing if needed"
echo


sudo apt-get install libgtk-3-0 libgtk-3-dev libgtk2.0-0 libgtk2.0-dev libglib2.0-0 libglib2.0-dev libusb-dev libusb-1.0-0-dev libcfitsio3-dev

# configure libusb-custom
cd libusb-custom
./configure
make
cd ..

#  Make clean first then compile for GTK2,  make clean again then compile for GTK3,  finally make clean again.

echo 
sudo make GTK_VERSION=2 install

echo "GTK2 installed"
echo "Removing unwanted files"
echo 

make GTK_VERSION=2 clean

echo 
sudo make update

echo "GTK3 installed"
echo "Removing unwanted files"
echo

make clean

echo "Compilation and transfer is now complete"


#  To start either of the two programs use.
#  for the GTK2 vresion    gtkImager
#  for the GTK3 vresion    gtk3Imager
