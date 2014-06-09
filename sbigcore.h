/*
 * sbig.h
 *
 *  Created on: 05.05.2014
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 *  This is a derived work from sbigcam.h, released under GPL v2 as part of
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
#ifndef _SBIG_CAM_
	#define _SBIG_CAM_
	
	#define SBIGLISTINFOLEN 12
	#define SBIGFRAMINFOLEN 20
	
	typedef struct 
	{
		int				camportId;
		char				camport[5];
		unsigned long 	camIp;
		char 			camName[64];
		unsigned int 		camId;
		char 			camSerial[10];
	} sbig_list_info;

	typedef struct 
	{
		char 			camlist[1024];
		int 				camnum;
		sbig_list_info 	listinfo[SBIGLISTINFOLEN];
	} sbig_list;

	typedef struct 
	{
		int				modeId;
		int				mode;
		int 				width;
		int				height;
		double 			pixW;
		double			pixH;
	} sbig_camframe_info;

	typedef struct 
	{
		int				camId;
		int				camType;
		int				camDevice;
		unsigned long 	camIp;
		char				camDevName[16];
		char				camName[64];
		char				camSerial[16];
		float			camFirmware;
		int				activeCcd;
		int				numCcd;
		char 			binList[256];
		int 				binModes;
		sbig_camframe_info 	frameInfo[SBIGFRAMINFOLEN];
		int 				pixDepth;
		double 			ccdpixW;
		double			ccdpixH;
		int				colorId;
		int				camShutter;
		int				minExp;
		char				ampList[256];
		char				spdList[256];
		char				modList[256];
	} sbig_camdetails;
	
	//=============================================================================
	// SBIG CCD camera port definitions:
	#define			SBIG_USB0 					"usb1"
	#define			SBIG_USB1 					"usb2"
	#define			SBIG_USB2 					"usb3"
	#define			SBIG_USB3 					"usb4"
	#define			SBIG_LPT0 					"lpt1"
	#define			SBIG_LPT1 					"lpt2"
	#define			SBIG_LPT2 					"lpt3"
	#define			SBIG_ETH0						"eth1"
	#define			SBIG_ETH1						"eth2"
	#define			SBIG_ETH2						"eth3"
	#define			SBIG_ETH4						"eth4"

	//=============================================================================
	// public
	int				sbig_core_init();
	int 				sbig_core_close();

	// Driver Related Commands:
	int 				sbig_OpenDevice(const char *devName);
	int				sbig_CloseDevice();
	int				sbig_EstablishLink();
	int				sbig_CheckLink();

	// High level functions:
	sbig_list	         *sbig_GetCameraList();
	char			    *sbig_GetDeviceName();
	sbig_camdetails    *sbig_GetCameraDetails();
	void 			sbig_GetFrameSize(int modeId, int *mode, int *width, int *height);
	char			    *sbig_GetErrorString();
	int 				sbig_GetCfwType();
	
	// Exposure Related Commands:
	int				sbig_StartExposure(qhy_exposure *expar);
	int				sbig_Readout(qhy_exposure *expar, unsigned char *databuffer);
	int 				sbig_KillExposure();
	int 				sbig_Shutter(int cmd);

	// Temperature Related Commands:
	int				sbig_SetTemperatureRegulation(int enable, double temperature);
	int				sbig_QueryTemperatureStatus(int *enabled, double *ccdTemp, double *setpointT, double *power);

	//=============================================================================
#endif //_SBIG_CAM_
