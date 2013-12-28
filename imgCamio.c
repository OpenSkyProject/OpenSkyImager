/*
 * imgCamio.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
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

// imgcamio "class" code
#include "imgCamio.h"
#include "imgBase.h"
#include "libusbio.h"
#include "qhycore.h"
#include "qhy2old.h"
#include "qhy5.h"
#include "qhy5ii.h"
#include "qhy6.h"
#include "qhy6old.h"
#include "qhy7.h"
#include "qhy8old.h"
#include "qhy8l.h"
#include "qhy9.h"
#include "qhy11.h"

static char *cammsg;
static unsigned char* databuffer[2] = {NULL, NULL};
static char *cammodel;
static int      camid = 0;
static int     loaded = 0;
static int  connected = 0;
static int curdataptr = 0;
//static int vendorid, productid;
static qhy_exposure expar;
static qhy_exposure shpar;
static qhy_tecpars  tecp;
static qhy_camui    camui;

void imgcam_exparcpy(qhy_exposure *copy, qhy_exposure *source)
{
	copy->gain    = source->gain;     // 0-100 gain value
	copy->offset  = source->offset;   // 0-255 ofset value
	copy->time    = source->time;     // in milliseconds
	copy->wtime   = source->wtime;    // in milliseconds
	copy->bin     = source->bin;      // 1-2...
	copy->width   = source->width;    // Subframe capture width,  if 0, no subframe
	copy->height  = source->height;   // Subframe capture height, if 0, no subframe
	copy->speed   = source->speed;    // 0 low download speed, 1 high download speed
	copy->mode    = source->mode;     // 0 For interlaced camera 1 single exposure 2 double
	copy->amp     = source->amp;      // 0 amp off during capture, 1 amp always on, 2 amp off if exposure > 550ms
	copy->denoise = source->denoise;  // 0 Off 1 On
	copy->bytepix = source->bytepix;  // Bytes x pixel 1 = 8Bit, 2 = 16Bit
	copy->bitpix  = source->bitpix;   // Bits  x pixel 8, 12, 16
	copy->totsize = source->totsize;
	copy->tsize   = source->tsize;    // Transfer size as needed for the bulk read, qhyX_setregisters will compile;
	copy->preview = source->preview;  // 1 = Focus, 0 = capture;
	copy->edit    = source->edit;
}

int imgcam_iscamera(const char *model) 
{
	int retcode = 0;

	if (strcmp(model, "QHY2-Old") == 0)
	{
		retcode = qhy2old_iscamera();
	}
	else if (strcmp(model, "QHY5") == 0)
	{
		retcode = qhy5_iscamera();
	}
	else if (strcmp(model, "QHY5II-Series") == 0)
	{
		retcode = qhy5ii_iscamera();
		camid = 52;
	}
	else if (strcmp(model, "QHY6") == 0)
	{
		retcode = qhy6_iscamera();
	}
	else if (strcmp(model, "QHY6-Old") == 0)
	{
		retcode = qhy6old_iscamera();
	}
	else if (strcmp(model, "QHY7") == 0)
	{
		retcode = qhy7_iscamera();
	}
	else if (strcmp(model, "QHY8-Old") == 0)
	{
		retcode = qhy8old_iscamera();
	}
	else if (strcmp(model, "QHY8L") == 0)
	{
		retcode = qhy8l_iscamera();
	}
	else if (strcmp(model, "QHY9") == 0)
	{
		retcode = qhy9_iscamera();
	}
	else if (strcmp(model, "QHY11") == 0)
	{
		retcode = qhy11_iscamera();
	}
	return (retcode);
}

qhy_tecpars *imgcam_get_tecp()
{
	return &tecp;
}

qhy_camui *imgcam_get_camui()
{
	return &camui;
}

qhy_exposure *imgcam_get_expar()
{
	return &expar;
}

qhy_exposure *imgcam_get_shpar()
{
	return &shpar;
}

char *imgcam_get_model()
{
	return cammodel;
}

int imgcam_get_camid()
{
	return camid;
}

void imgcam_set_model(const char *val)
{
	// Set all relevant properties from camera specific
	/// Value "No cam" of the models combo
	if (strcmp(val, C_("camio","None")) == 0)
	{
		imgcam_init();
		camid = 0;
	}
	else if (strcmp(val, "QHY2-Old") == 0)
	{
		qhy2old_init();
		camid = 20;
	}
	else if (strcmp(val, "QHY5") == 0)
	{
		qhy5_init();
		camid = 5;
	}
	else if (strcmp(val, "QHY5II-Series") == 0)
	{
		qhy5ii_init();
		camid = 52;
	}
	else if (strcmp(val, "QHY6") == 0)
	{
		qhy6_init();
		camid = 6;
	}
	else if (strcmp(val, "QHY6-Old") == 0)
	{
		qhy6old_init();
		camid = 60;
	}
	else if (strcmp(val, "QHY7") == 0)
	{
		qhy7_init();
		camid = 7;
	}
	else if (strcmp(val, "QHY8-Old") == 0)
	{
		qhy8old_init();
		camid = 80;
	}
	else if (strcmp(val, "QHY8L") == 0)
	{
		qhy8l_init();
		camid = 81;
	}
	else if (strcmp(val, "QHY9") == 0)
	{
		qhy9_init();
		camid = 9;
	}
	else if (strcmp(val, "QHY11") == 0)
	{
		qhy11_init();
		camid = 11;
	}
	strcpy(cammodel, val);
}

unsigned char *imgcam_get_data()
{
	return databuffer[curdataptr];
}

int imgcam_loaded()
{
	return (loaded);
}

int imgcam_connected()
{
	return connected;
}

char *imgcam_get_msg()
{
	return cammsg;
}

void imgcam_init()
{
	static int first_time = 1;
	qhy_core_init();
	if (databuffer[0] != NULL)
	{
		free(databuffer[0]);
		databuffer[0] = NULL;
	}
	if (databuffer[1] != NULL)
	{
		free(databuffer[1]);
		databuffer[1] = NULL;
	}
	camid = 0;
	if (first_time)
	{
		cammodel = (char*)realloc(cammodel, 16);
		cammsg = (char*)realloc(cammsg, 1024);
		first_time = 0;
	}
	cammodel[0] = '\0';	
	cammsg[0] = '\0';	
	imgcam_init_list(0);
	strcpy(imgcam_get_camui()->binstr, "");
	strcpy(imgcam_get_camui()->roistr, "");
	strcpy(imgcam_get_camui()->spdstr, "");
	strcpy(imgcam_get_camui()->modstr, ""); 
	strcpy(imgcam_get_camui()->moddsc, ""); // This is the current label for multi-purpose modstr
	strcpy(imgcam_get_camui()->ampstr, "");
	strcpy(imgcam_get_camui()->snrstr, "");
	strcpy(imgcam_get_camui()->bppstr, "");
	strcpy(imgcam_get_camui()->byrstr, "0");
	strcpy(imgcam_get_camui()->tecstr, "");
	strcpy(imgcam_get_camui()->whlstr, "");

	// Positively no tec
	imgcam_get_tecp()->istec      = 0;      // 0 = Not driveable tec or no tec 1 = Driveable tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 0;      // Basically 0 - tecmax
	imgcam_get_tecp()->tecmax     = 0;      // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 

	loaded = 0;
	connected = 0;
}

char *imgcam_init_list(int all)
{
	/// Value "No cam" of the models combo
	strcpy(imgcam_get_camui()->camstr, C_("camio","None"));
	if ((imgcam_iscamera("QHY2-Old")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY2-Old");
	}

	if ((imgcam_iscamera("QHY5")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY5");
	}
	
	if ((imgcam_iscamera("QHY5II-Series")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY5II-Series");
	}

	if ((imgcam_iscamera("QHY6")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY6");
	}
	
	if ((imgcam_iscamera("QHY6-Old")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY6-Old");
	}
	
	if ((imgcam_iscamera("QHY7")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY7");
	}
	
	if ((imgcam_iscamera("QHY8-Old")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY8-Old");
	}

	if ((imgcam_iscamera("QHY8L")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY8L");
	}
	
	if ((imgcam_iscamera("QHY9")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY9");
	}
	
	if ((imgcam_iscamera("QHY11")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY11");
	}
	strcat(imgcam_get_camui()->camstr, "|:0");
	return imgcam_get_camui()->camstr;
}

int imgcam_connect()
{
	int retval = 1;
	
	cammsg[0] = '\0';
	if (camid > 0)
	{
		switch (camid)
		{
			case 5:
				// Trigger an initial exposure (we won't use this one)
				if ((retval = qhy_OpenCamera(qhy_core_getcampars()->vid, qhy_core_getcampars()->pid)) == 1)
				{
					retval = qhy5_bonjour();
				}
				break;
			case 52:
				if ((retval = qhy_opencamera(qhy_core_getcampars()->vid, qhy_core_getcampars()->pid)) == 1)
				{
					retval = qhy5ii_bonjour();
				}
				break;
			case 6:
			case 20:
			case 60:
			case 80:
				retval = qhy_OpenCamera(qhy_core_getcampars()->vid, qhy_core_getcampars()->pid);
				break;
			case 7:
			case 81:
			case 9:
			case 11:
				if ((retval = qhy_OpenCamera(qhy_core_getcampars()->vid, qhy_core_getcampars()->pid)) == 1)
				{
					retval = imgcam_settec(tecp.tecpwr);
				}
				break;
		}
		if ((retval == 0) && (strlen(cammsg) == 0))
		{
			strcpy(cammsg, qhy_core_msg());
		}
		connected = retval;
	}
	return (retval);
}

int imgcam_disconnect()
{
	int retval = 1;

	cammsg[0] = '\0';
	switch (camid)
	{
		case 5:
		case 52:
		case 6:
		case 60:
		case 7:
		case 80:
		case 81:
		case 9:
		case 11:
			if ((retval = qhy_CloseCamera()) == 0)
			{
				strcpy(cammsg, qhy_core_msg());
			}
			break;
	}
	connected = (retval == 1) ? 0 : connected;
	return (retval);
}

int imgcam_reset()
{
	int retval = -1;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 20:
			retval = qhy2old_reset();
			break;
		case 5:
			retval = qhy5_reset();
			break;
		case 52:
			retval = qhy5ii_reset();
			break;
		case 6:
			retval = qhy6_reset();
			break;
		case 60:
			retval = qhy6old_reset();
			break;
		case 7:
			retval = qhy7_reset();
			break;
		case 80:
			retval = qhy8old_reset();
			break;
		case 81:
			retval = qhy8l_reset();
			break;
		case 9:
			retval = qhy9_reset();
			break;
		case 11:
			retval = qhy11_reset();
			break;
	}
	if (retval == 1)
	{
		sprintf(cammsg, C_("camio","Camera reset script complete"));
	}
	else if (retval == 0)
	{
		// Driver writes a specific message here
		//sprintf(cammsg, "Error resetting camera");
	}
	else
	{
		retval = 0;
		strcpy(cammsg, C_("camio","Reset for this camera model is unsupported"));
	}
	return (retval);
}

int imgcam_shoot()
{
	int retval = 1;
	
	cammsg[0] = '\0';
	if (expar.edit)
	{
		imgcam_exparcpy(&shpar, &expar);
		expar.edit = 0;
	}
	loaded = 0;
	switch (camid)
	{
		case 20:
			if ((retval = ((shpar.edit) ? qhy2old_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 5:
			if ((retval = ((shpar.edit) ? qhy5_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_cmosStartExposure(shpar.time);
			}
			break;
		case 52:
			if ((retval = qhy5ii_setregisters(&shpar)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 6:
			if ((retval = ((shpar.edit) ? qhy6_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 60:
			if ((retval = ((shpar.edit) ? qhy6old_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 7:
			if ((retval = ((shpar.edit) ? qhy7_setregisters(&shpar) : 1)) == 1)
			{
				// Shutter trick goes here
				if ((retval = qhy_ccdStartExposure(shpar.time)) == 1)
				{
					usleep(5000);
					if ((retval = qhy_ccdAbortCapture() == 1))
					{
						retval = qhy_ccdStartExposure(shpar.time);
					}	
				}	
			}
			break;
		case 80:
			if ((retval = ((shpar.edit) ? qhy8old_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			usleep(200000);
			break;
		case 81:
			if ((retval = ((shpar.edit) ? qhy8l_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 9:
			if ((retval = ((shpar.edit) ? qhy9_setregisters(&shpar) : 1)) == 1)
			{
				// In dark mode
				// Close the shutter, otherwise noop
				if ((retval = ((shpar.mode > 0) ? imgcam_shutter(1) : 1)) == 1)
				{
					retval = qhy_ccdStartExposure(shpar.time);
				}
			}
			break;
		case 11:
			if ((retval = ((shpar.edit) ? qhy11_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
}

int imgcam_readout()
{
	int retval = 1;
	int error = 0, allocsize = (MAX(shpar.tsize, shpar.totsize) + shpar.bytepix), length_transferred = 0;
	static int presize[2] = {0, 0};
	
	cammsg[0] = '\0';
	curdataptr = (curdataptr == 0) ? 1 : 0;
	if ((qhy_core_getcampars()->buftimes > 0) || (qhy_core_getcampars()->buftimef > 0))
	{
		//printf("Buffering\n");
		// Allow for buffering time on buffered camera
		usleep(((shpar.speed == 0) ?  qhy_core_getcampars()->buftimes : qhy_core_getcampars()->buftimef) / shpar.bin * 1000);	
		// Check if camera is good and ready thereafter
		while (qhy_getCameraStatus() == 0)  
		{
			usleep(1000);
			/*if (camid == 9)
			{
				// Qhy9 only allow one getCamerStatus call  
				break;
			}*/
		}
	}
	if ((allocsize != presize[curdataptr]) || (databuffer[curdataptr] == NULL))
	{
		presize[curdataptr] = allocsize;
		//printf("Realloc camio %d, %d\n", allocsize, presize[curdataptr]);
		databuffer[curdataptr] = (unsigned char*)realloc(databuffer[curdataptr], allocsize);
		//printf("Get Data\n");
	}
	if ((retval = qhy_getImgData(qhy_core_getendp()->bulk, shpar.tsize, databuffer[curdataptr], &error, &length_transferred)) == 1)
	{
		switch (camid)
		{
			case 20:
				qhy2old_decode(databuffer[curdataptr]);
				break;
			case 5:
				qhy5_decode(databuffer[curdataptr]);
				break;
			case 52:
				qhy5ii_decode(databuffer[curdataptr]);
				break;
			case 6:
				qhy6_decode(databuffer[curdataptr]);	
				break;
			case 60:
				//printf("Decode\n");
				qhy6old_decode(databuffer[curdataptr]);	
				break;
			case 7:
				qhy7_decode(databuffer[curdataptr]);	
				break;
			case 80:
				qhy8old_decode(databuffer[curdataptr]);	
				break;
			case 81:
				qhy8l_decode(databuffer[curdataptr]);	
				break;
			case 9:
				if (shpar.mode > 0)
				{
					// In dark mode
					// Release shutter go avoid excess strain
					imgcam_shutter(2);
				}
				qhy9_decode(databuffer[curdataptr]);	
				break;
			case 11:
				qhy11_decode(databuffer[curdataptr]);	
				break;
		}
		loaded = 1;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
}

int imgcam_abort()
{
	int retval = 0;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 5:
		//	retval = qhy_cmosAbortCapture(shpar.tsize);
		//	break;
		case 20:
		case 60:
		case 80:
			if (qhy_CloseCamera())
			{
				usleep(100000);
				if (imgcam_reset())
				{
					retval = qhy_OpenCamera(qhy_core_getcampars()->vid, qhy_core_getcampars()->pid);
					usleep(100000);
				}
			}
			break;
		case 52:
			retval = qhy5ii_AbortCapture();
			break;
		case 6:
		case 7:
		case 81:
		case 9:
		case 11:
			retval = qhy_ccdAbortCapture();
			usleep(100000);
			break;
	}
	loaded = (retval == 1) ? 0 : loaded;
	expar.edit = (retval == 1) ? 1 : expar.edit;
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
}

int imgcam_settec(int pwm)
{
	int retval = 0;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 20:
		case 5:
		case 52:
		case 6:
		case 60:
		case 80:
			break;
		case 7:
		case 81:
		case 9:
		case 11:
			retval = qhy_setDC201_i(pwm, 1);
			break;
	}
	imgcam_get_tecp()->tecerr = (retval == 0) ? 1 : 0;
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
} 

int imgcam_gettec(double *tC, double *mV)
{
	int retval = 0;
	double imgtC, imgmV;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 20:
		case 5:
		case 6:
		case 60:
		case 80:
			break;
		case 52:
			imgtC = qhy5lii_GetTemp();
			retval = 1;
			break;			
		case 7:
		case 81:
		case 9:
		case 11:
			retval = qhy_getDC201_i(&imgtC, &imgmV);
			break;
	}
	if (retval)
	{
		imgcam_get_tecp()->tecerr = 0;
		*tC = imgtC;
		if (mV != NULL)
		{
			*mV = imgmV;
		}
	}
	else
	{
		imgcam_get_tecp()->tecerr = 1;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
} 

int imgcam_shutter(int cmd)
{
	int retval = 1;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 20:
			break;
		case 5:
			break;
		case 52:
			break;
		case 6:
			break;
		case 60:
			break;
		case 7:
			break;
		case 80:
			break;
		case 81:
			break;
		case 9:
			cammsg[0] = '\0';
			retval = qhy_Shutter(cmd);
			break;
		case 11:
			break;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
} 

int imgcam_wheel(int pos)
{
	int retval = 1;
	
	cammsg[0] = '\0';
	switch (camid)
	{
		case 20:
		case 5:
		case 52:
		case 6:
		case 60:
		case 80:
		case 81:
			break;
		case 9:
			cammsg[0] = '\0';
			retval = qhy9_setColorWheel(pos);
			break;
		case 7:
		case 11:
			cammsg[0] = '\0';
			retval = qhy_setColorWheel(pos);
			break;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
} 


