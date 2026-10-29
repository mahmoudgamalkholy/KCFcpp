#pragma once
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace g5v {

    struct Point {
        float x;
        float y;
        Point(float _x = 0, float _y = 0) : x(_x), y(_y) {}
    };

    struct Size {
        int width;
        int height;
        Size(int w = 0, int h = 0) : width(w), height(h) {}
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

        static Matrix zeros(int r, int c, int ch = 1) {
            return Matrix(r, c, ch);
        }

        Matrix operator+(const Matrix& other) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] + other.data[i];
            return res;
        }

        Matrix operator+(float scalar) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] + scalar;
            return res;
        }
        Matrix operator-(float scalar) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] - scalar;
            return res;
        }

        Matrix operator-(const Matrix& other) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] - other.data[i];
            return res;
        }

        Matrix operator*(float scalar) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] * scalar;
            return res;
        }

        friend Matrix operator*(float scalar, const Matrix& m) {
            return m * scalar;
        }

        Matrix mul(const Matrix& other) const {
            Matrix res(rows, cols, channels);
            for(int i = 0; i < rows * cols * channels; i++) res.data[i] = data[i] * other.data[i];
            return res;
        }

        static float sum(const Matrix& m) {
            float s = 0;
            for(int i = 0; i < m.rows * m.cols * m.channels; i++) s += m.data[i];
            return s;
        }

        static Matrix max(const Matrix& m, float val) {
            Matrix res(m.rows, m.cols, m.channels);
            for(int i = 0; i < m.rows * m.cols * m.channels; i++) res.data[i] = std::max(m.data[i], val);
            return res;
        }

        static Matrix exp(const Matrix& m) {
            Matrix res(m.rows, m.cols, m.channels);
            for(int i = 0; i < m.rows * m.cols * m.channels; i++) res.data[i] = std::exp(m.data[i]);
            return res;
        }
        
        // Activated when creation
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
        
        // edit pixel
        inline float& at(int r, int c, int ch = 0) {
            return data[(r * cols + c) * channels + ch];
        }
        // only read pixel
        inline const float& at(int r, int c, int ch = 0) const {
            return data[(r * cols + c) * channels + ch];
        }
    };

}