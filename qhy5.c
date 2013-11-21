/*
 * qhy5.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * Original author of device access code Tom Vandeneede formerly Astrosoft.be
 * Copyright owner of device access code QHYCCD Astronomy http://www.qhyccd.com/
 * as per: http://qhyccd.com/ccdbbs/index.php?topic=1154.msg6531#msg6531
 * Original code: Copyright(c) 2009 Geoffrey Hausheer.
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
#define PRODUCT_ID  0x296D
#define SHORTEXP    0
#define FWFILE      "qhy5.hex"
#define ROWSIZE    1558
#define ROWSNUM    1048
#define MAXW       1280
#define MAXH       1024

#include "imgBase.h"
#include <glib/gi18n.h>
#include "libusbio.h"
#include "qhycore.h"
#include "imgCamio.h"
#include "qhy5.h"

static char REG[64];
static char REGBCK[64];

// These are shared with decode
static int bin, denoise = 0, width, height, totalsize, transfer_size;
static int setgain[74]={0x000,0x004,0x005,0x006,0x007,0x008,0x009,0x00A,0x00B,
				   0x00C,0x00D,0x00E,0x00F,0x010,0x011,0x012,0x013,0x014,
				   0x015,0x016,0x017,0x018,0x019,0x01A,0x01B,0x01C,0x01D,
				   0x01E,0x01F,0x051,0x052,0x053,0x054,0x055,0x056,0x057,
				   0x058,0x059,0x05A,0x05B,0x05C,0x05D,0x05E,0x05F,0x6CE,
				   0x6CF,0x6D0,0x6D1,0x6D2,0x6D3,0x6D4,0x6D5,0x6D6,0x6D7,
				   0x6D8,0x6D9,0x6DA,0x6DB,0x6DC,0x6DD,0x6DE,0x6DF,0x6E0,
				   0x6E1,0x6E2,0x6E3,0x6E4,0x6E5,0x6E6,0x6E7,0x6FC,0x6FD,0x6FE,0x6FF};

void qhy5_init()
{
	qhy_core_init();

	qhy_core_getendp()->info   = 0;
	qhy_core_getendp()->read   = 0;
	qhy_core_getendp()->write  = 0XC2;
	qhy_core_getendp()->iread  = 0;
	qhy_core_getendp()->iwrite = 0;
	qhy_core_getendp()->bulk   = 0x82;
	qhy_core_getendp()->aux    = 0x42;

	qhy_core_getreq()->sendregs  = 0x13;
	qhy_core_getreq()->startexp  = 0x12;
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
	imgcam_get_tecp()->tecpwr     = 0;      // Basically 0 - tecmax, value here is used for initial set on camera open 
	imgcam_get_tecp()->tecmax     = 0;      // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
	
	strcpy(imgcam_get_camui()->binstr, "1x1|2x2:0");
	strcpy(imgcam_get_camui()->roistr, "1280x1024|1024x768|960x720|800x800|800x600|640x480|400x400|320x240:0");
	strcpy(imgcam_get_camui()->spdstr, "");
	strcpy(imgcam_get_camui()->ampstr, "");
	strcpy(imgcam_get_camui()->modstr, "");
	strcpy(imgcam_get_camui()->moddsc, "");
	/// Combo box values list, keep N-<desc> format. Just translate <desc>
	strcpy(imgcam_get_camui()->snrstr, C_("camio","0-Off|1-0n:0"));
	strcpy(imgcam_get_camui()->bppstr, "1-8Bit|:0");
	strcpy(imgcam_get_camui()->byrstr, "0");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");
	
	imgcam_get_expar()->bitpix  = 8;	
	imgcam_get_expar()->bytepix = 1;	
	imgcam_get_expar()->tsize   = 0;
	imgcam_get_expar()->edit    = 0;	
}

int  qhy5_iscamera()
{
	return find_camera(VENDOR_ID, PRODUCT_ID);
}

int  qhy5_reset()
{
	int retval = 0;
	char cmd[2048];

	sprintf(cmd,"%s %04x:%04x %04x:%04x %s/%s", "./qhyReset.bash", 0x1618, 0x901, VENDOR_ID, PRODUCT_ID, qhy_getFwPath(), FWFILE);
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

int  qhy5_setregisters(qhy_exposure *expar) 
{
	int retval = 1;	
	static int first_time = 1;
	int op_height, offset, value, index;
	int gain, gain_val;
	
	denoise = expar->denoise;
	bin = expar->bin;
	expar->wtime = expar->time;
	// Check for ROI (if valid)
	if ((expar->width < 1) || (expar->width > (MAXW / bin))) 
	{
		width = (MAXW / bin);
		expar->width = width;
	}
	else
	{
		width = expar->width;
	}
	if ((expar->height < 1) || (expar->height > (MAXH / bin))) 
	{
		height = (MAXH / bin);
		expar->height = height;
	}
	else
	{
		height = expar->height;
	}

	op_height = height * bin;
	
	op_height -=  (op_height % 4);
	offset = (ROWSNUM - op_height) / 2;
	index = (ROWSIZE * (op_height + 26)) >> 16;
	value = (ROWSIZE * (op_height + 26)) & 0xffff;
	gain = (int)(73. * (expar->gain / 100.));
	gain_val = setgain[gain];  // * 0x6ff / 100;
	STORE_WORD_BE(REG + 0,  gain_val);
	STORE_WORD_BE(REG + 2,  gain_val);
	STORE_WORD_BE(REG + 4,  gain_val);
	STORE_WORD_BE(REG + 6,  gain_val);
	STORE_WORD_BE(REG + 8,  offset);
	STORE_WORD_BE(REG + 10, 0);
	STORE_WORD_BE(REG + 12, op_height - 1);
	STORE_WORD_BE(REG + 14, 0x0521);
	STORE_WORD_BE(REG + 16, op_height + 25);
	REG[18] = 0xcc;
	
	if (memcmp(REG, REGBCK, sizeof(REG)) || expar->edit)
	{
		// If different from the last sent values
		if (qhy_cameraIO(qhy_core_getendp()->aux, qhy_core_getreq()->sendregs, (unsigned char *) REG, sizeof(REG), value, index))
		{
			usleep(2000);
			if (qhy_cameraIO(qhy_core_getendp()->aux, (qhy_core_getreq()->sendregs + 1), (unsigned char *) REG, 0, 0x31a5, 0))
			{
				usleep(1000);
				if (qhy_cameraIO(qhy_core_getendp()->aux, (qhy_core_getreq()->sendregs + 3), (unsigned char *) REG, 0, first_time, 0))
				{	
					first_time = 0;
					totalsize = width * bin * height * bin;
					expar->totsize = totalsize;
					transfer_size = ROWSIZE * (op_height + 26);
					expar->tsize = transfer_size;
					// Store last sent values
					memcpy(REGBCK , REG, sizeof(REG) );
					expar->edit = 0;
					retval = 1;
					usleep(2000);
				}
			}
		}
	}
	return (retval);
}

void qhy5_decode(unsigned char *databuffer)
{
	unsigned char *src, *tgt;
	unsigned int  p_height = (height * bin), p_width = (width * bin);
	unsigned int  row, col, right_offset = (int)((1024 - p_height) / 2);
	unsigned int  p, pix, pir, rmin, rmax;
	int avgcnt;
	float rratio;
	static unsigned int ravg[1024];
		
	// Initial positioning for output
	tgt = databuffer;
	src = databuffer;
	
	//Proper decode
	switch(bin) 
	{
		case 1:  //1X1 binning
			if (height < ROWSNUM)
			{
				for(row = 0; row < p_height; row++)
				{
					// Code in the default section should be able to get to the same image result
					// This however is faster, so is kept for seed capture purposes
					//Reposition to the beginning of each logical row
					src = databuffer + ROWSIZE * row + right_offset + 20;
					memmove(tgt, src, p_width);
					tgt += p_width;
				}
			}
			break;
	
		default:  //2X2, 3x3, 4x4 binning		
			for(row = 0; row < p_height; row+=bin)
			{
				for(col = 0; col < p_width; col+=bin)
				{
					//This will scan the bin matrix (whichever size)
					//The src pointer is left on the last element of the bin matrix
					//The src positioning is absolute to buffer beginning, though
					p = 0;
					for(pir = 0; pir < bin; pir++)
					{
						src = databuffer + ROWSIZE * row + right_offset + 20 + col;
						src += (ROWSIZE * pir);
						for(pix = 0; pix < bin; pix++)
						{
							src += pix;
							p += *src;
						}
					}
					p = MIN(p, 255);
					*tgt = (unsigned char) p;
					tgt+=1;				
				}
			}
			break;
	}
	
	if (denoise)
	{
		if (bin == 1)
		{
			// Initial positioning (now the matrix is img_w x img_h)
			src = databuffer;
			ravg[0] = *src;
			src++;
			ravg[1] = *src;
			src += (width - 1);
			ravg[2] = *src;
			src++;
			ravg[3] = *src;
			src = databuffer;
			if (abs(ravg[0] - ravg[1]) < abs(ravg[2] - ravg[3]))
			{
				//printf("Rows: Even\n");
				// Start on even row
				src += width;
			}
			//Experimental denoise (de-checkerboard)
			for(row = ((abs(ravg[0] - ravg[1]) < abs(ravg[2] - ravg[3])) ? 1 : 0); row < height; row += 2)
			{
				ravg[0] = *src;
				src++;
				ravg[1] = *src;
				src--;
				if (ravg[0] < ravg[1])
				{
					//printf("Cols: Even\n");
					src++;
				}
				pir = 0;
				for(col = ((ravg[0] < ravg[1]) ? 1 : 0); col < width; col++)
				{
					if (pir == 0)
					{
						p = *src;
						pir++;
						src += 1;
					}
					else
					{
						src += 1;
						// Average two odd pixels
						p = (p + *src) / 2;
						pir = 0;
						// Reset even pixel in between (low levels only)
						src -= 1;
						if (*src < 150)
						{
							*src = (unsigned char) p;
						}
						src += 1;
					}
				}
				// Jump one row (move on odd rows)
				src += width;
			}
		}
		// Initial positioning (now the matrix is img_w x img_h)
		src = databuffer;
		//Denoise (prepare phase, maximum average)
		rmax = 0;
		for(row = 0; row < height; row++)
		{
			ravg[row] = 0;
			avgcnt = 0;
			for(col = 0; col < width; col++)
			{
				// Exclude saturated pixels from average
				if (*src < 255) 
				{
					ravg[row] += *src; 
					avgcnt += 1;
				}
				src+=1;
			}
			// Row's average value
			if (avgcnt > 0 && ravg[row] > 0)
			{ 
				ravg[row] = ravg[row] / avgcnt;
				rmax = MAX(rmax, ravg[row]);
			}
		}
		// Initial positioning (now the matrix is img_w x img_h)
		src = databuffer;
		//Denoise (prepare phase, minimum average)
		// We increase rmax (upper limit of pixels used to average row values) 
		// to avoid artifacts when image DATAMIN - DATAMAX distance is tiny.
		rmax = (rmax * 1.3) < 255 ? (rmax * 1.3) : 255;
		rmin = 255;
		for(row = 0; row < height; row++)
		{
			ravg[row] = 0;
			avgcnt = 0;
			for(col = 0; col < width; col++)
			{
				// Exclude pixels above maximum average
				if (*src < rmax) 
				{
					ravg[row] += *src; 
					avgcnt += 1;
				}
				src+=1;
			}
			// Row's average value
			if (avgcnt > 0 && ravg[row] > 0)
			{ 
				ravg[row] = ravg[row] / avgcnt;
				rmin = MIN(rmin, ravg[row]);
			}
		}

		// Initial positioning (now the matrix is img_w x img_h)
		tgt = databuffer;
		src = databuffer;
		// Denoise (final phase)
		rmin = (rmax + rmin) / 2;
		if (rmin > 0)
		{
			for(row = 0; row < height; row++)
			{
				// This excludes work on "strange" rows that have true 0 avg or have all saturated pixels (see above)
				if (ravg[row] > 0)
				{
					//rratio is < 1
					rratio = (float)rmin / ravg[row];				
					//printf("rratio: %f\n", rratio);
					for(col = 0; col < width; col++)
					{				
						p = *src;
						//p = p * rratio / pow(rratio, pow((double)p/255., 10.));
						p = p * rratio / pow(rratio, (double)p/255.);
						p = (p > 255) ? 255 : p;
						*tgt = (unsigned char) p;
						src+=1;
						tgt+=1;				
					}
				}
			}
		}
	}
	return;
}

int  qhy5_bonjour()
{
	int retval = 0;
	qhy_exposure tmpar;

	tmpar.time = 100;
	tmpar.bin = 1;
	tmpar.width = MAXW;
	tmpar.height = MAXH;
	tmpar.gain = 50;
	tmpar.bitpix  = 8;	
	tmpar.bytepix = 1;	
	tmpar.edit = 1;
	if ((retval = qhy5_setregisters(&tmpar)) == 1)
	{
		retval = qhy_cmosAbortCapture(tmpar.tsize);
	}
	return (retval);	
}


