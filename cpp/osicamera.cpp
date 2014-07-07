/*
 *
 *  Created on: 17.06.2014
 *      Author: Marco Gulino (marco@gulinux.net)
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

#include "osicamera.h"
#include <string>
#include <iostream>
#include <sstream>
#define DECLARE_MAIN
extern "C" {
#include "imgBase.h"
#include "imgCamio.h"
}
#include <fstream>
#include <algorithm>

using namespace std;


class OSICameraRAII::Private {
public:
};

OSICameraRAII::OSICameraRAII() : d(new Private)
{
  static bool initialized = false;
  if(initialized) {
    throw runtime_error("Only one instance of OSICameraRAII can be created");
  }
  initialized = true;
  imgcam_init();
}

vector<string> split(const std::string &src, const std::string &sep, function<bool(const string&)> addIf = [](const string&) {return true; } )
{
  vector<string> strings;
  size_t current;
  size_t next = -1;
  do
  { 
    current = next + 1; 
    next = src.find_first_of( sep, current );
    string currentToken = src.substr( current, next - current );
    if( addIf(currentToken) )
      strings.push_back(currentToken);
  }
  while (next != string::npos);
  return strings;
}

vector<string> OSICameraRAII::connectedCameras() const
{
  return split(imgcam_init_list(0), "|", [](const string &token) { return token != ":0" && token != "None"; } );
}
vector<string> OSICameraRAII::allCameras() const
{
  return split(imgcam_init_list(1), "|", [](const string &token) { return token != ":0" && token != "None"; } );
}

OSICameraRAII::~OSICameraRAII()
{
  imgcam_end();
}

class OSICamera::Private {
public:
  void setIntField(int &field, int value, const string &desc = string{}, bool setEditField = true);
  string deviceName;
};

void OSICamera::Private::setIntField(int &field, int value, const string &desc, bool setEditField )
{
  if(field == value)
    return;
  field = value;
  if(setEditField)
    imgcam_get_expar()->edit = 1;
}

OSICamera::OSICamera(const shared_ptr<OSICameraRAII> &driverInitialization)
  : d(new Private)
{
  if(!driverInitialization)
    throw new runtime_error("Driver must first be initialized!");
}

OSICamera::~OSICamera()
{
  disconnect();
}

void OSICamera::reset()
{
  imgcam_reset();
}

void OSICamera::abort()
{
  int prevEdit = imgcam_get_expar()->edit;
  imgcam_abort();
    imgcam_get_expar()->edit = prevEdit; // TODO: fix this?
}

bool OSICamera::disconnect()
{
  if(!connected())
    return true;
  return imgcam_disconnect() != 0;
}

void OSICamera::connect(const string &cameraModel)
{
  imgcam_set_model(cameraModel.c_str());
  if( imgcam_connect() == 0 || !connected())
    throw runtime_error(imgcam_get_msg());
}

bool OSICamera::editMode() const
{
  return imgcam_get_expar()->edit != 0;
}

uint8_t *OSICamera::readData()
{
  if(imgcam_readout() == 0)
    throw runtime_error(imgcam_get_msg());
  return imgcam_get_data();
}

void OSICamera::shoot()
{
  if(imgcam_shoot() == 0)
    throw runtime_error(imgcam_get_msg() );
}

bool OSICamera::setResolution(const OSICamera::Resolution &resolution)
{
  d->setIntField(imgcam_get_expar()->width, resolution.width, "width");
  d->setIntField(imgcam_get_expar()->height, resolution.height, "height");
  d->setIntField(imgcam_get_expar()->bytepix, resolution.bytesPerPixel);
  d->setIntField(imgcam_get_expar()->bitpix, resolution.bitsPerPixel);
  return true; // TODO: resolution validation
}

OSICamera::Resolution::Resolution(int width, int height, int bitsPerPixel)
  : width(width), height(height)
{
  this->bitsPerPixel = static_cast<BitsPerPixel>(bitsPerPixel);
  this->bytesPerPixel = bitsPerPixel>8 ? _16bit : _8bit;
}


bool OSICamera::Resolution::operator<(const Resolution &other) const
{
  return width*height<other.width*other.height;
}

vector<OSICamera::Resolution> OSICamera::supportedResolutions() const
{
  vector<Resolution> resolutions;
  auto resolutionStrings = split(imgcam_get_camui()->roistr, "|:", [](const string &token){ return token != "0" && token != "Full"; });
  for(auto r: resolutionStrings) {
    auto wxh = split(r, "x");
    Resolution res;
    stringstream s(wxh[0] + " " + wxh[1]);
    s >> res.width >> res.height;
    resolutions.push_back(res);
  }
  sort(resolutions.rbegin(), resolutions.rend());
  return resolutions;
}

OSICamera::Resolution OSICamera::resolution() const
{
  Resolution resolution{ imgcam_get_expar()->width, imgcam_get_expar()->height};
  resolution.bitsPerPixel = static_cast<Resolution::BitsPerPixel>(imgcam_get_expar()->bitpix);
  resolution.bytesPerPixel = static_cast<Resolution::BytesPerPixel>(imgcam_get_expar()->bytepix);
  return resolution;
}

OSICamera::PixelSize OSICamera::pixelSize() const
{
  return {imgcam_get_camui()->pszx, imgcam_get_camui()->pszy };
}

bool OSICamera::connected() const
{
  return imgcam_connected() != 0;
}

int OSICamera::gain() const
{
  return imgcam_get_expar()->gain;
}

int OSICamera::exposure() const
{
  return imgcam_get_expar()->time;
}

int OSICamera::exposureRemaining() const
{
  return imgcam_get_shpar()->wtime;
}

int OSICamera::mode() const
{
  return imgcam_get_expar()->mode;
}

int OSICamera::speed() const
{
  return imgcam_get_expar()->speed;
}

void OSICamera::gain(int newGain)
{
  d->setIntField(imgcam_get_expar()->gain, newGain, "gain");
}

void OSICamera::speed(int newSpeed)
{
  d->setIntField(imgcam_get_expar()->speed, newSpeed, "speed");
}

void OSICamera::mode(int newMode)
{
  d->setIntField(imgcam_get_expar()->mode, newMode, "mode");
}

void OSICamera::exposure(int milliseconds)
{
  d->setIntField(imgcam_get_expar()->time, milliseconds, "exposure");
}

bool OSICamera::guide(GuiderAxis axis, GuiderMovement movement)
{
  return imgcam_guide(axis, movement);
}

OSICamera::Tec OSICamera::tec() const
{
  Tec t;
  int enabled;
  if(imgcam_gettec(&t.celsius, &t.millivolts, &t.power, &enabled) == 0)
    throw runtime_error(imgcam_get_msg() );
  t.enabled = enabled!=0;
  return t;
}