/*
 Generic CCD
 CCD Template for INDI Developers
 Copyright (C) 2012 Jasem Mutlaq (mutlaqja@ikarustech.com) (Original generic_ccd code)
 Copyright (C) 2014 Marco Gulino (marco@gulinux.net) (OSI Adaptation)

 Multiple device support Copyright (C) 2013 Peter Polakovic (peter.polakovic@cloudmakers.eu)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <memory>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <memory>

#include "indidevapi.h"
#include "eventloop.h"

#include "indi_ccd.h"

#define MAX_CCD_TEMP	45		/* Max CCD temperature */
#define MIN_CCD_TEMP	-55		/* Min CCD temperature */
#define MAX_X_BIN	16		/* Max Horizontal binning */
#define MAX_Y_BIN	16		/* Max Vertical binning */
#define MAX_PIXELS	4096		/* Max number of pixels in one dimension */
#define POLLMS		1000		/* Polling time (ms) */
#define TEMP_THRESHOLD  .25		/* Differential temperature threshold (C)*/

#include "osicamera.h"

std::shared_ptr<OSICCD> camera;
std::shared_ptr<OSICameraRAII> initializeDriver;

static void cleanup() {
  camera.reset();
  initializeDriver.reset();
}

void ISInit() {
  static bool isInit = false;
  if (!isInit) {
    initializeDriver.reset(new OSICameraRAII);
    auto availDevices = initializeDriver->connectedCameras();
    if(availDevices.empty()) {
      return; // TODO: error message? how?
    }
    camera.reset(new OSICCD(availDevices[0] )); // TODO: allow user to choose which one? how?

    atexit(cleanup);
    isInit = true;
  }
}

void ISGetProperties(const char *dev) {
  ISInit();
  if (dev == NULL || string{dev} == camera->getDeviceName() ) {
    camera->ISGetProperties(dev);
  }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num) {
  ISInit();
  if (dev == NULL || string{dev} == camera->getDeviceName() ) {
    camera->ISNewSwitch(dev, name, states, names, num);
  }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num) {
  ISInit();
  if (dev == NULL || string{dev} == camera->getDeviceName() ) {
    camera->ISNewText(dev, name, texts, names, num);
  }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num) {
  ISInit();
  if (dev == NULL || string{dev} == camera->getDeviceName() ) {
    camera->ISNewNumber(dev, name, values, names, num);
  }
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n) {
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root) {
  INDI_UNUSED(root);
}

OSICCD::OSICCD(const std::string &name) : osiCamera(new OSICamera(initializeDriver)), osiName(name) {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  setDeviceName("OSI CCD");
  sim = false;
}



OSICCD::~OSICCD() {
}

const char * OSICCD::getDefaultName() {
  return "OSI CCD";
}

bool OSICCD::initProperties() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  // Init parent properties first
  INDI::CCD::initProperties();
  DEBUGF(INDI::Logger::DBG_SESSION, "%s", __PRETTY_FUNCTION__);
  

  IUFillSwitch(&ResetS[0], "RESET", "Reset", ISS_OFF);
  IUFillSwitchVector(&ResetSP, ResetS, 1, getDeviceName(), "FRAME_RESET", "Frame Values", IMAGE_SETTINGS_TAB, IP_WO, ISR_1OFMANY, 0, IPS_IDLE);
  
  auto devices = initializeDriver->connectedCameras();
  DevicesS.resize(devices.size());
  for(uint32_t i=0; i<devices.size(); i++)
    IUFillSwitch(&DevicesS[i], devices[i].c_str(), devices[i].c_str(), i==2?ISS_ON:ISS_OFF);
  IUFillSwitchVector(&DevicesSP, DevicesS.data(), DevicesS.size(), getDeviceName(), "DEVICE_MODEL_VECTOR", "Device", MAIN_CONTROL_TAB, IP_WO, ISR_1OFMANY, 0, IPS_IDLE);
  
  IUFillNumber(&GainN[0], "GAIN", "Gain", "%0.f", 1., 255, 1., osiCamera->gain() );
  IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "CCD_GAIN", "Gain", IMAGE_SETTINGS_TAB, IP_RW, 60, IPS_IDLE);
  IUFillNumber(&SpeedN[0], "SPEED", "USB Speed", "%0.f", 0., 1, 1., osiCamera->speed() );
  IUFillNumberVector(&SpeedNP, SpeedN, 1, getDeviceName(), "CCD_SPEED", "USB Speed", IMAGE_SETTINGS_TAB, IP_RW, 60, IPS_IDLE);
  IUFillNumber(&ModeN[0], "MODE", "Camera Mode", "%0.f", 0., 255., 1., osiCamera->mode() );
  IUFillNumberVector(&ModeNP, ModeN, 1, getDeviceName(), "CCD_MODE", "Camera Mode", IMAGE_SETTINGS_TAB, IP_RW, 60, IPS_IDLE);
  
  Capability cap;

  cap.canAbort = true;
  cap.canBin = false;
  cap.canSubFrame = false;
  cap.hasCooler = false;
  cap.hasGuideHead = false;
  cap.hasShutter = true;
  cap.hasST4Port = true;

  SetCapability(&cap);

  return true;
}

bool OSICCD::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
  DEBUGF(INDI::Logger::DBG_SESSION, "%s: %d properties updated, group name: %s", __PRETTY_FUNCTION__, n, name);
  
  auto setCameraProperty = [&values, &names, &n](INumberVectorProperty &property, std::function<void()> setF){
    property.s = IPS_BUSY;
    IDSetNumber(&property, NULL);
    IUUpdateNumber(&property, values, names, n);
    setF();
    property.s = IPS_OK;
    IDSetNumber(&property, NULL);
  }; 
  
  for(uint16_t i=0; i<n; i++) {
    DEBUGF(INDI::Logger::DBG_SESSION, "%s - property %d/%d: dev=%s, name=%s, value=%f, names=%s", __PRETTY_FUNCTION__, i, n, dev, name, values[i], names[i]);
    if(string{names[i]} == string{"GAIN"}) {
      setCameraProperty(GainNP, [=]{ osiCamera->gain(values[i]); });
      return true;
    }
    if(string{names[i]} == string{"SPEED"}) {
      setCameraProperty(SpeedNP, [=]{ osiCamera->speed(values[i]); });
      return true;
    }
    if(string{names[i]} == string{"MODE"}) {
      setCameraProperty(ModeNP, [=]{ osiCamera->mode(values[i]); });
      return true;
    }
  }
  return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}


void OSICCD::ISGetProperties(const char *dev) {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  INDI::CCD::ISGetProperties(dev);

  // Add Debug, Simulator, and Configuration controls
  addDebugControl();
  addConfigurationControl();
  defineSwitch (&DevicesSP);
  // addSimulationControl();
}

bool OSICCD::updateProperties() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  INDI::CCD::updateProperties();

  defineSwitch (&DevicesSP);
  if (isConnected()) {
    defineSwitch(&ResetSP);
    // Let's get parameters now from CCD
    setupParams();
    GainN[0].value = osiCamera->gain();
    SpeedN[0].value = osiCamera->speed();
    ModeN[0].value = osiCamera->mode();
    defineNumber(&GainNP);
    defineNumber(&SpeedNP);
    defineNumber(&ModeNP);

    timerID = SetTimer(POLLMS);
  } else {
    deleteProperty(ResetSP.name);
    // TODO: delete other properties too

    rmTimer(timerID);
  }

  return true;
}

bool OSICCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) {
  DEBUGF(INDI::Logger::DBG_DEBUG,"%s: dev=%s, name=%s, n=%d", __PRETTY_FUNCTION__, dev, name, n);
  if (strcmp(dev, getDeviceName()) == 0) {

    /* Reset */
    if (!strcmp(name, ResetSP.name)) {
      if (IUUpdateSwitch(&ResetSP, states, names, n) < 0)
        return false;
      resetFrame();
      return true;
    }
    
    if(string{name} == DevicesSP.name) {
      if(osiCamera->connected())
	return false;
      bool updated = IUUpdateSwitch(&DevicesSP, states, names, n) == 0;
      if(updated)
	osiName = names[0];
      IDSetSwitch(&DevicesSP, NULL);
      ResetSP.s = updated==0?IPS_IDLE : IPS_ALERT;
      return updated;
    }

  }

  //  Nobody has claimed this, so, ignore it
  return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool OSICCD::Connect() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  sim = isSimulation();

  ///////////////////////////
  // Guide Port?
  ///////////////////////////
  // Do we have a guide port?

  if (sim)
    return true;

  if(osiCamera->connected() ) {
    IDMessage(getDeviceName(), "Error, already connected to camera");
    return false;
  }
  
  IDMessage(getDeviceName(), "Attempting to find the CCD device %s...", osiName.c_str());

  if (isDebug()) {
    DEBUGF(INDI::Logger::DBG_DEBUG, "Connecting CCD: Attempting to find the camera %s", osiName.c_str());
  }
  try {
    osiCamera->connect(osiName);
    osiCamera->gain(10);
    osiCamera->mode(250);
    osiCamera->speed(0);
  } catch(std::exception &e)
  {
    IDMessage(getDeviceName(), "Error, connecting to camera %s: %s", osiName.c_str(), e.what() );
    return false;
  }

  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Connect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to connect due to ...");
   *  return false;
   *
   *
   **********************************************************/

  /* Success! */
  IDMessage(getDeviceName(), "CCD is online. Retrieving basic data.");
  if (isDebug())
    DEBUG(INDI::Logger::DBG_DEBUG, "CCD is online. Retrieving basic data.");
  osiCamera->reset();
  return true;
}

bool OSICCD::Disconnect() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  if (sim)
    return true;
  
  if(!osiCamera->disconnect() )
  {
    IDMessage(getDeviceName(), "Error, unable to disconnect.");
    return false;
  }

  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD disonnect function
   *  If you encameraCounter an error, send the client a message
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to disconnect due to ...");
   *  return false;
   *
   *
   **********************************************************/

  IDMessage(getDeviceName(), "CCD is offline.");
  return true;
}

bool OSICCD::setupParams() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  if (isDebug())
    DEBUG(INDI::Logger::DBG_DEBUG, "In setupParams");

  float x_pixel_size, y_pixel_size;
  int bit_depth = 16;
  int x_1, y_1, x_2, y_2;

  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Get basic CCD parameters here such as
   *  + Pixel Size X
   *  + Pixel Size Y
   *  + Bit Depth?
   *  + X, Y, W, H of frame
   *  + Temperature
   *  + ...etc
   *
   *
   *
   **********************************************************/

  ///////////////////////////
  // 1. Get Pixel size
  ///////////////////////////
  if (sim) {
    x_pixel_size = 5.4;
    y_pixel_size = 5.4;
  } else {
    x_pixel_size = osiCamera->pixelSize().x;
    y_pixel_size = osiCamera->pixelSize().y;
  }

  ///////////////////////////
  // 2. Get Frame
  ///////////////////////////
  if (sim) {
    x_1 = y_1 = 0;
    x_2 = 1280;
    y_2 = 1024;
  } else {
    auto availableResolutions = osiCamera->supportedResolutions();
    if(availableResolutions.empty()) {
      IDMessage(getDeviceName(), "Error, list of supported resolutions is empty");
      return false;
    }
    x_1 = y_1 = 0;
    x_2 = availableResolutions[0].width;
    y_2 = availableResolutions[0].height;
  }

  ///////////////////////////
  // 3. Get temperature
  ///////////////////////////
  if (sim)
    TemperatureN[0].value = 25.0;
  else {
    TemperatureN[0].value = osiCamera->tec().celsius;
    GainN[0].value = osiCamera->gain();
    ModeN[0].value = osiCamera->mode();
    SpeedN[0].value = osiCamera->speed();
    // Actucal CALL to CCD to get temperature here
  }

  IDMessage(getDeviceName(), "The CCD Temperature is %f.\n", TemperatureN[0].value);
  IDSetNumber(&TemperatureNP, NULL);

  if (isDebug())
    DEBUGF(INDI::Logger::DBG_DEBUG, "The CCD Temperature is %f.\n", TemperatureN[0].value);

  ///////////////////////////
  // 4. Get temperature
  ///////////////////////////

  if (sim)
    bit_depth = 16;
  else {
    bit_depth = osiCamera->resolution().bitsPerPixel;
    // Set your actual CCD bit depth
  }

  SetCCDParams(x_2 - x_1, y_2 - y_1, bit_depth, x_pixel_size, y_pixel_size);

  if (sim)
    minDuration = 0.05;
  else {
    // Set your actual CCD minimum exposure duration
  }

  // Now we usually do the following in the hardware
  // Set Frame to LIGHT or NORMAL
  // Set Binning to 1x1
  /* Default frame type is NORMAL */

  // Let's calculate required buffer
  int nbuf;
  nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;    //  this is pixel cameraCount
  nbuf += 512;    //  leave a little extra at the end
  PrimaryCCD.setFrameBufferSize(nbuf);

  return true;

}

int OSICCD::SetTemperature(double temperature)
{
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  //TODO
  return -1;
    // If there difference, for example, is less than 0.1 degrees, let's immediately return OK.
    if (fabs(temperature- TemperatureN[0].value))
        return 1;

    /**********************************************************
     *
     *  IMPORRANT: Put here your CCD Set Temperature Function
     *  We return 0 if setting the temperature will take some time
     *  If the requested is the same as current temperature, or very
     *  close, we return 1 and INDI::CCD will mark the temperature status as OK
     *  If we return 0, INDI::CCD will mark the temperature status as BUSY
     **********************************************************/

    // Otherwise, we set the temperature request and we update the status in TimerHit() function.
    TemperatureRequest = temperature;
    DEBUGF(INDI::Logger::DBG_SESSION, "Setting CCD temperature to %+06.2f C", temperature);
    return 0;
}



bool OSICCD::StartExposure(float duration)
{
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  if (duration < minDuration)
  {
    DEBUGF(INDI::Logger::DBG_WARNING, "Exposure shorter than minimum duration %g s requested. \n Setting exposure time to %g s.", duration, minDuration);
    duration = minDuration;
  }

  if (imageFrameType == CCDChip::BIAS_FRAME)
  {
    duration = minDuration;
    DEBUGF(INDI::Logger::DBG_SESSION, "Bias Frame (s) : %g\n", minDuration);
  }

  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD start exposure here
   *  Please note that duration passed is in seconds.
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to start exposure due to ...");
   *  return -1;
   *
   *
   **********************************************************/

  PrimaryCCD.setExposureDuration(duration);
  ExposureRequest = duration;

  gettimeofday(&ExpStart, NULL);
  osiCamera->exposure(duration * 1000.);
  osiCamera->setResolution({PrimaryCCD.getSubW(), PrimaryCCD.getSubH(), PrimaryCCD.getBPP()});

  DEBUGF(INDI::Logger::DBG_SESSION, "Image Settings: w=%d, h=%d, bitpix=%d, bytepix=%d, gain=%d, speed=%d, mode=%d, exposure=%d, edit: %d",
      osiCamera->resolution().width,
      osiCamera->resolution().height,
      osiCamera->resolution().bitsPerPixel,
      osiCamera->resolution().bytesPerPixel,
      osiCamera->gain(),
      osiCamera->speed(),
      osiCamera->mode(),
      osiCamera->exposure(),
      osiCamera->editMode()
    );
  
  DEBUGF(INDI::Logger::DBG_SESSION, "Taking a %g seconds frame...", ExposureRequest);
  
  try {
    osiCamera->shoot();
    InExposure = true;
    DEBUGF(INDI::Logger::DBG_SESSION, "shoot started correctly (edit=%d)", osiCamera->editMode());
  } catch(std::exception &e) {
    InExposure = false;
    DEBUGF(INDI::Logger::DBG_ERROR, "shoot error: %s, editMode: %d", e.what(), osiCamera->editMode() );
  }
  return InExposure;

}



bool OSICCD::AbortExposure() {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD abort exposure here
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to abort exposure due to ...");
   *  return false;
   *
   *
   **********************************************************/
  DEBUGF(INDI::Logger::DBG_SESSION, "%s (edit=%d)", __PRETTY_FUNCTION__, osiCamera->editMode());
  osiCamera->abort();
  InExposure = false;
  DEBUGF(INDI::Logger::DBG_SESSION, "%s: exit (edit=%d)", __PRETTY_FUNCTION__, osiCamera->editMode());
  return true;
}

bool OSICCD::UpdateCCDFrameType(CCDChip::CCD_FRAME fType) {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  int err = 0;
  CCDChip::CCD_FRAME imageFrameType = PrimaryCCD.getFrameType();

  if (fType == imageFrameType || sim)
    return true;

  switch (imageFrameType) {
  case CCDChip::BIAS_FRAME:
  case CCDChip::DARK_FRAME:
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  BIAS and DARK are taken with shutter closed, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be closed. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  IDMessage(getDeviceName(), "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
    break;

  case CCDChip::LIGHT_FRAME:
  case CCDChip::FLAT_FRAME:
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Frame type here
     *  LIGHT and FLAT are taken with shutter open, so _usually_
     *  most CCD this is a call to let the CCD know next exposure shutter
     *  must be open. Customize as appropiate for the hardware
     *  If there is an error, report it back to client
     *  e.g.
     *  IDMessage(getDeviceName(), "Error, unable to set frame type to ...");
     *  return false;
     *
     *
     **********************************************************/
    break;
  }

  PrimaryCCD.setFrameType(fType);

  return true;

}

bool OSICCD::UpdateCCDFrame(int x, int y, int w, int h) {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  /* Add the X and Y offsets */
  long x_1 = x;
  long y_1 = y;

  long bin_width = x_1 + (w / PrimaryCCD.getBinX());
  long bin_height = y_1 + (h / PrimaryCCD.getBinY());

  if (bin_width > PrimaryCCD.getXRes() / PrimaryCCD.getBinX()) {
    IDMessage(getDeviceName(), "Error: invalid width requested %d", w);
    return false;
  } else if (bin_height > PrimaryCCD.getYRes() / PrimaryCCD.getBinY()) {
    IDMessage(getDeviceName(), "Error: invalid height request %d", h);
    return false;
  }

  if (isDebug())
    DEBUGF(INDI::Logger::DBG_DEBUG, "The Final image area is (%ld, %ld), (%ld, %ld)\n", x_1, y_1, bin_width, bin_height);

  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Frame dimension call
   *  The values calculated above are BINNED width and height
   *  which is what most CCD APIs require, but in case your
   *  CCD API implementation is different, don't forget to change
   *  the above calculations.
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to set frame to ...");
   *  return false;
   *
   *
   **********************************************************/

  // Set UNBINNED coords
  PrimaryCCD.setFrame(x_1, y_1, w, h);

  int nbuf;
  nbuf = (bin_width * bin_height * PrimaryCCD.getBPP() / 8);    //  this is pixel count
  nbuf += 512;    //  leave a little extra at the end
  PrimaryCCD.setFrameBufferSize(nbuf);

  if (isDebug())
    DEBUGF(INDI::Logger::DBG_DEBUG, "Setting frame buffer size to %d bytes.\n", nbuf);

  return true;
}

bool OSICCD::UpdateCCDBin(int binx, int biny) {
  DEBUG(INDI::Logger::DBG_DEBUG, __PRETTY_FUNCTION__);
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Binning call
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to set binning to ...");
   *  return false;
   *
   *
   **********************************************************/

  PrimaryCCD.setBin(binx, biny);

  return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

float OSICCD::CalcTimeLeft() {
  double timesince;
  double timeleft;
  struct timeval now;
  gettimeofday(&now, NULL);

  timesince = (double) (now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double) (ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
  timesince = timesince / 1000;

  timeleft = ExposureRequest - timesince;
  return timeleft;
}

/* Downloads the image from the CCD.
 N.B. No processing is done on the image */
bool OSICCD::grabImage() {
  char * image = PrimaryCCD.getFrameBuffer();
  int width = PrimaryCCD.getSubW() / PrimaryCCD.getBinX() * PrimaryCCD.getBPP() / 8;
  int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();
  DEBUGF(INDI::Logger::DBG_DEBUG, "GrabImage Width: %d - Height: %d; buf size: %d bytes.", width, height,  width * height);

  if (sim) {

    for (int i = 0; i < height; i++)
      for (int j = 0; j < width; j++)
        image[i * width + j] = rand() % 255;
    ExposureComplete(&PrimaryCCD);
    return true;
  }
  
  try {
    DEBUG(INDI::Logger::DBG_DEBUG, "Executing camera readout");
    uint8_t *data = osiCamera->readData();
    DEBUG(INDI::Logger::DBG_DEBUG, "Copying image data to buffer");
    PrimaryCCD.setFrameBufferSize(width*height, false);
    PrimaryCCD.setFrameBuffer((char*)data);
    DEBUG(INDI::Logger::DBG_DEBUG, "Copied image data to buffer");
    ExposureComplete(&PrimaryCCD);
    DEBUG(INDI::Logger::DBG_DEBUG, "ExposureComplete");
    return true;
  } catch(exception &e) {
      DEBUGF(INDI::Logger::DBG_DEBUG, "readout failed: %s", e.what() );
      return false;
  }
}

void OSICCD::addFITSKeywords(fitsfile *fptr, CCDChip *targetChip) {
  INDI::CCD::addFITSKeywords(fptr, targetChip);

  int status = 0;
  fits_update_key_s(fptr, TDOUBLE, "CCD-TEMP", &(TemperatureN[0].value), "CCD Temperature (Celsius)", &status);
  fits_write_date(fptr, &status);

}

void OSICCD::resetFrame() {
  UpdateCCDBin(1, 1);
  UpdateCCDFrame(0, 0, PrimaryCCD.getXRes(), PrimaryCCD.getYRes());
  IUResetSwitch(&ResetSP);
  ResetSP.s = IPS_IDLE;
  IDSetSwitch(&ResetSP, "Resetting frame and binning.");

  return;
}

void OSICCD::TimerHit() {
  int timerID = -1;
  int err = 0;
  long timeleft;
  double ccdTemp;

  if (isConnected() == false)
    return;  //  No need to reset timer if we are not connected anymore

  if (InExposure) {
    timeleft = CalcTimeLeft();

    if (timeleft < 1.0) {
      if (timeleft > 0.25) {
        //  a quarter of a second or more
        //  just set a tighter timer
        timerID = SetTimer(250);
      } else {
        if (timeleft > 0.07) {
          //  use an even tighter timer
          timerID = SetTimer(50);
        } else {
          //  it's real close now, so spin on it
          while (!sim && timeleft > 0) {

            /**********************************************************
             *
             *  IMPORRANT: If supported by your CCD API
             *  Add a call here to check if the image is ready for download
             *  If image is ready, set timeleft to 0. Some CCDs (check FLI)
             *  also return timeleft in msec.
             *
             **********************************************************/

            int slv;
            slv = 100000 * timeleft;
            usleep(slv);
          }

          /* We're done exposing */
          IDMessage(getDeviceName(), "Exposure done, downloading image...");

          if (isDebug())
            DEBUG(INDI::Logger::DBG_DEBUG, "Exposure done, downloading image...\n");

          PrimaryCCD.setExposureLeft(0);
          InExposure = false;
          /* grab and save image */
          if(!grabImage())
	  {
	    DEBUGF(INDI::Logger::DBG_ERROR, "Marking exposure as failed (edit mode: %d).", osiCamera->editMode() );
	    PrimaryCCD.setExposureFailed();
	  }

        }
      }
    } else {

      if (isDebug()) {
        DEBUGF(INDI::Logger::DBG_DEBUG, "With time left %ld\n", timeleft);
        DEBUG(INDI::Logger::DBG_DEBUG, "image not yet ready....\n");
      }

      PrimaryCCD.setExposureLeft(timeleft);

    }

  }

  switch (TemperatureNP.s) {
  case IPS_IDLE:
  case IPS_OK:
    /**********************************************************
     *
     *
     *
     *  IMPORRANT: Put here your CCD Get temperature call here
     *  If there is an error, report it back to client
     *  e.g.
     *  IDMessage(getDeviceName(), "Error, unable to get temp due to ...");
     *  return false;
     *
     *
     **********************************************************/

    if (fabs(TemperatureN[0].value - ccdTemp) >= TEMP_THRESHOLD) {
      TemperatureN[0].value = ccdTemp;
      IDSetNumber(&TemperatureNP, NULL);
    }
    break;

  case IPS_BUSY:
    if (sim) {
      ccdTemp = TemperatureRequest;
      TemperatureN[0].value = ccdTemp;
    } else {
      /**********************************************************
       *
       *
       *
       *  IMPORRANT: Put here your CCD Get temperature call here
       *  If there is an error, report it back to client
       *  e.g.
       *  IDMessage(getDeviceName(), "Error, unable to get temp due to ...");
       *  return false;
       *
       *
       **********************************************************/
    }

    // If we're within threshold, let's make it BUSY ---> OK
    if (fabs(TemperatureRequest - ccdTemp) <= TEMP_THRESHOLD) {
      TemperatureNP.s = IPS_OK;
      IDSetNumber(&TemperatureNP, NULL);
    }

    TemperatureN[0].value = ccdTemp;
    IDSetNumber(&TemperatureNP, NULL);
    break;

  case IPS_ALERT:
    break;
  }

  if (timerID == -1)
    SetTimer(POLLMS);
  return;
}


struct StopGuideTimerData {
  GuiderAxis axis;
  OSICCD *osiCCD;
};

bool OSICCD::GuideNorth(float duration) {
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Guide call
   *  Some CCD API support pulse guiding directly (i.e. without timers)
   *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
   *  will have to start a timer and then stop it after the 'duration' seconds
   *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
   *  available in INDI 3rd party repository
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to guide due ...");
   *  return false;
   *
   *
   **********************************************************/
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: duration=%f", __PRETTY_FUNCTION__, duration);

  if(osiCamera->guide(GuideDeclination, GuideIncrease) != 0)
  {
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s: guide success, adding timer", __PRETTY_FUNCTION__);
    IEAddTimer(duration, ::GuideTimerCallbackDEC, this);
    return true;
  }
  return false;
}

bool OSICCD::GuideSouth(float duration) {
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Guide call
   *  Some CCD API support pulse guiding directly (i.e. without timers)
   *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
   *  will have to start a timer and then stop it after the 'duration' seconds
   *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
   *  available in INDI 3rd party repository
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to guide due ...");
   *  return false;
   *
   *
   **********************************************************/
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: duration=%f", __PRETTY_FUNCTION__, duration);
  if(osiCamera->guide(GuideDeclination, GuideDecrease) != 0)
  {
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s: guide success, adding timer", __PRETTY_FUNCTION__);
    IEAddTimer(duration, ::GuideTimerCallbackDEC, this);
    return true;
  }
  return false;
}

bool OSICCD::GuideEast(float duration) {
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Guide call
   *  Some CCD API support pulse guiding directly (i.e. without timers)
   *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
   *  will have to start a timer and then stop it after the 'duration' seconds
   *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
   *  available in INDI 3rd party repository
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to guide due ...");
   *  return false;
   *
   *
   **********************************************************/
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: duration=%f", __PRETTY_FUNCTION__, duration);
  if(osiCamera->guide(GuideRightAscension, GuideDecrease) != 0)
  {
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s: guide success, adding timer", __PRETTY_FUNCTION__);
    IEAddTimer(duration, ::GuideTimerCallbackAR, this);
    return true;
  }
  return false;
}

bool OSICCD::GuideWest(float duration) {
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: duration=%f", __PRETTY_FUNCTION__, duration);
  /**********************************************************
   *
   *
   *
   *  IMPORRANT: Put here your CCD Guide call
   *  Some CCD API support pulse guiding directly (i.e. without timers)
   *  Others implement GUIDE_ON and GUIDE_OFF for each direction, and you
   *  will have to start a timer and then stop it after the 'duration' seconds
   *  For an example on timer usage, please refer to indi-sx and indi-gpusb drivers
   *  available in INDI 3rd party repository
   *  If there is an error, report it back to client
   *  e.g.
   *  IDMessage(getDeviceName(), "Error, unable to guide due ...");
   *  return false;
   *
   *
   **********************************************************/
  if(osiCamera->guide(GuideRightAscension, GuideIncrease) != 0)
  {
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s: guide success, adding timer", __PRETTY_FUNCTION__);
    IEAddTimer(duration, ::GuideTimerCallbackAR, this);
    return true;
  }
  return false;
}

void GuideTimerCallbackAR(void *p)
{
   ((OSICCD *) p)->StopGuideAR();
}
void GuideTimerCallbackDEC(void *p)
{
   ((OSICCD *) p)->StopGuideDec();
}

void OSICCD::StopGuideDec()
{
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: stopping guide", __PRETTY_FUNCTION__);
  bool result = osiCamera->guide(GuideDeclination, GuideStop) != 0;
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: stopping guide result: %s", __PRETTY_FUNCTION__, result ? "true":"false");
}
void OSICCD::StopGuideAR()
{
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: stopping guide", __PRETTY_FUNCTION__);
  bool result = osiCamera->guide(GuideDeclination, GuideStop) != 0;
  DEBUGF(INDI::Logger::DBG_DEBUG, "%s: stopping guide result: %s", __PRETTY_FUNCTION__, result ? "true":"false");
}

