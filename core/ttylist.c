/*
 * ttylist.c
 *
 *  Created on: 20.11.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 Original code from from Søren Holm on a StakOverflow page:
 http://stackoverflow.com/questions/2530096/how-to-find-all-serial-devices-ttys-ttyusb-on-linux-without-opening-them 
 Licensed under cc-wiki with attibution required as per: 
 http://blog.stackoverflow.com/2009/06/attribution-required/ 
 *
 * Ported from cpp original to gcc and be part of: "OpenSkyImager".
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
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#define SYSDIR 	"/sys/class/tty/"
#define TRYRFCOM	0

void get_driver(char *ttydir, char *ttydrv) 
{
	struct stat st;
	char devicedir[1024];
	char buffer[1024];
	int len = 0;
	
	// Append '/device' to the tty-path
	sprintf(devicedir, "%s/device", ttydir);

	// Stat the devicedir and handle it if it is a symlink
	if (lstat(devicedir, &st)==0 && S_ISLNK(st.st_mode))
	{
		//memset(buffer, 0, sizeof(buffer));
		buffer[0] = '\0';
		// Append '/driver' and return basename of the target
		strcat(devicedir, "/driver");

		if ((len = readlink(devicedir, buffer, sizeof(buffer))) > 0)
		{
			buffer[len] = '\0';
			strcpy(ttydrv, basename(buffer));
		}
		else
		{
			ttydrv[0] = '\0';
		}
	}
	else
	{
		ttydrv[0] = '\0';
	}
}


int getComList(char *ttylist)
{
	int retval = 0;
	struct dirent **namelist;
	struct serial_struct serinfo;
	char devicedir[1024];
	char devfile[256];
	char driver[64];
	int n = 0, nn, fd;

	ttylist[0] = '\0';
	if ((nn = scandir(SYSDIR, &namelist, NULL, NULL)) > 0)
	{
		while (n < nn) 
		{
			if (strcmp(namelist[n]->d_name,"..") && strcmp(namelist[n]->d_name,".")) 
			{

				// Construct full absolute file path
				sprintf(devicedir, "%s%s", SYSDIR, namelist[n]->d_name);
				if (strstr(devicedir, "rfcomm") != NULL)
				{
					// Since rfcomm it's a special case.
					sprintf(devfile, "/dev/%s", namelist[n]->d_name);	
					#if TRYRFCOM
						// Check rfcommXX-devices separeately
						if ((fd = open(devfile, O_RDWR | O_NONBLOCK | O_NOCTTY)) >= 0) 
						{
							// If device open
							if (ioctl(fd, TIOCGSERIAL, &serinfo) == 0) 
							{
								// If can get get serial_info
								if (serinfo.type != PORT_UNKNOWN)
								{
									// If device type is no PORT_UNKNOWN we accept the port
									//printf("Device rfcommXX has port, accepted\n");
									strcat(ttylist, "|");
									strcat(ttylist, devfile);
								}
							}
							close(fd);
						}
					#else
						strcat(ttylist, "|");
						strcat(ttylist, devfile);
					#endif
				}
				else
				{
					// Get the device driver
					get_driver(devicedir, driver);
					if (strlen(driver) > 0)
					{
						// Non empty drivers might be ok
						//printf("Device: /dev/%s, Driver: %s\n", namelist[n]->d_name, driver);
						sprintf(devfile, "/dev/%s", namelist[n]->d_name);					

						if (strstr(driver, "8250") != NULL)
						{
							// Check serial8250-devices separeately
							if ((fd = open(devfile, O_RDWR | O_NONBLOCK | O_NOCTTY)) >= 0) 
							{
								// If device open
								if (ioctl(fd, TIOCGSERIAL, &serinfo) == 0) 
								{
									// If can get get serial_info
									if (serinfo.type != PORT_UNKNOWN)
									{
										// If device type is no PORT_UNKNOWN we accept the port
										//printf("Device 8250 has port, accepted\n");
										strcat(ttylist, "|");
										strcat(ttylist, devfile);
									}
								}
								close(fd);
							}
						} 
						else
						{
							// whatever has a driver and is not serial8250 is sure good
							strcat(ttylist, "|");
							strcat(ttylist, devfile);
						}
					}
				}
			}
			free(namelist[n]);
			n++;
		}
		free(namelist);	
	}
	return (retval);	
}























