#include <atomic>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <limits.h>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <mutex>

#include <libcamera/controls.h>
#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>
#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>
#include <libcamera/formats.h>
#include <libcamera/transform.h>

#include "flat_libcamera.h"

using namespace libcamera;

typedef struct {
    uint8_t *imageData;
    uint32_t size;
    uint64_t request;
} LibcameraOutData;


class LibCamera {
    public:
        LibCamera(){};
        ~LibCamera(){};
        
        //int initCamera(int width, int height, PixelFormat format, int buffercount, int rotation);

        int initCamera(camera_mode camera_modes[]);

        //int startCamera();
        int startCamera(int mode);

        bool readFrame(LibcameraOutData *frameData);
        void returnFrameBuffer(LibcameraOutData frameData);

        void set(ControlList controls);
        void stopCamera();
        void closeCamera();
    private:
        //int startCapture();
        int startCapture(int mode);
        int queueRequest(Request *request);
        void requestComplete(Request *request);
        void processRequest(Request *request);

        unsigned int cameraIndex_;
	    uint64_t last_;
        std::unique_ptr<CameraManager> cm;
        std::shared_ptr<Camera> camera_;
        bool camera_acquired_ = false;
        bool camera_started_ = false;

	// ## Note: now maintains multiple configs and controls 
	std::vector<std::unique_ptr<CameraConfiguration>> configs_;
        std::vector<ControlList> controls_;

        std::unique_ptr<FrameBufferAllocator> allocator_;

	// ## Now trying to keep a vector of each mode's vector of requests.
        // std::vector<std::vector<std::unique_ptr<Request>>> requests_;
	// ## no, that's not working yet. Back to one set.
        std::vector<std::unique_ptr<Request>> requests_;

        // std::map<std::string, Stream *> stream_;
        std::map<int, std::pair<void *, unsigned int>> mappedBuffers_;

        std::queue<Request *> requestQueue;

        std::mutex control_mutex_;
        std::mutex camera_stop_mutex_;
        std::mutex free_requests_mutex_;
};
