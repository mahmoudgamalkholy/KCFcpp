#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "kcftracker.hpp"
#include "gem5_vision.hpp" 

using namespace std;
using namespace cv;

int main(int argc, char* argv[]) {
    bool HOG = true;
    bool FIXEDWINDOW = false;
    bool MULTISCALE = true;
    bool LAB = false;
    
    KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);

    Mat frame = Mat::zeros(480, 640, CV_8UC3); 
    rectangle(frame, cv::Rect(100, 100, 50, 50), Scalar(255, 255, 255), -1);

    g5v::Rect roi(100, 100, 50, 50);
    tracker.init(roi, frame);
    
    cout << "Tracker initialized successfully." << endl;

    int num_frames = 10;
    for (int i = 0; i < num_frames; ++i) {
        
        frame = Mat::zeros(480, 640, CV_8UC3);
        rectangle(frame, cv::Rect(100 + i, 100 + i, 50, 50), Scalar(255, 255, 255), -1);

        g5v::Rect result = tracker.update(frame);

        cout << "Frame " << i << " | Target Position -> "
             << "x: " << result.x << ", y: " << result.y 
             << ", w: " << result.width << ", h: " << result.height << endl;
    }

    cout << "Simulation completed without errors!" << endl;
    return 0;
}