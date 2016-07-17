/*
 * cAtik.cpp
 *
 *  Created on: 21.10.2015
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
 *
 * C frontend to libAtikCcd
 *
 * A special thank to my dear friend Mario Meo Colombo for hint and advices
 * about C++ and C/C++ interactions.
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
 
/*
ORIGINAL_HSC = 4000/11000
IC24 = 3xx/4xx/One (e.g. your 314L)
QUICKER = Titan
IIDC = GP
SONY_SCI = VS/Infinity
*/

/*
Gain and offset in case of 314L it is camera limitation, it seems it can't be 
changed, at least I don't see it in original windows SDK. 
Actually, it should not be changed even on VS cameras, it was added for VS OEM 
providers doing it on their own risk. I did it on my VS60 and spent one weekend
by trying to revert it back.
*/

#ifdef HAVE_ATIK

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <atikccdusb.h>
#include "atikcore.h"

using namespace std;

static AtikCamera *listDevices[MAX_CAMERA];
static AtikCamera *selectedDevice = NULL;
static CAMERA_TYPE camtype;
static AtikCapabilities camcapabilities;
static int cameraCount = 0;
static char *cameraList = NULL;
static int colorId = -1;
static char binList[CAMLENGTH+1];
static char cfwList[CAMLENGTH+1];


int atik_list_create()
{
	AtikDebug = ATIK_DEBUG;
	char curName[CAMLENGTH+1];
	
	cameraList = (char *)malloc(1);	
	cameraList[0] = '\0';
	cameraCount = AtikCamera::list(listDevices, MAX_CAMERA);
	for (int i = 0; i < cameraCount; i++) 
	{
		AtikCamera *device = listDevices[i];
		strcpy(curName, device->getName());
		cameraList = (char *)realloc(cameraList, strlen(cameraList)+strlen(curName)+2);
		strcat(cameraList, "|");
		strcat(cameraList, curName);
		#if (ATIK_DEBUG==1)
			cerr << endl << "found " << curName << " --------------------" << endl << endl;
		#endif
	}
	#if (ATIK_DEBUG==1)
		cerr << endl << "cameraList " << cameraList << " --------------------" << endl << endl;
	#endif
	return cameraCount;
}

char *atik_list_get()
{
	return cameraList;
}

void atik_list_destroy()
{
	for (int i = 0; i < cameraCount; i++) 
	{
		#if (ATIK_DEBUG==1)
			cerr << endl << "destroy " << listDevices[i]->getName() << " --------------------" << endl << endl;
		#endif
		AtikCamera_destroy(listDevices[i]);
	}
	selectedDevice = NULL;
}

int atik_list_cleanup(char *camname)
{
	int retval = 0;
	
	for (int i = 0; i < cameraCount; i++) 
	{
		AtikCamera *device = listDevices[i];
		if  (strcmp(camname, device->getName()) != 0)
		{
			#if (ATIK_DEBUG==1)
				cerr << endl << "cleanup " << device->getName() << " --------------------" << endl << endl;
			#endif
			AtikCamera_destroy(device);
			retval++;
		}
	}
	return (retval != cameraCount);
}

int atik_list_item_count()
{
	return cameraCount;
}

int atik_list_item_select(char *camname)
{
	int retval = 0;
	
	for (int i = 0; i < cameraCount; i++) 
	{
		AtikCamera *device = listDevices[i];
		if  (strcmp(camname, device->getName()) == 0)
		{
			selectedDevice = listDevices[i];
			retval = 1;
			#if (ATIK_DEBUG==1)
				cerr << endl << "selected " << selectedDevice->getName() << " --------------------" << endl << endl;
			#endif
			break;
		}
	}
	return (retval);
}

int atik_list_item_destroy(char *camname)
{
	int retval = 0;
	
	for (int i = 0; i < cameraCount; i++) 
	{
		AtikCamera *device = listDevices[i];
		if  (strcmp(camname, device->getName()) == 0)
		{
			#if (ATIK_DEBUG==1)
				cerr << endl << "item_destroy " << device->getName() << " --------------------" << endl << endl;
			#endif
			AtikCamera_destroy(device);
			retval = 1;
			break;
		}
	}
	return (retval);
}

const char *atik_camera_name()
{
	return (selectedDevice == NULL) ? "" : selectedDevice->getName();
}

int atik_camera_open()
{
	int retval = 0;
	int maxBin = 1;
	int i;
	char tmpstr[CAMLENGTH+1];
	unsigned int filterCount;
	
	if (selectedDevice->open())
	{
		if (selectedDevice->getCapabilities(NULL, &camtype, &camcapabilities))
		{
			// Patch 4 wrong maxBin (255)
			camcapabilities.maxBinX = (camcapabilities.maxBinX > 8) ? 8 : camcapabilities.maxBinX;
			camcapabilities.maxBinY = (camcapabilities.maxBinY > 8) ? 8 : camcapabilities.maxBinY;
			retval = 1;
			
			//ColorId
			colorId = -1;
			if (camcapabilities.colour == 2)	
			{
				colorId = 2;
				if ((camcapabilities.offsetX) && (!camcapabilities.offsetY))
				{
					colorId = 3;
				}
				else if ((!camcapabilities.offsetX) && (camcapabilities.offsetY))
				{
					colorId = 1;
				}
				else if ((camcapabilities.offsetX) && (camcapabilities.offsetY))
				{
					colorId = 4;
				}
			}
			//Binlist
			binList[0] = '\0';
			maxBin = (camcapabilities.maxBinX < camcapabilities.maxBinY) ? camcapabilities.maxBinX : camcapabilities.maxBinY;
			
			for (i = 1; i <= maxBin; i++)
			{
				// Compiling binlist
				sprintf(tmpstr, "%dx%d|", i, i);
				strcat(binList, tmpstr);
			}
			strcat(binList, ":0");
			//CfwList
			cfwList[0] = '\0';
			if (camcapabilities.hasFilterWheel)
			{
				if (selectedDevice->getFilterWheelStatus(&filterCount, NULL, NULL, NULL))
				{
					sprintf(binList, "%u-CFW|:0", filterCount);
				}
			}
		}
	}
	return (retval);
}

void atik_camera_close()
{
	if (!(selectedDevice == NULL)) 
	{
		selectedDevice->close();
	}
}

int atik_camera_setParam(PARAM_TYPE code, long value)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setParam(code, value);
}

AtikCapabilities *atik_camera_getCapabilities()
{
	return &camcapabilities;
}

CAMERA_TYPE atik_camera_getType()
{
	return camtype;
}

int atik_camera_getTemperatureSensorStatus(unsigned int sensor, float *currentTemp)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getTemperatureSensorStatus(sensor, currentTemp);
}

int atik_camera_getCoolingStatus(COOLING_STATE *state, float *targetTemp, float *power)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getCoolingStatus(state, targetTemp, power);
}

int atik_camera_setCooling(float targetTemp)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setCooling(targetTemp);
}

int atik_camera_initiateWarmUp()
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->initiateWarmUp();
}

int atik_camera_getFilterWheelStatus(unsigned int *filterCount, int *moving, unsigned int *current, unsigned int *target)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getFilterWheelStatus(filterCount, (bool *)moving, current, target);
}

int atik_camera_setFilter(unsigned int index)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setFilter(index);
}

int atik_camera_setPreviewMode(int useMode)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setPreviewMode((useMode != 0));
}

int atik_camera_set8BitMode(int useMode)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->set8BitMode((useMode != 0));
}

int atik_camera_startExposure(int amp)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->startExposure((amp != 0));
}

int atik_camera_abortExposure()
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->abortExposure();
}

int atik_camera_readCCD(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->readCCD(startX, startY, sizeX, sizeY, binX, binY);
}

int atik_camera_readCCD_delay(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY, double delay)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->readCCD(startX, startY, sizeX, sizeY, binX, binY, delay);
}

int atik_camera_getImage(unsigned short *imgBuf, unsigned int imgSize)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getImage(imgBuf, imgSize);
}

int atik_camera_setShutter(int open)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setShutter(open);
}

int atik_camera_setGuideRelays(unsigned short mask)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setGuideRelays(mask);
}

int atik_camera_setGPIODirection(unsigned short mask)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setGPIODirection(mask);
}

int atik_camera_getGPIO(unsigned short *mask)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getGPIO(mask);
}

int atik_camera_setGPIO(unsigned short mask)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setGPIO(mask);
}

int atik_camera_getGain(int *gain, int *offset)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->getGain(gain, offset);
}

int atik_camera_setGain(int gain, int offset)
{
	return (selectedDevice == NULL) ? 0 : selectedDevice->setGain(gain, offset);
}

unsigned int atik_camera_delay(double delay)
{
	return (selectedDevice == NULL) ? -1 : selectedDevice->delay(delay);
}

unsigned int atik_camera_imageWidth(unsigned int width, unsigned int binX)
{
	return (selectedDevice == NULL) ? -1 : selectedDevice->imageWidth(width, binX);
}

unsigned int atik_camera_imageHeight(unsigned int height, unsigned int binY)
{
	return (selectedDevice == NULL) ? -1 : selectedDevice->imageHeight(height, binY);
}

int atik_camera_getColorId()
{
	return colorId;
}

char *atik_camera_getBinList()
{
	return binList;
}
char *atik_camera_getCfwList()
{
	return cfwList;
}

#endif //HAVE_ATIK
