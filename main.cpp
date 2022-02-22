#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include "LibCamera.h"
#include "flat_libcamera.h"

// xres, yres, buf#, s_role, p_format, color_s, mfps, bright, cont, gain, time, wb_blue, wb_red, rot, ... 

camera_mode camera_modes[] = {
	{1152, 864, 4, LSR_VIEWFINDER, LPF_RGB888, LCS_REC709, 60, 1100, 1050, 1015, 40000, 1205, 1210, 0},  
	{640, 480, 3, LSR_VIDEO_RECORDING, LPF_RGB888, LCS_REC709, 60, 1101, 1051, 1014, 40000, 1204, 1211, 0},  
	{800, 600, 4, LSR_VIDEO_RECORDING, LPF_RGB888, LCS_REC709, 60, 1102, 1052, 1013, 40000, 1203, 1212, 0},  
	{4032, 3040, 1, LSR_STILL_CAPTURE, LPF_BGR888, LCS_REC709, 60, 1005, 1010, 1051, 40000, 1250, 1251, 0},
	{0}
	};

// using namespace cv;

int main()
{
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    time_t start_time = time(0);
    int frame_count = 0, j;
    LibCamera cam;

    char key;

    if ((cam.initCamera(camera_modes)) == 0)
    {
        LibcameraOutData frameData;
        int current_mode = 0;
        cam.startCamera(current_mode);

        for (;;)
	{
	    for (j = 1; j < (int) camera_modes[current_mode].buffer_count; j++)
            {
                while (!cam.readFrame(&frameData)) {}; // get one frame successfully
                cam.returnFrameBuffer(frameData);
                frame_count++;
	    }
            while (!cam.readFrame(&frameData)) {}; // and a fourth.
            frame_count++;

            cv::Mat im(camera_modes[current_mode].height, camera_modes[current_mode].width, CV_8UC3, frameData.imageData);

            cv::imshow("simpler-both", im);

            if ((time(0) - start_time) >= 1)
	    {
                printf("fps: %d\n", frame_count);
                frame_count = 0;
                start_time = time(0);
            }
            cam.returnFrameBuffer(frameData);

	    // ## Note: don't restart camera until returnFrameBuffer is done, otherwise segfault.

            key = cv::waitKey(1);
            if (key == 'q')
                break;
            if (key == '0')
	    {
	        current_mode = 0;
                cam.stopCamera();
		cv::destroyAllWindows();
                cam.startCamera(current_mode);
 	    }
            if (key == '1')
	    {
	        current_mode = 1;
                cam.stopCamera();
		cv::destroyAllWindows();
                cam.startCamera(current_mode);
 	    }
            if (key == '2')
	    {
	        current_mode = 2;
                cam.stopCamera();
		cv::destroyAllWindows();
                cam.startCamera(current_mode);
 	    }
            if (key == 'c' or key == 'C')
	    {
		std::cout << "Capture an image!" << std::endl;
                cam.stopCamera();
                cam.startCamera(3);
                while (!cam.readFrame(&frameData)) {}; // get one frame successfully
		std::cout << "Got an image!" << std::endl;
                cam.returnFrameBuffer(frameData);
                cam.stopCamera();
                cam.startCamera(current_mode);
 	    }

        }
        cv::destroyAllWindows();
        cam.stopCamera();
    }
    cam.closeCamera();
    return 0;
}
