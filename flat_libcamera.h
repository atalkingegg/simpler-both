#ifndef flat_libcamera_h
#define flat_libcamera_h

#include <stdint.h>

// A flat simple version of things needed to configure and setup ONE complete camera mode.
// Why? Eventually we need to load this in or do updates from remote applications.
// Also, need to keep and configure an array of these.

struct camera_mode {
    uint32_t width;
    uint32_t height;
    uint32_t buffer_count;
    uint32_t stream_role; // LSR
    uint32_t pixel_format; // LPF
    uint32_t color_space; // LCS
    uint32_t max_fps;
    uint32_t brightness;	 // normally -1.0 to +1.0, 0.0 default, mapped to 0 to 1000 to 2000
    uint32_t contrast; // normally 0 to 15.99, 1.0 default, mapped to 0 to 1000 to 15999
    uint32_t analog_gain; // normally 1.0 default, mapped to 0 to 1000 to 8000 (4k real, auto mix of digital).
    uint32_t exposure_time;
    uint32_t wb_blue_gain; // normally 1.0 default, mapped to 0 to 1000 to 10000
    uint32_t wb_red_gain; // normally 1.0 default, mapped to 0 to 1000 to 10000
    uint32_t rotation;	// 0 and 180 in camera, otherwise opencv?
};

// libcamera stream roles

#define LSR_VIEWFINDER 1
#define LSR_STILL_CAPTURE 2
#define LSR_RAW 3
#define LSR_VIDEO_RECORDING 4
// could add mixs of two or more later if needed. See Raw plus Still in libcamera-apps

// libcamera pixel formats

#define LPF_RGB888 1
#define LPF_BGR888 2

// libcamera color spaces

#define LCS_REC709 1

#endif /* flat_libcamera_h */
