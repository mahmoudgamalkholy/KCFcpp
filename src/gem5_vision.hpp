#pragma once
#include <iostream>
#include <cstring> 

namespace g5v {

    struct Point {
        float x;
        float y;
        Point(float _x = 0, float _y = 0) : x(_x), y(_y) {}
    };

    struct Rect {
        float x;
        float y;
        float width;
        float height;
        Rect(float _x = 0, float _y = 0, float _w = 0, float _h = 0) 
            : x(_x), y(_y), width(_w), height(_h) {}
    };

    class Matrix {
    public:
        int rows;
        int cols;
        int channels;
        float* data; 

        //  Constructor
        Matrix() : rows(0), cols(0), channels(0), data(nullptr) {}

        // Parameterized Constructor
        Matrix(int r, int c, int ch = 1) : rows(r), cols(c), channels(ch) {
            int total_elements = rows * cols * channels;
            data = new float[total_elements];
            std::memset(data, 0, total_elements * sizeof(float)); 
        }

        Matrix(const Matrix& other) : rows(other.rows), cols(other.cols), channels(other.channels) {
            int total_elements = rows * cols * channels;
            if (total_elements > 0) {
                data = new float[total_elements];
                std::memcpy(data, other.data, total_elements * sizeof(float));
            } else {
                data = nullptr;
            }
        }

        // Destructor
        ~Matrix() {
            if (data != nullptr) {
                delete[] data;
                data = nullptr;
            }
        }

        // Assignment Operator (=)
        Matrix& operator=(const Matrix& other) {
            if (this != &other) { 
                if (data != nullptr) delete[] data;
                
                rows = other.rows;
                cols = other.cols;
                channels = other.channels;
                
                int total_elements = rows * cols * channels;
                if (total_elements > 0) {
                    data = new float[total_elements];
                    std::memcpy(data, other.data, total_elements * sizeof(float));
                } else {
                    data = nullptr;
                }
            }
            return *this;
        }


        inline float& at(int r, int c, int ch = 0) {
            return data[(r * cols + c) * channels + ch];
        }

        inline const float& at(int r, int c, int ch = 0) const {
            return data[(r * cols + c) * channels + ch];
        }
    };

} 