/*
 * qhy2old.c
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
#define PRODUCT_ID  0x081E
#define SHORTEXP    0
#define FWFILE      "qhy2.hex"

#define BUFWORD(b,p,W)  b[p] = ((unsigned char)(W>>8)) ; b[p+1] = ((unsigned char)(W&0xFF))

#include "imgBase.h"
#include <glib/gi18n.h>
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy6old.h"

static unsigned char REG[24];
static unsigned char REGBCK[24];

// These are shared with decode
static int bin, etime, speed, width, height, vbe, totalsize, transfer_size;

void qhy2old_init()
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
	strcpy(imgcam_get_camui()->modstr, C_("camio","1-Single|2-Double:0"));
	/// Descriptiopn for "mode" combo box
	strcpy(imgcam_get_camui()->moddsc, C_("camio","Mode S/D Image"));
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "1");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy2old_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy2old_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", 0x1618, 0x0412, VENDOR_ID, PRODUCT_ID, qhy_getFwPath(), FWFILE);
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

int  qhy2old_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	unsigned char RM =  0x04;
	unsigned char HM =  0x00;
	int H_SIZE, V_SIZE, Padding, amp;

	bin   = expar->bin;
	etime = expar->time;
	vbe   = expar->mode;
	speed = expar->speed;

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
			H_SIZE = 2140*2; 
			V_SIZE = 1560;
			width  = 2140; 
			height = 1560;
			REG[6] = ((vbe < 3) ? (vbe - 1) : 0);
			break;
		
		case 2: 
			H_SIZE =  2140; 
			V_SIZE =  768; 
			RM |= 0x08; 
			width  =  1070; 
			height =  768;
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

void qhy2old_decode(unsigned char *databuffer)
{
	unsigned short *src1, *src2, *tgt;
	unsigned char  *swb;
	unsigned int   t, allocsize = (MAX(transfer_size, totalsize) + 2);
	unsigned short *processed = NULL;
	unsigned short *dbuffer = NULL;

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
		case 1:  //1X1 binning
			dbuffer   = (unsigned short*)realloc(dbuffer, allocsize);
			processed = (unsigned short*)realloc(processed, allocsize);
			memcpy(dbuffer, databuffer, width * height * 2);
			src1 = dbuffer;
			src2 = dbuffer + width * (height>>1);
			tgt = processed;
			t = height>>1;
			while (t--) 
			{
				memcpy( tgt, src1, width * 2);
				tgt  += width;
				memcpy( tgt, src2, width * 2);
				tgt  += width;
				src1 += width; 
				src2 += width;
			}
			memcpy(databuffer, processed, width * height * 2);          
			break;

		case 2:  //2X2 binning
			break;
	}
}

