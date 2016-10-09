/*
 * minicam5.c
 *
 *  Created on: 30.06.2013
 *      Author: Anat Ruangrassamee (aruangrassamee@gmail.com)
 *      		Giampiero Spezzano (gspezzano@gmail.com)
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
#define PRODUCT_ID  0x0931
#define SHORTEXP    0
#define FWFILE      "miniCam5.hex"
#define IMGOFFSET   5
#define USBTRAFFIC  30 // The higher the slower, max 255. (30, suggested)

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "minicam5.h"

// Not meant to be used elsewhere
static int    minicam5_SetSpeed(int i);
static int    minicam5_SetUSBTraffic(int i);
static int    minicam5_SetHDR(int on);
static int    minicam5_SetDepth(int Bpp);
static int    minicam5_SetExposureTime(int etime);
static int    minicam5_SetGain(int gain);
static void   minicam5_SetGainMono(double gain);
static void   minicam5_SetGainColor(double gain, double RG, double BG);
static int    minicam5_set_imgsize(int width, int height);
static void   minicam5_set_1280x960();
static void   minicam5_set_1024x768();
static void   minicam5_set_800x600();
static void   minicam5_set_640x480();
static void   minicam5_set_320x240();
static void   minicam5InitRegs();
static double minicam5_setPLL(unsigned char clk);

static int camvariant = 1, camcolor = 0;

// These are shared within the module and set from setRegisters
static int exptime, gain, bin, speed, bytepix, hdr, width, height, totalsize, transfer_size;
static int maxwidth, maxheight;
static int longExpMode, usbspd;
static double pllratio;

void minicam5_init()
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
	qhy_core_getreq()->setdc201  = 0XC6;
	qhy_core_getreq()->getdc201  = 0XC5;
	qhy_core_getreq()->shutter   = 0XC7;
	qhy_core_getreq()->wheel     = 0xD0;
	
	qhy_core_getcampars()->vid        = VENDOR_ID;
	qhy_core_getcampars()->pid        = PRODUCT_ID;
	qhy_core_getcampars()->shortexp   = SHORTEXP;
	qhy_core_getcampars()->buftimes   = 0;
	qhy_core_getcampars()->buftimef   = 0;
	
	// Positively yes tec
	imgcam_get_tecp()->istec      = 1;      // 0 = Not driveable tec or no tec 1 = Driveable tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 5;      // Basically 0 - tecmax, value here is used for initial set on camera open 
	imgcam_get_tecp()->tecmax     = 255;    // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
		
	imgcam_get_camui()->hasgain = 1;
	imgcam_get_camui()->hasoffset = 0;
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
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 1;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  minicam5_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  minicam5_reset()
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

int minicam5_setregisters(qhy_exposure *expar)
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
		//exptime = (expar->time > 1000) ? expar->time / 1.3 : expar->time; // Temporary fix for clock inaccuracy
		exptime = (expar->time > 1000) ? expar->time / 1.4 : expar->time; // Temporary fix for clock inaccuracy
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
			retval = (retval == 1) ? minicam5_SetDepth(bytepix) : 0;
			retval = (retval == 1) ? minicam5_SetHDR(hdr) : 0; 
			// If change size the whole shebang must be reset
			retval = (retval == 1) ? minicam5_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? minicam5_SetSpeed(speed) : 0;
			retval = (retval == 1) ? minicam5_set_imgsize(width, height) : 0;
			retval = (retval == 1) ? minicam5_SetExposureTime(exptime) : 0;
			retval = (retval == 1) ? minicam5_SetGain(gain) : 0;
			retval = (retval == 1) ? minicam5_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? minicam5_SetExposureTime(exptime): 0;
		}
		else
		{
			// If not change size each parameters can be set
			if (cu)
			{ 
				retval = (retval == 1) ? minicam5_SetUSBTraffic(usbspd) : 0;
				ce = 1;
			}
			if (cs)
			{ 
				retval = (retval == 1) ? minicam5_SetSpeed(speed) : 0;
				ce = 1;
			}
			if (ce)
			{ 
				retval = (retval == 1) ? minicam5_SetExposureTime(exptime) : 0;
			}
			if (cg)
			{ 
				retval = (retval == 1) ? minicam5_SetGain(gain) : 0;
			}
		}
		// Reg set
		totalsize      = width * height * bytepix;
		// IMGOFFSET bytes are not returned when in 800x600 mode. Firmware issue?
		transfer_size  = totalsize + IMGOFFSET;
		//transfer_size  = totalsize + (((width != 800) && (width != 2592)) ? IMGOFFSET : 0);
		expar->totsize = totalsize;
		expar->tsize   = transfer_size;

		expar->edit = 0;
	}
	else
	{
		if (exptime > 1000)
		{
			retval = (retval == 1) ? minicam5_SetExposureTime(exptime) : 0;
		}
	}
	return (retval);
}

void minicam5_decode(unsigned char *databuffer)
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

int minicam5_AbortCapture()
{
	unsigned char REG[4];
	REG[0] = 0; REG[1] = 0; REG[2] = 0; REG[3] = 0;
	return qhy_cameraIO(qhy_core_getendp()->write, 0xc1, REG, sizeof(REG), 0, 0);
}

int minicam5_bonjour()
{
	int retval = 0;
	unsigned char buf[16];

	
	if ((retval = qhy_EepromRead(0x10, buf, 16)) == 1)
	{
		// Color mode?
		camcolor = (buf[1] == 1);
		// This will reset and "wake" the camera
		if ((retval = minicam5_AbortCapture()) == 1)
		{
			// set Ui
			imgcam_get_tecp()->istec = 1;  
			strcpy(imgcam_get_camui()->roistr, "1280x960|1024x768|800x600|640x480|320x240:0");
			strcpy(imgcam_get_camui()->snrstr, "");
			strcpy(imgcam_get_camui()->bppstr, "2-12Bit|1-8Bit|3-Hdr:0");
			if(camcolor == 1)
			{
				strcpy(imgcam_get_camui()->byrstr, "3");
			}
			else 
			{
				strcpy(imgcam_get_camui()->byrstr, "-1");
			}
			// Header values
			imgcam_get_camui()->pszx = 3.75;
			imgcam_get_camui()->pszy = 3.75;
			maxwidth  = 1280;
			maxheight =  960;
			width   = 1280;
			height  = 960;
			bytepix = 2;
			hdr     = 0;
			retval = minicam5_SetDepth(bytepix);
			if(buf[0] == 7) 
			{
				// minicam5F
				camvariant = 2;
				strcpy(imgcam_get_camui()->whlstr, C_("camio","9-Positions|:0"));
			}
			else
			{
				// Minicam5S
				camvariant = 1;
				strcpy(imgcam_get_camui()->whlstr, "");
			}
			// Complete camera init
			usbspd  = 255;
			longExpMode = 0;
			exptime = 1;
			gain    = 20;
			bin     = 1;
			speed   = 0;
			retval = (retval == 1) ? minicam5_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? minicam5_SetSpeed(speed) : 0;
			retval = (retval == 1) ? minicam5_set_imgsize(width, height) : 0;
			retval = (retval == 1) ? minicam5_SetExposureTime(exptime) : 0;
			retval = (retval == 1) ? minicam5_SetGain(gain) : 0;
			retval = (retval == 1) ? minicam5_SetUSBTraffic(usbspd) : 0;
			retval = (retval == 1) ? minicam5_SetExposureTime(exptime): 0;
		}
	}
	if (retval == 0)
	{
		strcpy(imgcam_get_msg(), qhy_core_msg());
	}
	return (retval);
}

int minicam5_SetSpeed(int spd)
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

int minicam5_SetUSBTraffic(int i)
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

int minicam5_SetHDR(int on) 
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

int minicam5_SetDepth(int Bpp)
{
	unsigned char buf[2];
	
	buf[0] = (Bpp == 2) ? 1 : 0;
	return (qhy_cameraIO(qhy_core_getendp()->write, 0xcd, buf, 1, 0x00, 0x00));
}

int minicam5_SetExposureTime(int etime) 
{
	/*if(emodchg == 1)
	{
		//when exposure mode changed, reset resolution to get the new value of QHY5L_PLL_Ratio
		minicam5_set_imgsize(width, height);
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
	
	// ADDED TO AVOID THE PROBLEM FOR EXPOSURE about 2s in 12BIT MODE
	if (ExpTime >1000000 && ExpTime <3000000){
		ExpTime=3000000;
	}
	//END	

	unsigned char buf[4];

	if (ExpTime > MaxShortExpTime) 
	{
		if (longExpMode == 0) 
		{
			// Camera need to "enter" the long exp mode
			//printf("Enter long exp mode\n");
			longExpMode = 1;
			//when exposure mode changed, reset resolution to get the new value of QHY5L_PLL_Ratio
			//minicam5_set_imgsize(width, height);
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
			//minicam5_set_imgsize(width, height);
			// After switching over, it should be performed once more
			minicam5_AbortCapture();

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

		minicam5_AbortCapture();

		usleep(100);
		REG3012 = (unsigned short)(ExpTime / RowTime);
		if (REG3012 < 1)
		{
			REG3012 = 1;
		}
		qhy_I2CTwoWrite(0x3012, REG3012);
		ExpTime = (unsigned long)(REG3012 * RowTime);
	}
	return 1;
}

int minicam5_SetGain(int gain)
{
	if (longExpMode == 1)
	{
		minicam5_SetExposureTime(1);
		if(camcolor == 1)
		{
			/*double RG,BG;
			RG = (double)QCam.wbred / 100.;
			BG = (double)QCam.wbblue / 100.;
			minicam5_SetGainColor(gain,RG,BG);*/
			minicam5_SetGainColor(gain, 1, 1);
		}
		else
		{
			minicam5_SetGainMono(gain);
		}
		usleep(500000);
		minicam5_SetExposureTime(exptime);
	}
	else
	{
		if(camcolor == 1)
		{
			/*double RG,BG;
			RG = (double)QCam.wbred / 100.;
			BG = (double)QCam.wbblue / 100.;
			minicam5_SetGainColor(gain,RG,BG);*/
			minicam5_SetGainColor(gain, 1, 1);
		}
		else
		{
			minicam5_SetGainMono(gain);
		}
	}
	return 1;
}

void minicam5_SetGainMono(double gain)
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

void minicam5_SetGainColor(double gain, double RG, double BG)
{
	double Gain_Min, Gain_Max;

	Gain_Min = 0.;
	//Gain_Max = 39.8;
	Gain_Max = 79.6;

	if (gain < 2.6)
	{
		gain = 2.6;
	}
	gain = (Gain_Max - Gain_Min) * gain / 100;
	gain = MAX(1, gain);
	
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

int minicam5_set_imgsize(int width, int height)
{
	int retval = 1;
	
	switch (width)
	{
		case 1280:
			minicam5_set_1280x960();
			break;
		case 1024:
			minicam5_set_1024x768();
			break;
		case 800:
			minicam5_set_800x600();
			break;
		case 640:
			minicam5_set_640x480();
			break;
		case 320:
			minicam5_set_320x240();
			break;
		default:
			retval = 0;
			break;
	}
	return (retval);
}

int minicam5_setTec(int pwm, int fan) 
{
	unsigned char REG[3];
	REG[0] = 0x01;
	if (camvariant == 2)
	{
		REG[2] = 0xDF; 
	}
	else
	{
		REG[2] = 0xEB; 
	}

	if (fan==0)
	{
		pwm = 0;
	}
	if( pwm == 0 )	// TEC off
	{
		REG[1]=0;
	}
	else			// TEC manual
	{
		REG[1]=(unsigned char)pwm;
	}

   	return (qhy_cameraiIO(qhy_core_getendp()->iwrite, REG, sizeof(REG)));
}

void minicam5_set_1280x960()
{
	minicam5InitRegs();
	pllratio = minicam5_setPLL(0);

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

void minicam5_set_1024x768() 
{
	minicam5InitRegs();
	pllratio = minicam5_setPLL(0);

	int xstart = 4 + (maxwidth - width) / 2;
	int ystart = 4 + (maxheight - height) / 2;
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

void minicam5_set_800x600()
{
	minicam5InitRegs();
     pllratio = minicam5_setPLL(2);

	int xstart = 4 + (maxwidth - width) / 2; ;
	int ystart = 4 + (maxheight - height) / 2; ;
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

void minicam5_set_640x480()
{
	minicam5InitRegs();
	pllratio = minicam5_setPLL(1);

	int xstart = 4 + (maxwidth - width) / 2; ;
	int ystart = 4 + (maxheight - height) / 2; ;
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

void minicam5_set_320x240() 
{
	minicam5InitRegs();
	pllratio = minicam5_setPLL(1);

	int xstart = 4 + (maxwidth - width) / 2; ;
	int ystart = 4 + (maxheight - height) / 2; ;
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

void minicam5InitRegs()
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

double minicam5_setPLL(unsigned char clk)
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


int minicam5_guide(enum GuiderAxis axis, enum GuiderMovement direction)
{
  unsigned char buf[2] = { 0, 0 };
  int v = 2;
  switch(axis)
  {
    case GuideRightAscension:
      v = 1;
      break;
    case GuideDeclination:
      v = 2;
      break;
    default:
      return 0;
  }
  switch(direction) {
    case GuideIncrease:
      qhy_cameraIO(qhy_core_getendp()->write, 0xc0, buf, 0x02, v, axis == GuideRightAscension ? 0x80 : 0x40);
      return 1;
    case GuideDecrease:
      qhy_cameraIO(qhy_core_getendp()->write, 0xc0, buf, 0x02, v, axis == GuideRightAscension ? 0x10 : 0x20);
      return 1;
    default:
      qhy_cameraIO(qhy_core_getendp()->write, 0xc0, buf, 0x02, v, 0);
      return 1;
  }
}
