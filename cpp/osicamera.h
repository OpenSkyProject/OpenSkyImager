/*
 * imgCamio.h
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
#ifndef OSI_CAMERA
#define OSI_CAMERA
#include <memory>
#include <vector>
#include "guider.h"

// A new instance of this class initializes the driver. Deleting it will deinitialize the driver.
class OSICameraRAII {
public:
  OSICameraRAII();
  ~OSICameraRAII();
  std::vector<std::string> connectedCameras() const;
private:
  class Private;
  friend class Private;
  std::unique_ptr<Private> const d;
};

class OSICamera {
public:
  OSICamera(const std::shared_ptr<OSICameraRAII> &driverInitialization);
  ~OSICamera();
  void reset();
  void connect(const std::string &cameraModel);
  bool disconnect();
  void shoot();
  void abort();
  uint8_t *readData();
  
  int gain() const;
  int speed() const;
  int mode() const;
  int exposure() const;
  int exposureRemaining() const;
  bool editMode() const;
  bool connected() const;
  
  struct Resolution {
    int width, height;
    
    enum BytesPerPixel {
      _8bit = 1, _16bit = 2
    };
    BytesPerPixel bytesPerPixel = _8bit;
    enum BitsPerPixel {
      _8 = 8, _12 = 12, _16 = 16
    };
    BitsPerPixel bitsPerPixel = _8;
    Resolution() = default;
    Resolution(int width, int height, int bitsPerPixel = 8);
    bool operator<(const Resolution &other) const;
  };
  
  struct PixelSize {
    double x, y;
  };
  
  PixelSize pixelSize() const;
  
  bool setResolution(const Resolution &resolution);
  Resolution resolution() const;
  // TODO: bpp and bytesPerPixel are not filled
  std::vector<Resolution> supportedResolutions() const;
  
  struct Tec {
    double celsius, millivolts;
    int power;
    bool enabled;
  };
  
  Tec tec() const;
  
  void gain(int newGain);
  void mode(int newMode);
  void speed(int newSpeed);
  void exposure(int milliseconds);
  bool guide(GuiderAxis axis, GuiderMovement movement);
  
private:
  class Private;
  friend class Private;
  std::unique_ptr<Private> const d;
};

#endif
