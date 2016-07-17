/*
 * qhy9l.c
 *
 *  Created on: 07.07.2014
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
#define PRODUCT_ID  0x8311
#define SHORTEXP    10
#define FWFILE      "qhy9l.hex"

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy9l.h"

static unsigned char REG[64];
static unsigned char REGBCK[64];

// These are shared with decode
static int bin, i_width, width, height, totalsize, transfer_size, top_skip_pix;

void qhy9l_init()
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
	qhy_core_getreq()->wheel     = 0xC1;
	
	qhy_core_getcampars()->vid        = VENDOR_ID;
	qhy_core_getcampars()->pid        = PRODUCT_ID;
	qhy_core_getcampars()->shortexp   = SHORTEXP;
	qhy_core_getcampars()->buftimes   = 0;
	qhy_core_getcampars()->buftimef   = 0;
	
	// Positively yess tec
	imgcam_get_tecp()->istec      = 1;      // 0 = Not driveable tec or no tec 1 = Driveable tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 5;      // Basically 0 - tecmax, value here is used for initial set on camera open 
	imgcam_get_tecp()->tecmax     = 255;    // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
	
	imgcam_get_camui()->hasgain = 1;
	imgcam_get_camui()->hasoffset = 1;
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2|3x3|4x4:0");
	/// Capture size values list, just translate "Full" (frame)
	strcpy(imgcam_get_camui()->roistr, C_("camio","Full|512x512|256x256:0"));
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
	strcpy(imgcam_get_camui()->ampstr, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
	//strcpy(imgcam_get_camui()->modstr, C_("camio","0-Light|1-Dark:0"));
	strcpy(imgcam_get_camui()->modstr, "");
	/// Descriptiopn for "mode" combo box
	//strcpy(imgcam_get_camui()->moddsc, C_("camio","Light/Dark mode"));
	strcpy(imgcam_get_camui()->moddsc, "");
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "0");
	strcpy(imgcam_get_camui()->tecstr, "0:255:1:2");
	strcpy(imgcam_get_camui()->whlstr, C_("camio","5-Positions|6-Positions|7-Positions|8-Positions:0"));
	// Shutter
	//imgcam_get_camui()->shutterMode = 1;
	imgcam_get_camui()->shutterMode = 0;
	// Header values
	imgcam_get_camui()->pszx = 5.40;
	imgcam_get_camui()->pszy = 5.40;
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy9l_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy9l_reset()
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

int  qhy9l_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	int time, Vbin, Hbin, ShortExp, antiamp, PatchNumber;
	int top_skip = 0, bot_skip = 0;
	int top_skip_null = 30;
	unsigned char time_H,time_M,time_L;
	
	expar->wtime = expar->time;
	bin = expar->bin;
	time = expar->time;
	ShortExp = (time < SHORTEXP);
	
	antiamp = 0;
	switch ( expar->amp ) 
	{
		case 0: 
			antiamp = 1;
			break;
		case 1: 
			antiamp = 0;
			break;
		case 2: if (time > 550) 
			antiamp = 1;
			break;
		default:
			printf( "Error: Registers NOT set (amp)!\n");
			return 2;
	}
	// It looks like libusb1.0 is very picky about "transfer_size" value.
	// Please have a look at the complete comment near the bulk_read

	//printf("ix: %d, iy %d\n", img_w, img_h);
	switch ( bin ) 
	{
		case 1: i_width  = 3584; height = 2574; width = i_width; Vbin = bin; Hbin = bin; top_skip_pix = 1150;
			break;
		case 2: i_width  = 1792; height = 1287; width = i_width; Vbin = bin; Hbin = bin; top_skip_pix = 1100;
			break;
		case 3: i_width  = 3584; height =  858; width = i_width / 3; Vbin = 3; Hbin = 1; top_skip_pix = 0;
			break;
		case 4: i_width  = 1792; height =  644; width = i_width / 2; Vbin = 4; Hbin = 2; top_skip_pix = 0;
			break;
		default:
			printf( "Error: Registers NOT set (bin)!\n");
			return 2;
	}
	// Check for ROI (if valid)
	if (expar->width > 0 && expar->width < (i_width / (bin/Hbin)))
	{
		width = expar->width; 
	}
	else
	{
		expar->width = width;
	}
	if (expar->height > 0 && expar->height < height)
	{
		top_skip_null = 25;
		top_skip = (height - expar->height);
		if (bin == 1)
		{
			top_skip = (int)(top_skip / 2);
		}
		bot_skip = height - top_skip - expar->height;
		height = expar->height;
	}
	else
	{
		expar->height = height;
	}
	//printf("x: %d, y %d, tops %d, bots %d fx %d\n", i_width, height, top_skip, bot_skip, width);

	totalsize = (i_width * 2 * height) + (top_skip_pix * 2);
	expar->totsize = totalsize;

	// Looks like the actual transfer size is a multiple of 1024 or 512 (Camera dependent. QHY7=1024, QHY9=512)
	// Either is the nearest (round) or is the floor value (Again camera dependent QHY6=512 round)
	transfer_size = 1024 * (int)(totalsize / 1024);
	expar->tsize = transfer_size;
	
	PatchNumber = qhy_getPatch(totalsize, 1024, 0);

	time_L=fmod(time,256);
	time_M=(time-time_L)/256;
	time_H=(time-time_L-time_M*256)/65536;


	REG[0]=(int)((expar->gain * 63) / 100);  	//Camera gain   range 0-63  
	
	REG[1]=expar->offset;					//Offset : range 0-255 default is 120
	
	REG[2]=time_H;  				//unit is ms       24bit
	REG[3]=time_M;
	REG[4]=time_L;
	
	REG[5]=Hbin;					// Horizonal BINNING    0 = 1= No bin
	REG[6]=Vbin;					// Vertical Binning     0 = 1= No bin
	
	REG[7]=qhy_MSB(i_width);			// The readout X  Unit is pixel 16Bit
	REG[8]=qhy_LSB(i_width);
	
	REG[9]= qhy_MSB(height);			// The readout Y  unit is line 16Bit
	REG[10]=qhy_LSB(height);
	
	REG[11]=qhy_MSB(top_skip);		// use for subframe    Skip lines on top 16Bit
	REG[12]=qhy_LSB(top_skip);
	
	REG[13]=qhy_MSB(bot_skip);		// use for subframe    Skip lines on Buttom 16Bit
	REG[14]=qhy_LSB(bot_skip);		// VerticalSize + SKIP_TOP +  SKIP_BOTTOM  should be the actual CCD Y size 
	
	REG[15]=0;	     			// LiveVideo no use for QHY7-8-9   16Bit set to 0
	REG[16]=0;

	REG[17]=qhy_MSB(PatchNumber);		// PatchNumber 16Bit
	REG[18]=qhy_LSB(PatchNumber);
	
	REG[19]=0;					// AnitInterlace no use for QHY8-9-11  16Bit set to 0
	REG[20]=0;
	
	REG[22]=0;					// MultiFieldBIN no use for QHY9  set to 0
	
	REG[29]=0x0000;				// ClockADJ no use for QHY9-11  16Bit set to 0
	REG[30]=0;
	
	REG[32]=antiamp;				// 1: anti-amp light mode 
	
	REG[33]=expar->speed;			// 0: low speed     1: high speed
	
	REG[35]=0; 					// TgateMode if set to 1 , the camera will exposure forever, till the ForceStop command coming
	REG[36]=ShortExp;				// ShortExposure no use for QHY9 set to 0
	REG[37]=0;					// VSUB no use for QHY8-9-11   set to 0
	REG[38]=0;					// Unknown reg.CLAMP
	
	REG[42]=0;					// TransferBIT no use for QHY8-9-11 set to 0
	
	REG[46]=top_skip_null;			// TopSkipNull unit is line.
	
	REG[47]=qhy_MSB(top_skip_pix);	// TopSkipPix no use for QHY9-11 16Bit set to 0 
	REG[48]=qhy_LSB(top_skip_pix);
	
	//REG[51]=SHUTTER;				// QHY9 0: programme control mechanical shutter automaticly   1: programme will not control shutter. 
	REG[51]=(expar->mode > 0) ? 1 : 0;	// QHY9 0: programme control mechanical shutter automaticly   1: programme will not control shutter. 
	REG[52]=0;					// DownloadCloseTEC no use for QHY9   set to 0
	
	REG[53]=0;					// Unknown: (reg.WindowHeater&~0xf0)*16+(reg.MotorHeating&~0xf0)
	
	
	REG[58]=100;					// SDRAM_MAXSIZE no use for QHY8-9-11   set to 0
	REG[63]=0;					// Unknown reg.Trig
		  
	if (memcmp(REG, REGBCK, sizeof(REG)) || expar->edit)
	{
		// If different from the last sent values
		if ((retval = qhy_cameraIO(qhy_core_getendp()->write, qhy_core_getreq()->sendregs, REG, sizeof(REG), 0, 0)) == 1)
		{
			expar->edit = 0;
		}
		// Store last sent values
		memcpy(REGBCK , REG, sizeof(REG) );
	}
	return retval;
}

void qhy9l_decode(unsigned char *databuffer)
{
	unsigned char *src1, *src2, *src3, *tgt;
	unsigned int   t, w, p;
	unsigned char  *swb;
	int left_skip = 0, right_skip = 0; 

	//Swap bytes
	swb = databuffer;
	t = totalsize>>1;
	while (t--)
	{
		qhy_swap(swb);
		swb+=2;
	}
	
	//DECODE
	switch(bin) 
	{
		case 1: //1X1 & 2X2 binning, in case of subframe only
		case 2: 
			if (width < i_width)
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)((i_width - width) / 2);
				right_skip = i_width - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				src1 = (databuffer + (top_skip_pix * 2));
				tgt = databuffer;
				t = height;
				w = width * 2;
				while (t--) 
				{
					// Skip the left part
					src1 += left_skip * 2;
					// Copy img_w pixels on the line
					memcpy(tgt, src1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						src1 += (width + right_skip) * 2;
						tgt  += w;
					}
				}
			}
			else
			{
				// Skip top pixels
				src1 = (databuffer + (top_skip_pix * 2));
				memcpy(databuffer, src1, i_width * height * 2);
			}
			break;
			
		case 3: //3X3 binning
				//This indeed is partially software binning from Bin1x3 data got from the camera
				// Copy onto databuffer from databuffer(!)
			src1 = databuffer;
			src2 = databuffer + 2;
			src3 = databuffer + 4;
			tgt = databuffer;
			t = height;
			while (t--) 
			{
				w = (i_width / 3);
				right_skip = (i_width % 3) * 2;
				while (w--) 
				{
					p  = (src1[0] + src1[1] * 256);
					p += (src2[0] + src2[1] * 256);
					p += (src3[0] + src3[1] * 256);
					p = MIN(p, 65535);
					tgt[0] = (int)(p % 256);
					tgt[1] = (int)(p / 256);
					src1 += 6;
					src2 += 6;
					src3 += 6;
					tgt  += 2;
				}
				if (t > 0)
				{
					src1 += right_skip;
					src2 += right_skip;
					src3 += right_skip;
				}
			}
			if (width < (int)(i_width / 3))
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)(((int)(i_width / 3) - width) / 2);
				right_skip = (int)(i_width / 3) - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				src1 = databuffer;
				tgt = databuffer;
				t = height;
				w = width * 2;
				while (t--) 
				{
					// Skip the left part
					src1 += left_skip * 2;
					// Copy img_w pixels on the line
					memcpy(tgt, src1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						src1 += (width + right_skip) * 2;
						tgt  += w;
					}
				}
			}
			break;

		case 4: //4X4 binning
				//This indeed is partially software binning from Bin2x4 data got from the camera
				// Copy onto databuffer from databuffer(!)
			src1 = databuffer;
			src2 = databuffer + 2;
			tgt = databuffer;
			t = height;
			while (t--) 
			{
				w = (i_width / 2);
				right_skip = (i_width % 2);
				while (w--) 
				{
					p  = (src1[0] + src1[1] * 256);
					p += (src2[0] + src2[1] * 256);
					p = MIN(p, 65535);
					tgt[0] = (int)(p % 256);
					tgt[1] = (int)(p / 256);
					src1 += 4;
					src2 += 4;
					tgt  += 2;
				}
				if (t > 0)
				{
					src1 += right_skip;
					src2 += right_skip;
				}
			}
			if (width < (int)(i_width / 2))
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)(((int)(i_width / 2) - width) / 2);
				right_skip = (int)(i_width / 2) - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				src1 = databuffer;
				tgt = databuffer;
				t = height;
				w = width * 2;
				while (t--) 
				{
					// Skip the left part
					src1 += left_skip * 2;
					// Copy img_w pixels on the line
					memcpy(tgt, src1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						src1 += (width + right_skip) * 2;
						tgt  += w;
					}
				}
			}
			break;			
	}  
	return;
}


