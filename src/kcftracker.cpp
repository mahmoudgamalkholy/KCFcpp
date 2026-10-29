/*

Tracker based on Kernelized Correlation Filter (KCF) [1] and Circulant Structure with Kernels (CSK) [2].
CSK is implemented by using raw gray level features, since it is a single-channel filter.
KCF is implemented by using HOG features (the default), since it extends CSK to multiple channels.

[1] J. F. Henriques, R. Caseiro, P. Martins, J. Batista,
"High-Speed Tracking with Kernelized Correlation Filters", TPAMI 2015.

[2] J. F. Henriques, R. Caseiro, P. Martins, J. Batista,
"Exploiting the Circulant Structure of Tracking-by-detection with Kernels", ECCV 2012.

Authors: Joao Faro, Christian Bailer, Joao F. Henriques
Contacts: joaopfaro@gmail.com, Christian.Bailer@dfki.de, henriques@isr.uc.pt
Institute of Systems and Robotics - University of Coimbra / Department Augmented Vision DFKI


Constructor parameters, all boolean:
    hog: use HOG features (default), otherwise use raw pixels
    fixed_window: fix window size (default), otherwise use ROI size (slower but more accurate)
    multiscale: use multi-scale tracking (default; cannot be used with fixed_window = true)

Default values are set for all properties of the tracker depending on the above choices.
Their values can be customized further before calling init():
    interp_factor: linear interpolation factor for adaptation
    sigma: gaussian kernel bandwidth
    lambda: regularization
    cell_size: HOG cell size
    padding: area surrounding the target, relative to its size
    output_sigma_factor: bandwidth of gaussian target
    template_size: template size in pixels, 0 to use ROI size
    scale_step: scale step for multi-scale estimation, 1 to disable it
    scale_weight: to downweight detection scores of other scales for added stability

For speed, the value (template_size/cell_size) should be a power of 2 or a product of small prime numbers.

Inputs to init():
   image is the initial frame.
   roi is a g5v::Rect with the target positions in the initial frame

Inputs to update():
   image is the current frame.

Outputs of update():
   g5v::Rect with target positions for the current frame


By downloading, copying, installing or using the software you agree to this license.
If you do not agree to this license, do not download, install,
copy or use the software.


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

#ifndef _KCFTRACKER_HEADERS
#include "kcftracker.hpp"
#include "ffttools.hpp"
#include "recttools.hpp"
#include "fhog.hpp"
#include "labdata.hpp"
#endif

// Constructor
KCFTracker::KCFTracker(bool hog, bool fixed_window, bool multiscale, bool lab)
{

    // Parameters equal in all cases
    lambda = 0.0001;
    padding = 2.5; 
    //output_sigma_factor = 0.1;
    output_sigma_factor = 0.125;


    if (hog) {    // HOG
        // VOT
        interp_factor = 0.012;
        sigma = 0.6; 
        // TPAMI
        //interp_factor = 0.02;
        //sigma = 0.5; 
        cell_size = 4;
        _hogfeatures = true;

        if (lab) {
            interp_factor = 0.005;
            sigma = 0.4; 
            //output_sigma_factor = 0.025;
            output_sigma_factor = 0.1;

            _labfeatures = true;
            _labCentroids = g5v::Matrix::zeros(nClusters, 3, 1);
            for(int i = 0; i < nClusters; i++) {
                _labCentroids.at(i, 0, 0) = data[i][0];
                _labCentroids.at(i, 1, 0) = data[i][1];
                _labCentroids.at(i, 2, 0) = data[i][2];
            }
            cell_sizeQ = cell_size*cell_size;
        }
        else{
            _labfeatures = false;
        }
    }
    else {   // RAW
        interp_factor = 0.075;
        sigma = 0.2; 
        cell_size = 1;
        _hogfeatures = false;

        if (lab) {
            printf("Lab features are only used with HOG features.\n");
            _labfeatures = false;
        }
    }


    if (multiscale) { // multiscale
        template_size = 96;
        //template_size = 100;
        scale_step = 1.05;
        scale_weight = 0.95;
        if (!fixed_window) {
            //printf("Multiscale does not support non-fixed window.\n");
            fixed_window = true;
        }
    }
    else if (fixed_window) {  // fit correction without multiscale
        template_size = 96;
        //template_size = 100;
        scale_step = 1;
    }
    else {
        template_size = 1;
        scale_step = 1;
    }
}

// Initialize tracker 
void KCFTracker::init(const g5v::Rect &roi, g5v::Matrix image)
{
    _roi = roi;
    // assert(roi.width >= 0 && roi.height >= 0);
    _tmpl = getFeatures(image, 1);
    _prob = createGaussianPeak(size_patch[0], size_patch[1]);
    _alphaf = g5v::Matrix::zeros(size_patch[0], size_patch[1], 2);
    //_num = g5v::Matrix(size_patch[0], size_patch[1], CV_32FC2, float(0));
    //_den = g5v::Matrix(size_patch[0], size_patch[1], CV_32FC2, float(0));
    train(_tmpl, 1.0); // train with initial frame
 }
// Update position based on the new frame
g5v::Rect KCFTracker::update(g5v::Matrix image)
{
    if (_roi.x + _roi.width <= 0) _roi.x = -_roi.width + 1;
    if (_roi.y + _roi.height <= 0) _roi.y = -_roi.height + 1;
    if (_roi.x >= image.cols - 1) _roi.x = image.cols - 2;
    if (_roi.y >= image.rows - 1) _roi.y = image.rows - 2;

    float cx = _roi.x + _roi.width / 2.0f;
    float cy = _roi.y + _roi.height / 2.0f;


    float peak_value;
    g5v::Point res = detect(_tmpl, getFeatures(image, 0, 1.0f), peak_value);

    if (scale_step != 1) {
        // Test at a smaller _scale
        float new_peak_value;
        g5v::Point new_res = detect(_tmpl, getFeatures(image, 0, 1.0f / scale_step), new_peak_value);

        if (scale_weight * new_peak_value > peak_value) {
            res = new_res;
            peak_value = new_peak_value;
            _scale /= scale_step;
            _roi.width /= scale_step;
            _roi.height /= scale_step;
        }

        // Test at a bigger _scale
        new_res = detect(_tmpl, getFeatures(image, 0, scale_step), new_peak_value);

        if (scale_weight * new_peak_value > peak_value) {
            res = new_res;
            peak_value = new_peak_value;
            _scale *= scale_step;
            _roi.width *= scale_step;
            _roi.height *= scale_step;
        }
    }

    // Adjust by cell size and _scale
    _roi.x = cx - _roi.width / 2.0f + ((float) res.x * cell_size * _scale);
    _roi.y = cy - _roi.height / 2.0f + ((float) res.y * cell_size * _scale);

    if (_roi.x >= image.cols - 1) _roi.x = image.cols - 1;
    if (_roi.y >= image.rows - 1) _roi.y = image.rows - 1;
    if (_roi.x + _roi.width <= 0) _roi.x = -_roi.width + 2;
    if (_roi.y + _roi.height <= 0) _roi.y = -_roi.height + 2;

    // assert(_roi.width >= 0 && _roi.height >= 0);
    g5v::Matrix x = getFeatures(image, 0);
    train(x, interp_factor);

    return _roi;
}


// Detect object in the current frame.
// Z is the template 
// X is the feature
g5v::Point KCFTracker::detect(g5v::Matrix z, g5v::Matrix x, float &peak_value)
{
    using namespace FFTTools;

    g5v::Matrix k = gaussianCorrelation(x, z);   // Kxz
    g5v::Matrix res = (real(fftd(complexMultiplication(_alphaf, fftd(k)), true)));  // eq.22

    //minMaxLoc only accepts doubles for the peak, and integer points for the coordinates
    // g5v::Point2i pi;
    double pv = -1e10;
    int pi_x = 0, pi_y = 0;
    for (int r = 0; r < res.rows; r++) {
        for (int c = 0; c < res.cols; c++) {
            float val = res.at(r, c);
            if (val > pv) {
                pv = val;
                pi_x = c;
                pi_y = r;
            }
        }
    }
    peak_value = (float) pv;

    // subpixel peak estimation
    g5v::Point p((float)pi_x, (float)pi_y);

    if (pi_x > 0 && pi_x < res.cols - 1) {
        p.x += subPixelPeak(res.at(pi_y, pi_x - 1), peak_value, res.at(pi_y, pi_x + 1));
    }

    if (pi_y > 0 && pi_y < res.rows - 1) {
        p.y += subPixelPeak(res.at(pi_y - 1, pi_x), peak_value, res.at(pi_y + 1, pi_x));
    }

    p.x -= (res.cols) / 2.0f;
    p.y -= (res.rows) / 2.0f;

    return p;
}

// train tracker with a single image
void KCFTracker::train(g5v::Matrix x, float train_interp_factor)
{
    using namespace FFTTools;

    g5v::Matrix k = gaussianCorrelation(x, x);
    g5v::Matrix alphaf = complexDivision(_prob, (fftd(k) + lambda));
    
    _tmpl = (1.0f - train_interp_factor) * _tmpl + train_interp_factor * x;
    _alphaf = (1.0f - train_interp_factor) * _alphaf + train_interp_factor * alphaf;
}


// Evaluates a Gaussian kernel with bandwidth SIGMA for all relative shifts between input images X and Y, which must both be MxN. They must    also be periodic (ie., pre-processed with a cosine window).
g5v::Matrix KCFTracker::gaussianCorrelation(g5v::Matrix x1, g5v::Matrix x2)
{
    using namespace FFTTools;
    g5v::Matrix c = g5v::Matrix::zeros(size_patch[0], size_patch[1], 1);

    if (_hogfeatures) {
        for (int f = 0; f < size_patch[2]; f++) {
            g5v::Matrix x1aux = g5v::Matrix::zeros(size_patch[0], size_patch[1], 1);
            g5v::Matrix x2aux = g5v::Matrix::zeros(size_patch[0], size_patch[1], 1);
            
            for (int r = 0; r < size_patch[0]; r++) {
                for (int col = 0; col < size_patch[1]; col++) {
                    int idx = r * size_patch[1] + col;
                    x1aux.at(r, col, 0) = x1.at(f, idx, 0); 
                    x2aux.at(r, col, 0) = x2.at(f, idx, 0);
                }
            }
            
            g5v::Matrix X1 = fftd(x1aux);
            g5v::Matrix X2 = fftd(x2aux);
            g5v::Matrix caux = complexMultiplication(X1, X2, true); // true = مرافق (Conjugate)
            caux = fftd(caux, true); // IFFT
            rearrange(caux);
            c = c + real(caux);
        }
    }
    else {
        g5v::Matrix X1 = fftd(x1);
        g5v::Matrix X2 = fftd(x2);
        g5v::Matrix caux = complexMultiplication(X1, X2, true);
        c = fftd(caux, true);
        rearrange(c);
        c = real(c);
    }

    float sum_x1 = g5v::Matrix::sum(x1.mul(x1));
    float sum_x2 = g5v::Matrix::sum(x2.mul(x2));
    
    // d = max( (sum(x1*x1) + sum(x2*x2) - 2*c) / (w*h*d), 0 )
    g5v::Matrix d = g5v::Matrix::max((c * -2.0f) + (sum_x1 + sum_x2), 0.0f);
    d = d * (1.0f / (size_patch[0] * size_patch[1] * size_patch[2]));

    g5v::Matrix k = g5v::Matrix::exp(d * (-1.0f / (sigma * sigma)));
    return k;
}

// Create Gaussian Peak. Function called only in the first frame.
// First frame for object detection (Frequency response from the image) 
g5v::Matrix KCFTracker::createGaussianPeak(int sizey, int sizex)
{
    g5v::Matrix res(sizey, sizex, 1);

    int syh = (sizey) / 2;
    int sxh = (sizex) / 2;

    float output_sigma = std::sqrt((float) sizex * sizey) / padding * output_sigma_factor;
    float mult = -0.5f / (output_sigma * output_sigma);

    for (int i = 0; i < sizey; i++) {
        for (int j = 0; j < sizex; j++) {
            int ih = i - syh;
            int jh = j - sxh;
            res.at(i, j) = std::exp(mult * (float) (ih * ih + jh * jh));
        }
    }
    return FFTTools::fftd(res);
}
// Obtain sub-window from image, with replication-padding and extract features
g5v::Matrix KCFTracker::getFeatures(const g5v::Matrix & image, bool inithann, float scale_adjust)
{
    g5v::Rect extracted_roi;
    float cx = _roi.x + _roi.width / 2;
    float cy = _roi.y + _roi.height / 2;

    if (inithann) {
        int padded_w = _roi.width * padding;
        int padded_h = _roi.height * padding;
        
        if (template_size > 1) { 
            if (padded_w >= padded_h)
                _scale = padded_w / (float) template_size;
            else
                _scale = padded_h / (float) template_size;
            _tmpl_sz.width = padded_w / _scale;
            _tmpl_sz.height = padded_h / _scale;
        }
        else {
            _tmpl_sz.width = padded_w;
            _tmpl_sz.height = padded_h;
            _scale = 1;
        }

        if (_hogfeatures) {
            _tmpl_sz.width = ( ( (int)(_tmpl_sz.width / (2 * cell_size)) ) * 2 * cell_size ) + cell_size*2;
            _tmpl_sz.height = ( ( (int)(_tmpl_sz.height / (2 * cell_size)) ) * 2 * cell_size ) + cell_size*2;
        }
        else {
            _tmpl_sz.width = (_tmpl_sz.width / 2) * 2;
            _tmpl_sz.height = (_tmpl_sz.height / 2) * 2;
        }
    }

    extracted_roi.width = scale_adjust * _scale * _tmpl_sz.width;
    extracted_roi.height = scale_adjust * _scale * _tmpl_sz.height;
    extracted_roi.x = cx - extracted_roi.width / 2;
    extracted_roi.y = cy - extracted_roi.height / 2;

    g5v::Matrix FeaturesMap;  
    g5v::Matrix z = RectTools::subwindow(image, extracted_roi);
    
    if (z.cols != _tmpl_sz.width || z.rows != _tmpl_sz.height) {
        z = RectTools::resizeImage(z, _tmpl_sz);
    }   

    if (_hogfeatures) {
        CvLSVMFeatureMapCaskade *map;
        getFeatureMaps(z, cell_size, &map);
        normalizeAndTruncate(map, 0.2f);
        PCAFeatureMaps(map);
        
        size_patch[0] = map->sizeY;
        size_patch[1] = map->sizeX;
        size_patch[2] = map->numFeatures;

        FeaturesMap = g5v::Matrix(map->numFeatures, map->sizeX * map->sizeY, 1);
        for (int r = 0; r < map->sizeY * map->sizeX; ++r) {
            for (int c = 0; c < map->numFeatures; ++c) {
                FeaturesMap.at(c, r, 0) = map->map[r * map->numFeatures + c];
            }
        }
        freeFeatureMapObject(&map);
    }
    else {
        FeaturesMap = RectTools::getGrayImage(z);
        FeaturesMap = FeaturesMap - 0.5f; 
        size_patch[0] = z.rows;
        size_patch[1] = z.cols;
        size_patch[2] = 1;  
    }
    
    if (inithann) {
        createHanningMats();
    }
    FeaturesMap = hann.mul(FeaturesMap);
    return FeaturesMap;
}
    
// Initialize Hanning window. Function called only in the first frame.
void KCFTracker::createHanningMats()
{   
    g5v::Matrix hann1t(1, size_patch[1], 1);
    g5v::Matrix hann2t(size_patch[0], 1, 1); 

    for (int i = 0; i < size_patch[1]; i++)
        hann1t.at(0, i, 0) = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846f * i / (size_patch[1] - 1)));
    for (int i = 0; i < size_patch[0]; i++)
        hann2t.at(i, 0, 0) = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979323846f * i / (size_patch[0] - 1)));

    g5v::Matrix hann2d(size_patch[0], size_patch[1], 1);
    for(int r = 0; r < size_patch[0]; r++){
        for(int c = 0; c < size_patch[1]; c++){
            hann2d.at(r, c, 0) = hann2t.at(r, 0, 0) * hann1t.at(0, c, 0);
        }
    }

    if (_hogfeatures) {
        hann = g5v::Matrix(size_patch[2], size_patch[0] * size_patch[1], 1); 
        for (int f = 0; f < size_patch[2]; f++) {
            for (int i = 0; i < size_patch[0] * size_patch[1]; i++) {
                hann.data[f * (size_patch[0] * size_patch[1]) + i] = hann2d.data[i]; 
            }
        }
    }
    else {
        hann = hann2d;
    }
}

// Calculate sub-pixel peak for one dimension
float KCFTracker::subPixelPeak(float left, float center, float right)
{   
    float divisor = 2 * center - right - left;

    if (divisor == 0)
        return 0;
    
    return 0.5 * (right - left) / divisor;
}
