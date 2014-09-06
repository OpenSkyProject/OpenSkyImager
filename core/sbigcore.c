/*
 * sbigcore.c
 *
 *  Created on: 05.05.2014
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 *  This is a derived work from sbigcam.c, released under GPL v2 as part of
 *  indilib. Indilib is not used anymore.
 *  Original author of device access code:
 *  Copyright (C) 2005-2006 Jan Soldan (jsoldan AT asu DOT cas DOT cz)
 *  251 65 Ondrejov-236, Czech Republic
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

//==========================================================================
#ifdef HAVE_SBIG

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <linux/limits.h>
#include <glib/gi18n.h>
#include <sbigudrv.h>
#include "imgCamio.h"
#include "sbigcore.h"

// InitVars sections
#define VARCAM 1
#define VARDRV 2
#define VARALL	4
#define VARLST 256

// Cozy
#define CHECK_BIT(var,pos) ((var & (1 << pos)) != 0)

//=============================================================================
const int		INVALID_HANDLE_VALUE	= -1;	// for file operations
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

// protected
// Basic driver
static int			m_fd 		= -1;
static int			m_drv_handle 	= -1; //INVALID_HANDLE_VALUE; 
static float			m_drv_version	= 0.;
static int			m_link_status 	= 0;
static int			m_dev_type	= DEV_NONE;
static int			m_dev_ip		= 0;
static char			m_dev_name[16];
static char 			coremsg[128];
static char 			basePath[PATH_MAX];
// Camera details
static sbig_list		m_camera_list;
static sbig_camdetails	m_camera_details;
//==========================================================================
static inline int		IsDeviceOpen(){return((m_fd == -1) ? 0 : 1);}
//
static int 			SetDriverHandle(SetDriverHandleParams *sdhp);
static int			GetDriverHandle(GetDriverHandleResults *gdhr);
//
static int 			UnivDrvCommand(PAR_COMMAND command, void *params, void *results);
static void			InitVars(int section);
static char 		    *GetErrorString(int err);
static void 		     SetErrorString(int err);
static int			OpenDriver();
static int			CloseDriver();
static int 			GetDriverVersion();
static int 			my_ethernet_query(char *list, int *cams);
static int 			OpenDevice(int devId, int devIp);
//static int 			GetLinkStatus(int *linkstat);
static int 			GetCameraList();
static int 			GetCameraDetails();
static int 			GetNumOfCcdChips();
static int 			QueryCommandStatus(PAR_COMMAND cmd, unsigned short *status);
static int 			EndExposure();
static int 			IsExposureComplete(int *complete);
static unsigned short 	CalcSetpoint(double temperature);
static double			CalcTemperature(short thermistorType, short ccdSetpoint);
static double  		BcdPixel2double(ulong bcd);
static float			bcd2float(unsigned short bcd);

//==========================================================================
int sbig_core_init(char *path)
{	
	int res;
	static int first_time = 1;

	InitVars(VARLST);
	InitVars(VARALL);
	strcpy(basePath, path);
	if ((res = OpenDriver()) == CE_NO_ERROR)
	{
		if ((res = GetDriverVersion()) == CE_NO_ERROR)
		{
			if (first_time)
			{
	 			printf ("SBIG driver Version %1.2lf\n", m_drv_version);
	 			first_time = 0;
	 		}
			// Load complete list of available camera
			//res = GetCameraList();
		}
	}
	else
	{
		SetErrorString(res);
	}
 	return(res);
}
//==========================================================================
int sbig_core_reload_list()
{	
	return (GetCameraList());
}	
//==========================================================================
int sbig_core_close()
{
	int res = CE_NO_ERROR;
	
	if(IsDeviceOpen()) 
	{
		if ((res = sbig_CloseDevice()) != CE_NO_ERROR)
		{
			SetErrorString(res);
		}
	}
	if(res == CE_NO_ERROR)
	{
		if ((res = CloseDriver()) == CE_NO_ERROR)
		{
			InitVars(VARDRV);
		}
		else
		{
			SetErrorString(res);
		}
	}
 	return(res);
}
//==========================================================================
sbig_list	*sbig_GetCameraList()
{
	return &m_camera_list;
}
//==========================================================================
int sbig_OpenDevice(const char *devName)
{
	int i, res = CE_BAD_PARAMETER;

	// Check if device already opened:
	if(IsDeviceOpen()) return(CE_NO_ERROR);
	
	for (i = 0; i < m_camera_list.camnum; i++)
	{
		if (strcmp(m_camera_list.listinfo[i].camport, devName) == 0)
		{
			if ((res = OpenDevice(m_camera_list.listinfo[i].camportId, m_camera_list.listinfo[i].camIp)) == CE_NO_ERROR)
			{
				strcpy(m_dev_name, devName);
			}
			else
			{
				SetErrorString(res);
			}
			break;
		}
	}
 	return(res);
}
//==========================================================================
int sbig_CloseDevice()
{	
	int res = CE_NO_ERROR;

 	if(IsDeviceOpen())
 	{	
 		if (m_link_status == 1)
 		{
 			// Close CFW
			CFWParams  	cfwp;
			CFWResults	cfwr;

			cfwp.cfwModel   = m_camera_details.cfwType;
			cfwp.cfwCommand = CFWC_CLOSE_DEVICE;
			UnivDrvCommand(CC_CFW, &cfwp, &cfwr);
 		}
 		if((res = UnivDrvCommand(CC_CLOSE_DEVICE, 0, 0)) == CE_NO_ERROR)
 		{
			InitVars(VARCAM);
		}
		else
		{
			SetErrorString(res);
		}
	}
 	return(res);
}
//==========================================================================
char	 *sbig_GetDeviceName()
{
	return(m_dev_name);
}
//==========================================================================
int sbig_EstablishLink()
{
	int 					res = CE_NO_ERROR;
 	EstablishLinkParams  	elp;
 	EstablishLinkResults 	elr;

 	elp.sbigUseOnly = 0;
 	if((res = UnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr)) == CE_NO_ERROR)
 	{
		//printf("Link established\n");
	    	/*if ((res = GetLinkStatus(&m_link_status)) == CE_NO_ERROR)
	    	{
	    		res = GetCameraDetails();
	    	}*/
	    	m_link_status = 1;
	    	res = GetCameraDetails();
 	}
	else
	{
		SetErrorString(res);
	}
 	return(res);
}
//==========================================================================
sbig_camdetails *sbig_GetCameraDetails()
{
	return &m_camera_details;
}
//==========================================================================
void sbig_GetFrameSize(int modeId, int *mode, int *width, int *height)
{
	int i;

	for (i = 0; i < m_camera_details.binModes; i++)
	{
		if (m_camera_details.frameInfo[i].modeId == modeId)
		{
			*mode   = m_camera_details.frameInfo[i].mode;
			*width  = m_camera_details.frameInfo[i].width;
			*height = m_camera_details.frameInfo[i].height;
			break;
		}
	}
}
//==========================================================================
int sbig_StartExposure(qhy_exposure *expar)
{
	int ret = CE_DEVICE_NOT_OPEN;
	StartExposureParams  sep;
	StartExposureParams2 sep2;
	unsigned int ulExposure;
	unsigned int	shutterState;
	unsigned int ccd;
	int readoutMode, top, left, width, height;
	int exptime = expar->time;	//In millisecond
	int speed   = expar->speed; 
	int bin     = expar->bin;
	int mode    = expar->mode;	//Light - Dark
	int amp     = expar->amp;
	int iWidth  = expar->width; 
	int iHeight = expar->height; 
	
	expar->wtime = expar->time;
		
	if (sbig_CheckLink())
	{
		// end any exposure in case one in progress
		ret = EndExposure();

		if ((ret == CE_NO_ERROR) || (ret == CE_NO_EXPOSURE_IN_PROGRESS))
		{
			// For exposures above 255 millisecond driver will use nearest 
			// 1/100th second on it's own but we need to record for image header
			// Same if exptime is below minimim ;-)
			exptime = (exptime < m_camera_details.minExp) ? m_camera_details.minExp : ((exptime < 1) ? 1 : exptime);
			if (m_camera_details.camShutter != 0)
			{
				// For other models we basically set the shutter to ignore
				if (mode == 1)
				{
					shutterState = SC_CLOSE_SHUTTER;
				}
				else
				{
					shutterState = SC_OPEN_SHUTTER;
				}
			}
			else
			{
				shutterState = SC_LEAVE_SHUTTER;
			}
		
			if (exptime < 255)
			{
				ulExposure = (unsigned int)exptime;
				ulExposure |= EXP_MS_EXPOSURE;
			}
			else
			{
				ulExposure = (unsigned int) (exptime / 10.0 + 0.5);
				if (ulExposure < 1)
				{
					ulExposure = 1;
				}
				exptime = ulExposure * 10;
			}

			if (m_camera_details.spdList[0] != '\0')
			{
				// Camera has download speed setting
				if (speed)
				{
					ulExposure |= EXP_FAST_READOUT;
				}
				else
				{
					ulExposure &= ~EXP_FAST_READOUT;
				}
			}
			ccd = (unsigned int)m_camera_details.activeCcd;
			if (m_camera_details.ampList[0] != '\0')
			{
				if ((amp == 1) || ((amp == 2) && (exptime < 550)))
				{
					ccd |= START_SKIP_VDD;
				}
				else
				{
					ccd &= ~START_SKIP_VDD;
				}
				if (exptime < m_camera_details.minExp)
				{
					ccd |= END_SKIP_DELAY;
				}
			}
			
			// Read frame geometry from bin mode
			sbig_GetFrameSize(bin, &readoutMode, &width, &height);
			// Need to derive these from subframe (we allow only centered)
			top  = 0;
			left = 0;
			if ((iHeight != 0) && (iHeight < height))
			{
				top  = (height - iHeight) / 2;
			}
			else
			{
				// No ROI or not valid
				iHeight = height;
			}
			if ((iWidth != 0) && (iWidth < width)) 
			{
				left = (width - iWidth) / 2;
			}
			else
			{
				// No ROI or not valid
				iWidth = width;
			}
			expar->tsize = (iWidth * iHeight * 2);
			expar->totsize = (iWidth * iHeight * 2);
			if (m_drv_version < 4.69)
			{
				// StartExposure param
				sep.ccd = ccd;
				sep.exposureTime = ulExposure;
				sep.abgState = ABG_CLK_MED7;
				sep.openShutter = shutterState;
				// StartExposure
		 		if ((ret = UnivDrvCommand(CC_START_EXPOSURE, &sep, NULL)) != CE_NO_ERROR)
				{
					SetErrorString(ret);
				}
			}
			else
			{
				// StartExposure2 param
				sep2.ccd          = ccd;
				sep2.exposureTime = ulExposure;
				sep2.abgState     = ABG_CLK_MED7;
				sep2.openShutter  = shutterState;
				sep2.top          = top;
				sep2.left         = left;
				sep2.height       = iHeight;
				sep2.width        = iWidth;
				sep2.readoutMode  = (unsigned int)readoutMode;
				// StartExposure2
		 		if ((ret = UnivDrvCommand(CC_START_EXPOSURE2, &sep2, NULL)) != CE_NO_ERROR)
				{
					SetErrorString(ret);
				}
			}
			//printf("Start exposure state: %d, %s\n", ret, GetErrorString(ret));
	 	}
		else
		{
			SetErrorString(ret);
		}
	}
 	return(ret);
}
//==========================================================================
int sbig_Readout(qhy_exposure *expar, unsigned char *databuffer)
{
	int res = CE_DEVICE_NOT_OPEN;
	StartReadoutParams srp;
	ReadoutLineParams  rlp;
	EndReadoutParams   erp;
	int readoutMode, top, left, width, height;
	int i, expComp = 0;
	unsigned short *rowbuffer;
	int bin     = expar->bin;
	int iWidth  = expar->width; 
	int iHeight = expar->height; 	

	if (sbig_CheckLink())
	{
		// Check and wait for exposure to complete
      	while ((res = IsExposureComplete (&expComp)) == CE_NO_ERROR && !expComp);
		// end exposure
		res = EndExposure();

		if ((res == CE_NO_ERROR) || (res == CE_NO_EXPOSURE_IN_PROGRESS))
		{
			// Read frame geometry from bin mode
			sbig_GetFrameSize(bin, &readoutMode, &width, &height);
			// Need to derive these from subframe (we allow only centered)
			top  = 0;
			left = 0;
			if ((iHeight != 0) && (iHeight < height))
			{
				top  = (height - iHeight) / 2;
			}
			else
			{
				// No ROI or not valid
				expar->height = height;
			}
			if ((iWidth != 0) && (iWidth < width)) 
			{
				left = (width - iWidth) / 2;
			}
			else
			{
				// No ROI or not valid
				expar->width = width;
			}
			// readout the CCD param
			srp.ccd         = m_camera_details.activeCcd;
			srp.left        = left;
			srp.top         = top;
			srp.height      = expar->height;
			srp.width       = expar->width;
			srp.readoutMode = readoutMode;
			//printf("srp: ccd: %d, mode: %d, top: %d, left: %d, width: %d, height: %d\n", srp.ccd, srp.readoutMode, srp.top, srp.left, srp.width, srp.height);
			// Start readout the CCD
			if ((res = UnivDrvCommand(CC_START_READOUT, &srp, NULL)) == CE_NO_ERROR)
			{
				rowbuffer = (unsigned short*)malloc(width * sizeof(unsigned short));
				rlp.ccd = m_camera_details.activeCcd;
				rlp.pixelStart = left;
				rlp.pixelLength = expar->width;
				rlp.readoutMode = readoutMode;
				for (i = 0; i < expar->height; i++)
				{
					if ((res = UnivDrvCommand(CC_READOUT_LINE, &rlp, rowbuffer)) == CE_NO_ERROR)
					{
						memcpy(databuffer + (i * expar->width * 2), rowbuffer, expar->width * 2);
					}
					else
					{
						SetErrorString(res);
						break;
					}
				}
				free(rowbuffer);
			}
			else
			{
				SetErrorString(res);
			}
			// End readout
			erp.ccd = m_camera_details.activeCcd;					
			res = UnivDrvCommand (CC_END_READOUT, &erp, NULL);
		}
		else
		{
			SetErrorString(res);
		}
	}
	//printf("Readout state %d, %s\n", res, GetErrorString(res));
 	return(res);
}
//==========================================================================
int 	sbig_KillExposure()
{
	int res;
	 
	if ((res = EndExposure()) != CE_NO_ERROR)
	{
		SetErrorString(res);
	}
	return res;
}
//==========================================================================
int sbig_Shutter(int cmd) //0 Open, 1 Close
{
	int res = CE_NO_ERROR, value, complete;
	MiscellaneousControlParams mcp;
	unsigned short status;
	
	if ((cmd > -1) && (cmd < 2) && (m_camera_details.camShutter) > 0)
	{
		complete = cmd;
		// Translate into sbig commands appropriate to camera models
		if (m_camera_details.camType == STL_CAMERA)
		{
			value = cmd + (m_camera_details.activeCcd == CCD_EXT_TRACKING) ? 4 : 1;
		}
		else
		{
			value = cmd + 1;
		}
		mcp.shutterCommand = value;
		if ((res = UnivDrvCommand(CC_MISCELLANEOUS_CONTROL, &mcp, NULL)) == CE_NO_ERROR)
		{
			//printf("command sent\n");
			usleep(m_camera_details.minExp);
			//while ((res = QueryCommandStatus(CC_MISCELLANEOUS_CONTROL, &status)) == CE_NO_ERROR)
			while (QueryCommandStatus(CC_MISCELLANEOUS_CONTROL, &status) == CE_NO_ERROR)
			{
				if (CHECK_BIT(status, 10) == complete)
				{
					//printf("command sent\n");
					break;
				}
			}
		}
		else
		{
			SetErrorString(res);
		}
	}
	//printf("Shutter cmd %d/%d, state %d, %s\n", value, complete, res, GetErrorString(res));
	return (res);
}
//==========================================================================
int sbig_SetTemperatureRegulation(int enable, double temperature)
{
	int res;
	//if (m_drv_version < 4.65)
	//{
	 	SetTemperatureRegulationParams strp;

	 	if(sbig_CheckLink())
	 	{
		    	strp.regulation  = enable;
		    	// This is for manual overraid proper operation
		    	strp.ccdSetpoint = (enable != 2) ? CalcSetpoint(temperature) : temperature;
		    	if ((res = UnivDrvCommand(CC_SET_TEMPERATURE_REGULATION, &strp, 0)) != CE_NO_ERROR)
			{
				SetErrorString(res);
			}
		}
		else
		{
			res = CE_DEVICE_NOT_OPEN;
			SetErrorString(res);
		}
	/*}
	else
	{
	 	SetTemperatureRegulationParams2 strp;

	 	if(sbig_CheckLink())
	 	{
		    	strp.regulation  = enable;
		    	strp.ccdSetpoint = temperature;
		    	if ((res = UnivDrvCommand(CC_SET_TEMPERATURE_REGULATION2, &strp, 0)) != CE_NO_ERROR)
			{
				SetErrorString(res);
			}
		}
		else
		{
			res = CE_DEVICE_NOT_OPEN;
			SetErrorString(res);
		}
	}*/
	//printf("SetTemperatureRegulation status: %d, %s\n", res, GetErrorString(res));
 	return(res);
}
//==========================================================================
int sbig_QueryTemperatureStatus(int *enabled, double *ccdTemp, double *setpointTemp, int *power)
{
	int res;
	//if (m_drv_version < 4.65)
	//{
		QueryTemperatureStatusParams  qtrq;
	 	QueryTemperatureStatusResults qtsr;

	 	if(sbig_CheckLink())
	 	{
	 		qtrq.request = TEMP_STATUS_STANDARD;
			res = UnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, &qtrq, &qtsr);
			if(res == CE_NO_ERROR)
			{
				*enabled      = qtsr.enabled;
				*ccdTemp      = CalcTemperature(CCD_THERMISTOR, qtsr.ccdThermistor);
				*setpointTemp = CalcTemperature(CCD_THERMISTOR, qtsr.ccdSetpoint);
				*power        = qtsr.power;
			}
			else
			{
				SetErrorString(res);
			}
	 	}
	 	else
	 	{
			res = CE_DEVICE_NOT_OPEN;
			SetErrorString(res);
		}
	/*}
	else
	{
		QueryTemperatureStatusParams   qtrq;
	 	QueryTemperatureStatusResults2 qtsr;

	 	if(sbig_CheckLink())
	 	{
	 		qtrq.request = TEMP_STATUS_ADVANCED2;
			if ((res = UnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, &qtrq, &qtsr)) == CE_NO_ERROR)
			{
				*enabled      = qtsr.coolingEnabled;
				if (m_camera_details.activeCcd == CCD_IMAGING)
				{
					*setpointTemp = qtsr.ccdSetpoint;
					*ccdTemp      = qtsr.imagingCCDTemperature;
					*power        = qtsr.imagingCCDPower;
				}
				else if (m_camera_details.activeCcd == CCD_TRACKING)
				{
					*setpointTemp = qtsr.trackingCCDSetpoint;
					*ccdTemp      = qtsr.trackingCCDTemperature;
					*power        = qtsr.trackingCCDPower;
				}
				else if (m_camera_details.activeCcd == CCD_EXT_TRACKING)
				{
					*setpointTemp = qtsr.trackingCCDSetpoint;
					*ccdTemp      = qtsr.externalTrackingCCDTemperature;
					*power        = qtsr.externalTrackingCCDPower;
				}
			}
			else
			{
				SetErrorString(res);
			}
	 	}
	 	else
	 	{
			res = CE_DEVICE_NOT_OPEN;
			SetErrorString(res);
		}
	}*/
	//printf("QueryTemperatureStatus status: %d, %s\n", res, GetErrorString(res));	
 	return(res);
}
//==========================================================================
int sbig_CfwGoto(int position)
{
	int 			res;
	CFWParams  	cfwp;
	CFWResults	cfwr;

	cfwp.cfwModel   = m_camera_details.cfwType;
	cfwp.cfwCommand = CFWC_GOTO;
	cfwp.cfwParam1  = position;
 	if ((res = UnivDrvCommand(CC_CFW, &cfwp, &cfwr)) == CE_CFW_ERROR)
	{
		sprintf(coremsg, C_("sbigcore", "CFW error: %d"), cfwr.cfwError);
	}
	return (res == CE_CFW_ERROR);
}
//==========================================================================
int sbig_CfwQueryStatus(int *status, int *position)
{
	int 			res;
	CFWParams  	cfwp;
	CFWResults	cfwr;

	cfwp.cfwModel   = m_camera_details.cfwType;
	cfwp.cfwCommand = CFWC_QUERY;
 	if ((res = UnivDrvCommand(CC_CFW, &cfwp, &cfwr)) != CE_CFW_ERROR)
 	{
 		*status = cfwr.cfwStatus;
 		if ((m_camera_details.cfwType != CFWSEL_CFW6A) && ((m_camera_details.cfwType != CFWSEL_CFW8)))
 		{
 			*position = cfwr.cfwPosition;
 		}
 	}
	else
	{
		sprintf(coremsg, C_("sbigcore", "CFW error: %d"), cfwr.cfwError);
	}
	return (res == CE_CFW_ERROR);
}
//==========================================================================
int sbig_CfwReset()
{
	int 			res;
	CFWParams  	cfwp;
	CFWResults	cfwr;

	cfwp.cfwModel   = m_camera_details.cfwType;
	cfwp.cfwCommand = CFWC_INIT;
 	if ((res = UnivDrvCommand(CC_CFW, &cfwp, &cfwr)) == CE_CFW_ERROR)
	{
		sprintf(coremsg, C_("sbigcore", "CFW error: %d"), cfwr.cfwError);
	}
	return (res == CE_CFW_ERROR);
}
//==========================================================================
int sbig_CheckLink()
{
 	return(m_camera_details.camType != NO_CAMERA && m_link_status == 1);
}	
//==========================================================================
char *sbig_GetErrorString()
{
 	return coremsg;
}
//==========================================================================
// Protected
//==========================================================================
// UnivDrvCommand:
// Bottleneck function for all calls to the driver that logs the command
// and error. First it activates our handle and then it calls the driver.
// Activating the handle first allows having multiple instances of this
// class dealing with multiple cameras on different communications port.
// Also allows direct access to the SBIG Universal Driver after the driver
// has been opened.
static int UnivDrvCommand(PAR_COMMAND command, void *params, void *results)
{	
	int 	res;
 	
 	// Make sure we have a valid handle to the driver.
 	if(m_drv_handle == INVALID_HANDLE_VALUE)
 	{
		res = CE_DRIVER_NOT_OPEN;
 	}
 	else
 	{
		//printf("uni try\n");
		res = SBIGUnivDrvCommand(command, params, results);
		//printf("uni ok\n");
 	}
 	return(res);
}
//==========================================================================
static void InitVars(int section)
{
	int i;
	
 	m_camera_details.camType = NO_CAMERA;

	if (section == VARLST)
	{
		m_camera_list.camlist[0]	= '\0';
		m_camera_list.camnum	= 0;
		for(i = 0; i < SBIGLISTINFOLEN; i++)
		{
			m_camera_list.listinfo[SBIGLISTINFOLEN].camportId 	= DEV_NONE;
			m_camera_list.listinfo[SBIGLISTINFOLEN].camport[0]	= '\0';
			m_camera_list.listinfo[SBIGLISTINFOLEN].camIp		= 0;
			m_camera_list.listinfo[SBIGLISTINFOLEN].camName[0]	= '\0';
			m_camera_list.listinfo[SBIGLISTINFOLEN].camId		= -1;
			m_camera_list.listinfo[SBIGLISTINFOLEN].camSerial[0]	= '\0';
		}
	}
	else
	{
		if (section >= VARDRV)
		{
			m_drv_handle = INVALID_HANDLE_VALUE;	
			coremsg[0] = '\0';
			basePath[0] = '\0';
		}
		if (section >= VARCAM)
		{
			// General flags
			m_fd 		= -1;
		 	m_link_status 	= 0;
		 	m_dev_type 	= DEV_NONE;
		 	m_dev_ip		= 0;
		 	m_dev_name[0]	= 0;
		 	// Camera details
			m_camera_details.camId 		= -1;
			m_camera_details.camType 	= NO_CAMERA;
			m_camera_details.camDevice 	= DEV_NONE;
			m_camera_details.camIp		= 0;
			m_camera_details.camName[0]	= '\0';
			m_camera_details.camSerial[0]	= '\0';
			m_camera_details.camFirmware	= 0.;
			m_camera_details.activeCcd 	= CCD_IMAGING;
			m_camera_details.numCcd		= 0;
			m_camera_details.binList[0]	= '\0';
			m_camera_details.binModes	= 0;
			m_camera_details.pixDepth	= 0;
			m_camera_details.ccdpixW		= 0.;
			m_camera_details.ccdpixH		= 0.;
			m_camera_details.colorId		= -1;
			m_camera_details.camShutter	= 0;
			m_camera_details.minExp		= 0;
			m_camera_details.cfwType		= CFWSEL_AUTO;
			m_camera_details.ampList[0]	= '\0';
			m_camera_details.spdList[0]	= '\0';
			m_camera_details.modList[0]	= '\0';
			m_camera_details.cfwList[0]	= '\0';
			for(i = 0; i < SBIGFRAMINFOLEN; i++)
			{
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].modeId	= 0;
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].mode		= 0;
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].width	= 0;
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].height 	= 0;
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].pixW		= 0.;
				m_camera_details.frameInfo[SBIGFRAMINFOLEN].pixH 	= 0.;
			}
		}
	}
}
//==========================================================================
static char *GetErrorString(int err)
{
	SetErrorString(err);
	return coremsg;
}
//==========================================================================
static void SetErrorString(int err)
{
	int res;
 	GetErrorStringParams		gesp;
 	GetErrorStringResults		gesr;
 
 	gesp.errorNo = err;
 	res = UnivDrvCommand(CC_GET_ERROR_STRING, &gesp, &gesr);
	if(res == CE_NO_ERROR) 
	{
		strcpy(coremsg, gesr.errorString);
	}
	else
	{
		sprintf(coremsg, C_("sbigcore", "No error string found! Error code: %d"), err);
	}
}
//==========================================================================
static int OpenDriver()
{
	int					res;
	GetDriverHandleResults 	gdhr;
 	SetDriverHandleParams  	sdhp;

 	// This is to prevent traps in sbig_* functions to prevent connection
 	m_drv_handle = 0;
 	// Call the driver directly.
 	if((res = UnivDrvCommand(CC_OPEN_DRIVER, 0, 0)) == CE_NO_ERROR)
 	{
	   	// The driver was not open, so record the driver handle.
	   	res = GetDriverHandle(&gdhr);
 	}
 	else if(res == CE_DRIVER_NOT_CLOSED)
 	{
		// The driver is already open which we interpret as having been
		// opened by another instance of the class so get the driver to 
		// allocate a new handle and then record it.
		sdhp.handle = INVALID_HANDLE_VALUE;
		res = SetDriverHandle(&sdhp);
		if(res == CE_NO_ERROR)
		{
			if((res = UnivDrvCommand(CC_OPEN_DRIVER, 0, 0)) == CE_NO_ERROR)
			{
				res = GetDriverHandle(&gdhr);
			}
		}
 	}
 	if (res != CE_NO_ERROR)
 	{
 		// Open failed
 		InitVars(VARDRV);
 	}
 	return(res);
}
//==========================================================================
static int CloseDriver()
{	
	int res;

	if((res = UnivDrvCommand(CC_CLOSE_DRIVER, 0, 0)) == CE_NO_ERROR)
	{
 		res = SetDriverHandle(NULL);
	}
 	return(res);
}
//=========================================================================
static int GetDriverVersion()
{
	int res;
	GetDriverInfoParams gdip;
	GetDriverInfoResults0 gdir;
	
	gdip.request = 0;
 	if ((res = UnivDrvCommand(CC_GET_DRIVER_INFO, &gdip, &gdir)) == CE_NO_ERROR)
 	{
 		m_drv_version = bcd2float(gdir.version);
 	}
 	return (res);
}
//==========================================================================
static int SetDriverHandle(SetDriverHandleParams *sdhp)
{
	int ret;
	
	if (sdhp != NULL)
	{
 		ret = UnivDrvCommand(CC_SET_DRIVER_HANDLE, sdhp, 0);
		m_drv_handle = (ret == CE_NO_ERROR) ? sdhp->handle : INVALID_HANDLE_VALUE;
 	}
 	else
 	{
 		m_drv_handle = INVALID_HANDLE_VALUE;
 		ret = CE_NO_ERROR;
 	}
 	return (ret);
}
//==========================================================================
static int GetDriverHandle(GetDriverHandleResults *gdhr)
{
	int ret;
	
	ret = UnivDrvCommand(CC_GET_DRIVER_HANDLE, 0, gdhr);
	m_drv_handle = (ret == CE_NO_ERROR) ? gdhr->handle : INVALID_HANDLE_VALUE;
	return (ret);
}
//==========================================================================
static int OpenDevice(int devId, int devIp)
{
	int res;
	OpenDeviceParams odp;

	// Check if device already opened:
	if(IsDeviceOpen()) return(CE_NO_ERROR);
	
	switch (devId)
	{
		case DEV_LPT1:
		case DEV_LPT2:
		case DEV_LPT3:
		case DEV_USB:
		case DEV_ETH:
		case DEV_USB1:
		case DEV_USB2:
		case DEV_USB3:
		case DEV_USB4:
			odp.deviceType 	= devId;
			odp.lptBaseAddress 	= 0;
			odp.ipAddress  	= devIp;
			break;
			
		default:
			return(CE_BAD_PARAMETER);
	}
	
 	if((res = UnivDrvCommand(CC_OPEN_DEVICE, &odp, NULL)) == CE_NO_ERROR)
 	{
		m_fd = 1;
		m_dev_type = devId;
		m_dev_ip   = devIp;
 	}
 	return(res);
}
//==========================================================================
/*int GetLinkStatus(int *linkstat)
{
	int ret;
	GetLinkStatusResults glsr;
	
	if ((ret = UnivDrvCommand(CC_GET_LINK_STATUS, &glsr, 0)) == CE_NO_ERROR)
	{
		*linkstat = (int)glsr.linkEstablished;
	}
	else
	{
		*linkstat = 0;
	}
	return (ret);
}*/	
//==========================================================================
static void _GetCameraName(GetCCDInfoResults0	*gcr, char *camname)
{
 	char *p1, *p2;
	char tmpname[64];
	
    	strcpy(tmpname, gcr->name);
    	// Camera type
    	m_camera_details.camType = gcr->cameraType;
    	switch(m_camera_details.camType)
    	{
    		case NO_CAMERA:
    		 	camname[0]='\0';
			break;
		
 		case	ST237_CAMERA:
	  	 	if(gcr->readoutInfo[0].gain >= 0x100)
	  	 	{
		  	 	sprintf(camname,"ST-237A");
	  	 	} 
	  	 	else
	  	 	{
		  	 	sprintf(camname,"ST237");
	  	 	}
    		 	break;
    		 	
  		default:
    		 	// driver reports name as "SBIG ST-L-XXX..."
    	 		p1 = tmpname + 5;
    	 		if((p2  = strchr(p1,' ')) != NULL)
    	 		{
  				*p2  = '\0';
    	 		}
			strcpy(camname, tmpname);
			if(strstr(gcr->name, "Color") != NULL)
			{
				strcat(camname, " Color");
			}
    		 	break;    		 	
    	}
}
//==========================================================================
static int GetCameraList()
{
	int  res, i, cams, usbcams, ethcams;
	char tmpname[64];
	char tmpnam1[64];
	char tmplist[1024];
	char *p1, *p2;
	QueryUSBResults 		qusbr;
	QueryEthernetResults	qethr;
	GetCCDInfoParams		gcp;
	GetCCDInfoResults0		gcr;
	//QUERY_USB_INFO		usbInfo;
	//QUERY_ETHERNET_INFO	ethinfo;
	
	gcp.request = 0;
	cams = 0;
	tmplist[0] = '\0';
	if (m_drv_handle != INVALID_HANDLE_VALUE)
	{
		//Query USB, then ETH, then try open lpt
		if (UnivDrvCommand(CC_QUERY_USB, 0, &qusbr) == CE_NO_ERROR)
		{
			usbcams = (int)qusbr.camerasFound;
			//printf("Found %d usb camera\n", usbcams);
			for (i = 0; i < usbcams; i++)
			{
				//printf("Camera: %d, found: %d\n", i, (int)qusbr.usbInfo[i].cameraFound);
				if ((int)qusbr.usbInfo[i].cameraFound)
				{
					strcpy(tmpname, qusbr.usbInfo[i].name);
			    		//printf("Camera: %d, type: %d\n", i,(int)qusbr.usbInfo[i].cameraType);
				    	switch(qusbr.usbInfo[i].cameraType)
				    	{
				    		case NO_CAMERA:
				    		 	tmpnam1[0]='\0';
							break;

				 		case	ST237_CAMERA:
				 			// Need to connect to tell if A or vanilla
							if (OpenDevice(DEV_USB1 + i, 0) == CE_NO_ERROR)
							{
								if (sbig_EstablishLink() == CE_NO_ERROR)
								{
								  	if ((res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, &gcr)) == CE_NO_ERROR)
									{
										// Get camera name
										_GetCameraName(&gcr, tmpname);
										strcpy(tmpnam1, tmpname);
									}
								}
								else
								{
									sprintf(tmpnam1,"%s", tmpname);
								}
								sbig_CloseDevice();
							}
				    		 	break;
				    		 	
				  		default:
			 	    		 	// driver reports name as "SBIG ST-L-XXX..."
			    	    	 		p1 = tmpname + 5;
			    	    	 		if((p2  = strchr(p1,' ')) != NULL)
			    	    	 		{
				  				*p2  = '\0';
			    	    	 		}
					  	 	sprintf(tmpnam1,"%s", tmpname);
							if(strstr(qusbr.usbInfo[i].name, "Color") != NULL)
							{
								strcat(tmpnam1, " Color");
							}
				    		 	break;
				    	}
					m_camera_list.listinfo[cams].camportId	= DEV_USB1 + i;
					m_camera_list.listinfo[cams].camIp 	= 0;
					m_camera_list.listinfo[cams].camId	= (qusbr.usbInfo[i].cameraType != NO_CAMERA) ? qusbr.usbInfo[i].cameraType + 2000 : -1;
					strcpy(m_camera_list.listinfo[cams].camName, tmpnam1);
					strcpy(m_camera_list.listinfo[cams].camSerial, qusbr.usbInfo[i].serialNumber);
					sprintf(m_camera_list.listinfo[cams].camport, "usb%d", i+1);
					// UI list
					sprintf(tmpname, "%s usb%d", tmpnam1, i+1);
					strcat(tmplist, "|");
					strcat(tmplist, tmpname);
					// Totcams
					cams++;
				}
			}
		}
		// Now try open lpt 
		// LPT no logner supported through sbigunidrv, please see URVC
		/* if (OpenDevice(DEV_LPT1, 0) == CE_NO_ERROR)
		{
			if (sbig_EstablishLink() == CE_NO_ERROR)
			{
				strcpy(tmpnam1, m_camera_details.camName);
				m_camera_list.listinfo[cams].camportId	= DEV_LPT1;
				m_camera_list.listinfo[cams].camIp 	= 0;
				m_camera_list.listinfo[cams].camId	= m_camera_details.camId;
				strcpy(m_camera_list.listinfo[cams].camName, tmpnam1);
				sprintf(m_camera_list.listinfo[cams].camport, "lpt1");
				// UI list
				sprintf(tmpname, "%s lpt1", tmpnam1);
				strcat(tmplist, "|");
				strcat(tmplist, tmpname);
				// Totcams
				cams++;
			}
			sbig_CloseDevice();
		}
		if (OpenDevice(DEV_LPT2, 0) == CE_NO_ERROR)
		{
			if (sbig_EstablishLink() == CE_NO_ERROR)
			{
				strcpy(tmpnam1, m_camera_details.camName);
				m_camera_list.listinfo[cams].camportId	= DEV_LPT2;
				m_camera_list.listinfo[cams].camIp 	= 0;
				m_camera_list.listinfo[cams].camId	= m_camera_details.camId;
				strcpy(m_camera_list.listinfo[cams].camName, tmpnam1);
				sprintf(m_camera_list.listinfo[cams].camport, "lpt2");
				// UI list
				sprintf(tmpname, "%s lpt2", tmpnam1);
				strcat(tmplist, "|");
				strcat(tmplist, tmpname);
				// Totcams
				cams++;
			}
			sbig_CloseDevice();
		}
		if (OpenDevice(DEV_LPT3, 0) == CE_NO_ERROR)
		{
			if (sbig_EstablishLink() == CE_NO_ERROR)
			{
				strcpy(tmpnam1, m_camera_details.camName);
				m_camera_list.listinfo[cams].camportId	= DEV_LPT3;
				m_camera_list.listinfo[cams].camIp 	= 0;
				m_camera_list.listinfo[cams].camId	= m_camera_details.camId;
				strcpy(m_camera_list.listinfo[cams].camName, tmpnam1);
				sprintf(m_camera_list.listinfo[cams].camport, "lpt3");
				// UI list
				sprintf(tmpname, "%s lpt3", tmpnam1);
				strcat(tmplist, "|");
				strcat(tmplist, tmpname);
				// Totcams
				cams++;
			}
			sbig_CloseDevice();
		}*/
		if (m_drv_version < 4.60)
		{
			// Try a list of 4 ip got from a config file
			if ((i = my_ethernet_query(tmplist, &cams)) != CE_NO_ERROR)
			{
				printf("my_ethernet_query Error: %s\n", GetErrorString(i));
			}
		}
		else
		{
			// Now try list ethernet camera
			if ((i = UnivDrvCommand(CC_QUERY_ETHERNET, NULL, &qethr)) == CE_NO_ERROR)
			{
				ethcams = (int)qethr.camerasFound;
				//printf("Found %d eth camera\n", ethcams);
				for (i = 0; i < ethcams; i++)
				{
					//printf("Camera: %d, found: %d\n", i, (int)qethr.ethernetInfo[i].cameraFound);
					if ((int)qethr.ethernetInfo[i].cameraFound)
					{
						strcpy(tmpname, qethr.ethernetInfo[i].name);
				    		//printf("Camera: %d, type: %d\n", i,(int)qethr.ethernetInfo[i].cameraType);
					    	switch(qethr.ethernetInfo[i].cameraType)
					    	{
					    		case NO_CAMERA:
					    		 	tmpnam1[0]='\0';
								break;
					    		 	
					  		default:
				 	    		 	// driver reports name as "SBIG ST-L-XXX..."
				    	    	 		p1 = tmpname + 5;
				    	    	 		if((p2  = strchr(p1,' ')) != NULL)
				    	    	 		{
					  				*p2  = '\0';
				    	    	 		}
						  	 	sprintf(tmpnam1,"%s", tmpname);
								if(strstr(qethr.ethernetInfo[i].name, "Color") != NULL)
								{
									strcat(tmpnam1, " Color");
								}
					    		 	break;
					    	}
						m_camera_list.listinfo[cams].camportId	= DEV_ETH;
						m_camera_list.listinfo[cams].camIp 	= qethr.ethernetInfo[i].ipAddress;
						m_camera_list.listinfo[cams].camId	= (qethr.ethernetInfo[i].cameraType != NO_CAMERA) ? qethr.ethernetInfo[i].cameraType + 2000 : -1;
						strcpy(m_camera_list.listinfo[cams].camName, tmpnam1);
						strcpy(m_camera_list.listinfo[cams].camSerial, qethr.ethernetInfo[i].serialNumber);
						sprintf(m_camera_list.listinfo[cams].camport, "eth%d", i+1);
						// UI list
						sprintf(tmpname, "%s eth%d", tmpnam1, i+1);
						strcat(tmplist, "|");
						strcat(tmplist, tmpname);
						// Totcams
						cams++;
					}
				}
			}
			//else
			//{
			//	printf("Error: %s\n", GetErrorString(i));
			//}		
		}
		strcpy(m_camera_list.camlist, tmplist);
		m_camera_list.camnum = cams;
		res = CE_NO_ERROR;
	}
	else
	{
		res = CE_DRIVER_NOT_OPEN;
	}
	return(res);
}
//==========================================================================
static int GetCameraDetails()
{
	int  res, curW, modes, i;
	char tmpstr[256];
  	GetCCDInfoParams	gcp;
  	GetCCDInfoResults0	gcr;
 	GetCCDInfoResults2	gcr2;
 	GetCCDInfoResults3	gcr3;
 	GetCCDInfoResults4	gcr4;
 	//GetCCDInfoResults6	gcr6;
 	char				tmpname[128];

	if(IsDeviceOpen())
	{
		//printf("GetCameraDetails0\n");
		// Gathering all info first
	  	gcp.request = 0;
	  	if ((res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, &gcr)) == CE_NO_ERROR)
		{
			//printf("GetCameraDetails1\n");
  			gcp.request = 4;
		  	if ((res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, &gcr4)) == CE_NO_ERROR)
			{
				//printf("GetCameraDetails2\n");
			    	if ((gcr.cameraType != ST237_CAMERA) && (gcr.cameraType != ST5C_CAMERA))
			    	{
					gcp.request = 2;
				  	if ((res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, &gcr2)) == CE_NO_ERROR)
					{
						//printf("GetCameraDetails3\n");
						//gcp.request = 6;
					  	//if ((res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, &gcr6)) != CE_NO_ERROR)
						//{
						//	printf("Unable to read CCD_INFO request 6,\n");
						//	printf("Error: %s\n", GetErrorString(res));
						//	res = CE_NO_ERROR;
						//}
					}
			    	}
		  		else
		  		{
					//printf("GetCameraDetails3\n");
					gcp.request = 3;
					res = UnivDrvCommand(CC_GET_CCD_INFO,&gcp, (void *)&gcr3);
		  		}
		  	}
		}	  	
	  	
		// Info collected, if there's no error...	  	
  		if (res == CE_NO_ERROR)
		{
			//printf("Collected\n");
			// Get camera name
			_GetCameraName(&gcr, tmpname);
			strcpy(m_camera_details.camName, tmpname);
		    	// Camera type
		    	m_camera_details.camType = gcr.cameraType;
		    	// Camera ID
		    	m_camera_details.camId = ((m_camera_details.camType != NO_CAMERA) ? (int)m_camera_details.camType + 2000 : -1);
		    	// Camera Device
		    	m_camera_details.camDevice  = m_dev_type;
		    	// Camera IP
		    	m_camera_details.camIp = m_dev_ip;
		    	strcpy(m_camera_details.camDevName, m_dev_name);
			// Camera Serial
		    	m_camera_details.camSerial[0] = '\0';
		    	if ((gcr.cameraType != ST237_CAMERA) && (gcr.cameraType != ST5C_CAMERA))
		    	{
		  		strcpy(m_camera_details.camSerial, gcr2.serialNumber);
		    	}
			// Firmware revision
			m_camera_details.camFirmware = bcd2float(gcr.firmwareVersion);
			// Active Ccd
			m_camera_details.activeCcd = CCD_IMAGING;
			// Number of ccd
			m_camera_details.numCcd = GetNumOfCcdChips();
			// Loading camera frame modes
			curW = INT_MAX;
			modes = 0;
			for (i = 0; i < gcr.readoutModes; i++)
			{
				// Selecting suitable modes
				if ((gcr.readoutInfo[i].width < curW) && (gcr.readoutInfo[i].height > 0))
				{
					curW = gcr.readoutInfo[i].width;
					m_camera_details.frameInfo[modes].modeId = (gcr.readoutInfo[i].mode < 9) ? gcr.readoutInfo[i].mode + 1: gcr.readoutInfo[i].mode;
					m_camera_details.frameInfo[modes].mode   = gcr.readoutInfo[i].mode;
					m_camera_details.frameInfo[modes].width  = gcr.readoutInfo[i].width;
					m_camera_details.frameInfo[modes].height = gcr.readoutInfo[i].height;
					m_camera_details.frameInfo[modes].pixW   = BcdPixel2double(gcr.readoutInfo[i].pixelWidth);
					m_camera_details.frameInfo[modes].pixH   = BcdPixel2double(gcr.readoutInfo[i].pixelHeight);
					sprintf(tmpstr, "%dx%d|", m_camera_details.frameInfo[modes].modeId, m_camera_details.frameInfo[modes].modeId);
					// Compiling binlist
					strcat(m_camera_details.binList, tmpstr);
					// Modes
					modes++;
				}
			}
			// Reading pixel size
			if (gcr.readoutModes > 0)
			{
				m_camera_details.ccdpixW = m_camera_details.frameInfo[0].pixW; 
				m_camera_details.ccdpixH = m_camera_details.frameInfo[0].pixH;
				// Closing binstr
				m_camera_details.binList[strlen(m_camera_details.binList)-1] = '\0';
				strcat(m_camera_details.binList, ":0");				
			}
			// Set total number of modes found
			m_camera_details.binModes = modes;
		    	if (m_camera_details.camType == ST237_CAMERA)
		    	{
				// Pix depth
		    		m_camera_details.pixDepth  = (gcr3.adSize == 1) ? 12 : 16; 
		    		m_camera_details.colorId  = -1;
		    		//m_camera_details.camShutter = 0;
		    	}
		    	else
		    	{
				// Pix depth
		    		m_camera_details.pixDepth = 16;
		    		// Color mode (said to be bggr)
		    		//if (m_camera_details.camType == STI_CAMERA)
		    		//{
		    			m_camera_details.colorId = (strstr(gcr.name, "Color") != NULL) ? 4 : -1;
		    		//}
		    		//else
		    		//{
				//	m_camera_details.colorId = (CHECK_BIT(gcr6.ccdBits, 0)) ? 4 : -1;
		    		//}
				//m_camera_details.camShutter = !(CHECK_BIT(gcr6.cameraBits, 1));
		    	}
		    	//Tec
		    	m_camera_details.camTec = ((m_camera_details.camType == STI_CAMERA) ? 0 : 1);
		    	//Min exposure exptime
		    	if (CHECK_BIT(gcr4.capabilitiesBits, 1))
		    	{
		    		//printf("Capabilities eshutter\n");
		    		m_camera_details.minExp = 1;
		    	}
		    	else
		    	{
		    		//Set minimum ms for the various models
		    		switch (m_camera_details.camType)
		    		{
		    			case ST5C_CAMERA:
		    				m_camera_details.minExp = 100;
		    				break;
					case ST237_CAMERA:
		    				m_camera_details.minExp = 10;
		    				break;
		    			case ST7_CAMERA:
		    			case ST8_CAMERA:
		    			case ST9_CAMERA:
		    			case ST10_CAMERA:
		    			case ST1K_CAMERA:
		    				m_camera_details.minExp = MIN_ST7_EXPOSURE * 10;
		    				break;
		    			case ST2K_CAMERA:
		    			case ST4K_CAMERA:
		    				m_camera_details.minExp = 1;
		    				break;
		    			case ST402_CAMERA:
		    				m_camera_details.minExp = MIN_ST3200_EXPOSURE * 10;
		    				break;
		    			case STL_CAMERA:
	    					m_camera_details.minExp = 110;
					    	break;    			
		    			case STX_CAMERA:
	    					m_camera_details.minExp = MIN_STX_EXPOSURE * 10;
		    				break;
		    			case STI_CAMERA:
		    				m_camera_details.minExp = 1;
		    				break;
		    			case STT_CAMERA:
		    				m_camera_details.minExp = MIN_STT_EXPOSURE * 10;
		    				break;
		    			case STF_CAMERA:
		    				m_camera_details.minExp = MIN_STF8300_EXPOSURE * 10;
		    				break;
		    			default:
		    				m_camera_details.minExp = 1;
		    		}
		    	}
		    	//printf("Min exposure %d\n", m_camera_details.minExp);
		    	// Shutter
	    		switch (m_camera_details.camType)
	    		{
	    			case ST5C_CAMERA:
				case ST237_CAMERA:
		    			m_camera_details.camShutter = 0;
	    				break;
		    		default:
		    			// Shutter 2 is for dark only ;-)
		    			m_camera_details.camShutter = (m_camera_details.minExp > 1) ? 1 : 2;
				    	strcpy(m_camera_details.modList, C_("camio","0-Light|1-Dark:0"));
			}		    	
    			//printf("ShutterMode: %d\n", m_camera_details.camShutter);
		    	switch (m_camera_details.camType)
		    	{
				case ST7_CAMERA:
	    			case ST8_CAMERA:
	    			case ST9_CAMERA:
	    			case ST10_CAMERA:
	    			case ST402_CAMERA:
	    				strcpy(m_camera_details.ampList, C_("camio","0-AmpOff|1-AmpOn|2-Auto:2"));
					break;
				case STF_CAMERA:
					strcpy(m_camera_details.spdList, C_("camio","0-Slow|1-Fast:0"));
					break;
		    	}
		}

		// Cfw
		CFWParams  	cfwp;
		CFWResults	cfwr;
	
		cfwp.cfwModel   = CFWSEL_AUTO;
		cfwp.cfwCommand = CFWC_OPEN_DEVICE;
	 	if ((res = UnivDrvCommand(CC_CFW, &cfwp, &cfwr)) != CE_CFW_ERROR)
	 	{
	 		m_camera_details.cfwType  = cfwr.cfwModel;
	 		switch (m_camera_details.cfwType)
	 		{
	 			case CFWSEL_UNKNOWN:
	 				// Unknown
	 				break;
	 			case CFWSEL_CFW2:
	 				// CFW2
	 				strcpy(m_camera_details.cfwList, "2-CFW2|:0");
	 				break;
	 			case CFWSEL_CFW5:
	 				// CFW5
	 				strcpy(m_camera_details.cfwList, "5-CFW5|:0");
	 				break;
	 			case CFWSEL_CFW8:
	 				// CFW8
	 				strcpy(m_camera_details.cfwList, "5-CFW8|:0");
	 				break;
	 			case CFWSEL_CFWL:
	 				// CFWL
	 				strcpy(m_camera_details.cfwList, "5-CFWL|:0");
	 				break;
	 			case CFWSEL_CFW402:
	 				// CFW402
	 				strcpy(m_camera_details.cfwList, "4-CFW402|:0");
	 				break;
	 			case CFWSEL_AUTO:
	 				// Unknown
	 				break;
				case CFWSEL_CFW6A: 
	 				// CFW6A
	 				strcpy(m_camera_details.cfwList, "6-CFW6A|:0");
	 				break;
				case CFWSEL_CFW10:
	 				// CFW10
	 				strcpy(m_camera_details.cfwList, "10-CFW10|:0");
	 				break;
				case CFWSEL_CFW9: 
	 				// CFW10
	 				strcpy(m_camera_details.cfwList, "5-CFW9|:0");
	 				break;
				case CFWSEL_CFWL8: 
	 				// CFW8L
	 				strcpy(m_camera_details.cfwList, "8-CFW8L|:0");
	 				break;
				case CFWSEL_CFWL8G:
	 				// CFWL8G
	 				strcpy(m_camera_details.cfwList, "8-CFW8LG|:0");
	 				break;
				case CFWSEL_CFW1603: 
	 				// CFW1603???
	 				strcpy(m_camera_details.cfwList, "4-CFW1603|:0");
	 				break;
				case CFWSEL_FW5_STX: 
	 				// FW5STX
	 				strcpy(m_camera_details.cfwList, "5-FW5STX|:0");
	 				break;
				case CFWSEL_FW5_8300:
	 				// FW58300
	 				strcpy(m_camera_details.cfwList, "5-FW58300|:0");
	 				break;
				case CFWSEL_FW8_8300: 
	 				// FW88300
	 				strcpy(m_camera_details.cfwList, "8-FW88300|:0");
	 				break;
				case CFWSEL_FW7_STX: 
	 				// FW7STX
	 				strcpy(m_camera_details.cfwList, "7-FW7STX|:0");
	 				break;
				case CFWSEL_FW8_STT:
	 				// FW8STT
	 				strcpy(m_camera_details.cfwList, "8-FW8STT|:0");
	 				break;
	 		}
	 	}
	}
	else
	{
		res = CE_DEVICE_NOT_OPEN;
	}	
	return (res);
}
//==========================================================================
static int QueryCommandStatus(PAR_COMMAND cmd, unsigned short *status)
{
	int res;
	QueryCommandStatusParams  qcsp;
  	QueryCommandStatusResults qcsr;

	qcsp.command = cmd;
 	if ((res = UnivDrvCommand(CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr)) == CE_NO_ERROR)
 	{
 		*status = qcsr.status;
 	}
 	return (res);
}
//==========================================================================
static int IsExposureComplete(int *complete)
{
	int res;
	unsigned short status;
	
	if ((res = QueryCommandStatus(CC_START_EXPOSURE, &status)) == CE_NO_ERROR)
	{
		if (m_camera_details.activeCcd == CCD_IMAGING)
		{
			*complete = (status & 0x03) != 0x02;
		}
		else
		{
			*complete = (status & 0x0C) != 0x08;
		}
	}
	return (res);
}
//==========================================================================
static int GetNumOfCcdChips()
{	
	int res;

	switch(m_camera_details.camType)
	{
		case 	ST237_CAMERA:
		case 	ST5C_CAMERA:
		case 	ST402_CAMERA:
	 		res = 1;
	 		break;
		case 	ST7_CAMERA:
		case 	ST8_CAMERA:
		case 	ST9_CAMERA:
		case 	ST2K_CAMERA:
	 		res = 2;
	 		break;
		case 	STL_CAMERA:
	 		res = 3;
	 		break;
		case 	NO_CAMERA:
		default:
	 		res = 0;
	 		break;
	}
	return(res);
}
//==========================================================================
/*static int IsFanControlAvailable()
{
	if(m_camera_details.camType == ST5C_CAMERA || m_camera_details.camType == ST402_CAMERA) return(0);
	return(1);
}*/
//==========================================================================
static int EndExposure()
{
	int ret = CE_DEVICE_NOT_OPEN;
	EndExposureParams eep;

	if (sbig_CheckLink())
	{
		eep.ccd = m_camera_details.activeCcd;
		ret = UnivDrvCommand (CC_END_EXPOSURE, &eep, NULL);
	}
 	return(ret);
}
//==========================================================================
static double BcdPixel2double(ulong bcd)
{
	double value = 0.0;
	double digit = 0.01;
	int i;
  
	for(i = 0; i < 8; i++)
	{
		value += (bcd & 0x0F) * digit;
		digit *= 10.0;
		bcd  >>= 4;
	} 
	return(value);
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
static int my_ethernet_query(char *list, int *cams)
{
	int res = CE_NO_ERROR, i = 0;
	char cureth[5];
	char tmpname[64], tmpnam1[64];
	char configstr[121];
	char *configptr = configstr;
	char configfile[PATH_MAX];
	unsigned int ip1, ip2, ip3, ip4, camip;
	FILE *fp_file;
	
	strcpy(configfile, basePath);
	strcat(configfile, SBIGETHQUERYFIL);
	//printf("configfile: %s\n", configfile);
	
	if ((fp_file = fopen(configfile, "r")) != NULL)
	{
		while (configstr == fgets(configstr, 80, fp_file))
		{
			if (configstr[0] != '*')
			{
				i += 1;
				sprintf(cureth, "eth%d", i);
				configptr = strstr(configstr, cureth);
				if ( configptr != NULL)
				{
					configptr = strstr(configstr,"=");
					if ( configptr != NULL)
					{
						configptr = configptr + 1;
						sscanf(configptr,"%d.%d.%d.%d",&ip1, &ip2, &ip3, &ip4);
						//printf("read %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
						ip1 = ip1 << 24;
						ip2 = ip2 << 16;
						ip3 = ip3 << 8;
						camip = ip1 + ip2 + ip3 + ip4;
						//printf("Opening %u\n", camip);
						if (OpenDevice(DEV_ETH, camip) == CE_NO_ERROR)
						{
							//printf("%u open\n", camip);
							if (sbig_EstablishLink() == CE_NO_ERROR)
							{
								strcpy(tmpnam1, m_camera_details.camName);
								m_camera_list.listinfo[*cams].camportId	= DEV_ETH;
								m_camera_list.listinfo[*cams].camIp 	= camip;
								m_camera_list.listinfo[*cams].camId	= m_camera_details.camId;
								strcpy(m_camera_list.listinfo[*cams].camName, tmpnam1);
								sprintf(m_camera_list.listinfo[*cams].camport, "%s", cureth);
								// UI list
								sprintf(tmpname, "%s %s", tmpnam1, cureth);
								strcat(list, "|");
								strcat(list, tmpname);
								// Totcams
								*cams += 1;
							}
							sbig_CloseDevice();
						}
					}
				}
			}
		}
		fclose(fp_file);
	}	
	return (res);
}


#endif // HAVE_SBIG

