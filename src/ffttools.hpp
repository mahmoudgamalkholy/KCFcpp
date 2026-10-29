/* 
Author: Christian Bailer
Contact address: Christian.Bailer@dfki.de 
Department Augmented Vision DFKI 

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are disclaimed.
In no event shall copyright holders or contributors be liable for any direct,
indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/
#pragma once
#include "gem5_vision.hpp"
// #include <math.h>
#include <klib.h>

namespace FFTTools
{
    inline g5v::Matrix fftd(g5v::Matrix img, bool backwards = false)
    {
        g5v::Matrix in = img;
        if (in.channels == 1) {
            in = g5v::Matrix::zeros(img.rows, img.cols, 2);
            for(int i = 0; i < img.rows * img.cols; i++) {
                in.data[i * 2] = img.data[i];
            }
        }

        int R = in.rows;
        int C = in.cols;
        g5v::Matrix out = g5v::Matrix::zeros(R, C, 2);
        g5v::Matrix temp = g5v::Matrix::zeros(R, C, 2);
        float sign = backwards ? 1.0f : -1.0f;      // to insert in the exponetial
        
        for(int r = 0; r < R; r++){
            for(int k = 0; k < C; k++){
                float sum_re = 0.0f, sum_im = 0.0f;
                for(int n = 0; n < C; n++){
                    float angle = sign * 2.0f * 3.14159265359f * k * n / C;
                    float cos_a = cosf(angle);
                    float sin_a = sinf(angle);
                    float val_re = in.at(r, n, 0);
                    float val_im = in.at(r, n, 1);
                    sum_re += val_re * cos_a - val_im * sin_a;
                    sum_im += val_re * sin_a + val_im * cos_a;
                }
                temp.at(r, k, 0) = sum_re;
                temp.at(r, k, 1) = sum_im;
            }
        }
        
        for(int c = 0; c < C; c++){
            for(int k = 0; k < R; k++){
                float sum_re = 0.0f, sum_im = 0.0f;
                for(int n = 0; n < R; n++){
                    float angle = sign * 2.0f * 3.14159265359f * k * n / R;
                    float cos_a = cosf(angle);
                    float sin_a = sinf(angle);
                    float val_re = temp.at(n, c, 0);
                    float val_im = temp.at(n, c, 1);
                    sum_re += val_re * cos_a - val_im * sin_a;
                    sum_im += val_re * sin_a + val_im * cos_a;
                }
                if (backwards) {
                    out.at(k, c, 0) = sum_re / (R * C); // Scale for IFFT
                    out.at(k, c, 1) = sum_im / (R * C);
                } else {
                    out.at(k, c, 0) = sum_re;
                    out.at(k, c, 1) = sum_im;
                }
            }
        }
        return out;
    }
    // real extraction
    inline g5v::Matrix real(g5v::Matrix img) {
        g5v::Matrix res(img.rows, img.cols, 1);
        for(int i = 0; i < img.rows * img.cols; i++) res.data[i] = img.data[i * 2];
        return res;
    }
    // instead of cv::mulSpectrums
    inline g5v::Matrix complexMultiplication(g5v::Matrix a, g5v::Matrix b, bool conj_b = false) {
        g5v::Matrix res(a.rows, a.cols, 2);
        for(int i = 0; i < a.rows * a.cols; i++) {
            float a_re = a.data[i * 2];
            float a_im = a.data[i * 2 + 1];
            float b_re = b.data[i * 2];
            float b_im = b.data[i * 2 + 1] * (conj_b ? -1.0f : 1.0f);
            
            res.data[i * 2] = a_re * b_re - a_im * b_im;
            res.data[i * 2 + 1] = a_re * b_im + a_im * b_re;
        }
        return res;
    }

    inline g5v::Matrix complexDivision(g5v::Matrix a, g5v::Matrix b) {
        g5v::Matrix res(a.rows, a.cols, 2);
        for(int i = 0; i < a.rows * a.cols; i++) {
            float a_re = a.data[i * 2];
            float a_im = a.data[i * 2 + 1];
            float b_re = b.data[i * 2];
            float b_im = b.data[i * 2 + 1];
            float divisor = b_re * b_re + b_im * b_im + 1e-6f; // Avoid divid by 0
            
            res.data[i * 2] = (a_re * b_re + a_im * b_im) / divisor;
            res.data[i * 2 + 1] = (a_im * b_re - a_re * b_im) / divisor;
        }
        return res;
    }
    // get the low frequency in the middle
    inline void rearrange(g5v::Matrix &img) {
        int cx = img.cols / 2;
        int cy = img.rows / 2;
        for(int r = 0; r < cy; r++){
            for(int c = 0; c < cx; c++){
                for(int ch = 0; ch < img.channels; ch++) {
                    float tmp1 = img.at(r, c, ch);
                    img.at(r, c, ch) = img.at(r + cy, c + cx, ch);
                    img.at(r + cy, c + cx, ch) = tmp1;

                    float tmp2 = img.at(r, c + cx, ch);
                    img.at(r, c + cx, ch) = img.at(r + cy, c, ch);
                    img.at(r + cy, c, ch) = tmp2;
                }
            }
        }
    }
}