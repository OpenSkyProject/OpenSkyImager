/*
 * libusbio.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original code adapted from examle code in the lib docs: http://libusb.sourceforge.net/doc/examples-code.html
 *
 * This file is part of "OpenSkyImager".
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include "libusbio.h"

int find_camera(int vendorid, int productid) 
{
	int retcode = 0;
	libusb_device **list;
	ssize_t i = 0;
	struct libusb_device_descriptor dev_desc;

	if(libusb_init(NULL) == 0)
	{
		ssize_t cnt = libusb_get_device_list(NULL, &list);
	
		if (cnt >= 0)
		{
			for (i = 0; i < cnt; i++) 
			{
				if (libusb_get_device_descriptor(list[i], &dev_desc) == 0) 
				{
					if (dev_desc.idVendor == vendorid && dev_desc.idProduct == productid) 
					{
						retcode = 1;
						break;
					}
				}
			}
		}
		libusb_free_device_list(list, 1);
	}
	return (retcode);
}

int open_camera(int vendorid, int productid, libusb_device_handle **handle, char *msg) 
{
	int retcode = 0;
	libusb_device **list;
	libusb_device *found = NULL;
	ssize_t i = 0;
	struct libusb_device_descriptor dev_desc;
	
	if(libusb_init(NULL) == 0)
	{
		ssize_t cnt = libusb_get_device_list(NULL, &list);
		
		if (cnt >= 0)
		{
			for (i = 0; i < cnt; i++) {
				libusb_device *device = list[i];
			    if (libusb_get_device_descriptor(device, &dev_desc) == 0) 
			    {
					if (dev_desc.idVendor == vendorid && dev_desc.idProduct == productid) 
					{
						found = device;
						break;
					}
			    }
			}

			if (found) 
			{
				if (libusb_open(found, handle) == 0)
				{
					retcode = 1;
				}
				else
				{
					if (msg != NULL)
					{
						sprintf(msg, C_("usbio","Error: Camera found, but did not open!"));
					}
					found = NULL;
				}
			}
			else
			{
				if (msg != NULL)
				{
					sprintf( msg, C_("usbio","Error: Camera NOT found!"));
				}
				found = NULL;
			}
		}
		libusb_free_device_list(list, 1);
	}
	return retcode;
}


