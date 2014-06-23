#include <iostream>
#include "osicamera.h"
extern "C" {
#include "imgFitsio.h"
//#include "imgCamio.h"
}
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <iomanip>
#include <map>
#include <sstream>

using namespace std;
namespace po = boost::program_options;

int main(int argc, char **argv)
{
  imgfit_init();
  shared_ptr<OSICameraRAII> driver(new OSICameraRAII);
  po::options_description generic("Generic Options");
  generic.add_options()
      ("help", "produce help message")
  ;
  po::options_description actions("Actions");
  actions.add_options()
      ("list-drivers,L", "list all available camera drivers")
      ("list-cameras,l", "list connected cameras")
      ("information,i", "print camera information")
      ("save,s", po::value<int>()->implicit_value(1), "save a number of images")
      ("fps-test,f", po::value<int>()->implicit_value(60), "fps test. Argument is the number of seconds to run the test.")
  ;
  po::options_description cameraSettings("Camera Settings");
  cameraSettings.add_options()
      ("driver,d", po::value<string>(), "camera driver to use (default: autodetect)")
      ("gain,g", po::value<int>()->default_value(10), "gain")
      ("mode,m", po::value<int>()->default_value(30), "mode (aka USB Limit, on some cameras)")
      ("usb-speed,u", po::value<int>()->default_value(0), "USB Speed (0=slow, 1=fast")
      ("width,w", po::value<int>()->default_value(0), "resolution width")
      ("height,h", po::value<int>()->default_value(0), "resolution height")
      ("bpp,b", po::value<int>()->default_value(8), "bits per pixel (8, 12, 16)")
      ("Bpp,B", po::value<int>()->default_value(1), "bytes per pixel (1=8 bit, 2=16 bit)")
      ("exposure,e", po::value<int>()->default_value(100), "exposure time, in milliseconds")
  ;
  po::options_description outputSettings("Output Settings");
  outputSettings.add_options()
    ("directory", po::value<string>(), "fits output directory (default: current directory)")
  ;
  po::options_description cmdline_options;
  cmdline_options.add(generic).add(actions).add(cameraSettings).add(outputSettings);
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
  po::notify(vm);
  
  
  auto help = [=,&cmdline_options] {
    cout << "Usage: " << argv[0] << " [options]" << endl;
    cout << cmdline_options << endl;
  };

  if(vm.count("help") || (!vm.count("list-drivers") && ! vm.count("list-cameras") && ! vm.count("information") && ! vm.count("save") && ! vm.count("fps-test") ) ) {
    help();
    return 0;
  }
  
  if(vm.count("list-drivers")) {
    cout << "Available drivers: ";
    string separator;
    for(auto d: driver->allCameras()) {
      cout << separator << d;
      separator = ", ";
    }
    cout << endl;
    return 0;
  }
  
  auto connectedCameras = driver->connectedCameras();
  if(connectedCameras.size() ==  0) {
    cerr << "Error: no cameras found" << endl;
    return 1;
  }
  if(vm.count("list-cameras")) {
    cout << "Connected cameras: ";
    string sep;
    for(auto camera: connectedCameras) {
      cout << sep << camera;
      sep = ", ";
    }
    cout << endl;
    return 0;
  }
  string cameraModel = vm.count("driver") ? boost::any_cast<string>(vm["driver"].value()) : connectedCameras[0];
  if(find(begin(connectedCameras), end(connectedCameras), cameraModel) == end(connectedCameras)) {
    cerr << "Error: camera model '" << cameraModel << "' unknown or not connected" << endl;
    return 1;
  }
  cout << "Found camera: " << cameraModel << endl;
  OSICamera camera(driver);
  try {
    camera.connect(cameraModel);
  } catch(std::exception &e) {
    cerr << "Error connecting to " << cameraModel << ": " << e.what() << endl;
    return 1;
  }
  cout << "connected to " << cameraModel << endl;
  OSICamera::Resolution resolution = camera.supportedResolutions()[0];
  if(boost::any_cast<int>(vm["width"].value()) > 0 || boost::any_cast<int>(vm["height"].value()) > 0) {
    resolution.width = boost::any_cast<int>(vm["width"].value());
    resolution.height = boost::any_cast<int>(vm["height"].value());
  }
  resolution.bitsPerPixel = static_cast<OSICamera::Resolution::BitsPerPixel>(boost::any_cast<int>(vm["bpp"].value()));
  resolution.bytesPerPixel = boost::any_cast<int>(vm["Bpp"].value()) == 1 ? OSICamera::Resolution::_8bit : OSICamera::Resolution::_16bit;
  camera.setResolution(resolution);
  camera.gain(boost::any_cast<int>(vm["gain"].value()));
  camera.mode(boost::any_cast<int>(vm["mode"].value()));
  camera.speed(boost::any_cast<int>(vm["usb-speed"].value()));
  camera.exposure(boost::any_cast<int>(vm["exposure"].value()));
  
  if(vm.count("information")) {
    cout << "Camera resolution: " << camera.resolution().width << "x" << camera.resolution().height 
      << ", bitsPerPixel: " << camera.resolution().bitsPerPixel << ", bytesPerPixel: " << camera.resolution().bytesPerPixel << endl;
    cout << "Pixels size: " << camera.pixelSize().x << "x" << camera.pixelSize().y << " microns" << endl;
    cout << "Temperature: " << camera.tec().celsius << "°" << endl;
    cout << "USB Speed [0=slow, 1=fast]: " << camera.speed() << endl;
    cout << "Camera mode (model dependant): " << camera.mode() << endl;
    cout << "Gain: " << camera.gain() << endl;
    cout << "Exposure: " << camera.exposure() << endl;
  }
  
  if(vm.count("save")) {
    boost::filesystem::path outputDirectory = vm.count("directory") ? boost::any_cast<string>(vm["directory"].value() ) : boost::filesystem::current_path();
    try {
      boost::filesystem::create_directories(outputDirectory);
    } catch(exception &e) {
      cerr << "Error creating output directory " << outputDirectory << ": " << e.what() << endl;
      help();
      return 1;
    }
    if(! boost::filesystem::is_directory(outputDirectory)) {
      help();
      return 1;
    }
    int count = boost::any_cast<int>(vm["save"].value());
    cout << "Saving " << count << " images to directory " << outputDirectory << endl;
    for(int i=0; i<count; i++) {
      auto now = boost::posix_time::microsec_clock::local_time();
      boost::filesystem::path filename = outputDirectory / (boost::posix_time::to_iso_extended_string(now) + ".fit");
      try {
        camera.shoot();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(camera.exposureRemaining()));

        imgfit_set_width(camera.resolution().width);
        imgfit_set_height(camera.resolution().height);
        imgfit_set_bytepix(camera.resolution().bytesPerPixel);
        auto data = camera.readData();
        imgfit_set_data( data );
        fit_rowhdr header;
        imgfit_save_file((char*) filename.string().c_str(), &header, 0);
        cout << "Saved " << filename << endl;
      } catch(exception &e) {
        cerr << "Error during image acquisition: " << e.what() << endl;
      }
    }
    return 0;
  }
  
  if(vm.count("fps-test")) {
    int seconds = boost::any_cast<int>(vm["fps-test"].value());
    cout << "Starting fps test for " << seconds << " seconds." << endl;
    uint64_t frames = 0;
    uint64_t lastFPS = 0;
    bool keepGoing = true;
    auto start = boost::chrono::system_clock::now();
    auto firstStart = start;
    while(keepGoing) {
      try {
        camera.shoot();
        camera.readData();
        frames++;
        if(lastFPS == 0 || frames/lastFPS > 2) {
          boost::chrono::duration<double> elapsed = boost::chrono::system_clock::now() - start;
          lastFPS = static_cast<double>(frames) / elapsed.count();
          cout << setw(6) << frames << " frames in " << setw(6) << fixed << setprecision(3) << elapsed.count() << " seconds: " << setw(4) << lastFPS << endl;
          start = boost::chrono::system_clock::now();
          boost::chrono::duration<double> elapsedSinceFirstStart  = start - firstStart;
          keepGoing = elapsedSinceFirstStart.count() < seconds;
          frames = 0;
        }
      } catch(exception &e) {
        cerr << "Wrong frame: " << e.what() << endl;
      }
    }
  }
  
  return 0;
}
