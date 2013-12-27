/*
 * qhy6old.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code Tom Vandeneede formerly Astrosoft.be
 * Copyright owner of device access code QHYCCD Astronomy http://www.qhyccd.com/
 * as per: http://qhyccd.com/ccdbbs/index.php?topic=1154.msg6531#msg6531
 * Original code: Tom Vandeneede formerly Astrosoft.be.
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

#define VENDOR_ID   0x16C0
#define PRODUCT_ID  0x081D
#define SHORTEXP    0
#define FWFILE      "qhy6.hex"

#define BUFWORD(b,p,W)  b[p] = ((unsigned char)(W>>8)) ; b[p+1] = ((unsigned char)(W&0xFF))

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy6old.h"

static unsigned char REG[24];
static unsigned char REGBCK[24];

// These are shared with decode
static int bin, etime, speed, width, height, vbe, totalsize, transfer_size;

void qhy6old_init()
{
	qhy_core_init();

	qhy_core_getendp()->info   = 0;
	qhy_core_getendp()->read   = 0xC2;
	qhy_core_getendp()->write  = 0x42;
	qhy_core_getendp()->iread  = 0;
	qhy_core_getendp()->iwrite = 0;
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
	
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2:0");
	strcpy(imgcam_get_camui()->roistr, "");
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
	strcpy(imgcam_get_camui()->ampstr, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
	strcpy(imgcam_get_camui()->modstr, C_("camio","1-Single|2-Double|3-Vbe Sampling|4-Vbe Timing:0"));
	/// Descriptiopn for "mode" combo box
	strcpy(imgcam_get_camui()->moddsc, C_("camio","Mode S/D Image"));
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "-1");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy6old_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy6old_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", 0x1618, 0x0259, VENDOR_ID, PRODUCT_ID, qhy_getFwPath(), FWFILE);
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

int  qhy6old_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	unsigned char RM =  0x04;
	unsigned char HM =  0x00;
	int H_SIZE, V_SIZE, Padding, amp;

	bin   = expar->bin;
	etime = expar->time;
	vbe   = expar->mode;
	speed = expar->speed;
	expar->wtime = (vbe == 2) ? expar->time * 2 : expar->time;

	//account for general 1% error on exp time for all QHY camera tested so far
	etime = (int) (etime - (etime / 100));

	if (expar->amp == 2) 
	{    
		amp = (etime > 550) ? 0 : expar->amp;
	}
	if (amp == 0) 
	{
		HM = 0x40;
	}

	memset(REG , 0 , 24);

	switch ( bin ) 
	{
		case 1: 
			H_SIZE = 796 * 2; 
			V_SIZE = 596;
			width  = 796; 
			height = 596;
			REG[6] = ((vbe < 3) ? (vbe - 1) : 0);
			break;
		
		case 2: 
			H_SIZE =  800; 
			V_SIZE =  298; 
			RM |= 0x08; 
			width  =  400; 
			height =  298;
			REG[6] = 2;
			RM |= 0x48;
			break;

		default:
			printf( "Error: Registers NOT set (bin)!\n");
			return 0;
	}
	expar->width  = width;
	expar->height = height;

	// Image data size
	totalsize = H_SIZE * V_SIZE;
	expar->totsize = totalsize;
	// Transfer size, padding
	totalsize = ((totalsize>>10)+1)<<10;
	Padding = totalsize-(H_SIZE*V_SIZE);	
	expar->tsize = expar->totsize + Padding;
	transfer_size = expar->tsize;

	if (speed) 
	{
		RM |= 0x80;
	}

	REG[0] =  (int)((expar->gain * 63) / 100);
	REG[1] =  (etime >> 16) & 0xFF;
	REG[2] =  (etime >> 8 ) & 0xFF;
	REG[3] =  (etime      ) & 0xFF;
	REG[4] =  RM;
	REG[5] =  HM;
	REG[7] =  expar->offset; 
	BUFWORD( REG,  8 , 0xACAC );
	BUFWORD( REG, 14 , H_SIZE );
	BUFWORD( REG, 16 , V_SIZE );
	BUFWORD( REG, 18 , Padding );
	REG[20] = 0;
	REG[23] = 0xAC;

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

void qhy6old_decode(unsigned char *databuffer)
{
	// For decode
	int idx, framesize;
	int n1, n2, k;
	float v1, v2;                    // added for venetion blind calc
	int tmp, allocsize = (MAX(transfer_size, totalsize) + 2);
	unsigned short *src1, *src2, *tgt;
	unsigned char  *swb;             // remove unsigned
	unsigned int   t;
	unsigned short *processed = NULL;
	unsigned short *dbuffer = NULL;
	// For vbe compensation
	float tot1[300];                // total each line field 1
	float tot2[300];                // total each line field 2
	float vfac[300];                // factor to multiply field 1 = field2 / field1
	float vfac1; //, vfac2;             // average in non pixel areas
	float vfactor = 1.0;            // average value VBcorrect version 1
	float vfactor2 = 1.0;           // timing  value VBcorrect version 2
	
	//Swap bytes
	swb = databuffer;
	t = totalsize>>1;
	while (t--)
	{
		qhy_swap(swb);
		swb+=2;
	}

	//DECODE
	framesize = width * (height>>1);
	switch(bin) 
	{
		case 1:  //1X1 binning
			//printf("Realloc: %d\n", allocsize);
			dbuffer   = (unsigned short*)realloc(dbuffer, allocsize);
			processed = (unsigned short*)realloc(processed, allocsize);
			memcpy(dbuffer, databuffer, width * height * 2);
			src1 = dbuffer;
			src2 = dbuffer + framesize;
			tgt = processed;
			t = height>>1;
			k = 0;
			while (t--)
			{
				// preliminary venetian blind correction version 1: only complete this if correction requested.
				// first do the analysis, for debugging, we decide later to apply it...
				tot1[k] = 0; tot2[k] = 0;
				n1 = 0; n2 = 0;
				for (idx=0; idx<width; idx++)
				{
					if (*src2 < 62000)
					{
						tot2[k] += *src2;
						n2++;
					}
					*tgt++ = *src2++;
				}
				for (idx=0; idx<width; idx++)
				{
					if (*src1 < 62000)      // add up unsaturated pixels and average them
					{
						tot1[k] += *src1;
						n1++;
					}
					*tgt++ = *src1++;
				}
				if (n1) 
				{
					tot1[k] /= n1;
				}
				if (n2) 
				{
					tot2[k] /= n2;
				}
				if (tot1[k] == 0.0 || tot2[k] == 0.0) 
				{
					vfac[k] = 1.0;
				}
				else
				{
					vfac[k] = tot2[k] / tot1[k];
				}
				k++;
			}
			memcpy(dbuffer, processed, width * height * 2);
			free(processed);        
			// correct venetian blind: only if required
			// version 1: brighten the whole field1 by calculated average
			// version 2: subtract background noise before correction by timing ratio
			if (vbe > 2)
			{
				if (vbe == 3)     
				{
					// correct by sampling ratio: field2 / field1
					// find average factor        
					vfactor = 0.0;
					for (t = 0; t < height/2; t++)
					{
						vfactor += vfac[t]; // sum and average
					}
					vfactor /= height/2;
					vfactor *= 1.04;
					for (t = 1; t < height; t+=2)        // change field 1 to match field 2
					{
						src1 = dbuffer + (t * width);  // start of line
						for (idx=0; idx<width; idx++)
						{
							tmp = (int)((*src1) * vfactor);  // save in tmp: over range for unsigned int?
							tmp = MIN(tmp, 65535);
							tmp = MAX(tmp, 0);               // watch for dead pixels
							*src1++ = tmp; 
						}
					}
				}
				else if (vbe == 4)   
				{
					// correct by timing ratio only, skipping background pixels
					// for VB correction version 2, determine background noise by averaging non-pixel areas
					// field1: 12,4 to 32,294 field2 746,306 to 768,592
					vfac1 = add_field(dbuffer, 12, 4, 32, 294);
					//vfac2 = add_field(dbuffer, 746, 306, 768, 592);
					v1 = (float)(etime);
					v2 = v1 + ((speed == 1) ? 60.0 : 330.0); // these value may need tweeking
					vfactor2 = (etime > v2) ? (v2 / v1): v2;
					for (t = 1; t < height; t+=2)        // change field 1 to match field 2
					{
						src1 = dbuffer + (t * width);  // start of line
						for (idx=0; idx<width; idx++)
						{
							tmp = (int)((*src1 - vfac1) * vfactor2) + vfac1;  // save in tmp: over range?
							tmp = MIN(tmp, 65535);
							tmp = MAX(tmp, 0);               // watch for dead pixels
							*src1++ = tmp; 
						}
					}
				}
			}
			memcpy(databuffer, dbuffer, width * height * 2);
			free(dbuffer);
			break;   

		case 2:  //2X2 binning
		break;
	}  

	return;
}

// find average value in rectangular field of raw image buffer

float add_field(unsigned short *databuffer, int x1, int y1, int x2, int y2)
{
	float value;
	int cx, cy, k, offset;

	value = 0.0;
	k = 0;
	for (cy = y1; cy < y2; cy++)
	{
		offset = cy * width;
		for (cx = x1; cx < x2; cx++)
		{
			value += (float)(*(databuffer + offset + cx));
			k++;
		}
	}
	if (k == 0) return 0.0;
	return value / k;
}

