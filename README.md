# simpler-both
framework for rapid video mode switching / image captures mid video stream

A project to test out some libcamera video mode-switching features.

# The Arducam C++ libcamera opencv demo
Heavily modified for multi-mode, based on the C++ code found here...

https://forum.arducam.com/t/how-to-use-code-to-access-libcamera-c-python/2028

* now with Makefile rather than cmake.
* replaced config and control with C++ vectors and a flat loading table.
* controls for each mode applied on start of each config mode.

OpenCV pulling and displaying 1/N buffers so 40FPS = 8 FPS onscreen low-latency.
Buffer size adjustable per mode, as is resolution, and camera controls.

# Pre-Requirements:
* Install bullseye (latest 64-bit version is what I'm using).
* Get network working.
* "sudo apt install libcamera-dev libopencv-core-dev libopencv-highgui-dev"
* Get camera working - "libcamera-hello --verbose"

# To build and run:

git clone https://github.com/atalkingegg/simpler-both.git

cd simpler-both

make

simpler-both

# Runtime Usage
In window, keys "0", "1", "2" = change video resolutions on the fly.

Keys "C" or "c" = switches to 4032x3040 and captures a frame (not saveing to disk just yet), then switches back to video.

# See also 

* atalkingegg/simpler-cam : single shot camera, command line -> PNG file.
* atalkingegg/simpler-vid : simple OpenCV demo as well as nocv-demo, measure video performance without overhead.


