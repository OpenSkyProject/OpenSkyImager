#include <iostream>
#include "osicamera.h"
extern "C" {
#include "imgFitsio.h"
//#include "imgCamio.h"
}
#include <thread>
#include <iomanip>
#include <map>
#include <sstream>

using namespace std;

int main(int argc, char **argv)
{
  imgfit_init();
  shared_ptr<OSICameraRAII> driver(new OSICameraRAII);
  auto connectedCameras = driver->connectedCameras();
  for(auto cam: connectedCameras)
    cout << "Found camera: " << cam << endl;
  if(connectedCameras.empty())
    return 1;
  OSICamera camera(driver);
  bool connected = camera.connect(connectedCameras[0]);
  cout << "connected to " << connectedCameras[0] << ": " << boolalpha << connected << endl;
  if(!connected) return 1;
  
  camera.setResolution(camera.supportedResolutions()[0]);
  
  auto setResolution = [&camera] {
    auto supportedResolutions = camera.supportedResolutions();
    cout << "Supported resolutions: "; string sep;
    for(auto r: supportedResolutions) {
      cout << sep << r.width << "x" << r.height; sep = "; ";
    }
    cout << endl;
    cout << "Enter width and height: ";
    auto currentResolution = camera.resolution();
    cin >> currentResolution.width >> currentResolution.height;
    for(auto r: supportedResolutions) {
      if(r.width == currentResolution.width && r.height == currentResolution.height) {
	camera.setResolution(currentResolution);
	cout << "Resolution changed to " << currentResolution.width << "x" << currentResolution.height << endl;
	return;
      }
    }
    cerr << "Error: unsupported resolution" << endl;
  };
  
  auto printInformation = [&camera] {
    cout << "Camera resolution: " << camera.resolution().width << "x" << camera.resolution().height 
      << ", bitsPerPixel: " << camera.resolution().bitsPerPixel << ", bytesPerPixel: " << camera.resolution().bytesPerPixel << endl;
    cout << "Pixels size: " << camera.pixelSize().x << "x" << camera.pixelSize().y << " microns" << endl;
    cout << "Temperature: " << camera.tec().celsius << "°" << endl;
    cout << "USB Speed [0=slow, 1=fast]: " << camera.speed() << endl;
    cout << "Camera mode (model dependant): " << camera.mode() << endl;
    cout << "Gain: " << camera.gain() << endl;
  };
  
  auto setUSBSpeed = [&camera] {
    cout << "Type USB Speed [0=slow, 1=fast]: ";
    int speed;
    cin >> speed;
    if(speed != 0 && speed != 1) {
      cerr << "Error! wrong speed" << endl;
      return;
    }
    camera.speed(speed);
  };
  auto setCameraMode = [&camera] {
    cout << "Type Camera Mode: ";
    int mode;
    cin >> mode;
    if(mode < 0 || mode > 255) {
      cerr << "Error: wrong mode" << endl;
      return;
    }
    camera.mode(mode);
  };
  auto setGain = [&camera] {
    cout << "Type Camera Gain: ";
    int gain;
    cin >> gain;
    if(gain < 0 || gain > 255) {
      cerr << "Error: wrong gain" << endl;
      return;
    }
    camera.gain(gain);
  };
  
  string filename;
  auto askAndSetExposure = [&camera] {
    uint64_t exposure;
    cout << "Type image exposure duration (milliseconds): ";
    cin >> exposure;
    if(exposure <= 0) {
      cerr << "Error! wrong exposure duration." << endl;
      return;
    }
    camera.exposure(exposure);
  };
  
  auto setFileName = [&filename] {
    cout << "Enter destination file base name. This will be appended with a sequence number and \".fit\" file extension" << endl;
    cin >> filename;
  };
  uint64_t sequenceNumber = 0;
  auto saveImage = [&camera,&filename,&sequenceNumber, askAndSetExposure] {
    if(filename.empty() ) {
      cerr << "Error! file name is not set." << endl;
      return;
    }
    askAndSetExposure();
    stringstream path;
    path << filename << "-" << setw(10) << setfill('0') << sequenceNumber++ << ".fit";
    cout << "Saving to " << path.str() << endl;
    try {
      auto start = chrono::system_clock::now();
      camera.shoot();
      while(chrono::system_clock::now() - start <= chrono::milliseconds(camera.exposure() ))
	this_thread::sleep_for(chrono::milliseconds(10));
      imgfit_set_width(camera.resolution().width);
      imgfit_set_height(camera.resolution().height);
      imgfit_set_bytepix(camera.resolution().bytesPerPixel);
      auto data = camera.readData();
      imgfit_set_data( data );
      fit_rowhdr header;
      imgfit_save_file((char*) path.str().c_str(), &header, 0);
      cout << "Saved " << path.str() << endl;
    } catch(exception &e) {
      cerr << "Error during image acquisition: " << e.what() << endl;
      return;
    }
  };
  
  auto fpsTest = [&camera, askAndSetExposure] {
    askAndSetExposure();
    bool keepGoing = true;
    string typeQToStop;
    thread t([&keepGoing, &camera] {
      uint64_t frames = 0;
      uint64_t lastFPS = 0;
      auto start = chrono::system_clock::now();
      while(keepGoing) {
	try {
	  camera.shoot();
	  camera.readData();
	  frames++;
	  if(lastFPS == 0 || frames/lastFPS > 2) {
	    chrono::duration<double> elapsed = chrono::system_clock::now() - start;
	    lastFPS = static_cast<double>(frames) / elapsed.count();
	    cout << setw(6) << frames << " frames in " << setw(6) << fixed << setprecision(3) << elapsed.count() << " seconds: " << setw(4) << lastFPS << " FPS; type 'q' and enter to stop." << endl;
	    start = chrono::system_clock::now();
	    frames = 0;
	  }
	} catch(exception &e) {
	  cerr << "Wrong frame: " << e.what() << endl;
	}
      }
    });
    while(typeQToStop != "q")
      cin >> typeQToStop;
    keepGoing = false;
    t.join();
  };
  
  map<string, pair<string,function<void()>>> actions {
    { "q", {"quit", []{} } },
    { "r", {"set resolution", setResolution } },
    { "i", {"print information", printInformation } },
    { "f", {"set file base name", setFileName } },
    { "s", {"save image", saveImage } },
    { "u", {"set USB Speed", setUSBSpeed } },
    { "m", {"set camera mode", setCameraMode } },
    { "g", {"set gain", setGain } },
    { "c", {"reset counter", [&sequenceNumber] { sequenceNumber = 0; } } },
    { "t", {"fps test", fpsTest } },
  };
  string action;
  while(action != "q") {
    cout << "Available actions:" << endl;
    for(auto action: actions)
      cout << "\t" << action.first << ": " << action.second.first << endl;
    cout << endl << "Enter action: ";
    cin >> action;
    if(actions.count(action))
      actions[action].second();
  }
  return 0;
}
