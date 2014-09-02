/*
 * urvccore.h
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

#ifndef _URVC_CAM_
	#define _URVC_CAM_
	
	//=============================================================================
	#define URVCLISTINFOLEN 3
	#define URVCFRAMINFOLEN 20
	typedef struct 
	{
		int				camportId;
		char				camport[5];
		char 			camName[64];
		unsigned int 		camId;
	} urvc_list_info;

	typedef struct 
	{
		char 			camlist[1024];
		int 				camnum;
		urvc_list_info 	listinfo[URVCLISTINFOLEN];
	} urvc_list;

	typedef struct 
	{
		int				camType;
		int				camDevice;
		char				camDevName[16];
		char				camName[64];
		char				camSerial[16];
		float			camFirmware;
		int				activeCcd;
		int				numCcd;
		char 			binList[256];
		int 				binModes;
		int 				pixDepth;
		double 			ccdpixW;
		double			ccdpixH;
		int				camTec;
		int				camShutter;
		int				minExp;
		int				cfwType;
		char				ampList[256];
		char				spdList[256];
		char				modList[256];
		char				cfwList[256];
		int 				ccdActiveW;
		int 				ccdActiveH;
	} urvc_camdetails;

	//=============================================================================
	// CCD camera port definitions:
	#define			URVC_LPT0 					"lpt1"
	#define			URVC_LPT1 					"lpt2"
	#define			URVC_LPT2 					"lpt3"
	//=============================================================================
	// public
	void  urvc_core_init();
	char *urvc_GetErrorString();
	int   urvc_core_reload_list();
	int   urvc_OpenCamera(const char *devName);
	void  urvc_CloseCamera();
	int   urvc_StartExposure(qhy_exposure *expar);
	int   urvc_KillExposure();
	int   urvc_Readout(qhy_exposure *expar, unsigned char *databuffer);
	int   urvc_QueryTemperatureStatus(int *enabled, double *ccdTemp, double *setpointTemp, int *power);
	int   urvc_SetTemperatureRegulation (int enable, double setpt);

	urvc_list	      *urvc_GetCameraList();
	urvc_camdetails *urvc_GetCameraDetails();
	//=============================================================================
	
#endif //_URVC_CAM_
