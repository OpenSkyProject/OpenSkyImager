/*
 * ATIK CCD INDI Driver
 *
 * Copyright (c) 2013-2015 CloudMakers, s. r. o. All Rights Reserved.
 *
 * The code is based upon Linux library source developed by
 * Jonathan Burch, Artemis CCD Ltd. It is provided by CloudMakers
 * and contributors "AS IS", without warranty of any kind.
 */

#ifndef ATIKCCDUSB_H_
#define ATIKCCDUSB_H_

#define GUIDE_NORTH            0x01     /* DEC+ */
#define GUIDE_SOUTH            0x02     /* DEC- */
#define GUIDE_EAST             0x04     /* RA+ */
#define GUIDE_WEST             0x08     /* RA- */
#define GUIDE_CLEAR_WE         (0x0F & ~(GUIDE_WEST | GUIDE_EAST))
#define GUIDE_CLEAR_NS         (0x0F & ~(GUIDE_NORTH | GUIDE_SOUTH))

extern bool AtikDebug;
extern int AtikHostBulkTimeout;

enum CAMERA_TYPE {
  ORIGINAL_HSC = 1, IC24, QUICKER, IIDC, SONY_SCI
};

enum COOLER_TYPE {
  COOLER_NONE, COOLER_ALWAYSON, COOLER_ONOFF, COOLER_SELECTPOWER, COOLER_SETPOINT
};

enum COOLING_STATE {
  COOLING_INACTIVE, COOLING_ON, COOLING_SETPOINT, WARMING_UP
};

enum PARAM_TYPE {
  QUICKER_START_EXPOSURE_DELAY = 1, QUICKER_READ_CCD_DELAY, MAX_PACKET_SIZE
};

enum COLOUR_TYPE {
  COLOUR_NONE = 1, COLOUR_RGGB
};

struct AtikCapabilities {
  bool hasShutter;
  bool hasGuidePort;
  bool has8BitMode;
  bool hasFilterWheel;
  unsigned lineCount;
  unsigned pixelCountX, pixelCountY;
  double pixelSizeX, pixelSizeY;
  unsigned maxBinX, maxBinY;
  unsigned tempSensorCount;
  COOLER_TYPE cooler;
  COLOUR_TYPE colour;
  int offsetX, offsetY;
  bool supportsLongExposure;
  double minShortExposure;
  double maxShortExposure;
};

class AtikCamera {
  public:
    static int list(AtikCamera **cameras, int max);
    virtual const char *getName() = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool setParam(PARAM_TYPE code, long value) = 0;
    virtual bool getCapabilities(const char **name, CAMERA_TYPE *type, bool *hasShutter, bool* hasGuidePort, bool* has8BitMode, bool* hasFilterWheel, unsigned *lineCount, unsigned* pixelCountX, unsigned* pixelCountY, double* pixelSizeX, double* pixelSizeY, unsigned* maxBinX, unsigned* maxBinY, unsigned *tempSensorCount, COOLER_TYPE* cooler, COLOUR_TYPE* colour, int *offsetX, int *offsetY, bool *supportsLongExposure, double *minShortExposure, double *maxShortExposure) = 0;
    virtual bool getCapabilities(const char **name, CAMERA_TYPE *type, AtikCapabilities *capabilities) = 0;
    virtual bool getTemperatureSensorStatus(unsigned sensor, float *currentTemp) = 0;
    virtual bool getCoolingStatus(COOLING_STATE *state, float* targetTemp, float *power) = 0;
    virtual bool setCooling(float targetTemp) = 0;
    virtual bool initiateWarmUp() = 0;
    virtual bool getFilterWheelStatus(unsigned *filterCount, bool *moving, unsigned *current, unsigned *target) = 0;
    virtual bool setFilter(unsigned index) = 0;
    virtual bool setPreviewMode(bool useMode) = 0;
    virtual bool set8BitMode(bool useMode) = 0;
    virtual bool setDarkFrameMode(bool useMode) = 0;
    virtual bool startExposure(bool amp) = 0;
    virtual bool abortExposure() = 0;
    virtual bool readCCD(unsigned startX, unsigned startY, unsigned sizeX, unsigned sizeY, unsigned binX, unsigned binY) = 0;
    virtual bool readCCD(unsigned startX, unsigned startY, unsigned sizeX, unsigned sizeY, unsigned binX, unsigned binY, double delay) = 0;
    virtual bool getImage(unsigned short* imgBuf, unsigned imgSize) = 0;
    virtual bool setShutter(bool open) = 0;
    virtual bool setGuideRelays(unsigned short mask) = 0;
    virtual bool setGPIODirection(unsigned short mask) = 0;
    virtual bool getGPIO(unsigned short *mask) = 0;
    virtual bool setGPIO(unsigned short mask) = 0;
    virtual bool getGain(int *gain, int* offset) = 0;
    virtual bool setGain(int gain, int offset) = 0;
    virtual unsigned delay(double delay) = 0;
    virtual unsigned imageWidth(unsigned width, unsigned binX) = 0;
    virtual unsigned imageHeight(unsigned height, unsigned binY) = 0;
    virtual unsigned getSerialNumber() = 0;
    virtual unsigned getVersionMajor() = 0;
    virtual unsigned getVersionMinor() = 0;
    virtual const char *getLastError() = 0;
    virtual ~AtikCamera() { };
};


extern "C" int AtikCamera_list(AtikCamera **cameras, int max);
extern "C" void AtikCamera_destroy(AtikCamera *camera);

#endif
