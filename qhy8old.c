/*
 * qhy8old.c
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
#define PRODUCT_ID  0x2972
#define SHORTEXP    30
#define FWFILE      "qhy8.hex"

#define BUFWORD(b,p,W)  b[p] = ((unsigned char)(W>>8)) ; b[p+1] = ((unsigned char)(W&0xFF))

#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy8old.h"

static unsigned char REG[24];
static unsigned char REGBCK[24];

// These are shared with decode
static int bin, width, height, totalsize, transfer_size;

void qhy8old_init()
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
	
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2|4x4:0");
	strcpy(imgcam_get_camui()->roistr, "");
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
	strcpy(imgcam_get_camui()->ampstr, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
	strcpy(imgcam_get_camui()->modstr, "");
	strcpy(imgcam_get_camui()->moddsc, "");
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "1");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	// Header values
	imgcam_get_camui()->pszx = 7.40;
	imgcam_get_camui()->pszy = 7.40;
	
	imgcam_get_expar()->bitpix  = 16;	
	imgcam_get_expar()->bytepix = 2;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy8old_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy8old_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", 0x1618, 0x6000, VENDOR_ID, PRODUCT_ID, qhy_getFwPath(), FWFILE);
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

int  qhy8old_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	unsigned char RM =  0x04;
	unsigned char HM =  0x00;
	int H_SIZE, V_SIZE, Padding, time, amp;
	
	expar->wtime = expar->time;
	bin  = expar->bin;
	time = expar->time;

	//account for general 1% error on exp time for all QHY camera tested so far
	time = (int) (time - (time / 100));

	if (expar->amp == 2) 
	{    
		amp = (time > 550) ? 0 : expar->amp;
	}
	if (amp == 0) 
	{
		HM = 0x40;
	}

	if (time < SHORTEXP) 
	{
		RM |= 0x01;
	}

	switch ( bin ) 
	{
		case 1: H_SIZE = 12440; V_SIZE = 1015; width = 3110; height = 2030;
			break;
		case 2: H_SIZE =  6220; V_SIZE = 1015; width = 1555; height = 1015; RM |= 0x08; 
			break;
		case 4: H_SIZE =  6224; V_SIZE =  507; width = 778; height = 507; RM |= 0x08; 
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

	if (expar->speed) 
	{
		RM |= 0x80;
	}

	memset(REG , 0, 24);

	REG[0] =  (int)((expar->gain * 63) / 100);
	REG[1] =  (time >> 16) & 0xFF;
	REG[2] =  (time >> 8 ) & 0xFF;
	REG[3] =  (time      ) & 0xFF;
	REG[4] =  RM;
	REG[5] =  HM;
	REG[7] =  expar->offset; 
	BUFWORD( REG,  8 , 0xACAC );
	BUFWORD( REG, 14 , H_SIZE );
	BUFWORD( REG, 16 , V_SIZE );
	BUFWORD( REG, 18 , Padding );
	REG[20] =  (bin==4)?1:0;
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
		usleep(250000);
	}
	
	return retval;
}

void qhy8old_decode(unsigned char *databuffer)
{
	int idx,x,y, allocsize = (MAX(transfer_size, totalsize) + 2);
	unsigned short *src1, *src2, *tgt, *tgt2;
	unsigned short tmpline[6224*2];
	unsigned char  *swb;
	unsigned int   t;
	unsigned short *dbuffer = NULL;

	//Swap bytes
	swb = databuffer;
	t = totalsize>>1;
	while (t--)
	{
		qhy_swap(swb);
		swb+=2;
	}

	dbuffer   = (unsigned short*)realloc(dbuffer, allocsize);
	memcpy(dbuffer, databuffer, width * height * 2);

	//DECODE
	switch(bin) 
	{
		case 1:  //1X1 binning
			tgt2 = dbuffer;
			for ( y=0 ; y < 1015 ; y++ ) 
			{
				src1 = &dbuffer[ y * 3110 * 2];
				tgt  = tmpline;
				src2 = &tmpline[6219];
				for (x=0; x<1555; x++) 
				{
					*src2++ = *src1++;  *src2++=*src1++;
					*tgt++ = *src1++;   *tgt++ = *src1++;
				}
				memcpy(tgt2 , &tmpline[0] , 6220);
				tgt2+=3110;
				memcpy(tgt2 , &tmpline[6220] , 6220);
				tgt2+=3110;
			}
			break;
		case 2:  //2X2 binning
			tgt = src1 = dbuffer;
			idx = width*height;
			while (idx--) 
			{
				t = *src1++;
				t += *src1++;
				*tgt++ = MIN( 65535 , t);
			} 
			break;
		case 4:  //4X4 binning
			tgt = src1 = dbuffer;
			idx = width*height;
			while (idx--) 
			{
				t = *src1++;
				t += *src1++;
				t += *src1++;
				t += *src1++;
				*tgt++ = MIN( 65535 , t);
			} 
			break;             
	}  
	memcpy(databuffer, dbuffer, width * height * 2);
	free(dbuffer);
	return;
}

