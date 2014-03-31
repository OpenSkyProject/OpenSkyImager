#!/bin/bash
#  Install need libraries and compile the code and install
#  OpenSkyImager (OSI)
#  This bash script should compile both GTK2 and GTK3 versions.

#  Install libraries needed for compiling.

echo
echo "Checking for needed libraries and installing if needed"
echo


sudo apt-get install libgtk-3-0 libgtk-3-dev libgtk2.0-0 libgtk2.0-dev libglib2.0-0 libglib2.0-dev libusb-dev libusb-1.0-0-dev libcfitsio3-dev

#  Make clean first then compile for GTK2,  make clean again then compile for GTK3,  finally make clean again.
rm *.o
make -f gtkImager.mak

echo "GTK2 compiled"
echo "Removing unwanted files"
echo 

rm *.o
make -f gtk3Imager.mak
echo "GTK3 compiled"
echo "Removing unwanted files"
echo

rm *.o
echo "Copying files to /usr/local/bin/OpenSkyImager"
echo

sudo rm -r /usr/local/bin/OpenSkyImager
sudo mkdir /usr/local/bin/OpenSkyImager
sudo chown root:video /usr/local/bin/OpenSkyImager
sudo cp -p gtkImager /usr/local/bin/OpenSkyImager
sudo cp -p gtk3Imager /usr/local/bin/OpenSkyImager
sudo cp -p -r fr_FR /usr/local/bin/OpenSkyImager
sudo cp -p -r it_IT /usr/local/bin/OpenSkyImager
sudo cp -p -r zh_CN /usr/local/bin/OpenSkyImager
sudo cp -p -r po /usr/local/bin/OpenSkyImager
sudo cp -p qhyReset.bash /usr/local/bin/OpenSkyImager
sudo cp -p *.png /usr/local/bin/OpenSkyImager
sudo chown root:video /usr/local/bin/OpenSkyImager/*
sudo chmod a+rx /usr/local/bin/OpenSkyImager/gtkImager
sudo chmod a+rx /usr/local/bin/OpenSkyImager/gtk3Imager
sudo chmod a+rx /usr/local/bin/OpenSkyImager/qhyReset.bash
echo 

echo "Compilation and transfer is now complete"


#  To start either of the two programs use.
#  for the GTK2 vresion    gtkImager
#  for the GTK3 vresion    gtk3Imager
