/*
 * cAtik.h
 *
 *  Created on: 21.10.2015
 *      Author: Giampiero Spezzano (gspezzano@gmail.com)
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
#ifndef _ATIK_CAM_
	#define _ATIK_CAM_

	#define MAX_CAMERA 10
	#define CAMLENGTH  128
	#define ATIK_DEBUG 0

	#ifdef __cplusplus
		extern "C" int atik_list_create();
		extern "C" char *atik_list_get();
		extern "C" void atik_list_destroy();
		extern "C" int atik_list_cleanup(char *camname);
		extern "C" int atik_list_item_count();
		extern "C" int atik_list_item_select(char *camname);
		extern "C" int atik_list_item_destroy(char *camname);
		extern "C" const char *atik_camera_name();
		extern "C" int atik_camera_open();
		extern "C" void atik_camera_close();
		extern "C" int atik_camera_setParam(PARAM_TYPE code, long value);
    	extern "C" AtikCapabilities *atik_camera_getCapabilities();
    	extern "C" CAMERA_TYPE atik_camera_getType();
		extern "C" int atik_camera_getTemperatureSensorStatus(unsigned int sensor, float *currentTemp);
		extern "C" int atik_camera_getCoolingStatus(COOLING_STATE *state, float *targetTemp, float *power);
		extern "C" int atik_camera_setCooling(float targetTemp);
		extern "C" int atik_camera_initiateWarmUp();
		extern "C" int atik_camera_getFilterWheelStatus(unsigned int *filterCount, int *moving, unsigned int *current, unsigned int *target);
		extern "C" int atik_camera_setFilter(unsigned int index);
		extern "C" int atik_camera_setPreviewMode(int useMode);
		extern "C" int atik_camera_set8BitMode(int useMode);
    	extern "C" int atik_camera_setDarkFrameMode(int useMode);
		extern "C" int atik_camera_startExposure(int amp);
		extern "C" int atik_camera_abortExposure();
		extern "C" int atik_camera_readCCD(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY);
		extern "C" int atik_camera_readCCD_delay(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY, double delay);
		extern "C" int atik_camera_getImage(unsigned short *imgBuf, unsigned int imgSize);
		extern "C" int atik_camera_setShutter(int open);
		extern "C" int atik_camera_setGuideRelays(unsigned short mask);
		extern "C" int atik_camera_setGPIODirection(unsigned short mask);
		extern "C" int atik_camera_getGPIO(unsigned short *mask);
		extern "C" int atik_camera_setGPIO(unsigned short mask);
		extern "C" int atik_camera_getGain(int *gain, int *offset);
		extern "C" int atik_camera_setGain(int gain, int offset);
		extern "C" unsigned int atik_camera_delay(double delay);
		extern "C" unsigned int atik_camera_imageWidth(unsigned int width, unsigned int binX);
		extern "C" unsigned int atik_camera_imageHeight(unsigned int height, unsigned int binY);

		extern "C" int atik_camera_getColorId();
		extern "C" char *atik_camera_getBinList();
		extern "C" char *atik_camera_getCfwList();
	#else
		// The following definitions are copied from original <atikccdusb.h>
		// Inclusion of that file from within a c source is not possible
		#define GUIDE_EAST             0x04     /* RA+ */
		#define GUIDE_NORTH            0x01     /* DEC+ */
		#define GUIDE_SOUTH            0x02     /* DEC- */
		#define GUIDE_WEST             0x08     /* RA- */
		#define GUIDE_CLEAR_WE         (0x0F & ~(GUIDE_WEST | GUIDE_EAST))
		#define GUIDE_CLEAR_NS         (0x0F & ~(GUIDE_NORTH | GUIDE_SOUTH))

		typedef enum  {
			ORIGINAL_HSC = 1, IC24, QUICKER, IIDC, SONY_SCI
		} CAMERA_TYPE;

		typedef enum {
		  COOLER_NONE, COOLER_ALWAYSON, COOLER_ONOFF, COOLER_SELECTPOWER, COOLER_SETPOINT
		} COOLER_TYPE;

		typedef enum {
		  COOLING_INACTIVE, COOLING_ON, COOLING_SETPOINT, WARMING_UP
		} COOLING_STATE;

		typedef enum {
		  QUICKER_START_EXPOSURE_DELAY = 1, QUICKER_READ_CCD_DELAY, MAX_PACKET_SIZE
		} PARAM_TYPE;
		//

		typedef enum {
		  COLOUR_NONE = 1, COLOUR_RGGB
		} COLOUR_TYPE;

		typedef struct {
		  char hasShutter;
		  char hasGuidePort;
		  char has8BitMode;
		  char hasFilterWheel;
		  unsigned int lineCount;
		  unsigned int pixelCountX, pixelCountY;
		  double pixelSizeX, pixelSizeY;
		  unsigned int maxBinX, maxBinY;
		  unsigned int tempSensorCount;
		  COOLER_TYPE cooler;
		  COLOUR_TYPE colour;
		  int offsetX, offsetY;
		  char supportsLongExposure;
		  double minShortExposure;
		  double maxShortExposure;
		} AtikCapabilities;

		int atik_list_create();
		char *atik_list_get();
		void atik_list_destroy();
		int atik_list_cleanup(char *camname);
		int atik_list_item_count();
		int atik_list_item_select(char *camname);
		int atik_list_item_destroy(char *camname);
		const char *atik_camera_name();
		int atik_camera_open();
		void atik_camera_close();
		int atik_camera_setParam(PARAM_TYPE code, long value);
    	AtikCapabilities *atik_camera_getCapabilities();
    	CAMERA_TYPE atik_camera_getType();
		int atik_camera_getTemperatureSensorStatus(unsigned int sensor, float *currentTemp);
		int atik_camera_getCoolingStatus(COOLING_STATE *state, float *targetTemp, float *power);
		int atik_camera_setCooling(float targetTemp);
		int atik_camera_initiateWarmUp();
		int atik_camera_getFilterWheelStatus(unsigned int *filterCount, int *moving, unsigned int *current, unsigned int *target);
		int atik_camera_setFilter(unsigned int index);
		int atik_camera_setPreviewMode(int useMode);
		int atik_camera_set8BitMode(int useMode);
    	int atik_camera_setDarkFrameMode(int useMode);
		int atik_camera_startExposure(int amp);
		int atik_camera_abortExposure();
		int atik_camera_readCCD(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY);
		int atik_camera_readCCD_delay(unsigned int startX, unsigned int startY, unsigned int sizeX, unsigned int sizeY, unsigned int binX, unsigned int binY, double delay);
		int atik_camera_getImage(unsigned short *imgBuf, unsigned int imgSize);
		int atik_camera_setShutter(int open);
		int atik_camera_setGuideRelays(unsigned short mask);
		int atik_camera_setGPIODirection(unsigned short mask);
		int atik_camera_getGPIO(unsigned short *mask);
		int atik_camera_setGPIO(unsigned short mask);
		int atik_camera_getGain(int *gain, int *offset);
		int atik_camera_setGain(int gain, int offset);
		unsigned int atik_camera_delay(double delay);
		unsigned int atik_camera_imageWidth(unsigned int width, unsigned int binX);
		unsigned int atik_camera_imageHeight(unsigned int height, unsigned int binY);
		
		int atik_camera_getColorId();
		char *atik_camera_getBinList();
		char *atik_camera_getCfwList();
	#endif
#endif

