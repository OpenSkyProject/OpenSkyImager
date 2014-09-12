/*
 * urvccore.c
 *
 *  Created on: 02.09.2014
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
#ifdef HAVE_URVC

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "urvc.h"
#include "imgCamio.h"
#include "urvccore.h"

//=============================================================================
// SBIG temperature constants:
static const double		T0 					= 25.000;
static const double    	MAX_AD 				= 4096.000;
static const double    	R_RATIO_CCD 			= 2.570;
static const double    	R_BRIDGE_CCD			= 10.000;
static const double    	DT_CCD 				= 25.000;
static const double    	R0 					= 3.000;
static const double    	R_RATIO_AMBIENT		= 7.791;
static const double    	R_BRIDGE_AMBIENT		= 3.000;
static const double    	DT_AMBIENT 			= 45.000;
//=============================================================================
typedef enum
{
	CCD_THERMISTOR,
	AMBIENT_THERMISTOR
} THERMISTOR_TYPE;

#define DEFAULT_CAMERA  ST8_CAMERA	// in case geteeprom fails
//char *camModel, *ccdType, *serialNumber;
static int activeCcd = 0; //0 = Main, 1 = Tracking
CAMERA *urvcCam;

static char coremsg[128];
// Camera details
static urvc_list		m_camera_list;
static urvc_camdetails	m_camera_details;

//==========================================================================
static int   			IsCamera(int devId, int *camId, char *camName);
static void 			GetCameraDetails(int devId);
static float 			bcd2float (unsigned short bcd);
static int 			IsExposureComplete(int *complete);
static unsigned short 	CalcSetpoint(double temperature);
static double			CalcTemperature(short thermistorType, short ccdSetpoint);

//==========================================================================
void urvc_core_init()
{
	static int first_time = 1;
	coremsg[0] = '\0';
	if (first_time)
	{
		printf ("URVC2 driver, custom version available\n");
		first_time = 0;
	}
} 
//==========================================================================
char *urvc_GetErrorString()
{
 	return coremsg;
}
//==========================================================================
int urvc_core_reload_list()
{	
	int  ret = 0;
	int  devId;
	char tmpname[64], camName[64];
	int camId;
	
	tmpname[0] = '\0'; 
	m_camera_list.camlist[0] = '\0';
	m_camera_list.camnum = 0;
	for (devId = 0; devId < 3; devId++)
	{
		//printf("IsCamera %d\n", devId);
		if (IsCamera(devId, &camId, camName))
		{
			//List
			m_camera_list.camnum++;
			sprintf(tmpname, "%s lpt%d", camName, devId+1);
			strcat(m_camera_list.camlist, "|");
			strcat(m_camera_list.camlist, tmpname);
			// List details
			m_camera_list.listinfo[devId].camportId = devId;
			m_camera_list.listinfo[devId].camId = camId;
			sprintf(m_camera_list.listinfo[devId].camport, "lpt%d", devId+1);
			strcpy(m_camera_list.listinfo[devId].camName, camName);
		}
	}
	ret = (m_camera_list.camnum > 0);
	return (ret);
}	
//==========================================================================
int urvc_OpenCamera(const char *devName)
{
	short base;
	int i, devId = -1, ret = CE_NO_ERROR;

	coremsg[0] = '\0';
	for (i = 0; i < m_camera_list.camnum; i++)
	{
		if (strcmp(m_camera_list.listinfo[i].camport, devName) == 0)
		{
			devId = m_camera_list.listinfo[i].camportId;
			break;
		}
	}
	if (devId != -1)
	{
		base = getbaseaddr (devId);		 // sbig_port...?
		// This is a standard driver init procedure
		if (begin_realtime () == 0)
		{
			CameraInit (base);
			if ((ret = OpenCCD (activeCcd, &urvcCam)) == CE_NO_ERROR)
			{	
				//cameraID =  urvcCam->cameraID;
				// Load alla data
				GetCameraDetails(devId);
			}
			else
			{
				sprintf(coremsg, "CCD open failed (%d)", ret);
			}
		}
		else
		{
			sprintf(coremsg, "begin_realtime failed, see command line output");
			ret = -1;
		}
	}
	else
	{
		sprintf(coremsg, "Unknown port");
	}
	return (ret == CE_NO_ERROR);
}
//==========================================================================
void urvc_CloseCamera()
{
	end_realtime();
	CloseCCD(urvcCam);
}
//==========================================================================
int urvc_StartExposure(qhy_exposure *expar)
{
	int expTime = expar->time;
	int expMode = expar->mode;
	int ret = CCDExpose (urvcCam, (int) (expTime / 10), (expMode) ? 2 : 1);

	expar->wtime  = expar->time;
	expar->width  = m_camera_details.ccdActiveW / expar->bin;
	expar->height = m_camera_details.ccdActiveH / expar->bin;
	expar->tsize  = (expar->width * expar->height * expar->bytepix);
	expar->totsize = expar->tsize;
	
	coremsg[0] = '\0';
	if (ret)
	{
		sprintf(coremsg, "CCD expose failed (%d)", ret);
	}
	return (ret == 0);
}
//==========================================================================
int urvc_KillExposure()
{
	int ret = 0;
	EndExposureParams eep;
	eep.ccd = m_camera_details.activeCcd;
	
	coremsg[0] = '\0';
	ret = MicroCommand (MC_END_EXPOSURE, urvcCam->cameraID, &eep, NULL);
	if ((ret != CE_NO_ERROR) && (ret != CE_NO_EXPOSURE_IN_PROGRESS))
	{
		sprintf(coremsg, "MC end exposure failed (%d)", ret);
	}
	return ((ret == CE_NO_ERROR) || (ret == CE_NO_EXPOSURE_IN_PROGRESS));
}
//==========================================================================
int urvc_Readout(qhy_exposure *expar, unsigned char *databuffer)
{
	int ret = 0, expComp = 0;
	unsigned short *dbuffer = (unsigned short *) databuffer;
	
	coremsg[0] = '\0';
	// Check and wait for exposure to complete
	while ((ret = IsExposureComplete (&expComp)) == CE_NO_ERROR && !expComp)
	{
		usleep(100000);
	}
	// tx & ty are relative to full active frame in bin mode
	ret = CCDReadout(dbuffer, urvcCam, 0, 0, expar->width, expar->height, expar->bin);
	if (ret)
	{
		sprintf(coremsg, "CCD readout failed (%d)", ret);
	}
	return (ret == CE_NO_ERROR);
}
//==========================================================================
int urvc_QueryTemperatureStatus(int *enabled, double *ccdTemp, double *setpointTemp, int *power)
{
	QueryTemperatureStatusResults qtsr;
	int ret = 0;

	coremsg[0] = '\0';
	if ((ret = MicroCommand (MC_TEMP_STATUS, urvcCam->cameraID, NULL, &qtsr)) == CE_NO_ERROR)
	{
		*enabled      = qtsr.enabled;
		*ccdTemp      = CalcTemperature(CCD_THERMISTOR, qtsr.ccdThermistor);
		*setpointTemp = CalcTemperature(CCD_THERMISTOR, qtsr.ccdSetpoint);
		*power        = qtsr.power;
	}
	else
	{
		sprintf(coremsg, "MC temp status failed (%d)", ret);
	}
	return (ret == CE_NO_ERROR);
}
//==========================================================================
int urvc_SetTemperatureRegulation (int enable, double setpt)
{
	MicroTemperatureRegulationParams cool;
	int ret = 0;

	coremsg[0] = '\0';
	cool.regulation = enable;
	cool.ccdSetpoint = (enable != 2) ? CalcSetpoint(setpt) : setpt;
	cool.preload = (enable == 1) ? 0xaf : 0;

	ret = MicroCommand (MC_REGULATE_TEMP, urvcCam->cameraID, &cool, NULL) ;
	if (ret)
	{
		sprintf(coremsg, "MC temp regulation failed (%d)", ret);
	}
	return (ret == CE_NO_ERROR);
}
//==========================================================================
urvc_list	*urvc_GetCameraList()
{
	return &m_camera_list;
}
//==========================================================================
urvc_camdetails *urvc_GetCameraDetails()
{
	return &m_camera_details;
}
//==========================================================================
static int IsCamera(int devId, int *camId, char *camName)
{
	short base = getbaseaddr (devId);
	int ret = 0;
	GetVersionResults gvr;
	EEPROMContents eePtr;

	*camId = DEFAULT_CAMERA;
	camName[0] = '\0';
	//printf("devId %d, baseAddress %d\n", devId, base);
	if (base > 0)
	{
		if (begin_realtime () == 0)
		{
			CameraInit (base);

			if ((ret = MicroCommand (MC_GET_VERSION, DEFAULT_CAMERA, NULL, &gvr)) == CE_NO_ERROR)
			{
				// To be double sure we try read eeprom, it will mostly give same result unless camera has been upgraded
				if ((ret = GetEEPROM (gvr.cameraID, &eePtr)))
				{
					// Failure, use get version data
					if (camId != NULL)
					{
						*camId = gvr.cameraID;
					}
					if (camName != NULL)
					{
						sprintf(camName, "PSBIG %s", Cams[gvr.cameraID].fullName);
					}
				}
				else
				{
					if (camId != NULL)
					{
						*camId = eePtr.model;
					}
					if (camName != NULL)
					{
						sprintf(camName, "PSBIG %s", Cams[eePtr.model].fullName);
					}
				}
				ret = 1;	
			}
			end_realtime();
		}
		else
		{
			sprintf(coremsg, "begin_realtime failed, see command line output");
		}
	}
	return (ret);	
}
//==========================================================================
static void GetCameraDetails(int devId)
{
	m_camera_details.camType   = urvcCam->cameraID;
	m_camera_details.camDevice = devId;
	sprintf(m_camera_details.camDevName, "lpt%d", devId+1);
	strcpy(m_camera_details.camName, (char *) Cams[urvcCam->cameraID].fullName);
	strcpy(m_camera_details.camSerial, (char *) urvcCam->serialNumber);
	m_camera_details.camFirmware = bcd2float(urvcCam->firmwareVersion);
	m_camera_details.activeCcd = activeCcd;
	m_camera_details.numCcd = (Cams[urvcCam->cameraID].hasTrack) ? 2 : 1;
	strcpy(m_camera_details.binList, "1x1|2x2|3x3:0");
	m_camera_details.binModes = 3;
	m_camera_details.pixDepth = 16;
	m_camera_details.camTec = 1;
	m_camera_details.camShutter = 1;
	m_camera_details.minExp = 120;
	m_camera_details.cfwType = 6; //CFWSEL_AUTO
	strcpy(m_camera_details.ampList, "0-AmpOff|1-AmpOn:0");
	m_camera_details.spdList[0] = '\0';
	strcpy(m_camera_details.modList, "0-Light|1-Dark:0");
	m_camera_details.cfwList[0] = '\0';
	m_camera_details.ccdpixW = Cams[urvcCam->cameraID].pixelX / 100;
	m_camera_details.ccdpixH = Cams[urvcCam->cameraID].pixelX / 100;
	m_camera_details.ccdActiveW = urvcCam->horzImage;
	m_camera_details.ccdActiveH = urvcCam->vertImage;
		
	switch(urvcCam->cameraID)
	{
		case ST5C_CAMERA:
			strcpy(m_camera_details.binList, "1x1|2x2:0");
			m_camera_details.binModes = 2;
			m_camera_details.minExp = 100;
			m_camera_details.ampList[0] = '\0';
			m_camera_details.modList[0] = '\0';
			break;
			
		case ST237_CAMERA:
			m_camera_details.pixDepth = 12;
		case ST237A_CAMERA:
			m_camera_details.minExp = 10;
			m_camera_details.ampList[0] = '\0';
			m_camera_details.modList[0] = '\0';
			break;
		
		case ST7_CAMERA:
			break;
			
		case ST8_CAMERA:
			break;

		case ST9_CAMERA:
			break;

		case ST10_CAMERA:
			break;

		case ST1K_CAMERA:
			break;
	}
}
//==========================================================================
static int IsExposureComplete(int *complete)
{
	int ret = 0;
	
	if ((ret = CCDImagingState (urvcCam)) != -1)
	{
		*complete = (ret == 0);
	}
	return (ret != -1);	
}
//==============================================================
static float bcd2float (unsigned short bcd)
{  
	unsigned char b1 = bcd >> 8;	  
	unsigned char b2 = bcd;
	 
	float f1 = 10 * (b1 >> 4);
	float f2 = b1 & 0x0F;
	float f3 = 0.10 * (b2 >> 4);
	float f4 = 0.01 * (b2 & 0x0F);
	return (f1 + f2 + f3 + f4);
}
//==========================================================================
static unsigned short CalcSetpoint(double temperature)
{
 	// Calculate 'setpoint' from the temperature T in degr. of Celsius.
 	double expo = (log(R_RATIO_CCD) * (T0 - temperature)) / DT_CCD;
 	double r    = R0 * exp(expo);
 	return((unsigned short)(((MAX_AD / (R_BRIDGE_CCD / r + 1.0)) + 0.5)));
}
//==========================================================================
static double CalcTemperature(short thermistorType, short setpoint)
{
 	double r, expo, rBridge, rRatio, dt;

 	switch(thermistorType)
 	{
		case 	AMBIENT_THERMISTOR:
		rBridge = R_BRIDGE_AMBIENT;
		rRatio  = R_RATIO_AMBIENT;
		dt      = DT_AMBIENT;
		break;
   	case CCD_THERMISTOR:
   	default:
		rBridge = R_BRIDGE_CCD;
		rRatio  = R_RATIO_CCD;
		dt      = DT_CCD;
		break;
 	}

 	// Calculate temperature T in degr. Celsius from the 'setpoint'
 	r = rBridge / ((MAX_AD / setpoint) - 1.0);
 	expo = log(r / R0) / log(rRatio);
 	return(T0 - dt * expo);
}
//==========================================================================

#endif // HAVE_URVC

