/*
 * dsi2pro.c
 *
 *  Created on: 23.01.2014
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code by Maxim Parygin
 * Hints got from libdsi Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 * Hints got from lin_guider by Galaxy Master (http://galaxymstr.users.sourceforge.net/)
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

#define VENDOR_ID  0x156C
#define PRODUCT_ID 0x0101
#define SHORTEXP   0
#define FWFILE     "dsi2pro.hex"
#define FWPATH     "/etc/meade"
#define RAWPROD_ID 0x0100

// Endpoints
#define EP_IN 0x01
#define EP_OUT 0x81
#define EP_DATA 0x86

// ACK & NACK
#define RES_ACK 0x06
#define RES_NAK 0x15

// Commands
#define CMD_PING 0x00
#define CMD_RESET 0x01
#define CMD_ABORT 0x02
#define CMD_TRIGGER 0x03
#define CMD_CLEAR_TS 0x04

#define CMD_GET_VERSION 0x14
#define CMD_GET_STATUS 0x15
#define CMD_GET_TIMESTAMP 0x16
#define CMD_GET_EEPROM_LENGTH 0x1e
#define CMD_GET_EEPROM_BYTE 0x1f

#define CMD_SET_EEPROM_BYTE 0x20

#define CMD_GET_GAIN 0x32                        //
#define CMD_SET_GAIN 0x33                        //
#define CMD_GET_OFFSET 0x34                      //
#define CMD_SET_OFFSET 0x35                      //
#define CMD_GET_EXP_TIME 0x36                    //
#define CMD_SET_EXP_TIME 0x37                    //
#define CMD_GET_EXP_MODE 0x38                    //
#define CMD_SET_EXP_MODE 0x39                    //
#define CMD_GET_VDD_MODE 0x3a                    //
#define CMD_SET_VDD_MODE 0x3b                    //
#define CMD_GET_FLUSH_MODE 0x3c                  //
#define CMD_SET_FLUSH_MODE 0x3d                  //
#define CMD_GET_CLEAN_MODE 0x3e                  //
#define CMD_SET_CLEAN_MODE 0x3f                  //
#define CMD_GET_READOUT_SPD 0x40                 //
#define CMD_SET_READOUT_SPD 0x41                 //
#define CMD_GET_READOUT_MODE 0x42                //
#define CMD_SET_READOUT_MODE 0x43                //
#define CMD_GET_NORM_READOUT_DELAY 0x44          //
#define CMD_SET_NORM_READOUT_DELAY 0x45          //
#define CMD_GET_ROW_COUNT_ODD 0x46               //
#define CMD_SET_ROW_COUNT_ODD 0x47               //
#define CMD_GET_ROW_COUNT_EVEN 0x48              //
#define CMD_SET_ROW_COUNT_EVEN 0x49              //
#define CMD_GET_TEMP 0x4a                        //
#define CMD_GET_EXP_TIMER_COUNT 0x4b
#define CMD_PS_ON 0x64                           //
#define CMD_PS_OFF 0x65                          //
#define CMD_CCD_VDD_ON 0x66                      //
#define CMD_CCD_VDD_OFF 0x67                     //
#define CMD_AD_READ 0x68
#define CMD_AD_WRITE 0x69
#define CMD_TEST_PATTERN 0x6a
#define CMD_GET_DEBUG_VALUE 0x6b
#define CMD_GET_EEPROM_VIDPID 0x6c
#define CMD_SET_EEPROM_VIDPID 0x6d
#define CMD_ERASE_EEPROM 0x6e


// Parameters values
#define VDD_MODE_AUTOMATIC 0x00
#define VDD_MODE_ALWAYS_ON 0x01
#define VDD_MODE_ALWAYS_OFF 0x02

#define READOUT_MODE_DUAL_EXPOSURE 0x00
#define READOUT_MODE_SINGLE_EXPOSURE 0x01
#define READOUT_MODE_ODD_FIELDS 0x02
#define READOUT_MODE_EVEN_FIELDS 0x03

#define FLUSH_MODE_CONTINUOUS 0x00
#define FLUSH_MODE_BEFORE_EXPOSURE 0x01
#define FLUSH_MODE_BEFORE_NEVER 0x02

#define EXPOSURE_MODE_SINGLE 0x00
#define EXPOSURE_MODE_CONTINUOUS 0x01

#define CLEAN_MODE_ENABLED 0x00
#define CLEAN_MODE_DISABLED 0x01

#define READOUT_SPEED_NORMAL 0x00
#define READOUT_SPEED_HIGH 0x01


// HW properties
#define IMG_WIDTH 752
#define IMG_HEIGHT 582
#define IMG_EVEN 299
#define IMG_ODD 298
#define IMG_CHUNK 2048
#define IMG_CHUKN_EVEN (IMG_CHUNK*IMG_EVEN)
#define IMG_CHUKN_ODD (IMG_CHUNK*IMG_ODD)


#include "imgBase.h"
#include "libusbio.h"
#include "imgCamio.h"
#include "dsi2pro.h"

static char *coremsg;
static char fwpath[2048];
static int bin, width, height;

static struct libusb_device_handle *rc_dev_dsi = NULL;
static int seq = 1;
static unsigned char rawA[IMG_CHUKN_EVEN];
static unsigned char rawB[IMG_CHUKN_ODD];

/////////////////////
// Private methods //
/////////////////////
static int rc_responseImage(unsigned char *data, int len) 
{
	int r, t;

	coremsg[0] = '\0';
	// transfer
	r = libusb_bulk_transfer(rc_dev_dsi, EP_DATA, data, len, &t, 20000);

	// result	
	if (r < 0) 
	{
		sprintf(coremsg, "rc_responseImage error: %d\n", r);
	}
	return (r == 0);
}

static int send_command(int cmd, int arg, int in, int out) 
{
	unsigned char data[7];
	int retval, ret, t;
	unsigned int value;

	coremsg[0] = '\0';
	// request
	switch (in) 
	{
		case 0:
			data[0] = 3;
			data[1] = seq++;
			data[2] = cmd;
			ret = libusb_bulk_transfer(rc_dev_dsi, EP_IN, data, 3, &t, 0);
			break;
		case 1:
			data[0] = 4;
			data[1] = seq++;
			data[2] = cmd;
			data[3] = arg;
			ret = libusb_bulk_transfer(rc_dev_dsi, EP_IN, data, 4, &t, 0);
			break;
		case 2:
			data[0] = 5;
			data[1] = seq++;
			data[2] = cmd;
			data[3] = arg;
			data[4] = (arg >> 8);
			ret = libusb_bulk_transfer(rc_dev_dsi, EP_IN, data, 5, &t, 0);
			break;
		case 4:
			data[0] = 7;
			data[1] = seq++;
			data[2] = cmd;
			data[3] = arg;
			data[4] = (arg >> 8);
			data[5] = (arg >> 16);
			data[6] = (arg >> 24);
			ret = libusb_bulk_transfer(rc_dev_dsi, EP_IN, data, 7, &t, 0);
			break;
		default:
			sprintf(coremsg, "Unknown IN: %d\n", in);
			ret = 1;
	}
	
	if(ret == 0)
	{
		// read response
		switch(out) 
		{
			case 0:
				ret = libusb_bulk_transfer(rc_dev_dsi, EP_OUT, data, 3, &t, 0);
				retval = data[2];
				break;
			case 1:
				ret = libusb_bulk_transfer(rc_dev_dsi, EP_OUT, data, 4, &t, 0);
				retval = data[3] & 0xFF;
				break;
			case 2:
				ret = libusb_bulk_transfer(rc_dev_dsi, EP_OUT, data, 5, &t, 0);
				retval = (data[3] & 0xFF) + ((data[4] & 0xFF) << 8);
				break;
			case 4:
				ret = libusb_bulk_transfer(rc_dev_dsi, EP_OUT, data, 7, &t, 0);
				retval= (data[3] & 0xFF) + ((data[4] & 0xFF) << 8) + ((data[5] & 0xFF) << 16) + ((data[6] & 0xFF) << 24);
				break;
			case 5:
				ret = libusb_bulk_transfer(rc_dev_dsi, EP_OUT, data, 5, &t, 0);
				//Decode bytes 4-5 of the buffer as a 16-bit big-endian unsigned integer.
				value = (unsigned int) ((data[4] << 8) | data[3]);
				retval = value; 
				break;
			default:
				sprintf(coremsg, "Unknown OUT: %d\n", out);
				ret = 1;
		}
	}
	return (ret == 0) ? retval : 0;
}

static int RESET() 
{
	return send_command(CMD_RESET, 0, 0, 0) == RES_ACK;
}

static int SET_GAIN(int value) 
{
	return send_command(CMD_SET_GAIN, value, 1, 0) == RES_ACK;
}

static int SET_OFFSET(int value) 
{
	return send_command(CMD_SET_OFFSET, value, 2, 0) == RES_ACK;
}

static int SET_ROW_COUNT_EVEN(int value) 
{
	return send_command(CMD_SET_ROW_COUNT_EVEN, value, 2, 0) == RES_ACK;
}

static int SET_ROW_COUNT_ODD(int value) 
{
	return send_command(CMD_SET_ROW_COUNT_ODD, value, 2, 0) == RES_ACK;
}

static int SET_VDD_MODE(int value) 
{
	return send_command(CMD_SET_VDD_MODE, value, 1, 0) == RES_ACK;
}

static int SET_FLUSH_MODE(int value) 
{
	return send_command(CMD_SET_FLUSH_MODE, value, 1, 0) == RES_ACK;
}

static int SET_CLEAN_MODE(int value) 
{
	return send_command(CMD_SET_CLEAN_MODE, value, 1, 0) == RES_ACK;
}

static int SET_READOUT_MODE(int value) 
{
	return send_command(CMD_SET_READOUT_MODE, value, 1, 0) == RES_ACK;
}

static int SET_READOUT_SPD(int value) 
{
	return send_command(CMD_SET_READOUT_SPD, value, 1, 0) == RES_ACK;
}

static int SET_EXP_TIME(int value) 
{
	return send_command(CMD_SET_EXP_TIME, value, 4, 0) == RES_ACK;
}

static int SET_EXP_MODE(int value) 
{
	return send_command(CMD_SET_EXP_MODE, value, 1, 0) == RES_ACK;
}
   
static int SET_NORM_READOUT_DELAY(int value) 
{
	return send_command(CMD_SET_NORM_READOUT_DELAY, value, 2, 0) == RES_ACK;
}

////////////////////
// Public methods //
////////////////////
void dsi2pro_init()
{
	static int first_time = 1;
	if (first_time)
	{
		coremsg = (char*)realloc(coremsg, 1024);
		first_time = 0;
	}
	coremsg[0] = '\0';
	
	strcpy(fwpath, FWPATH);

	// Camio params
	// Positively no tec
	imgcam_get_tecp()->istec      = 0;      // 0 = Not driveable tec or no tec 1 = Driveable tec 2 = Read only tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 0;      // Basically 0 - tecmax
	imgcam_get_tecp()->tecmax     = 0;      // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
	
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2:0");
	strcpy(imgcam_get_camui()->roistr, "");
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, "");
	strcpy(imgcam_get_camui()->ampstr, C_("camio","0-Auto|1-AmpOn|2-AmpOff:0"));
	strcpy(imgcam_get_camui()->modstr, C_("camio","0-Double|1-Single:1"));
	/// Descriptiopn for "mode" combo box
	strcpy(imgcam_get_camui()->moddsc, C_("camio","Mode S/D Image"));
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "-1");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	// Header values
	imgcam_get_camui()->pszx = 8.60;
	imgcam_get_camui()->pszy = 8.30;
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;
}

int  dsi2pro_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int dsi2pro_OpenCamera()
{
	int retcode = 1;

	coremsg[0] = '\0';

	retcode = (libusb_init(NULL) == 0);
	coremsg[0] = '\0';
	if(retcode)
	{
		// Level 0: no messages ever printed by the library (default)
		// Level 1: error messages are printed to stderr
		// Level 2: warning and error messages are printed to stderr
		// Level 3: informational messages are printed to stdout, warning and error messages are printed to stderr
		libusb_set_debug(NULL,0);
		if (open_camera(VENDOR_ID, PRODUCT_ID, &rc_dev_dsi, coremsg))
		{
			if (libusb_claim_interface(rc_dev_dsi, 0) == 0) 
			{
				// Send bonjour to the camera
				RESET();
				//        SET_GAIN(0);
				//        SET_OFFSET(300);
				SET_ROW_COUNT_EVEN(IMG_EVEN);
				SET_ROW_COUNT_ODD(IMG_ODD);
				SET_VDD_MODE(VDD_MODE_AUTOMATIC); //original->//SET_VDD_MODE(1); I hope it can take amp mode from gui
				SET_FLUSH_MODE(FLUSH_MODE_BEFORE_EXPOSURE); //original->SET_FLUSH_MODE(0);unsure if continuous is a problem for single shot operation
				SET_CLEAN_MODE(CLEAN_MODE_ENABLED);
				SET_READOUT_MODE(READOUT_MODE_SINGLE_EXPOSURE); //original->SET_READOUT_MODE(0); I hope it can accept the 0/1 values from gui 
				SET_READOUT_SPD(READOUT_SPEED_HIGH); 
				SET_EXP_MODE(EXPOSURE_MODE_SINGLE);   //original->SET_EXP_MODE(1);unsure if that mean camera shoot in a tight loop, we don't want this
				//        SET_EXP_TIME(50);
				SET_NORM_READOUT_DELAY(1);
			}
			else
			{
				sprintf(coremsg, C_("dsi2pro","Error: Could not claim interface."));
				retcode = 0;
			}
		}
	}
	else
	{
		sprintf(coremsg, C_("dsi2pro","Error: Could not initialise libusb."));
		retcode = 0;
	}
	return retcode;
}

int dsi2pro_CloseCamera() 
{
	libusb_close(rc_dev_dsi);
	libusb_exit(NULL);
	return 1;
}

int dsi2pro_StartExposure(qhy_exposure *expar)
{
	int retval = 1;
	static int gain = 0, offset = 0, mode = 0, amp = 0;

//plouis
	expar->width   = IMG_WIDTH ;
	expar->height   = IMG_HEIGHT;
//end of plouis
	if (expar->edit)
	{
		// Wait time is just = exposure time in this case
		expar->wtime = expar->time;
		
		// These are pretty fixed as there's no real bin mode
		expar->tsize   = IMG_WIDTH * IMG_HEIGHT * 2;
		expar->totsize = IMG_WIDTH * IMG_HEIGHT * 2;
	
		// Support for software bin
		bin    = expar->bin;
		width  = (IMG_WIDTH / bin); 
		height = (IMG_HEIGHT / bin);
		
		// Send shoot parameters to camera
		if ((int)((expar->gain * 63) / 100) != gain)
		{
			gain = (int)((expar->gain * 63) / 100);
			retval = (retval == 1) ? SET_GAIN(gain) : retval;		
		}
		if ((int)(expar->offset * (1023 / 255)) != offset)
		{
			offset = (int)(expar->offset * (1023 / 255));
			retval = (retval == 1) ? SET_OFFSET(offset) : retval;
		}
		if (expar->mode != mode)
		{
			mode = expar->mode;
			retval = (retval == 1) ? SET_READOUT_MODE(mode) : retval;
		}
		if (expar->amp != amp)
		{
			amp = expar->amp;
			SET_VDD_MODE(amp);
		}
		retval = (retval == 1) ? SET_EXP_TIME(expar->time * 10) : retval;
	}
	return (retval == 1) ? (send_command(CMD_TRIGGER, 0, 0, 0) == RES_ACK) : retval;
}

int dsi2pro_AbortCapture()
{
	return (send_command(CMD_ABORT, 0, 0, 0) == RES_ACK);
}

double dsi2pro_GetTemp()
{
	// This is quite a guess out of libdsi
	return (double)send_command(CMD_GET_TEMP, 0, 0, 5);
}

char *dsi2pro_core_msg()
{
	return coremsg;
}

int dsi2pro_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", VENDOR_ID, RAWPROD_ID, VENDOR_ID, PRODUCT_ID, FWPATH, FWFILE);
	retval = system(cmd);
	switch (WEXITSTATUS(retval))
	{
		case 0:
			strcpy(imgcam_get_msg(), "");
			break;
		case 1:
			sprintf(imgcam_get_msg(), C_("camio","Camera still not found after reset"));
			break;
		case 2: 
			sprintf(imgcam_get_msg(), C_("camio","Neither raw device nor programmed camera found."));
			break;
		case 3: 
			sprintf(imgcam_get_msg(), C_("camio","This camera also needs a loader that was not found"));
			break;
	}
	return ((retval == 0) ? 1 : 0);
}

int dsi2pro_getImgData()
{
	int retval = 0;
	
	// get image by two bulk_reads
	if (rc_responseImage(rawA, IMG_CHUKN_EVEN))
	{
		if (rc_responseImage(rawB, IMG_CHUKN_ODD))
		{
			retval = 1;
		}
	}
	return (retval);
}

void dsi2pro_decode(unsigned char *databuffer)
{
	int i, j, k, l;
		
	// merge interleave blocks
	for (i = 5, j = 0, k = 5 * IMG_CHUNK + 58; i < 296; i++, j += IMG_WIDTH * 4, k += IMG_CHUNK) 
	{
		//System.arraycopy(rawA, k, raw, j, IMG_WIDTH * 2);
		for (l = 0; l < IMG_WIDTH * 2; l++) 
		{
			databuffer[j + l] = rawA[k + l];
		}
	}
	for (i = 5, j = IMG_WIDTH * 2, k = 5 * IMG_CHUNK + 58; i < 296; i++, j += IMG_WIDTH * 4, k += IMG_CHUNK) 
	{ 
		//System.arraycopy(rawB, k, raw, j, IMG_WIDTH * 2);
		for (l = 0; l < IMG_WIDTH * 2; l++) 
		{
			databuffer[j + l] = rawB[k + l];
		}
	}

	// swap bytes
	for (i = 0; i < IMG_WIDTH * IMG_HEIGHT * 2; i += 2) 
	{
		k = databuffer[i];
		databuffer[i] = databuffer[i+1];
		databuffer[i+1] = k;
	} 
	if (bin == 2)
	{
		// Software binning goes here
	}
}
