/*
 * imgCamio.c
 *
 *  Created on: 01.09.2013
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *	Modified by Daniel Holler (astrodan02@gmail.com)
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
#include "qhy9l.h"
#include "qhy90a.h"
#include "qhy10.h"
#include "qhy11.h"
#include "qhy12.h"
#include "minicam5.h"
#include "dsi2pro.h"
#ifdef HAVE_SBIG
#include "sbigcore.h"
#endif
#ifdef HAVE_URVC
#include "urvccore.h"
#endif
#ifdef HAVE_ATIK
#include "atikcore.h"
#endif

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
static 	int isizeX = 0, isizeY = 0;


char *get_core_msg()
{
	if (camid < 1000)
		return qhy_core_msg();
	if (camid == 1000)
		return dsi2pro_core_msg();
#ifdef HAVE_SBIG
	if (camid == 2000)
		return sbig_GetErrorString();
#endif
#ifdef HAVE_URVC
	if (camid == 2100)
		return urvc_GetErrorString();
#endif
	return NULL;
}

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
	else if (strcmp(model, "miniCAM5-Series") == 0)
	{
		retcode = minicam5_iscamera();
		camid = 53;
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
	else if (strcmp(model, "IC8300") == 0)
	{
		retcode = qhy9l_iscamera();
	}
	else if (strcmp(model, "QHY90A") == 0)
	{
		retcode = qhy90a_iscamera();
	}
	else if (strcmp(model, "QHY10") == 0)
	{
		retcode = qhy10_iscamera();
	}
	else if (strcmp(model, "QHY11") == 0)
	{
		retcode = qhy11_iscamera();
	}
	else if (strcmp(model, "QHY12") == 0)
	{
		retcode = qhy12_iscamera();
	}
	else if (strcmp(model, "DSI2PRO") == 0)
	{
		retcode = dsi2pro_iscamera();
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
	else if (strcmp(val, "miniCAM5-Series") == 0)
	{
		minicam5_init();
		camid = 53;
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
	else if (strcmp(val, "IC8300") == 0)
	{
		qhy9l_init();
		camid = 91;
	}
	else if (strcmp(val, "QHY90A") == 0)
	{
		qhy90a_init();
		camid = 92;
	}
	else if (strcmp(val, "QHY10") == 0)
	{
		qhy10_init();
		camid = 10;
	}
	else if (strcmp(val, "QHY11") == 0)
	{
		qhy11_init();
		camid = 11;
	}
	else if (strcmp(val, "QHY12") == 0)
	{
		qhy12_init();
		camid = 12;
	}
	else if (strcmp(val, "DSI2PRO") == 0)
	{
		dsi2pro_init();
		camid = 1000;
	}
#ifdef HAVE_SBIG
	else if (strncmp(val, "SBIG", 4) == 0)
	{
		char *devName = strrchr(val,' ');		
		int i;
		
		devName++;
		for (i = 0; i < sbig_GetCameraList()->camnum; i++)
		{
			if (strcmp(sbig_GetCameraList()->listinfo[i].camport, devName) == 0)
			{
				camid = 2000; //sbig_GetCameraList()->listinfo[i].camId;
				break;
			}
		}
	}
#endif
#ifdef HAVE_URVC
	else if (strncmp(val, "PSBIG", 5) == 0)
	{
		char *devName = strrchr(val,' ');		
		int i;
		
		devName++;
		for (i = 0; i < urvc_GetCameraList()->camnum; i++)
		{
			if (strcmp(urvc_GetCameraList()->listinfo[i].camport, devName) == 0)
			{
				camid = 2100; 
				break;
			}
		}
	}
#endif
#ifdef HAVE_ATIK
	else if (strncmp(val, "Atik", 4) == 0) 
	{
		if (atik_list_item_select(strdup(val)))
		{
			camid = 3000;
		}
	}
#endif
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
	if (!first_time)
	{
#ifdef HAVE_SBIG
		sbig_core_close();
#endif
#ifdef HAVE_ATIK
		atik_list_destroy();
#endif
	}
#ifdef HAVE_SBIG
	sbig_core_init(imgBasePath);
#endif
#ifdef HAVE_URVC
	urvc_core_init();
#endif

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
		cammodel = (char*)realloc(cammodel, 64);
		cammsg = (char*)realloc(cammsg, 1024);
		first_time = 0;
	}
	cammodel[0] = '\0';	
	cammsg[0] = '\0';	
	imgcam_init_list(0);
	imgcam_get_camui()->hasgain = 0;
	imgcam_get_camui()->hasoffset = 0;
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
	imgcam_get_camui()->shutterMode = 0;
	imgcam_get_camui()->pszx = 0;
	imgcam_get_camui()->pszy = 0;

	// Positively no tec
	imgcam_get_tecp()->istec      = 0;      // 0 = Not driveable tec or no tec 1 = Driveable tec
	imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
	imgcam_get_tecp()->tecpwr     = 0;      // Basically 0 - tecmax
	imgcam_get_tecp()->tecmax     = 0;      // 0-255
	imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
	imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
	imgcam_get_tecp()->tecedit	= 0;

	loaded = 0;
	connected = 0;
}

void imgcam_end()
{
#ifdef HAVE_SBIG
	sbig_core_close();
#endif
#ifdef HAVE_URVC
	urvc_core_init();
#endif
#ifdef HAVE_ATIK
	atik_list_destroy();
#endif
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

	if ((imgcam_iscamera("IC8300")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|IC8300");
	}
	
	if ((imgcam_iscamera("QHY90A")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY90A");
	}

	if ((imgcam_iscamera("QHY10")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY10");
	}

	if ((imgcam_iscamera("QHY11")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY11");
	}
	if ((imgcam_iscamera("QHY12")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|QHY12");
	}
	if ((imgcam_iscamera("DSI2PRO")) || (all))
	{
		strcat(imgcam_get_camui()->camstr, "|DSI2PRO");
	}
#ifdef HAVE_SBIG
	sbig_core_reload_list();
	strcat(imgcam_get_camui()->camstr, sbig_GetCameraList()->camlist);
#endif
#ifdef HAVE_URVC
	urvc_core_reload_list();
	strcat(imgcam_get_camui()->camstr, urvc_GetCameraList()->camlist);
#endif
#ifdef HAVE_ATIK
	atik_list_create();
	strcat(imgcam_get_camui()->camstr, atik_list_get());	
#endif
	strcat(imgcam_get_camui()->camstr, "|:0");
	return imgcam_get_camui()->camstr;
}

int imgcam_connect()
{
	int retval = 1;
#if defined(HAVE_SBIG) || defined(HAVE_URVC)
	char *devName;
#endif
	
	cammsg[0] = '\0';
	if (camid > 0)
	{
		switch (camid)
		{
			case 5:
				// Trigger an initial exposure (we won't use this one)
				if ((retval = qhy_OpenCamera()) == 1)
				{
					retval = qhy5_bonjour();
				}
				break;
			case 52:
				if ((retval = qhy_opencamera()) == 1)
				{
					retval = qhy5ii_bonjour();
				}
				break;
			case 53:
				if ((retval = qhy_opencamera()) == 1)
				{
					retval = minicam5_bonjour();
				}
				break;
			case 20:
			case 6:
			case 60:
			case 80:
				retval = qhy_OpenCamera();
				break;
			case 7:
			case 81:
			case 9:
			case 10:
			case 11:
			case 12:
				if ((retval = qhy_OpenCamera()) == 1)
				{
					retval = imgcam_settec(tecp.tecpwr, -1);
				}
				break;
			case 91:
			case 92:
				if ((retval = qhy_OpenCamera()) == 1)
				{
					// TODO send oled
					//retval = imgcam_settec(tecp.tecpwr, -1);
				}
				break;
			case 1000:
				retval = dsi2pro_OpenCamera();
				break;
#ifdef HAVE_SBIG
			case 2000:
				//Sbig camera
				devName = strrchr(cammodel,' ');		
				
				devName++;
				if ((retval = (sbig_OpenDevice(devName) == 0)) == 1)
				{
					if ((retval = (sbig_EstablishLink() == 0)) == 1)
					{
						//Set all UI related
						imgcam_get_camui()->hasgain = 0;
						imgcam_get_camui()->hasoffset = 0;
						strcpy(imgcam_get_camui()->binstr, sbig_GetCameraDetails()->binList);
						strcpy(imgcam_get_camui()->roistr, C_("camio","Full|512x512|256x256:0"));
						strcpy(imgcam_get_camui()->spdstr, sbig_GetCameraDetails()->spdList);
						strcpy(imgcam_get_camui()->ampstr, sbig_GetCameraDetails()->ampList);
						strcpy(imgcam_get_camui()->modstr, sbig_GetCameraDetails()->modList);
						strcpy(imgcam_get_camui()->moddsc, sbig_GetCameraDetails()->modList[0] != '\0' ? C_("camio","Light/Dark mode") : "");
						strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
						sprintf(imgcam_get_camui()->byrstr, "%d", sbig_GetCameraDetails()->colorId);
						// Cfw
						strcpy(imgcam_get_camui()->whlstr, sbig_GetCameraDetails()->cfwList);
						// Tec
						imgcam_get_tecp()->istec      = sbig_GetCameraDetails()->camTec * 3;      // Mode see imgCamio.h
						imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
						imgcam_get_tecp()->tecpwr     = 5;      // Basically 0 - tecmax
						imgcam_get_tecp()->tecmax     = 255;      // 0-255
						imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
						imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
						imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
						// Shutter
						imgcam_get_camui()->shutterMode = sbig_GetCameraDetails()->camShutter;
						// Header values
						imgcam_get_camui()->pszx = sbig_GetCameraDetails()->ccdpixW;
						imgcam_get_camui()->pszy = sbig_GetCameraDetails()->ccdpixH;
						// Basic expar
						imgcam_get_expar()->bitpix  = 16;	
						imgcam_get_expar()->bytepix = 2;	
						imgcam_get_expar()->tsize   = 0;
						imgcam_get_expar()->edit    = 0;
						// Tec initial set
						if (imgcam_get_tecp()->istec)
						{
							retval = imgcam_settec(imgcam_get_tecp()->tecpwr, 2);
						}
					}
					else
					{
						sbig_CloseDevice();
					}
				}
				break;
#endif
#ifdef HAVE_URVC
			case 2100:
				//Sbig parallel camera through urvc driver
				devName = strrchr(cammodel,' ');		
				
				devName++;
				if ((retval = urvc_OpenCamera(devName)) == 1)
				{
					//Set all UI related
					imgcam_get_camui()->hasgain = 0;
					imgcam_get_camui()->hasoffset = 0;
					strcpy(imgcam_get_camui()->binstr, urvc_GetCameraDetails()->binList);
					strcpy(imgcam_get_camui()->roistr, "");
					strcpy(imgcam_get_camui()->spdstr, urvc_GetCameraDetails()->spdList);
					strcpy(imgcam_get_camui()->ampstr, urvc_GetCameraDetails()->ampList);
					strcpy(imgcam_get_camui()->modstr, urvc_GetCameraDetails()->modList);
					strcpy(imgcam_get_camui()->moddsc, urvc_GetCameraDetails()->modList[0] != '\0' ? C_("camio","Light/Dark mode") : "");
					strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
					sprintf(imgcam_get_camui()->byrstr, "-1");
					// Cfw
					strcpy(imgcam_get_camui()->whlstr, urvc_GetCameraDetails()->cfwList);
					// Tec
					imgcam_get_tecp()->istec      = urvc_GetCameraDetails()->camTec * 3;      // Mode see imgCamio.h
					imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
					imgcam_get_tecp()->tecpwr     = 5;      // Basically 0 - tecmax
					imgcam_get_tecp()->tecmax     = 255;      // 0-255
					imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
					imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
					imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
					// Shutter
					imgcam_get_camui()->shutterMode = urvc_GetCameraDetails()->camShutter;
					// Header values
					imgcam_get_camui()->pszx = urvc_GetCameraDetails()->ccdpixW;
					imgcam_get_camui()->pszy = urvc_GetCameraDetails()->ccdpixH;
					// Basic expar
					imgcam_get_expar()->bitpix  = 16;	
					imgcam_get_expar()->bytepix = 2;	
					imgcam_get_expar()->tsize   = 0;
					imgcam_get_expar()->edit    = 0;
					// Tec initial set
					if (imgcam_get_tecp()->istec)
					{
						retval = imgcam_settec(imgcam_get_tecp()->tecpwr, 2);
					}
				}
				break;
#endif
#ifdef HAVE_ATIK
			case 3000:
				// Atik camera
				if ((retval = atik_camera_open()) == 1)
				{
					//Set all UI related
					imgcam_get_camui()->hasgain = 0;
					imgcam_get_camui()->hasoffset = 0;
					strcpy(imgcam_get_camui()->binstr, atik_camera_getBinList());
					strcpy(imgcam_get_camui()->roistr, C_("camio","Full|512x512|256x256:0"));
					// Gain and Offset and PreviewMode are said not functional on VS class camera
					// Waiting for info to gather wich are the camera type that can support that feature
					// This will work with version >= 1.4. No effect for version 1.1
					// No way to check lib version. No way to check if the command has effect
					if (atik_camera_getType() == IC24)
					{
						strcpy(imgcam_get_camui()->spdstr, C_("camio","0-Slow|1-Fast:0"));
					} 
					else
					{
						strcpy(imgcam_get_camui()->spdstr, "");
					}
					strcpy(imgcam_get_camui()->ampstr, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
					if (atik_camera_getCapabilities()->hasShutter)
					{
						strcpy(imgcam_get_camui()->modstr, C_("camio","0-Light|1-Dark:0"));
						strcpy(imgcam_get_camui()->moddsc, C_("camio","Light/Dark mode"));
					}
					else
					{
						strcpy(imgcam_get_camui()->modstr, "");
						strcpy(imgcam_get_camui()->moddsc, "");
					}
					if (atik_camera_getCapabilities()->has8BitMode)
					{	
						strcpy(imgcam_get_camui()->bppstr, "1-8Bit|2-16Bit|:1");
					}
					else
					{
						strcpy(imgcam_get_camui()->bppstr, "2-16Bit|:0");
					}
					sprintf(imgcam_get_camui()->byrstr, "%d", atik_camera_getColorId());
					// Cfw
					strcpy(imgcam_get_camui()->whlstr, atik_camera_getCfwList());
					// Tec
					imgcam_get_tecp()->istec      = (atik_camera_getCapabilities()->cooler == COOLER_SETPOINT) ? 4 : 0;  // Mode see imgCamio.h
					imgcam_get_tecp()->tecerr     = 0;      // Error reading / setting tec; 
					imgcam_get_tecp()->tecpwr     = 5;      // Basically 0 - tecmax
					imgcam_get_tecp()->tecmax     = 255;      // 0-255
					imgcam_get_tecp()->tecauto    = 0;      // 0 = Manual, 1 = Seek target temp
					imgcam_get_tecp()->tectemp    = 0.;     // Only meaningful when tecauto = 1; 
					imgcam_get_tecp()->settemp    = 0.;     // Only meaningful when tecauto = 1; 
					// Shutter
					imgcam_get_camui()->shutterMode = atik_camera_getCapabilities()->hasShutter;
					// Header values
					imgcam_get_camui()->pszx = atik_camera_getCapabilities()->pixelSizeX;
					imgcam_get_camui()->pszy = atik_camera_getCapabilities()->pixelSizeY;
					// Basic expar
					imgcam_get_expar()->bitpix  = 16;	
					imgcam_get_expar()->bytepix = 2;	
					imgcam_get_expar()->tsize   = 0;
					imgcam_get_expar()->edit    = 0;
				}
#endif
		}
		if ((retval == 0) && (strlen(cammsg) == 0))
		{
			strcpy(cammsg, get_core_msg());
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
		case 20:
		case 5:
		case 52:
		case 53:
		case 6:
		case 7:
		case 60:
		case 80:		
		case 81:
		case 9:
		case 10:
		case 11:
		case 12:
			retval = qhy_CloseCamera();
			break;
		case 91:
		case 92:
			// TODO send oled
			retval = qhy_CloseCamera();
			break;
		case 1000:
			retval = dsi2pro_CloseCamera();
			break;
#ifdef HAVE_SBIG
		case 2000:
			retval = (sbig_CloseDevice() == 0);
			break;
#endif
#ifdef HAVE_URVC
		case 2100:
			urvc_CloseCamera();
			retval = 1;
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			// Atik camera
			if (imgcam_get_tecp()->istec)
			{
				atik_camera_initiateWarmUp();
			}
			atik_camera_close();
#endif
	}
	if (retval == 0)
	{
		strcpy(cammsg, get_core_msg());
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
		case 53:
			retval = minicam5_reset();
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
		case 91:
			retval = qhy9l_reset();
			break;
		case 92:
			retval = qhy90a_reset();
			break;
		case 10:
			retval = qhy10_reset();
			break;
		case 11:
			retval = qhy11_reset();
			break;
		case 12:
			retval = qhy12_reset();
			break;
		case 1000:
			retval = dsi2pro_reset();
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
				usleep(200000);
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
		case 53:
			if ((retval = minicam5_setregisters(&shpar)) == 1)
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
		case 91:
			if ((retval = ((shpar.edit) ? qhy9l_setregisters(&shpar) : 1)) == 1)
			{
				// printf("setRegisters ok\n");
				// In dark mode
				// Close the shutter, otherwise noop
				if ((retval = ((shpar.mode > 0) ? imgcam_shutter(1) : 1)) == 1)
				{
					retval = qhy_ccdStartExposure(shpar.time);
					// printf("startExposure returned %d\n", retval);
				}
			}
			break;
		case 92:
			if ((retval = ((shpar.edit) ? qhy90a_setregisters(&shpar) : 1)) == 1)
			{
				// printf("setRegisters ok\n");
				// In dark mode
				// Close the shutter, otherwise noop
				if ((retval = ((shpar.mode > 0) ? imgcam_shutter(1) : 1)) == 1)
				{
					retval = qhy_ccdStartExposure(shpar.time);
					// printf("startExposure returned %d\n", retval);
				}
			}
			break;
		case 10:
			if ((retval = ((shpar.edit) ? qhy10_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 11:
			if ((retval = ((shpar.edit) ? qhy11_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 12:
			if ((retval = ((shpar.edit) ? qhy12_setregisters(&shpar) : 1)) == 1)
			{
				retval = qhy_ccdStartExposure(shpar.time);
			}
			break;
		case 1000:
			retval = dsi2pro_StartExposure(&shpar);
			break;
#ifdef HAVE_SBIG
		case 2000:
			retval = (sbig_StartExposure(&shpar) == 0);
			break;
#endif
#ifdef HAVE_URVC
		case 2100:
			retval = (urvc_StartExposure(&shpar));
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			{
				// Atik camera
				if (shpar.edit)
				{
					// Gain and offset are for OEM only and not all models do support it
					// Please have a look at comments on cAtik.cpp
					//if (imgcam_get_camui()->hasgain)
					//{
					//	printf("shpar.gain %d, shpar.offset %d\n", ((shpar.gain * 63) / 100), (shpar.offset * 2));
					//	retval = atik_camera_setGain(((shpar.gain * 63) / 100), (shpar.offset * 2));
					//	int gain = 0, offset = 0;
					//	atik_camera_getGain(&gain, &offset);
					//	printf("Gain %d, Offset %d\n", gain, offset);
					//}
					// This will work with version >= 1.4. No effect for version 1.1
					// No way to check lib version. No way to check if the command has effect
					if (strlen(imgcam_get_camui()->spdstr) > 0)
					{	
						atik_camera_setPreviewMode(shpar.speed);
					}
					if (atik_camera_getCapabilities()->has8BitMode)
					{	
						atik_camera_set8BitMode((shpar.bytepix==1));
					}
					isizeX = (shpar.width == 0) ? (int)atik_camera_getCapabilities()->pixelCountX : shpar.width;
					isizeY = (shpar.height == 0) ? (int)atik_camera_getCapabilities()->pixelCountY : shpar.height;
					shpar.width  = (int)atik_camera_imageWidth(((isizeX == 0)  ? atik_camera_getCapabilities()->pixelCountX : isizeX), (unsigned int)shpar.bin);
					shpar.height = (int)atik_camera_imageHeight(((isizeY == 0) ? atik_camera_getCapabilities()->pixelCountY : isizeY), (unsigned int)shpar.bin);
					shpar.tsize = isizeX * isizeY * shpar.bytepix;
					shpar.totsize = shpar.tsize;
					shpar.wtime = (int)(atik_camera_delay((double)(shpar.time/1000.0))/1000);
					//printf("shpar.wtime %d,%d\n", shpar.wtime, shpar.time);
					shpar.amp = ((shpar.time < 550) && (shpar.amp == 2)) ? 1 : shpar.amp; 

					shpar.edit = 0;
				}
			
				if(retval)
				{
					if (atik_camera_getCapabilities()->hasShutter)
					{
						// In light mode
						// Open shutter, otherwise ensure close
						retval = (shpar.mode == 0) ? atik_camera_setShutter(1) : atik_camera_setShutter(0);
					}
					if (retval)
					{
						retval = (atik_camera_startExposure(shpar.amp));
					}
				}
			}
#endif
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
	}
	return (retval);
}

int imgcam_readout()
{
	int allocsize = (MAX(shpar.tsize, shpar.totsize) + shpar.bytepix);
	static int presize[2] = {0, 0};
	
	cammsg[0] = '\0';
	curdataptr = (curdataptr == 0) ? 1 : 0;
	if ((allocsize != presize[curdataptr]) || (databuffer[curdataptr] == NULL))
	{
		presize[curdataptr] = allocsize;
		// printf("Realloc camio %d, %d\n", allocsize, presize[curdataptr]);
		//databuffer[curdataptr] = (unsigned char*)realloc(databuffer[curdataptr], allocsize);
		if (databuffer[curdataptr] == NULL)
		{
			free(databuffer[curdataptr]);
		}
		databuffer[curdataptr] = (unsigned char*)malloc(allocsize);
		// printf("Get Data\n");
	}
	return imgcam_readout_ext(databuffer[curdataptr]);
}


int imgcam_readout_ext(unsigned char *p)
{
	int retval = 1;
	int error = 0, length_transferred = 0;
	if (camid < 1000)
	{
		// QHY
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
		//char *buf = NULL;
		//printf("Begin readout: %s\n", gettimestamp(buf));
		retval = qhy_getImgData(shpar.tsize, p, &error, &length_transferred);
		//printf("End readout  : %s\n", gettimestamp(buf));
	}
	else if (camid == 1000) // plouis
	{
		//Meade dsi2pro
		if ((retval = dsi2pro_getImgData()) == 1)
		{
			length_transferred = shpar.tsize;
		}
	}
#ifdef HAVE_SBIG
	else if (camid == 2000)
	{
		//SBIG
		if ((retval = (sbig_Readout(&shpar, p) == 0)) == 1)
		{
			length_transferred = shpar.tsize;
		}
	}
#endif
#ifdef HAVE_URVC
	else if (camid == 2100)
	{
		//PSBIG = URVC
		if ((retval = urvc_Readout(&shpar, p)) == 1)
		{
			length_transferred = shpar.tsize;
		}
	}
#endif
#ifdef HAVE_ATIK
	else if (camid == 3000)
	{	
		//Atik camera
		if (atik_camera_getCapabilities()->hasShutter)
		{
			retval = atik_camera_setShutter(0);
		}
		//printf("Readout : %dx%d %d\n", isizeX, isizeY, shpar.bin);
		//printf("GetImage: %dx%d %d\n", shpar.width, shpar.height, shpar.bin);
		if ((retval) && (atik_camera_readCCD(0, 0, (unsigned int)isizeX, (unsigned int)isizeY, (unsigned int)shpar.bin, (unsigned int)shpar.bin)))
		{
			if ((retval = atik_camera_getImage((unsigned short *) p, (unsigned int)(shpar.width * shpar.height))))
			{
				length_transferred = shpar.tsize;	
			}
		}
	}
#endif
	else
	{
		// Unknown
		retval = 0;
	}
	if (retval == 1)
	{
		if (shpar.tsize == length_transferred)
		{
			switch (camid)
			{
				case 20:
					qhy2old_decode(p);
					break;
				case 5:
					qhy5_decode(p);
					break;
				case 52:
					qhy5ii_decode(p);
					break;
				case 53:
					minicam5_decode(p);
					break;
				case 6:
					qhy6_decode(p);	
					break;
				case 60:
					//printf("Decode\n");
					qhy6old_decode(p);	
					break;
				case 7:
					qhy7_decode(p);	
					break;
				case 80:
					qhy8old_decode(p);	
					break;
				case 81:
					qhy8l_decode(p);	
					break;
				case 9:
					if (shpar.mode > 0)
					{
						// In dark mode
						// Release shutter to avoid excess strain
						imgcam_shutter(2);
					}
					qhy9_decode(p);	
					break;
				case 91:
					if (shpar.mode > 0)
					{
						// In dark mode
						// Release shutter to avoid excess strain
						imgcam_shutter(2);
					}
					qhy9l_decode(p);	
					break;
				case 92:
					if (shpar.mode > 0)
					{
						// In dark mode
						// Release shutter to avoid excess strain
						imgcam_shutter(2);
					}
					qhy90a_decode(p);	
					break;
				case 10:
					qhy10_decode(p);	
					break;
				case 11:
					qhy11_decode(p);	
					break;
				case 12:
					qhy12_decode(p);	
					break;
				case 1000:	// plouis
					dsi2pro_decode(p);	
					break;
			}
			loaded = 1;
		}
		else
		{
			// printf("Data: %d, %d\n", shpar.tsize, length_transferred);
			sprintf(cammsg, C_("camio","Bad data received, discarded"));
			retval = 0;
		}
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, qhy_core_msg());
		// printf("%s\n", cammsg);
		// printf("TotalSize: %d, TSize: %d, Transferred: %d\n", shpar.totsize, shpar.tsize, length_transferred);
	}
	return (retval);
}

int imgcam_abort(int mode)
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
		case 53:
			retval = minicam5_AbortCapture();
			break;
		case 6:
		case 7:
		case 81:
		case 9:
		case 91:
		case 92:
		case 10:
		case 11:
		case 12:
			if (mode == REQKILL)
			{
				retval = qhy_ccdAbortCapture();
			}
			else
			{
				retval = qhy_ccdStopCapture() * -1;
			}
			usleep(100000);
			break;
		case 1000:
			retval = dsi2pro_AbortCapture();
			usleep(100000);
			break;
#ifdef HAVE_SBIG
		case 2000:
			retval = (sbig_KillExposure() == 0);
			break;
#endif
#ifdef HAVE_URVC
		case 2100:
			retval = urvc_KillExposure();
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			retval = atik_camera_abortExposure();
			break;
#endif

	}
	loaded = (retval == 1) ? 0 : loaded;
	expar.edit = (retval == 1) ? 1 : expar.edit;
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
	}
	return (retval);
}

int imgcam_settec(double setValue, int setMode)
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
		case 53:
			retval = minicam5_setTec((int)setValue, 1);
			break;			
		case 7:
		case 81:
		case 9:
		case 91:
		case 92:
		case 10:
		case 11:
		case 12:
			retval = qhy_setDC201_i((int)setValue, 1);
			break;
#ifdef HAVE_SBIG
		case 2000:
			// Set val can be temp or pwm (0-255) depending on set mode
			retval = (sbig_SetTemperatureRegulation(setMode, setValue) == 0);
			if (setMode == 1)
			{
				// If in auto mode also set auto-freeze mode
				sbig_SetTemperatureRegulation(5, setValue);
			}
			break;
#endif
#ifdef HAVE_URVC
		case 2100:
			// Set val can be temp or pwm (0-255) depending on set mode
			retval = urvc_SetTemperatureRegulation(setMode, setValue);
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			//Atik camera
			// setMode = 2 -> warm up (if tec is on)
			if (setMode == 1)
			{
				retval = atik_camera_setCooling((float)setValue);
			}
			else
			{
				COOLING_STATE state;
				
				if ((retval = atik_camera_getCoolingStatus(&state, NULL, NULL)))
				{
					if (state == COOLING_SETPOINT)
					{
						retval = atik_camera_initiateWarmUp();
					}
				}
			}			
			break;
#endif
	}
	imgcam_get_tecp()->tecerr = (retval == 0) ? 1 : 0;
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		printf("%s\n", get_core_msg());
		strcpy(cammsg, qhy_core_msg());
	}
	return (retval);
} 

int imgcam_gettec(double *tC, double *setTemp, int *power, int *enabled)
{
	int retval = 0;
	double mV = 0;
	float pwrPct, currentTemp;
	
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
			*tC = qhy5lii_GetTemp();
			retval = 1;
			break;			
		case 53:
		case 7:
		case 81:
		case 9:
		case 91:
		case 92:
		case 10:
			retval = qhy_getDC201_i(tC, &mV);
			break;
		case 11:
		case 12:
			retval = qhy_getDC201_i(tC, &mV);
			break;
		case 1000:
			*tC = dsi2pro_GetTemp();
			retval = 1;
			break;
#ifdef HAVE_SBIG
		case 2000:
			retval = (sbig_QueryTemperatureStatus(enabled, tC, setTemp, power) == 0);
			break;
#endif
#ifdef HAVE_URVC
		case 2100:
			retval = urvc_QueryTemperatureStatus(enabled, tC, setTemp, power);
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			//Atik camera
			retval = atik_camera_getCoolingStatus((COOLING_STATE *) enabled, (float *) setTemp, &pwrPct);
			if (retval)
			{
				*power = (int)(pwrPct*2.55);
				retval = atik_camera_getTemperatureSensorStatus((unsigned int) 1, &currentTemp);
				*tC = (double)currentTemp;
			}
			break;
#endif
	}
	imgcam_get_tecp()->tecerr = ((retval == 1) ? 0 : 1);
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		printf("%s\n", get_core_msg());
		strcpy(cammsg, get_core_msg());
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
		case 5:
		case 52:
		case 53:
		case 6:
		case 60:
		case 7:
		case 80:
		case 81:
		case 11:
		case 10:
		case 12:
		case 91:
		case 92:
			break;
		case 9:
		//case 91: //We need to understand why is shutter command not working.
			cammsg[0] = '\0';
			retval = qhy_Shutter(cmd);
			break;
		case 1000:
			break;
#ifdef HAVE_SBIG
		case 2000:
			retval = (sbig_Shutter(cmd) == 0);
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			//Atik camera
			retval = atik_camera_setShutter(cmd);
#endif
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
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
		case 10:
		case 12:
			break;
		case 53:
		case 92:
			cammsg[0] = '\0';
			retval = qhy_setColorWheel(pos+48);
			break;
		case 9:
			cammsg[0] = '\0';
			retval = qhy9_setColorWheel(pos);
			break;
		case 7:
		case 91:
		case 11:
			cammsg[0] = '\0';
			retval = qhy_setColorWheel(pos);
			break;
		case 1000:
			break;
#ifdef HAVE_SBIG
		case 2000:
			// Sbig
			retval = (sbig_CfwGoto(pos+1) == 0);
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			//Atik camera
			retval = atik_camera_setFilter((unsigned int) pos);
			break;
#endif
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
	}
	return (retval);
}

int imgcam_wheel_reset()
{
	int retval = 1;
	
	cammsg[0] = '\0';
	switch (camid)
	{
#ifdef HAVE_SBIG
		case 2000:
			// Sbig
			retval = (sbig_CfwReset() == 0);
			break;
#endif
		default:
			// QHY has no reset feature when driven through camera
			break;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
	}
	return (retval);
}

int imgcam_wheel_getstatus(int *status)
{
	int retval = 1;
#ifdef HAVE_SBIG
	int position = -1;
#endif
	
	cammsg[0] = '\0';
	switch (camid)
	{
#ifdef HAVE_SBIG
		case 2000:
			// Sbig
			if (sbig_CfwQueryStatus(status, &position) == 0)
			{
				retval = 1;
				*status -= 1;
			}
			else
			{
				retval = 0;
				*status = -1;
			}
			break;
#endif
#ifdef HAVE_ATIK
		case 3000:
			// Atik
			retval = atik_camera_getFilterWheelStatus(NULL, status, NULL, NULL);
			break;
#endif
		default:
			// QHY has no query feature when driven through camera
			*status = 0; //Idle
			break;
	}
	if ((retval == 0) && (strlen(cammsg) == 0))
	{
		strcpy(cammsg, get_core_msg());
	}
	return (retval);
}

int imgcam_guide(enum GuiderAxis axis, enum GuiderMovement movement)
{
	int retval = 0;
	switch (camid)
	{
		case 52:
			retval = qhy5lii_guide(axis, movement);
			break;
		case 53:
			retval = minicam5_guide(axis, movement);
			break;
		default:
			retval = 0;
			break;
	}
  return (retval);
}



