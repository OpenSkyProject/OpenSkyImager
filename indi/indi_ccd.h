/*
 Generic CCD
 CCD Template for INDI Developers
 Copyright (C) 2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#ifndef GENERIC_CCD_H
#define GENERIC_CCD_H

#include <indiccd.h>
#include <iostream>
#include <mutex>
#include <chrono>

using namespace std;
class OSICamera;

void GuideTimerCallbackAR(void *p);
void GuideTimerCallbackDEC(void *p);
void OSIExposureCallback(void *p);

class OSICCD: public INDI::CCD {
public:

  OSICCD(const std::string &name);
  virtual ~OSICCD();

  const char *getDefaultName();

  bool initProperties();
  void ISGetProperties(const char *dev);
  bool updateProperties();

  bool Connect();
  bool Disconnect();

  int  SetTemperature(double temperature);
  bool StartExposure(float duration);
  bool AbortExposure();

  virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
  virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
  void StopGuideDec();
  void StopGuideAR();
protected:
  void TimerHit();
  virtual bool UpdateCCDFrame(int x, int y, int w, int h);
  virtual bool UpdateCCDBin(int binx, int biny);
  virtual void addFITSKeywords(fitsfile *fptr, CCDChip *targetChip);
  virtual bool UpdateCCDFrameType(CCDChip::CCD_FRAME fType);

  // Guide Port
  virtual bool GuideNorth(float);
  virtual bool GuideSouth(float);
  virtual bool GuideEast(float);
  virtual bool GuideWest(float);
private:
  std::unique_ptr<OSICamera> osiCamera;
  std::string osiName;

  ISwitch ResetS[1];
  ISwitchVectorProperty ResetSP;
  
  std::vector<ISwitch> DevicesS;
  ISwitchVectorProperty DevicesSP;
  
  INumberVectorProperty GainNP;
  INumber GainN[1];
  INumberVectorProperty SpeedNP;
  INumber SpeedN[1];
  INumberVectorProperty ModeNP;
  INumber ModeN[1];

  double ccdTemp;
  double minDuration;
  unsigned short *imageBuffer;

  int timerID;

  CCDChip::CCD_FRAME imageFrameType;

  struct timeval ExpStart;

  float TemperatureRequest;

  float CalcTimeLeft();
  bool grabImage();
  bool setupParams();
  void resetFrame();

  bool sim;
   std::chrono::time_point<std::chrono::steady_clock> exposureStarted;

  friend void ::ISGetProperties(const char *dev);
  friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
  friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
  friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
  friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n);
  friend void ::OSIExposureCallback(void *p);
  void exposureCompleted();
  int exposureTimerId;
};

#endif // GENERIC_CCD_H
