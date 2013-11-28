/*
 * imgCFWio.c
 *
 *  Created on: 20.11.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Device access code (USB) is based on original QHYCCD.inc code from 
 * https://github.com/qhyccd-lzr
 *
 * Device access code (TTY) is original work based on the protocol document as
 * released by QHYCCD.inc:
 * http://qhyccd.com/en/left/qhy-colorwheel/
 * http://qhyccd.com/ccdbbs/index.php?topic=1083.0
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

#include "ttycom.h"
#include "ttylist.h"
#include "imgBase.h"
#include <sys/stat.h>
#include <glib/gi18n.h>
#include "imgCamio.h"
#include "imgCFWio.h"

#define READ_TIME   5

static char cfwmsg[1024];
static int  cfwmode;
static char cfwtty[256];
static int  cfwttyfd;
static char cfwmodel[256];
static int  cfwmodid;
static int  cfwslotc;
static int  cfwslots[16];
static int  cfwpos;

void imgcfw_init()
{
	cfwmsg[0] = '\0'; 
	cfwmode = 0;
	cfwtty[0] = '\0';
	cfwttyfd = -1;
	cfwmodid = -1;
	cfwmodel[0] = '\0';
	cfwslotc = 0;
	memset(cfwslots, 0, 16);
	cfwpos = -1;
}

int imgcfw_set_mode(int mode)
{
	int retval = 1;

	cfwmsg[0] = '\0';
	switch (mode)
	{
		case 0:
		case 1:
			// QHY-Serial
			cfwmode = mode;
			retval = 1;
			break;
		
		case 99:
			// QHY-Through-camera
			if ((imgcam_connected() == 1) && (strlen(imgcam_get_camui()->whlstr) > 0))
			{
				cfwmode = mode;
				retval = 1;
			}
			else if (imgcam_connected() == 0)
			{
				sprintf(cfwmsg, C_("cfw","Camera is not connected"));
			}
			else if (strlen(imgcam_get_camui()->whlstr) == 0)
			{
				sprintf(cfwmsg, C_("cfw","Camera does not support direct CFW connection"));
			}
	}
	return (retval);
}

int imgcfw_get_mode()
{
	return cfwmode;
}

int imgcfw_set_tty(char *tty)
{
	int retval = 0;
	struct stat st;
	
	cfwmsg[0] = '\0';
	if (lstat(tty, &st) == 0 && S_ISLNK(st.st_mode))
	{
		strcpy(cfwtty, tty);
		retval = 1;
	}
	if (retval == 0)
	{
		sprintf(cfwmsg, C_("cfw","Serial port does not exist"));	
	}
	return (retval);
}

char *imgcfw_get_tty()
{
	return cfwtty;
}

int imgcfw_connect()
{
	int retval = 0;
	int ttyresult = 0;
	
	cfwmsg[0] = '\0';
	switch (cfwmode)
	{
		case 0:
			break;
		
		case 1:
			// QHY-Serial
			if ((ttyresult = tty_connect(cfwtty, 9600, 8, PARITY_NONE, 1, &cfwttyfd)) == TTY_OK)
			{
				retval = (imgcfw_get_slotcount() > 0);
			}
			else
			{
				char ttyerr[512];
				tty_error_msg(ttyresult, ttyerr, 512);
				sprintf(cfwmsg, C_("cfw","Could not connect to CFW on serial port %s, error: %s"), cfwtty, ttyerr);
			}
			break;
			
		case 99:
			// Qhy-through-camera
			if ((imgcam_connected() == 1) && (strlen(imgcam_get_camui()->whlstr) > 0))
			{
				retval = 1;
			}
			else if (imgcam_connected() == 0)
			{
				sprintf(cfwmsg, C_("cfw","Camera is not connected"));
			}
			else if (strlen(imgcam_get_camui()->whlstr) == 0)
			{
				sprintf(cfwmsg, C_("cfw","Camera does not support direct CFW connection"));
			}
			break;
	}
	return (retval);
}

int imgcfw_disconnect()
{
	int retval = 0;
	
	cfwmsg[0] = '\0';
	switch (cfwmode)
	{
		case 0:
			break;
		
		case 1:
			// QHY-Serial
			if ((retval = tty_disconnect(cfwttyfd)) == TTY_OK)
			{
				cfwttyfd = -1;
				cfwmodel[0] = '\0';
				cfwslotc = 0;
				memset(cfwslots, 0, 16);
				cfwpos = -1;
			}
			else
			{
				char ttyerr[512];
				tty_error_msg(retval, ttyerr, 512);
				sprintf(cfwmsg, C_("cfw","Could not disconnect from CFW on serial port %s, error: %s"), cfwtty, ttyerr);
			}
			break;
			
		case 99:
			// Qhy-through-camera
			retval = 1;
			break;
	}
	return (retval);
}

char *imgcfw_get_model()
{
	// This is real time function so it will make the entire program wait
	// for answer. It's supposed to be immediate answer.
	// Should this will prove unsafe... we'll change.
	int nbrw = 0;
	char buf[32];
	char wbuf[3] = {0x53, 0x45, 0x47};
	int ttyresult = 0;
	
	cfwmsg[0] = '\0';
	cfwmodel[0] = '\0';
	if (cfwttyfd > -1)
	{
		/* Flush the input buffer */
		tcflush(cfwttyfd,TCIOFLUSH);
		// Sending "SEG"
		if ((ttyresult = tty_write(cfwttyfd, wbuf, sizeof(wbuf), &nbrw)) == TTY_OK)
		{
			// Reading answer
			if ((ttyresult = tty_read(cfwttyfd, buf, 17, READ_TIME, &nbrw)) == TTY_OK)
			{
				cfwmodid = buf[0];
				switch (cfwmodid)
				{
					case 0:
						// QHY 2" 5 positions
						/// Model name
						sprintf(cfwmodel, C_("cfw","QHY 5 positions 2\""));
						cfwslotc = 5;
						cfwslots[0] = buf[1] * 256 + buf[2];
						cfwslots[1] = buf[3] * 256 + buf[4];
						cfwslots[2] = buf[5] * 256 + buf[6];
						cfwslots[3] = buf[7] * 256 + buf[8];
						cfwslots[4] = buf[9] * 256 + buf[10];
						break;
						
					default:
						/// Model name
						sprintf(cfwmodel, C_("cfw","Unknown model using QHY serial protocol"));
						// Load them all just in case
						cfwslots[0] = buf[1] * 256 + buf[2];
						cfwslots[1] = buf[3] * 256 + buf[4];
						cfwslots[2] = buf[5] * 256 + buf[6];
						cfwslots[3] = buf[7] * 256 + buf[8];
						cfwslots[4] = buf[9] * 256 + buf[10];
						
						cfwslots[5] = buf[11] * 256 + buf[12];
						cfwslots[6] = buf[13] * 256 + buf[14];
						cfwslots[7] = buf[15] * 256 + buf[16];
						break;
				}
			}
			else
			{
				char ttyerr[512];
				tty_error_msg(ttyresult, ttyerr, 512);
				sprintf(cfwmsg, C_("cfw","Could not read from CFW on serial port %s, error: %s"), cfwtty, ttyerr);
			}
		}
		else
		{
			char ttyerr[512];
			tty_error_msg(ttyresult, ttyerr, 512);
			sprintf(cfwmsg, C_("cfw","Could not write to CFW on serial port %s, error: %s"), cfwtty, ttyerr);
		}
	}
	return cfwmodel;
}

int imgcfw_get_slotcount()
{
	int retval = 0;
	
	if (strlen(imgcfw_get_model()) > 0)
	{
		retval = cfwslotc;
	}
	return retval;
}

int imgcfw_set_slot(int slot)
{
	int retval = 0;
	int ttyresult = 0;
	int nbrw = 0;
	char wbuf[1];
	
	switch (cfwmodid)
	{
		case 1:
			// Qhy Serial
			wbuf[0] = slot;
			cfwmsg[0] = '\0';
			if (slot < cfwslotc)
			{
				/* Flush the input buffer */
				tcflush(cfwttyfd, TCIOFLUSH);
				if ((ttyresult = tty_write(cfwttyfd, wbuf, sizeof(wbuf), &nbrw)) == TTY_OK)
				{
					retval = 1;
					sprintf(cfwmsg, C_("cfw","Filter wheel moving to slot: %d"), slot);
				}		
				else
				{
					char ttyerr[512];
					tty_error_msg(ttyresult, ttyerr, 512);
					sprintf(cfwmsg, C_("cfw","Could not write to CFW on serial port %s, error: %s"), cfwtty, ttyerr);
				}
			}
			else
			{
				sprintf(cfwmsg, C_("cfw","Requested slot (%d) does not exist. Slots = %d"), slot, cfwslotc);
			}
			break;
		
		case 99:
			// Qky-through-camera
			if ((retval = imgcam_wheel(slot)) == 0)
			{
				sprintf(cfwmsg, "%s", imgcam_get_msg());
			}
			break;
	}
	return (retval);
}
