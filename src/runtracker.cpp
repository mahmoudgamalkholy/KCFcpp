#include <iostream>
#include "kcftracker.hpp"
#include "gem5_vision.hpp" 

using namespace std;

void draw_rectangle(g5v::Matrix& img, int x, int y, int w, int h, float val) {
    for (int r = y; r < y + h; ++r) {
        for (int c = x; c < x + w; ++c) {
            if (r >= 0 && r < img.rows && c >= 0 && c < img.cols) {
                for (int ch = 0; ch < img.channels; ++ch) {
                    img.at(r, c, ch) = val;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    bool HOG = true;
    bool FIXEDWINDOW = false;
    bool MULTISCALE = true;
    bool LAB = false;
    
    KCFTracker tracker(HOG, FIXEDWINDOW, MULTISCALE, LAB);

    g5v::Matrix frame = g5v::Matrix::zeros(480, 640, 3); 
    draw_rectangle(frame, 100, 100, 50, 50, 255.0f);

    g5v::Rect roi(100, 100, 50, 50);
    tracker.init(roi, frame);
    
    cout << "Tracker initialized successfully." << endl;

    int num_frames = 10;
    for (int i = 0; i < num_frames; ++i) {
        
        frame = g5v::Matrix::zeros(480, 640, 3);
        draw_rectangle(frame, 100 + i, 100 + i, 50, 50, 255.0f);

        g5v::Rect result = tracker.update(frame);

        cout << "Frame " << i << " | Target Position -> "
             << "x: " << result.x << ", y: " << result.y 
             << ", w: " << result.width << ", h: " << result.height << endl;
    }

    cout << "Simulation completed without errors!" << endl;
    return 0;
}