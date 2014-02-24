/*
 * qhy12.c
 *
 *  Created on: 11.02.2014
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
#define PRODUCT_ID  0x1201
#define SHORTEXP    10
#define FWFILE      "qhy12.hex"

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy12.h"

static unsigned char REG[64];
static unsigned char REGBCK[64];

// These are shared with decode
static int bin, i_width, width, height, totalsize, transfer_size, top_skip_pix;

void qhy12_init()
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
	qhy_core_getreq()->shutter   = 0;
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
	
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2|4x4:0");
	/// Capture size values list, just translate "Full" (frame)
	strcpy(imgcam_get_camui()->roistr, C_("camio","Full|512x512|256x256:0"));
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
	strcpy(imgcam_get_camui()->ampstr, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
	strcpy(imgcam_get_camui()->modstr, "");
	strcpy(imgcam_get_camui()->moddsc, "");
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "2");
	strcpy(imgcam_get_camui()->tecstr, "0:255:1:2");
	strcpy(imgcam_get_camui()->whlstr, "");
	// Header values
	imgcam_get_camui()->pszx = 5.12;
	imgcam_get_camui()->pszy = 5.12;
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy12_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy12_reset()
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

int  qhy12_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	int time, Vbin, Hbin, ShortExp, antiamp, vbe, PatchNumber;
	int top_skip = 0, bot_skip = 0;	
	int top_skip_null = 30;
	unsigned char time_H,time_M,time_L;
	
	expar->wtime = (expar->mode == 2) ? expar->time * 2 : expar->time;
	bin = expar->bin;
	vbe = (expar->mode - 1);
	vbe = (vbe >= 0) ? vbe : 0;
	if(expar->time < SHORTEXP) 
	{  
		ShortExp = 1;
		time = expar->time;
	}
	else                   
	{
		ShortExp = 0; 
		time = expar->time - SHORTEXP;
	}
	//account for general 1% error on exp time for all QHY camera tested so far
	time = (int) (time - (time / 100));
	
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
	// Please note this camera Width and Height are swapped.
	switch ( bin ) 
	{
		case 1: 
			i_width = 3328; height = 1170*4; Vbin = bin; Hbin = bin; width = i_width; top_skip_pix = 1190;
			break;
		case 2: 
			i_width = 3328; height = 1170*2; Vbin = bin; Hbin = 1; width = i_width / 2; top_skip_pix = 1190;
			break;
		case 4: 
			i_width = 3328; height = 1170;   Vbin = bin; Hbin = 1; width = i_width / 4; top_skip_pix = 1190;
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
		top_skip_null = 100;
		top_skip = (height - expar->height);
		if (bin == 1)
		{
			top_skip = (int)(top_skip / 4);
		}
		else
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
	transfer_size = 512 * lround(totalsize / 512);
	expar->tsize = transfer_size;

	PatchNumber = qhy_getPatch(totalsize, 512, 0);

	time_L=fmod(time,256);
	time_M=(time-time_L)/256;
	time_H=(time-time_L-time_M*256)/65536;


	REG[0]=(int)((expar->gain * 63) / 100);  	//Camera gain   range 0-63  
	
	REG[1]=expar->offset;			//Offset : range 0-255 default is 120
	
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
	
	REG[15]=0;					// LiveVideo no use for QHY7-8-9   16Bit set to 0
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
	
	REG[51]=0;					// QHY9 0: programme control mechanical shutter automaticly   1: programme will not control shutter. 
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

void qhy12_decode(unsigned char *databuffer)
{
	unsigned char *srcF1, *srcF2, *srcF3, *srcF4;
	unsigned char *tgt11, *tgt12, *tgt13, *tgt14, *tgt21, *tgt22, *tgt23, *tgt24;
	unsigned char *swb;
	int tgt1, tgt2;
	int sF1, sF2, sF3, sF4;
	int t, w, p, hgt2 = (height / 2), hgt22 = (height / 2) - 2;
	int p1, p2;
	unsigned char *processed = NULL;
	int left_skip = 0, right_skip = 0, allocsize = (MAX(transfer_size, totalsize) + 2);

	processed = (unsigned char*)realloc(processed, allocsize);
	
	//Swap bytes
	swb = databuffer;
	t = totalsize>>1;
	while (t--)
	{
		qhy_swap(swb);
		swb+=2;
	}

	// Top pix skip
	srcF1 = databuffer + (top_skip_pix * 2);
	memcpy(processed, srcF1, (i_width * height * 2));

	//DECODE
	switch(bin) 
	{
		case 1:  //1X1 binning
			// Field1 = R, Filed2 = Gr, Field3 = Gb, Field4 = B
			// Now we have to rebuild the whole frame starting from the dual oputput, dual field 
			// (R,R,R / B,B,B - Gr,Gr,Gr / Gb,Gb,Gb)
			// Please note that B and Gb data are vertically flipped 
			// Output should be a RGGB mask
			for (t = 0; t < hgt22; t += 2)
			{
				// Loop on rows
				// Jump two lines on the output
				tgt1 = (t * 2 * width);
				tgt2 = ((t + 1) * 2 * width);
				// Jump one line on the original camera data
				sF1 = width * (t + 3) + 1; 			//R
				sF2 = width * (height - t - 2) + 3;	//G
				sF3 = width * (hgt2 - t - 2) + 1;	//G
				sF4 = width * (hgt2 + t + 3) + 1;     //B
				// Please note 2 & 4 field are vertically flipped
				//In each output field 1 there's a pixel offset for unknown reason
				for (w = 0; w < width; w += 4)
				{
					for (p = 0; p < 2; p++)
					{
						tgt11 = databuffer + tgt1 * 2 + p * 4 + w * 2;
						tgt12 = tgt11 + 2;
						tgt13 = tgt11 + (width * 2);
						tgt14 = tgt13 + 2;
						srcF1 = processed + ((sF1 + w + p) * 2);
						srcF2 = processed + ((sF2 + w + p) * 2);
						srcF3 = processed + ((sF3 + w + p) * 2);
						srcF4 = processed + ((sF4 + w + p) * 2);
						
						*tgt11 = *srcF1; tgt11++;  srcF1++;  *tgt11 = *srcF1;
						*tgt12 = *srcF2; tgt12++;  srcF2++;  *tgt12 = *srcF2;
						*tgt13 = *srcF3; tgt13++;  srcF3++;  *tgt13 = *srcF3;
						*tgt14 = *srcF4; tgt14++;  srcF4++;  *tgt14 = *srcF4;
					}
					for (p = 0; p < 2; p++)
					{
						tgt21 = databuffer + tgt2 * 2 + p * 4 + w * 2 + 4;
						tgt22 = tgt21 + 2;
						tgt23 = tgt21 + (width * 2);
						tgt24 = tgt23 + 2;
						srcF1 = processed + ((sF1 + w + p + 2) * 2);
						srcF2 = processed + ((sF2 + w + p + 2) * 2);
						srcF3 = processed + ((sF3 + w + p + 2) * 2);
						srcF4 = processed + ((sF4 + w + p + 2) * 2);

						*tgt21 = *srcF1; tgt21++;  srcF1++;  *tgt21 = *srcF1;
						*tgt22 = *srcF2; tgt22++;  srcF2++;  *tgt22 = *srcF2;
						*tgt23 = *srcF3; tgt23++;  srcF3++;  *tgt23 = *srcF3;
						*tgt24 = *srcF4; tgt24++;  srcF4++;  *tgt24 = *srcF4;
					}
				}
			}
			if (width < i_width)
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)((i_width - width) / 2);
				right_skip = i_width - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				srcF1 = databuffer;
				tgt11 = databuffer;
				t = height;
				while (t--) 
				{
					// Skip the left part
					srcF1 += left_skip * 2;
					w = width * 2;
					// Copy img_w pixels on the line
					memcpy(tgt11, srcF1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						srcF1 += (width + right_skip) * 2;
						tgt11  += w;
					}
				}
			}
			break;

		case 2:  //2X2 binning
			// First of all we have to reorder the interlaced output
			// Fields are vertically flipped
			for (t = 0; t < height; t += 2)
			{
				tgt1 = width * t;
				tgt2 = width * (t + 1);
				sF1 = i_width * (t + 1);	 			//F1
				sF2 = i_width * (height - t - 2) + 1;	//F2
				
				for (w = 0; w < width; w++)
				{
					tgt11 = databuffer + (tgt1 + w) * 2;
					tgt21 = databuffer + (tgt2 + w) * 2;
					srcF1 = processed + sF1 * 2 + w * 4;
					srcF2 = processed + sF2 * 2 + w * 4;

					p1  = (srcF1[0] + srcF1[1] * 256);
					p1 += (srcF1[2] + srcF1[3] * 256);
					p1  = MIN(p1, 65535);
					p2  = (srcF2[0] + srcF2[1] * 256);
					p2 += (srcF2[2] + srcF2[3] * 256);
					p2  = MIN(p2, 65535);

					tgt11[0] = (int)(p1 % 256);
					tgt11[1] = (int)(p1 / 256);
					tgt21[0] = (int)(p2 % 256);
					tgt21[1] = (int)(p2 / 256);					
				}
			}			
			if (width < (int)(i_width / 2))
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)(((int)(i_width / 2) - width) / 2);
				right_skip = (int)(i_width / 2) - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				srcF1 = databuffer;
				tgt11 = databuffer;
				t = height;
				while (t--) 
				{
					// Skip the left part
					srcF1 += left_skip * 2;
					w = width * 2;
					// Copy img_w pixels on the line
					memcpy(tgt11, srcF1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						srcF1 += (width + right_skip) * 2;
						tgt11  += w;
					}
				}
			}
			break;
			
		case 4:  //2X2 binning
			// First of all we have to reorder the interlaced output
			// Fields are vertically flipped
			for (t = 0; t < height - 2; t += 2)
			{
				tgt1 = width * t;
				tgt2 = width * (t + 3);
				sF1 = i_width * (t + 1);	 			//F1
				sF2 = i_width * (height - t - 2) + 1;	//F2

				for (w = 0; w < width; w++)
				{
					tgt11 = databuffer + (tgt1 + w) * 2;
					tgt21 = databuffer + (tgt2 + w) * 2;
					srcF1 = processed + sF1 * 2 + w * 8;
					srcF2 = processed + sF2 * 2 + w * 8;

					p1  = (srcF1[0] + srcF1[1] * 256);
					p1 += (srcF1[2] + srcF1[3] * 256);
					p1 += (srcF1[4] + srcF1[5] * 256);
					p1 += (srcF1[6] + srcF1[7] * 256);
					p1  = MIN(p1, 65535);
					p2  = (srcF2[0] + srcF2[1] * 256);
					p2 += (srcF2[2] + srcF2[3] * 256);
					p2 += (srcF2[4] + srcF2[5] * 256);
					p2 += (srcF2[6] + srcF2[7] * 256);
					p2  = MIN(p2, 65535);

					tgt11[0] = (int)(p1 % 256);
					tgt11[1] = (int)(p1 / 256);
					tgt21[0] = (int)(p2 % 256);
					tgt21[1] = (int)(p2 / 256);
				}
			}			
			if (width < (int)(i_width / 4))
			{
				// Copy onto databuffer from databuffer(!) only the ROI data
				left_skip = (int)(((int)(i_width / 4) - width) / 2);
				right_skip = (int)(i_width / 4) - left_skip - width;
				//printf("Processing i_width: ls %d, iw %d, rs %d\n", left_skip, width, right_skip);
				srcF1 = databuffer;
				tgt11 = databuffer;
				t = height;
				while (t--) 
				{
					// Skip the left part
					srcF1 += left_skip * 2;
					w = width * 2;
					// Copy img_w pixels on the line
					memcpy(tgt11, srcF1, w);
					if (t > 0)
					{
						// Move & Skip the right part
						srcF1 += (width + right_skip) * 2;
						tgt11  += w;
					}
				}
			}
			break;			
	}
	free(processed);
	return;
}

