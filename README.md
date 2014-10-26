OpenSkyImager
=============
OpenSkyImager is a capture program written for Astronomy camera operation.

Some of the code has been borrowed and/or adapted from other GPL/LGPL or other
"free software" licensed projects.

Credits to individual authors or organizations can be found in source files
where most appropriate.

Therefore, this program is licensed under GNU GPL 3.0.

This program is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details. 

You should have received a copy of the GNU General Public License along with 
this program.  If not, see <http://www.gnu.org/licenses/>.


Supported camera list
==================
As of version 0.9.0 several QHY camera are actively developed against:
QHY2 (old color model),5,5II series,6 (both firmware),7,8(Old),8L,9,10,11,12,
IC8300.

Models that have been 100% tested are: QHY2(old color model), QHY5, 5II series,
6 (both firmware),7, 8(Old), 8L, 9, 11, 12, IC8300.

Please note that through the iAstroHub package:
<http://sourceforge.net/projects/iastrohub/>
OpenSkyImager can also be run on the IC8300 internal ARM system thus creating a
pc free astronomy setup.

An optional support for SBIG camera (usb/ethernet) has been add. This is through
sbigunidrv lib (Ver. 4.75). So ideally all models should be supported, while
ST10, ST2k, ST-i have been used for real. Other models have been tested on the 
SBIG "camera simulator".
All CFW models that are supported by unidrv should do, while CFW10 has been used
for real. Other models have been tested on the SBIG "camera simulator".
Library from latest devTookit has been provided to ease install process.

An optional support for SBIG parport camera has been add. Since unidrv dropped 
parport support, this is through a custom version URVC driver 
(<http://lascaux.asu.cas.cz/mates/urvc/index-en.php>).
Ideally ST237/4/5C/6/7/8/9/10/1001 should do, ST8 had been used for real.
No CFW is supported to date.

If your camera make/model does not appear in the list you can try contact us
and see if it can be add in a near future.
If your camera is in the list, but not 100% tested, please leat us know your
user experience. You're also most welcome to volunteer for helping bebug.

For different camera make a freely available official technical documentation 
must be available and legally available firmware (if applicable) are needed.
A GPL licensed linux SDK would sure speed up things a great deal.

For other QHY models we suggest to give a look at the official SDK release:
https://github.com/qhyccd-lzr 

If firmware for your camera appears there, we may already be able to proceed, 
but your assistence will be needed for testing.

If not a request can be submitted on the QHY forum http://www.qhyccd.com/ccdbbs


Color filter wheel support
=======================
As of version 0.9.0 both QHY 5 positions 2" and the new QHYCFW2 series are 
supported. Both with serial and through camera connection.
Models that are known to support cfw-through connection are QHY7/9/11/21/22/23.


Remote control
==============
Starting from version 0.8.0 OpenSkyImager offers a remote control feature.
For further details please see: [REMOTE.md](REMOTE.md)


Internationalization
==================
As of version 0.6.0 only embedded Us English, Italian, French and Chinese are 
available.
Please feel free to volunteer for additional languages.

The program does conform to "gettext", the de facto standard as far as linux 
internationalization is concerned.

More in-deep instructions and hints may be found in the "files/po" folder.


Compiling and installing
=====================
As of version 0.9.0 the build mechanism has moved to CMake (many thanks to 
Marco Gulino for his invaluable help in the process).

Usual CMake/Make commands can of course be used, but a couple of install scripts
have been provided, reworking those originally developed by Clive Rogers:

 - install_OSI.bash will install all that is needed to use QHY camera series
 - install_OSI_full.bash will add SBIG (unidrv) support

(Notice: to install with URVC (aka SBIG parport) please see manual)
 
Both scripts will check for dependencies, (prompt for install if needed), 
CMake / Make and install the program in a suitable folder (sudo will be needed
in order for the install process to be successful).

However should anyone prefer to do all by hands here's what's needed and how to:


Compiling your binary
===================
The program was written to compile with both GTK2 and GTK3 toolkit. 
Unless CMake is forced to, GTK3 will have precedence over GTK2, when both are
available on the system.

You will need these libraries, both binaries and dev.
Please get them from your repo:
GTK2 >= 2.20 / GTK3 >= 3.20
GLIB >= 2.24
CFITSIO 
LIBUDEV
FXLOAD
CMAKE

So far several combinations have been tested:
- GTK2 
	- Ubuntu 14.04 - unity (GTK2 2.24, Glib 2.40)
	- Mint13 (GTK2 2.24, Glib 2.32), 
	- Ubuntu 11.10 (GTK2 2.24 GLib 2.30)
	- Mint 9 (GTK2 2.20 GLib 2.24)
	
- GTK3 
	- Ubuntu 14.04 - unity (GTK3 3.10, Glib 2.40)
	- Mint LMDE amd64 (GTK3 3.82, Glib 2.36)
	- Mint13 (GTK3 3.42, Glib 2.32) 
	- Ubuntu 11.10 (GTK3 3.2 GLib 2.30)
	- Arm device (ASUS transformer TF101) using debian kit over android 4.1.x, 
	  running a deb wheezy, xrdp to display the X session

Please note that OpenSkyImager is now a part of iAstroHub automated astro 
imaging system (http://sourceforge.net/projects/iastrohub/), this means that 
all ARM devices there listed are also supported and tested for real.

To compile your binary, once you got suitable libs as listed above, create a 
"build" folder inside the OpenSkyImager tree.
Change to that folder and issue:
- cmake .. (cmake -D FORCE_QHY_ONLY=off .. in order to add SBIG unidrv support)
- make
- sudo make install

Install will alse create a desktop file to have the program show in the system
menu.

You can also use make clean to force a full recompile, make uninstall is also
available to cleanup installed files.
	  
You're not expected to see error messages or warnings while compiling. If so, 
let us know. 
Submit details for your hardware / software configuration so that we'll be 
able to track down any issue effectively.


User manual
==========
As of version 0.8.8 the first release of a user manual has been created, thanks
to the kind work of Mr. Clive Rogers.

Versions list
===========
Please see: [VERSIONS.md](VERSIONS.md)

Sincerely
*The OpenSkyProject team*


