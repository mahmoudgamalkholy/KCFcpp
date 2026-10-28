#pragma once

#include <cmath>
#include <algorithm>
#include "gem5_vision.hpp"

namespace RectTools
{
    inline g5v::Point center(const g5v::Rect &rect)
    {
        return g5v::Point(rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f);
    }

    inline float x2(const g5v::Rect &rect) { return rect.x + rect.width; }
    inline float y2(const g5v::Rect &rect) { return rect.y + rect.height; }

    inline void resize(g5v::Rect &rect, float scalex, float scaley = 0)
    {
        if (!scaley) scaley = scalex;
        rect.x -= rect.width * (scalex - 1.f) / 2.f;
        rect.width *= scalex;
        rect.y -= rect.height * (scaley - 1.f) / 2.f;
        rect.height *= scaley;
    }

    inline void limit(g5v::Rect &rect, g5v::Rect limit)
    {
        if (rect.x + rect.width > limit.x + limit.width) rect.width = (limit.x + limit.width - rect.x);
        if (rect.y + rect.height > limit.y + limit.height) rect.height = (limit.y + limit.height - rect.y);
        if (rect.x < limit.x)
        {
            rect.width -= (limit.x - rect.x);
            rect.x = limit.x;
        }
        if (rect.y < limit.y)
        {
            rect.height -= (limit.y - rect.y);
            rect.y = limit.y;
        }
        if(rect.width < 0) rect.width = 0;
        if(rect.height < 0) rect.height = 0;
    }

    inline void limit(g5v::Rect &rect, float width, float height, float x = 0, float y = 0)
    {
        limit(rect, g5v::Rect(x, y, width, height));
    }

    inline g5v::Rect getBorder(const g5v::Rect &original, g5v::Rect & limited)
    {
        g5v::Rect res;
        res.x = limited.x - original.x;
        res.y = limited.y - original.y;
        res.width = x2(original) - x2(limited);
        res.height = y2(original) - y2(limited);
        return res;
    }

    inline g5v::Matrix subwindow(const g5v::Matrix &in, const g5v::Rect &window)
    {
        int w = (int)window.width;
        int h = (int)window.height;
        int start_x = (int)window.x;
        int start_y = (int)window.y;

        g5v::Matrix res(h, w, in.channels);
        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                int in_r = start_y + r;
                int in_c = start_x + c;
                
                // BORDER_REPLICATE logic
                if (in_r < 0) in_r = 0;
                else if (in_r >= in.rows) in_r = in.rows - 1;
                
                if (in_c < 0) in_c = 0;
                else if (in_c >= in.cols) in_c = in.cols - 1;

                for (int ch = 0; ch < in.channels; ++ch) {
                    res.at(r, c, ch) = in.at(in_r, in_c, ch);
                }
            }
        }
        return res;
    }
    inline g5v::Matrix getGrayImage(const g5v::Matrix& img)
    {
        g5v::Matrix gray(img.rows, img.cols, 1);
        for (int r = 0; r < img.rows; ++r) {
            for (int c = 0; c < img.cols; ++c) {
                if (img.channels >= 3) {
                    float b = img.at(r, c, 0);
                    float g = img.at(r, c, 1);
                    float red = img.at(r, c, 2);
                    gray.at(r, c, 0) = (0.114f * b + 0.587f * g + 0.299f * red) / 255.0f;
                } else {
                    gray.at(r, c, 0) = img.at(r, c, 0) / 255.0f;
                }
            }
        }
        return gray;
    }

    inline g5v::Matrix resizeImage(const g5v::Matrix& src, const g5v::Size& dsize)
    {
        g5v::Matrix dst(dsize.height, dsize.width, src.channels);
        float scale_x = (float)src.cols / dsize.width;
        float scale_y = (float)src.rows / dsize.height;

        for (int r = 0; r < dsize.height; ++r) {
            for (int c = 0; c < dsize.width; ++c) {
                float src_x = c * scale_x;
                float src_y = r * scale_y;
                
                int x1 = std::floor(src_x);
                int y1 = std::floor(src_y);
                int x2 = std::min(x1 + 1, src.cols - 1);
                int y2 = std::min(y1 + 1, src.rows - 1);

                float dx = src_x - x1;
                float dy = src_y - y1;

                for (int ch = 0; ch < src.channels; ++ch) {
                    float p1 = src.at(y1, x1, ch);
                    float p2 = src.at(y1, x2, ch);
                    float p3 = src.at(y2, x1, ch);
                    float p4 = src.at(y2, x2, ch);

                    float val = (1.0f - dx) * (1.0f - dy) * p1 +
                                dx * (1.0f - dy) * p2 +
                                (1.0f - dx) * dy * p3 +
                                dx * dy * p4;
                    dst.at(r, c, ch) = val;
                }
            }
        }
        return dst;
    }
}