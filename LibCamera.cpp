#include "LibCamera.h"
#include "flat_libcamera.h"

int LibCamera::initCamera(camera_mode camera_modes[])
{
    static int run_once = 1;
    int ret;
    cm = std::make_unique<CameraManager>();
    ret = cm->start();
    if (ret)
    {
        std::cerr << "ERROR: Failed to start camera manager: "
                  << ret << std::endl;
        return ret;
    }
    if (cm->cameras().size() == 0)
    {
        std::cerr << "ERROR: No cameras found!" << std::endl;
        return -1;
    }

    std::string cameraId = cm->cameras()[0]->id();

    camera_ = cm->get(cameraId);
    if (!camera_)
    {
        std::cerr << "Camera " << cameraId << " not found" << std::endl;
        return 1;
    }

    if (camera_->acquire())
    {
        std::cerr << "Failed to acquire camera " << cameraId
                  << std::endl;
        return 1;
    }
    camera_acquired_ = true;

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);

    for (int i = 0; camera_modes[i].width != 0; i++)
    {

        std::unique_ptr<CameraConfiguration> config;
        config = camera_->generateConfiguration({ StreamRole::Viewfinder });

        libcamera::PixelFormat format;
        switch(camera_modes[i].pixel_format)
        {
        case LPF_RGB888:
            format = libcamera::formats::RGB888;
            break;
        case LPF_BGR888:
            format = libcamera::formats::BGR888;
            break;
        default:
            std::cout << "ERROR: Unknown format! " << camera_modes[i].pixel_format << std::endl;
            exit(1);
        }
        config->at(0).pixelFormat = format;

        libcamera::Size size(camera_modes[i].width, camera_modes[i].height);
        config->at(0).size = size;

        if (camera_modes[i].buffer_count)
        {
            config->at(0).bufferCount = camera_modes[i].buffer_count;
        }

        Transform transform = Transform::Identity;
        bool ok;
        Transform rot = transformFromRotation(camera_modes[i].rotation, &ok);
        if (!ok)
            throw std::runtime_error("illegal rotation value, Please use 0 or 180");
        transform = rot * transform;
        if (!!(transform & Transform::Transpose))
            throw std::runtime_error("transforms requiring transpose not supported");
        config->transform = transform;

        switch (config->validate())
        {
        case CameraConfiguration::Valid:
            std::cout << "Camera configuration validated" << std::endl;
            break;

        case CameraConfiguration::Adjusted:
            std::cout << "Camera configuration adjusted" << std::endl;
            break;

        case CameraConfiguration::Invalid:
            std::cout << "Camera configuration invalid" << std::endl;
            return 1;
        }

        configs_.push_back(std::move(config));

        ControlList controls;

        int64_t frame_time = 1000000 / camera_modes[i].max_fps;
        controls.set(libcamera::controls::FrameDurationLimits, { frame_time, frame_time });
        float brightness = (float) (camera_modes[i].brightness - 1000.0) / 1000;
        controls.set(libcamera::controls::Brightness, brightness); // -1.0 to 1.0
        float contrast = (float) (camera_modes[i].contrast) / 1000;
        controls.set(libcamera::controls::Contrast, contrast); // 0.0 to 15.99
        float analog_gain = (float) (camera_modes[i].analog_gain) / 1000;
        controls.set(libcamera::controls::AnalogueGain, analog_gain);
        controls.set(libcamera::controls::ExposureTime, camera_modes[i].exposure_time);
        float wb_blue_gain = (float) (camera_modes[i].wb_blue_gain) / 1000;
        float wb_red_gain = (float) (camera_modes[i].wb_red_gain) / 1000;
        float rb_gains[] = { wb_blue_gain, wb_red_gain };
        controls.set(libcamera::controls::ColourGains, rb_gains);

        controls_.push_back(std::move(controls));

        // set up buffers for this mode

        if (run_once)
        {
            run_once = 0;

// Note: you have put the camera into a configuration to set up buffers!

            int ret = camera_->configure(configs_.at(i).get());
            if (ret < 0)
            {
                std::cout << "Failed to configure camera" << std::endl;
                return ret;
            }

            unsigned int nbuffers = UINT_MAX;
            for (StreamConfiguration &cfg : *configs_.at(i).get())
            {
                ret = allocator_->allocate(cfg.stream());
                if (ret < 0)
                {
                    std::cerr << "Can't allocate buffers" << std::endl;
                    return -ENOMEM;
                }

                unsigned int allocated = allocator_->buffers(cfg.stream()).size();
                nbuffers = std::min(nbuffers, allocated);
            }

            for (unsigned int j = 0; j < nbuffers; j++)
            {

                std::unique_ptr<Request> request = camera_->createRequest();
                if (!request)
                {
                    std::cerr << "Can't create request" << std::endl;
                    return -ENOMEM;
                }

                for (StreamConfiguration &cfg : *configs_.at(i).get())
                {
                    Stream *stream = cfg.stream();
                    const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
                                allocator_->buffers(stream);
                    const std::unique_ptr<FrameBuffer> &buffer = buffers[j];

                    ret = request->addBuffer(stream, buffer.get());
                    if (ret < 0) {
                        std::cerr << "Can't set buffer for request"
                                  << std::endl;
                        return ret;
                    }
                    for (const FrameBuffer::Plane &plane : buffer->planes()) {
                        void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED,
                                            plane.fd.get(), 0);
                        mappedBuffers_[plane.fd.get()] =
                            std::make_pair(memory, plane.length);
                    }
                }

                requests_.push_back(std::move(request));
            }

        }
    }


    return 0;
}

int LibCamera::startCamera(int mode)
{
    int ret = camera_->configure(configs_.at(mode).get());
    if (ret < 0)
    {
        std::cout << "Failed to configure camera" << std::endl;
        return ret;
    }

    camera_->requestCompleted.connect(this, &LibCamera::requestComplete);

    ret = camera_->start(&(this->controls_.at(mode)));

    if (ret)
    {
        std::cout << "Failed to start capture" << std::endl;
        return ret;
    }

    camera_started_ = true;

    // only set up the request buffers we need.
    unsigned int buf_count = 0;
    for (std::unique_ptr<Request> &request : requests_)
    {
        if (buf_count++ == configs_[mode]->at(0).bufferCount)
            continue;

        // reset the usage flags
        request->reuse(Request::ReuseBuffers);

        ret = queueRequest(request.get());
        if (ret < 0) {
            std::cerr << "Can't queue request" << std::endl;
            camera_->stop();
            return ret;
        }
    }
    return 0;
}

int LibCamera::queueRequest(Request *request)
{
    std::lock_guard<std::mutex> stop_lock(camera_stop_mutex_);
    if (!camera_started_)
        return -1;
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        // ## Note: What is this doing, and how do we move it to multi-mode?
        // request->controls() = std::move(controls_);
    }
    return camera_->queueRequest(request);
}

void LibCamera::requestComplete(Request *request)
{
    if (request->status() == Request::RequestCancelled)
        return;
    processRequest(request);
}

void LibCamera::processRequest(Request *request)
{
    std::lock_guard<std::mutex> lock(free_requests_mutex_);
    requestQueue.push(request);
}

void LibCamera::returnFrameBuffer(LibcameraOutData frameData)
{
    uint64_t request = frameData.request;
    Request * req = (Request *)request;
    req->reuse(Request::ReuseBuffers);
    queueRequest(req);
}

bool LibCamera::readFrame(LibcameraOutData *frameData)
{
    std::lock_guard<std::mutex> lock(free_requests_mutex_);
    if (!requestQueue.empty()) {
        Request *request = this->requestQueue.front();

        const Request::BufferMap &buffers = request->buffers();
        for (auto it = buffers.begin(); it != buffers.end(); ++it) {
            FrameBuffer *buffer = it->second;
            for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
                const FrameBuffer::Plane &plane = buffer->planes()[i];
                const FrameMetadata::Plane &meta = buffer->metadata().planes()[i];

                void *data = mappedBuffers_[plane.fd.get()].first;
                unsigned int length = std::min(meta.bytesused, plane.length);
                //std::cout << "DEBUG: bytes vs length: " << meta.bytesused << ", " << plane.length << std::endl;

                frameData->size = length;
                frameData->imageData = (uint8_t *) data;
            }
        }
        this->requestQueue.pop();
        frameData->request = (uint64_t) request;
        return true;
    } else {
        Request *request = nullptr;
        frameData->request = (uint64_t) request;
        return false;
    }
}

void LibCamera::stopCamera()
{
    if (camera_) {
        {
            std::lock_guard<std::mutex> lock(camera_stop_mutex_);
            if (camera_started_) {
                if (camera_->stop())
                    throw std::runtime_error("failed to stop camera");
                camera_started_ = false;
            }
        }
        if (camera_started_) {
            if (camera_->stop())
                throw std::runtime_error("failed to stop camera");
            camera_started_ = false;
        }
        camera_->requestCompleted.disconnect(this, &LibCamera::requestComplete);
    }
    while (!requestQueue.empty())
        requestQueue.pop();

}

void LibCamera::closeCamera()
{
    requests_.clear();
    allocator_.reset();
    controls_.clear();
    if (camera_acquired_)
        camera_->release();
    camera_acquired_ = false;
    camera_.reset();
    cm.reset();
}
