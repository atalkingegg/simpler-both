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


git clone ###
cd simpler-both
make
simpler-both


# See also 

* atalkingegg/simpler-cam : single shot camera, command line -> PNG file.
* atalkingegg/simpler-vid : simple OpenCV demo as well as nocv-demo, measure video performance without overhead.


