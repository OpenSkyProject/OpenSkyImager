/*
 * qhi5ii.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Device access code is based on original QHY code from https://github.com/qhyccd-lzr
 *
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

#define VENDOR_ID   0x1618
#define PRODUCT_ID  0x0921
#define SHORTEXP    0
#define FWFILE      "qhy5ii.hex"
#define MAXW        1280
#define MAXH        960
#define IMGOFFSET   5
#define USBTRAFFIC  30 // The higher the slower, max 255. (30, suggested)

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy5ii.h"

static int camvariant = 0, camcolor = 0;

// These are shared within the module and set from setRegisters
static int exptime, gain, bin, speed, bytepix, hdr, width, height, totalsize, transfer_size;
static int longExpMode, usbspd;
static int setgain[73]={0x004,0x005,0x006,0x007,0x008,0x009,0x00A,0x00B,
				   0x00C,0x00D,0x00E,0x00F,0x010,0x011,0x012,0x013,0x014,
				   0x015,0x016,0x017,0x018,0x019,0x01A,0x01B,0x01C,0x01D,
				   0x01E,0x01F,0x051,0x052,0x053,0x054,0x055,0x056,0x057,
				   0x058,0x059,0x05A,0x05B,0x05C,0x05D,0x05E,0x05F,0x6CE,
				   0x6CF,0x6D0,0x6D1,0x6D2,0x6D3,0x6D4,0x6D5,0x6D6,0x6D7,
				   0x6D8,0x6D9,0x6DA,0x6DB,0x6DC,0x6DD,0x6DE,0x6DF,0x6E0,
				   0x6E1,0x6E2,0x6E3,0x6E4,0x6E5,0x6E6,0x6E7,0x6FC,0x6FD,0x6FE,0x6FF};
static double pllratio;

void qhy5ii_init()
{
	qhy_core_init();

	qhy_core_getendp()->info   = 0xC2;
	qhy_core_getendp()->read   = 0xC0;
	qhy_core_getendp()->write  = 0x40;
	qhy_core_getendp()->iread  = 0x81;
	qhy_core_getendp()->iwrite = 0x01;
	qhy_core_getendp()->bulk   = 0x82;
	qhy_core_getendp()->aux    = 0;

	qhy_core_getreq()->sendregs  = 0xB5;
	qhy_core_getreq()->startexp  = 0xB3;
	qhy_core_getreq()->setdc201  = 0;
	qhy_core_getreq()->getdc201  = 0;
	qhy_core_getreq()->shutter   = 0;
	qhy_core_getreq()->wheel     = 0;
	
	qhy_core_getcampars()->vid        = VENDOR_ID;
	qhy_core_getcampars()->pid        = PRODUCT_ID;
	qhy_core_getcampars()->shortexp   = SHORTEXP;
	qhy_core_getcampars()->buftimes   = 0;
	qhy_core_getcampars()->buftimef   = 0;
	
	// Positively no tec
	imgcam_get_tecp()->istec      = 0;      // 0 = Not driveable tec or no tec 1 = Driveable tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 0;      // Basically 0 - tecmax
	imgcam_get_tecp()->tecmax     = 0;      // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
	
	strcpy(imgcam_get_camui()->binstr, "");
	strcpy(imgcam_get_camui()->roistr, "");
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
	strcpy(imgcam_get_camui()->ampstr, "");
	strcpy(imgcam_get_camui()->modstr, "0:255:5:50");
	/// Descriptiopn for "mode" combo box
	strcpy(imgcam_get_camui()->moddsc, C_("camio","Usb bus speed"));
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "");
	strcpy(imgcam_get_camui()->byrstr, "-1");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	// Header values
	imgcam_get_camui()->pszx = 0;
	imgcam_get_camui()->pszy = 0;
	
	imgcam_get_expar()->bitpix  = 8;	
	imgcam_get_expar()->bytepix = 1;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy5ii_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy5ii_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", VENDOR_ID, (PRODUCT_ID - 1), VENDOR_ID, PRODUCT_ID, qhy_getFwPath(), FWFILE);
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

int qhy5ii_setregisters(qhy_exposure *expar)
{
	int retval = 1;
	int ch, cw, cu, ce, cg, cs, cB; 
	static int bpp = 1;
	
	expar->wtime = expar->time;
	expar->bin = 1;
	if (expar->edit)
	{
		// Set switches
		cw = (width   != expar->width);
		ch = (height  != expar->height);
		cu = (usbspd  != expar->mode);
		ce = (exptime != expar->time);
		cg = (gain    != expar->gain);
		cs = (speed   != expar->speed);
		cB = (bpp     != expar->bytepix);
		// Set "class" properties
		width   = expar->width;
		height  = expar->height;
		usbspd  = expar->mode;
		exptime = (expar->time > 1000) ? expar->time / 1.3 : expar->time; // Temporary fix for clock inaccuracy
		gain    = expar->gain;
		bin     = expar->bin;
		speed   = expar->speed;
		bytepix = (expar->bytepix > 1) ? 2 : 1;
		hdr     = (expar->bytepix > 2) ? 1 : 0;
		bpp     = expar->bytepix;
		//printf ("Bpp: %d, Hdr: %d\n", bytepix, hdr);
		// Camera set
		if (cw || ch || cB)
		{ 
			if (camvariant == 1)
			{
				//QHY5L-II
				retval = (retval == 1) ? qhy5lii_SetDepth(bytepix) : 0;
				retval = (retval == 1) ? qhy5lii_SetHDR(hdr) : 0; 
			}
			// If change size the whole shebang must be reset
			retval = (retval == 1) ? qhy5ii_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? qhy5ii_SetSpeed(speed) : 0;
			retval = (retval == 1) ? qhy5ii_set_imgsize(width, height) : 0;
			retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime) : 0;
			retval = (retval == 1) ? qhy5ii_SetGain(gain) : 0;
			retval = (retval == 1) ? qhy5ii_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime): 0;
		}
		else
		{
			// If not change size each parameters can be set
			if (cu)
			{ 
				retval = (retval == 1) ? qhy5ii_SetUSBTraffic(usbspd) : 0;
			}
			if (cs)
			{ 
				retval = (retval == 1) ? qhy5ii_SetSpeed(speed) : 0;
			}
			if (ce)
			{ 
				retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime) : 0;
			}
			if (cg)
			{ 
				retval = (retval == 1) ? qhy5ii_SetGain(gain) : 0;
			}
		}
		// Reg set
		totalsize      = width * height * bytepix;
		// IMGOFFSET bytes are not returned when in 800x600 mode. Firmware issue?
		transfer_size  = totalsize + ((width != 800) ? IMGOFFSET : 0);
		expar->totsize = totalsize;
		expar->tsize   = transfer_size;

		expar->edit = 0;
	}
	else
	{
		retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime) : 0;
	}
	return (retval);
}

void qhy5ii_decode(unsigned char *databuffer)
{
	int i = 0, pixval = 0;
		
	if ((bytepix > 1) && (hdr == 0))
	{
		// 12Bit -> 16Bit
		while (i < totalsize)
		{
			// Read value big endian (times 16)
			pixval = ((databuffer[i + 1] + (databuffer[i] << 4)) << 4);
			// Writes value little endian
			databuffer[i + 1] = pixval >> 8;
			databuffer[i]     = pixval - databuffer[i + 1];
			i += 2;
		}
	}
	else if ((bytepix > 1) && (hdr == 1))
	{
		// Hdr -> 16Bit
		while (i < totalsize)
		{
			// Read value big endian
			/*pixval = (databuffer[i + 1] + (databuffer[i] << 4));
			if (pixval > 2048)
			{
				// Hdr compromise decoding (20Bit -> 16Bit) see doc.
				pixval = ((pixval - 2048) * 31.014655594) + 2048;
			}*/
			// My version of hdr complete 20Bit -> 16Bit
			// First 2048 camera levels are mapped to 16384 (* 8 on whole array to simplify math)
			// From 2048 to 3040 (16384 to 24320) are mapped into 16384 to 32737 (see doc)
			// From 3041 to 4095 (24320 to 32760) are mapped into 32767 to 65535 (see doc)
			// Read value big endian
			pixval = ((databuffer[i + 1] + (databuffer[i] << 4)) << 3);
			if ((pixval > 16384) && (pixval <= 24320))
			{
				pixval = ((pixval - 16384) * 2.064390121) + 16384;
			}
			else if (pixval > 24320)
			{
				pixval = ((pixval - 24320) * 3.882464455) + 32767;
			}
			// Writes value little endian 
			databuffer[i + 1] = pixval >> 8;
			databuffer[i]     = pixval - databuffer[i + 1];
			i += 2;
		}
	}
}

int qhy5ii_AbortCapture()
{
	unsigned char REG[4];
	REG[0] = 0; REG[1] = 0; REG[2] = 0; REG[3] = 0;
	return qhy_cameraIO(qhy_core_getendp()->write, 0xc1, REG, sizeof(REG), 0, 0);
}

int qhy5ii_bonjour()
{
	int retval = 0;
	unsigned char buf[16];

	if ((retval = qhy_EepromRead(0x10, buf, 16)) == 1)
	{
		if(buf[1] == 1)
		{
			// Color
			camcolor = 1;
			strcpy(imgcam_get_camui()->byrstr, "3");
		}
		else 
		{
			// Mono
			camcolor = 0;
			strcpy(imgcam_get_camui()->byrstr, "-1");
		}
		// This will reset and "wake" the camera
		if ((retval = qhy5ii_AbortCapture()) == 1)
		{
			// set Ui
			if(buf[0] == 6)
			{
				// QHY5L-II
				camvariant = 1;
				// Positively no tec (temp read only, no thread)
				imgcam_get_tecp()->istec = 2;
				strcpy(imgcam_get_camui()->roistr, "1280x960|1024x768|800x600|640x480|320x240:0");
				strcpy(imgcam_get_camui()->snrstr, "");
				strcpy(imgcam_get_camui()->bppstr, "2-12Bit|1-8Bit|3-Hdr:1");
				// Header values
				imgcam_get_camui()->pszx = 3.75;
				imgcam_get_camui()->pszy = 3.75;
				width   = 1280;
				height  = 960;
				bytepix = 1;
				hdr     = 0;
				retval = qhy5lii_SetDepth(bytepix);
			}
			else if(buf[0] == 1)
			{
				// QHY5-II
				camvariant = 0;
				// Positively no tec
				imgcam_get_tecp()->istec = 0;
				strcpy(imgcam_get_camui()->roistr, "1280x1024|1280x720|1024x768|960x720|800x800|800x600|640x480|400x400|320x240:0");
				/// Combo box values list, keep N-<desc> format. Just translate <desc>
				strcpy(imgcam_get_camui()->snrstr, C_("camio","0-Off|1-On:0"));
				strcpy(imgcam_get_camui()->bppstr, "1-8Bit|:0");
				// Header values
				imgcam_get_camui()->pszx = 5.20;
				imgcam_get_camui()->pszy = 5.20;
				width   = 1280;
				height  = 1024;
				bytepix = 1;
				hdr     = 0;
				retval  = 1;
			}
			// Complete camera init
			usbspd  = 255;
			longExpMode = 0;
			exptime = 1;
			gain    = 20;
			bin     = 1;
			speed   = 0;
			retval = (retval == 1) ? qhy5ii_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? qhy5ii_SetSpeed(speed) : 0;
			retval = (retval == 1) ? qhy5ii_set_imgsize(width, height) : 0;
			retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime) : 0;
			retval = (retval == 1) ? qhy5ii_SetGain(gain) : 0;
			retval = (retval == 1) ? qhy5ii_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? qhy5ii_SetExposureTime(exptime): 0;
		}
	}
	if (retval == 0)
	{
		strcpy(imgcam_get_msg(), qhy_core_msg());
	}
	return (retval);
}

int qhy5ii_SetSpeed(int spd)
{
	// Both variants
	// i=0,1,2    0=12M  1=24M  2=48M
	unsigned char buf[2];

	if (spd == 1)
	{
		buf[0] = (bytepix > 1) ? 1 : 2;
	}
	else
	{
		buf[0] = (bytepix > 1) ? 0 : 1;
	}
	return (qhy_cameraIO(qhy_core_getendp()->write, 0xc8, buf, sizeof(buf), 0x00, 0x00));
}

int qhy5ii_SetUSBTraffic(int i)
{
	int retval = 0;
	
	// Both variants
	if (width == 1280)
	{
		retval = qhy_I2CTwoWrite(0x300c, 1650 + i*50);
	}
	else
	{
		retval = qhy_I2CTwoWrite(0x300c, 1388 + i*50);
	}
	return (retval);
}

int qhy5lii_SetHDR(int on) 
{
	int retval = 0;
	/*HDR only in 1280*960 and 1024*768 
	Interesting note, thank you. 
	The DR compression ratio depends on R0x3082 content, as I see. 
	So the QHY5L-II firmware set HDR mode with highest factors (16x/16x), right? 
	And reconstruction needs to replace the data like this:
	In range 0 - 2048 -> 0 - 2048 (slope 1.0)
	2049 - 3040 -> 2049 - 64k (slope 0.046)
	3041 - 4000 -> 64k - 1M (slope 0.0038), right? */
	
	//printf("Width: %d, On: %d\n", width, on);
	if ((width == 1280) || (width == 1024))
	{
		if (on == 1)
		{
			retval = qhy_I2CTwoWrite(0x3082, 0x0028);
		}
		else
		{
			retval = qhy_I2CTwoWrite(0x3082, 0x0001);
		}
	}
	else if (on == 0)
	{
		retval = 1;
	}
	else
	{
		sprintf(imgcam_get_msg(), "Hdr is only allowed for 1280x960 or 1024x768");
	}
	return (retval);
}

int qhy5lii_SetDepth(int Bpp)
{
	unsigned char buf[2];
	
	buf[0] = (Bpp == 2) ? 1 : 0;
	return (qhy_cameraIO(qhy_core_getendp()->write, 0xcd, buf, 1, 0x00, 0x00));
}

int qhy5ii_SetExposureTime(int etime) 
{
	// Both variants
	if (camvariant == 0)
	{
		//QHY5-II
		// Required input parameters: CMOSCLK  REG04  REG05 REG0C REG09

		double CMOSCLK;

		if (speed == 1)
		{
			CMOSCLK = 48;
		}
		else
		{
			CMOSCLK = 24;
		}
		
		double pixelPeriod;
		pixelPeriod = 1 / CMOSCLK; // unit: us

		double A, Q;
		double P1, P2;
		double RowTime; // unit: us
		unsigned long ExpTime; // unit: us
		unsigned short REG04, REG05, REG0C, REG09;
		double MaxShortExpTime;

		REG04 = qhy_I2CTwoRead(0x04);
		REG05 = qhy_I2CTwoRead(0x05);
		REG09 = qhy_I2CTwoRead(0x09);
		REG0C = qhy_I2CTwoRead(0x0C);

		// Get the microseconds
		ExpTime = etime * 1000;

		A = REG04 + 1;
		P1 = 242;
		P2 = 2 + REG05 - 19;
		Q = P1 + P2;
		RowTime = (A + Q) * pixelPeriod;

		MaxShortExpTime = 15000 * RowTime - 180 * pixelPeriod - 4 * REG0C * pixelPeriod;

		unsigned char buf[4];

		if (ExpTime > MaxShortExpTime) 
		{
			qhy_I2CTwoWrite(0x09, 15000);

			ExpTime = (unsigned long )(ExpTime - MaxShortExpTime);

			buf[0] = 0;
			buf[1] = (unsigned char) (((ExpTime / 1000)&~0xff00ffff) >> 16);
			buf[2] = ((ExpTime / 1000)&~0xffff00ff) >> 8;
			buf[3] = ((ExpTime / 1000)&~0xffffff00);

			qhy_cameraIO(qhy_core_getendp()->write, 0xc1, buf, 4, 0x00, 0x00);

			ExpTime = (unsigned long)(ExpTime + MaxShortExpTime);
			longExpMode = 1;
		}

		else 
		{
			buf[0] = 0;
			buf[1] = 0;
			buf[2] = 0;
			buf[3] = 0;

			qhy_cameraIO(qhy_core_getendp()->write, 0xc1, buf, 4, 0x00, 0x00);

			usleep(100);
			REG09 = (unsigned short)((ExpTime + 180 * pixelPeriod + 4 * REG0C * pixelPeriod)/ RowTime);
			if (REG09 < 1)
			{
				REG09 = 1;
			}	
			qhy_I2CTwoWrite(0x09, REG09);
			ExpTime = (unsigned long)(REG09 * RowTime - 180 * pixelPeriod - 4 * REG0C * pixelPeriod);
			longExpMode = 0;
		}
	}
	else if (camvariant == 1)
	{
		//QHY5L-II
		/*if(emodchg == 1)
		{
			//when exposure mode changed, reset resolution to get the new value of QHY5L_PLL_Ratio
			qhy5ii_set_imgsize(width, height);
			emodchg = 0;
		}*/
		
		// Required input parameters: CMOSCLK
		double CMOSCLK;

		if (speed == 1)
		{
			CMOSCLK = (bytepix == 2) ? 24 : 48;
		}
		else
		{
			CMOSCLK = (bytepix == 2) ? 12 : 24;
		}

		double pixelPeriod;
		pixelPeriod = 1 / (CMOSCLK * pllratio); // unit: us

		double RowTime;
		unsigned long ExpTime;
		unsigned short REG300C, REG3012;

		double MaxShortExpTime;

		REG300C = qhy_I2CTwoRead(0x300C);

		RowTime = REG300C * pixelPeriod;

		MaxShortExpTime = 65000 * RowTime;

		// Get the microseconds!
		ExpTime = etime * 1000;

		unsigned char buf[4];

		if (ExpTime > MaxShortExpTime) 
		{
			if (longExpMode == 0) 
			{
				// Camera need to "enter" the long exp mode
				//printf("Enter long exp mode\n");
				longExpMode = 1;
				//when exposure mode changed, reset resolution to get the new value of QHY5L_PLL_Ratio
				//qhy5ii_set_imgsize(width, height);
			}
			// Proper long exp mode command
			qhy_I2CTwoWrite(0x3012, 65000);
			ExpTime = ExpTime - MaxShortExpTime;

			buf[0] = 0;
			buf[1] = (unsigned char)(((ExpTime / 1000) & ~0xff00ffff) >> 16);
			buf[2] = (unsigned char)(((ExpTime / 1000) & ~0xffff00ff) >> 8);
			buf[3] = (unsigned char)((ExpTime / 1000) & ~0xffffff00);
		
			qhy_cameraIO(qhy_core_getendp()->write, 0xc1, buf, 4, 0x00, 0x00);
		
			ExpTime = ExpTime + MaxShortExpTime;
			REG3012 = 65000;
		}
		else 
		{

			if (longExpMode == 1) 
			{
				// Camera must "exit" long exp mode
				longExpMode = 0;
				//printf("Exit long exp mode\n");
				//when exposure mode changed, reset resolution to get the new value of QHY5L_PLL_Ratio
				//qhy5ii_set_imgsize(width, height);
				// After switching over, it should be performed once more
				qhy5ii_AbortCapture();

				usleep(100);
				REG3012 = (unsigned short)(ExpTime / RowTime);
				if (REG3012 < 1)
				{
					REG3012 = 1;
				}
				qhy_I2CTwoWrite(0x3012, REG3012);
				ExpTime = (unsigned long)(REG3012 * RowTime);
				longExpMode = 0;
			}

			qhy5ii_AbortCapture();

			usleep(100);
			REG3012 = (unsigned short)(ExpTime / RowTime);
			if (REG3012 < 1)
			{
				REG3012 = 1;
			}
			qhy_I2CTwoWrite(0x3012, REG3012);
			ExpTime = (unsigned long)(REG3012 * RowTime);
		}
	}
	return 1;
}

int qhy5ii_SetGain(int gain)
{
	if (camvariant == 0)
	{
		//QHY5-II
	    int i = (int)(72. * (gain / 100.));
	    qhy_I2CTwoWrite(0x35, setgain[i]);
	}
	else if (camvariant == 1)
	{
		//QHY5L-II
		if (longExpMode == 1)
		{
			qhy5ii_SetExposureTime(1);
			if(camcolor == 1)
			{
				/*double RG,BG;
				RG = (double)QCam.wbred / 100.;
				BG = (double)QCam.wbblue / 100.;
				qhy5lii_SetGainColor(gain,RG,BG);*/
				qhy5lii_SetGainColor(gain, 1, 1);
			}
			else
			{
				qhy5lii_SetGainMono(gain);
			}
			usleep(500000);
			qhy5ii_SetExposureTime(exptime);
		}
		else
		{
			if(camcolor == 1)
			{
				/*double RG,BG;
				RG = (double)QCam.wbred / 100.;
				BG = (double)QCam.wbblue / 100.;
				qhy5lii_SetGainColor(gain,RG,BG);*/
				qhy5lii_SetGainColor(gain, 1, 1);
			}
			else
			{
				qhy5lii_SetGainMono(gain);
			}
		}
	}
	return 1;
}

void qhy5lii_SetGainMono(double gain)
{
	double Gain_Min, Gain_Max;

	Gain_Min = 0.;
	//Gain_Max = 39.8;
	Gain_Max = 79.6;
	
	gain = (Gain_Max - Gain_Min) * gain / 100;
	gain = MAX(1, gain);
	
	//printf("Final gain: %f\n", gain);

	unsigned short REG30B0;

	if (longExpMode == 1)
	{
		REG30B0 = 0X5330;
	}
	else
	{
		REG30B0 = 0X1330;
	}

	unsigned short baseDGain;

	double C[8] = {10, 8, 5, 4, 2.5, 2, 1.25, 1};
	double S[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int A[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int B[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	double Error[8];

     int i;
	for (i = 0; i < 8; i++) 
     {
		S[i] = gain / C[i];
		A[i] = (int)(S[i]);
		B[i] = (int)((S[i] - A[i]) / 0.03125);
		if (A[i] > 7)
		{
			A[i] = 10000; // A limitation in the range of 1-3
		}
		if (A[i] == 0)
		{
			A[i] = 10000; // A limitation in the range of 1-3
		}
		Error[i] = fabs(((double)(A[i])+(double)(B[i]) * 0.03125) * C[i] - gain);
	}

	double minValue;
	int minValuePosition;

	minValue = Error[0];
	minValuePosition = 0;

	for (i = 0; i < 8; i++) 
	{

		if (minValue > Error[i]) 
		{
			minValue = Error[i];
			minValuePosition = i;
		}
	}
	// Form1->Edit6->Text=Form1->Edit6->Text+"minPosition="+AnsiString(minValuePosition)+"minValue="+minValue;
	int AA, BB; //, CC;
	//double DD;
	//double EE;

	AA = A[minValuePosition];
	BB = B[minValuePosition];
	if (minValuePosition == 0) 
	{
		//CC = 8;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x30);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 1) 
	{
		//CC = 8;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x30);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 2) 
	{
		//CC = 4;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x20);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 3) 
	{
		//CC = 4;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x20);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 4) 
	{
		//CC = 2;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x10);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 5) 
	{
		//CC = 2;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x10);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 6) 
	{
		//CC = 1;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x00);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 7) 
	{
		//CC = 1;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x00);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}

	//EE = fabs(((double)(AA)+(double)(BB) * 0.03125) * CC * DD - gain);

	baseDGain = BB + AA * 32;
	qhy_I2CTwoWrite(0x305E, baseDGain);

}

void qhy5lii_SetGainColor(double gain, double RG, double BG)
{
	double Gain_Min, Gain_Max;

	Gain_Min = 0.;
	Gain_Max = 39.8;

	if (gain < 26.)
	{
		gain = 26.;
	}
	gain = (Gain_Max - Gain_Min) * gain / 100;

	unsigned short REG30B0;

	if (longExpMode == 1)
	{
		REG30B0 = 0x5330;
	}
	else
	{
		REG30B0 = 0x1330;
	}

	unsigned short baseDGain;

	double C[8] = {10, 8, 5, 4, 2.5, 2, 1.25, 1};
	double S[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int A[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int B[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	double Error[8];

     int i;
	for (i = 0; i < 8; i++) 
	{
		S[i] = gain / C[i];
		A[i] = (int)(S[i]);
		B[i] = (int)((S[i] - A[i]) / 0.03125);
		if (A[i] > 3)
		{
			A[i] = 10000; // A limitation in the range of 1-3
		}
		if (A[i] == 0)
		{
			A[i] = 10000; // A limitation in the range of 1-3
		}
		Error[i] = fabs(((double)(A[i])+(double)(B[i]) * 0.03125) * C[i] - gain);
	}

	double minValue;
	int minValuePosition;

	minValue = Error[0];
	minValuePosition = 0;

	for (i = 0; i < 8; i++) 
	{
		if (minValue > Error[i]) 
		{
			minValue = Error[i];
			minValuePosition = i;
		}
	}

	int AA, BB; //CC;
	//double DD;
	//double EE;

	AA = A[minValuePosition];
	BB = B[minValuePosition];
	if (minValuePosition == 0) 
	{
		//CC = 8;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x30);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 1) 
	{
		//CC = 8;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x30);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 2) 
	{
		//CC = 4;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x20);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 3) 
	{
		//CC = 4;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x20);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 4) 
	{
		//CC = 2;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x10);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 5) 
	{
		//CC = 2;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x10);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}
	else if (minValuePosition == 6) 
	{
		//CC = 1;
		//DD = 1.25;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x00);
		qhy_I2CTwoWrite(0x3EE4, 0XD308);
	}
	else if (minValuePosition == 7) 
	{
		//CC = 1;
		//DD = 1;
		qhy_I2CTwoWrite(0x30B0, (REG30B0 &~0x0030) + 0x00);
		qhy_I2CTwoWrite(0x3EE4, 0XD208);
	}

	//EE = fabs(((double)(AA)+(double)(BB) * 0.03125) * CC * DD - gain);

	baseDGain = BB + AA * 32;

	qhy_I2CTwoWrite(0x3058, (unsigned short)(baseDGain*BG));
	qhy_I2CTwoWrite(0x305a, (unsigned short)(baseDGain*RG));
	qhy_I2CTwoWrite(0x305c, baseDGain);
	qhy_I2CTwoWrite(0x3056, baseDGain);
}

int qhy5ii_set_imgsize(int wdt, int hgt)
{
	// Both variants
	int retval = 1;
	
	if (camvariant == 0)
	{
		//QHY5-II
		switch (width)
		{
			case 1280:
				if (height == 1024)
				{
					qhy5ii_set_Resolution();
				}
				else if (height == 720)
				{
					qhy5ii_set_Resolution();
				}
				break;
			case 1024:
				qhy5ii_set_Resolution();
			case 960:
				qhy5ii_set_Resolution();
				break;
			case 800:
				if (height == 800)
				{
					qhy5ii_set_Resolution();
				}
				else if (height == 600)
				{
					qhy5ii_set_Resolution();
				}
				break;
			case 640:
				qhy5ii_set_Resolution();
			case 400:
				qhy5ii_set_Resolution();
				break;
			case 320:
				qhy5ii_set_Resolution();
				break;
			default:
				retval = 0;
				break;
		}
	}
	else if (camvariant == 1)
	{
		//QHY5L-II
		switch (width)
		{
			case 1280:
				qhy5lii_set_1280x960();
				break;
			case 1024:
				qhy5lii_set_1024x768();
				break;
			case 800:
				qhy5lii_set_800x600();
				break;
			case 640:
				qhy5lii_set_640x480();
				break;
			case 320:
				qhy5lii_set_320x240();
				break;
			default:
				retval = 0;
				break;
		}
	}
	else
	{
		retval = 0;
	}
	return (retval);
}

double qhy5lii_GetTemp()
{
	double slope;
	double T0;
	uint16_t sensed, calib1, calib2;

	// start measuring
	qhy_I2CTwoWrite(0x30B4, 0x0011);

	// reading the calibration params gives just enough time	
	calib1 = qhy_I2CTwoRead(0x30C6);
	calib2 = qhy_I2CTwoRead(0x30C8);
	
	// stop measuring
	qhy_I2CTwoWrite(0x30B4, 0x0000);
	sensed = qhy_I2CTwoRead(0x30B2);
	
	slope = (70.0 - 55.0)/(calib1 - calib2);
	T0 = (slope*calib1 - 70.0);
	return slope * sensed - T0;
}

void qhy5ii_set_Resolution()
{	
	qhy_I2CTwoWrite(0x09, 200);
	qhy_I2CTwoWrite(0x01, 8 + (1024 - height) / 2); // y start
	qhy_I2CTwoWrite(0x02, 16 + (1280 - width) / 2); // x start
	qhy_I2CTwoWrite(0x03, (unsigned short)(height - 1)); // y size
	qhy_I2CTwoWrite(0x04, (unsigned short)(width - 1)); // x size
	qhy_I2CTwoWrite(0x22, 0x00); // normal bin
	qhy_I2CTwoWrite(0x23, 0x00); // normal bin
}

void qhy5lii_set_1280x960()
{
	qhy5liiInitRegs();
	pllratio = qhy5lii_setPLL(0);

	int xstart = 4;
	int ystart = 4;
	int xsize = width - 1;
	int ysize = height - 1;

	qhy_I2CTwoWrite(0x3002, ystart); // y start
	qhy_I2CTwoWrite(0x3004, xstart); // x start
	qhy_I2CTwoWrite(0x3006, ystart + ysize); // y end
	qhy_I2CTwoWrite(0x3008, xstart + xsize); // x end
	qhy_I2CTwoWrite(0x300a, 990); // frame length
	qhy_I2CTwoWrite(0x300c, 1650); // line  length
	qhy_I2CTwoWrite(0x301A, 0x10DC); // RESET_REGISTER
}

void qhy5lii_set_1024x768() 
{
	qhy5liiInitRegs();
	pllratio = qhy5lii_setPLL(0);

	int xstart = 4 + (MAXW - width) / 2;
	int ystart = 4 + (MAXH - height) / 2;
	int xsize = width - 1;
	int ysize = height - 1;

	qhy_I2CTwoWrite(0x3002, ystart); // y start
	qhy_I2CTwoWrite(0x3004, xstart); // x start
	qhy_I2CTwoWrite(0x3006, ystart + ysize); // y end
	qhy_I2CTwoWrite(0x3008, xstart + xsize); // x end
	qhy_I2CTwoWrite(0x300a, 795); // frame length
	qhy_I2CTwoWrite(0x300c, 1388); // line  length
	qhy_I2CTwoWrite(0x301A, 0x10DC); // RESET_REGISTER
}

void qhy5lii_set_800x600()
{
	qhy5liiInitRegs();
     pllratio = qhy5lii_setPLL(2);

	int xstart = 4 + (MAXW - width) / 2; ;
	int ystart = 4 + (MAXH - height) / 2; ;
	int xsize = width - 1;
	int ysize = height - 1;

	qhy_I2CTwoWrite(0x3002, ystart); // y start
	qhy_I2CTwoWrite(0x3004, xstart); // x start
	qhy_I2CTwoWrite(0x3006, ystart + ysize); // y end
	qhy_I2CTwoWrite(0x3008, xstart + xsize); // x end
	qhy_I2CTwoWrite(0x300a, 626); // frame length
	qhy_I2CTwoWrite(0x300c, 1388); // line  length
	qhy_I2CTwoWrite(0x301A, 0x10DC); // RESET_REGISTER
}

void qhy5lii_set_640x480()
{
	qhy5liiInitRegs();
	pllratio = qhy5lii_setPLL(1);

	int xstart = 4 + (MAXW - width) / 2; ;
	int ystart = 4 + (MAXH - height) / 2; ;
	int xsize = width - 1;
	int ysize = height - 1;

	qhy_I2CTwoWrite(0x3002, ystart); // y start
	qhy_I2CTwoWrite(0x3004, xstart); // x start
	qhy_I2CTwoWrite(0x3006, ystart + ysize); // y end
	qhy_I2CTwoWrite(0x3008, xstart + xsize); // x end
	qhy_I2CTwoWrite(0x300a, 506); // frame length
	qhy_I2CTwoWrite(0x300c, 1388); // line  length
	qhy_I2CTwoWrite(0x301A, 0x10DC); // RESET_REGISTER
}

void qhy5lii_set_320x240() 
{
	qhy5liiInitRegs();
	pllratio = qhy5lii_setPLL(1);

	int xstart = 4 + (MAXW - width) / 2; ;
	int ystart = 4 + (MAXH - height) / 2; ;
	int xsize = width - 1;
	int ysize = height - 1;

	qhy_I2CTwoWrite(0x3002, ystart); // y start
	qhy_I2CTwoWrite(0x3004, xstart); // x start
	qhy_I2CTwoWrite(0x3006, ystart + ysize); // y end
	qhy_I2CTwoWrite(0x3008, xstart + xsize); // x end
	qhy_I2CTwoWrite(0x300a, 266); // frame length
	qhy_I2CTwoWrite(0x300c, 1388); // line  length
	qhy_I2CTwoWrite(0x301A, 0x10DC); // RESET_REGISTER
}

void qhy5liiInitRegs()
{
	// [720p, 25fps input27Mhz,output50Mhz, ]
	qhy_I2CTwoWrite(0x301A, 0x0001); // RESET_REGISTER
	qhy_I2CTwoWrite(0x301A, 0x10D8); // RESET_REGISTER
	usleep(100000);
	/////Linear sequencer
	qhy_I2CTwoWrite(0x3088, 0x8000); // SEQ_CTRL_PORT
	qhy_I2CTwoWrite(0x3086, 0x0025); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2D26); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0828); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0D17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0926); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0526); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA728); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0725); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x8080); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2925); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0040); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2702); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2706); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1F17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x3626); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA617); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0326); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA417); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1F28); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0526); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0425); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2700); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x171D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0519); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2706); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1741); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2660); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x175A); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2317); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1122); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1741); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x9027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0026); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1828); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x002E); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2A28); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x081C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7003); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7004); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7005); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7009); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x170C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0014); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0014); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0314); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0314); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0414); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0414); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0514); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2405); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5001); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2550); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x502D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2608); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x280D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1709); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2600); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A7); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2807); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2580); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x8029); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0216); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1627); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0620); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1736); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A6); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A4); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x171F); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2620); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2804); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2520); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1D25); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1710); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1A17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0327); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0617); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0317); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4126); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x6017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xAE25); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0090); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2700); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2618); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2800); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2E2A); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2808); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1D05); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7009); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1720); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2024); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5002); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2550); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x502D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2608); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x280D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1709); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2600); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A7); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2807); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2580); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x8029); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0216); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1627); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0617); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x3626); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA617); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0326); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA417); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1F28); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0526); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0425); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2700); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x171D); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2021); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1710); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1B17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0327); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0617); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0317); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4126); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x6017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xAE25); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0090); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2700); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2618); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2800); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2E2A); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2808); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1E17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0A05); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7009); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2024); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x502B); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x302C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2C2C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2C00); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0225); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2D26); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0828); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0D17); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0926); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0526); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0xA728); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0725); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x8080); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2917); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0525); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0040); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2702); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1616); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2706); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1736); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A6); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x26A4); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x171F); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2805); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2620); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2804); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2520); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1E25); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2117); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1028); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x051B); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2706); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1703); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1747); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2660); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x17AE); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2500); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x9027); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0026); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1828); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x002E); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2A28); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x081E); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0831); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1440); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4014); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1410); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1034); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1014); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x4013); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1802); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7004); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7003); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1470); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x7017); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2002); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2002); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5004); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2004); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x1400); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x5022); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0314); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0020); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0314); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x0050); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2C2C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x3086, 0x2C2C); // SEQ_DATA_PORT
	qhy_I2CTwoWrite(0x309E, 0x018A); // RESERVED_MFR_309E
	qhy_I2CTwoWrite(0x301A, 0x10D8); // RESET_REGISTER
	qhy_I2CTwoWrite(0x3082, 0x0029); // OPERATION_MODE_CTRL
	qhy_I2CTwoWrite(0x301E, 0x00C8); // DATA_PEDESTAL
	qhy_I2CTwoWrite(0x3EDA, 0x0F03); // RESERVED_MFR_3EDA
	qhy_I2CTwoWrite(0x3EDE, 0xC007); // RESERVED_MFR_3EDE
	qhy_I2CTwoWrite(0x3ED8, 0x01EF); // RESERVED_MFR_3ED8
	qhy_I2CTwoWrite(0x3EE2, 0xA46B); // RESERVED_MFR_3EE2
	qhy_I2CTwoWrite(0x3EE0, 0x067D); // RESERVED_MFR_3EE0
	qhy_I2CTwoWrite(0x3EDC, 0x0070); // RESERVED_MFR_3EDC
	qhy_I2CTwoWrite(0x3044, 0x0404); // DARK_CONTROL
	qhy_I2CTwoWrite(0x3EE6, 0x4303); // RESERVED_MFR_3EE6
	qhy_I2CTwoWrite(0x3EE4, 0xD208); // DAC_LD_24_25
	qhy_I2CTwoWrite(0x3ED6, 0x00BD); // RESERVED_MFR_3ED6
	qhy_I2CTwoWrite(0x3EE6, 0x8303); // RESERVED_MFR_3EE6
	qhy_I2CTwoWrite(0x30E4, 0x6372); // RESERVED_MFR_30E4
	qhy_I2CTwoWrite(0x30E2, 0x7253); // RESERVED_MFR_30E2
	qhy_I2CTwoWrite(0x30E0, 0x5470); // RESERVED_MFR_30E0
	qhy_I2CTwoWrite(0x30E6, 0xC4CC); // RESERVED_MFR_30E6
	qhy_I2CTwoWrite(0x30E8, 0x8050); // RESERVED_MFR_30E8
	usleep(200);
	qhy_I2CTwoWrite(0x302A, 14); // DIV           14
	qhy_I2CTwoWrite(0x302C, 1); // DIV
	qhy_I2CTwoWrite(0x302E, 3); // DIV
	qhy_I2CTwoWrite(0x3030, 65); // MULTI          44
	qhy_I2CTwoWrite(0x3082, 0x0029);
	// OPERATION_MODE_CTRL
	qhy_I2CTwoWrite(0x30B0, 0x5330);
	qhy_I2CTwoWrite(0x305e, 0x00ff); // gain
	qhy_I2CTwoWrite(0x3012, 0x0020);
	// coarse integration time
	qhy_I2CTwoWrite(0x3064, 0x1802);
}

double qhy5lii_setPLL(unsigned char clk)
{
	double i = 0;

	if (clk == 0) 
	{

		qhy_I2CTwoWrite(0x302A, 14); // DIV           14
		qhy_I2CTwoWrite(0x302C, 1); // DIV
		qhy_I2CTwoWrite(0x302E, 3); // DIV
		qhy_I2CTwoWrite(0x3030, 42); // MULTI          44

		qhy_I2CTwoWrite(0x3082, 0x0029);
		// OPERATION_MODE_CTRL

		if (longExpMode == 1) 
		{
			qhy_I2CTwoWrite(0x30B0, 0x5330);
			// DIGITAL_TEST    5370: PLL BYPASS   1370  USE PLL
			i = 1.0;
		}
		else 
		{
			qhy_I2CTwoWrite(0x30B0, 0x1330);
			i = 1.0;
		}
		// 5330
		qhy_I2CTwoWrite(0x305e, 0x00ff); // gain
		qhy_I2CTwoWrite(0x3012, 0x0020);
		// coarse integration time

		qhy_I2CTwoWrite(0x3064, 0x1802);
		// EMBEDDED_DATA_CTRL
	}
	else if (clk == 1) 
	{

		qhy_I2CTwoWrite(0x302A, 14); // DIV           14
		qhy_I2CTwoWrite(0x302C, 1); // DIV
		qhy_I2CTwoWrite(0x302E, 3); // DIV
		qhy_I2CTwoWrite(0x3030, 65); // MULTI          44

		qhy_I2CTwoWrite(0x3082, 0x0029);
		// OPERATION_MODE_CTRL

		if (longExpMode == 1) 
		 {
			qhy_I2CTwoWrite(0x30B0, 0x5330);
			// DIGITAL_TEST    5370: PLL BYPASS   1370  USE PLL
			i = 1.0;
		}
		else 
		{
			qhy_I2CTwoWrite(0x30B0, 0x1330);
			i = 65. / 14. / 3.;
		}
		qhy_I2CTwoWrite(0x305e, 0x00ff); // gain
		qhy_I2CTwoWrite(0x3012, 0x0020);
		// coarse integration time

		qhy_I2CTwoWrite(0x3064, 0x1802);
		// EMBEDDED_DATA_CTRL
	}
	else if (clk == 2) 
	{

		qhy_I2CTwoWrite(0x302A, 14); // DIV           14
		qhy_I2CTwoWrite(0x302C, 1); // DIV
		qhy_I2CTwoWrite(0x302E, 3); // DIV
		qhy_I2CTwoWrite(0x3030, 57); // MULTI          44

		qhy_I2CTwoWrite(0x3082, 0x0029);
		// OPERATION_MODE_CTRL

		if (longExpMode == 1)  
		{
			qhy_I2CTwoWrite(0x30B0, 0x5330);
			// DIGITAL_TEST    5370: PLL BYPASS   1370  USE PLL
			i = 1.0;
		}
		else 
		{
			qhy_I2CTwoWrite(0x30B0, 0x1330);
			i = 57. / 14. / 3.;
		}

		qhy_I2CTwoWrite(0x305e, 0x00ff); // gain
		qhy_I2CTwoWrite(0x3012, 0x0020);
		// coarse integration time

		qhy_I2CTwoWrite(0x3064, 0x1802);
		// EMBEDDED_DATA_CTRL
	}
	return i;
}

