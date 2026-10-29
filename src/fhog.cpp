/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2010-2013, University of Nizhny Novgorod, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/


//Modified from latentsvm module's "lsvmc_featurepyramid.cpp".

//#include "precomp.hpp"
//#include "_lsvmc_latentsvm.h"
//#include "_lsvmc_resizeimg.h"

#include "fhog.hpp"
#include "gem5_vision.hpp"
#include <klib.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

/*
// Getting feature map for the selected subimage
//
// API
// int getFeatureMaps(const IplImage * image, const int k, featureMap **map);
// INPUT
// image             - selected subimage
// k                 - size of cells
// OUTPUT
// map               - feature map
// RESULT
// Error status
*/
int getFeatureMaps(const g5v::Matrix& image, const int k, CvLSVMFeatureMapCaskade **map)
{
    int sizeX, sizeY;
    int p, px, stringSize;
    int height, width, numChannels;
    int i, j, kk, ii, jj, d;

    int ch;
    float boundary_x[NUM_SECTOR + 1];
    float boundary_y[NUM_SECTOR + 1];
    float max_val, dotProd;
    int maxi;

    height = image.rows;
    width = image.cols;
    numChannels = image.channels;

    sizeX = width / k;
    sizeY = height / k;
    px = 3 * NUM_SECTOR;
    p = px;
    stringSize = sizeX * p;
    allocFeatureMapObject(map, sizeX, sizeY, p);

    float arg_vector;
    for (i = 0; i <= NUM_SECTOR; i++)
    {
        arg_vector = ((float)i) * ((float)(PI) / (float)(NUM_SECTOR));
        boundary_x[i] = cosf(arg_vector);
        boundary_y[i] = sinf(arg_vector);
    }

    float* r = (float*)malloc(sizeof(float) * (width * height));
    int* alfa = (int*)malloc(sizeof(int) * (width * height * 2));

    // حساب الـ Gradients يدوياً كبديل لـ cvFilter2D
    for (j = 1; j < height - 1; j++)
    {
        for (i = 1; i < width - 1; i++)
        {
            float max_mag = -1.0f;
            float best_dx = 0.0f, best_dy = 0.0f;

            for (ch = 0; ch < numChannels; ch++)
            {
                float dx = image.at(j, i + 1, ch) - image.at(j, i - 1, ch);
                float dy = image.at(j + 1, i, ch) - image.at(j - 1, i, ch);
                float magnitude = sqrtf(dx * dx + dy * dy);
                
                if (magnitude > max_mag)
                {
                    max_mag = magnitude;
                    best_dx = dx;
                    best_dy = dy;
                }
            }

            r[j * width + i] = max_mag;
            float x = best_dx;
            float y = best_dy;

            max_val = boundary_x[0] * x + boundary_y[0] * y;
            maxi = 0;
            for (kk = 0; kk < NUM_SECTOR; kk++)
            {
                dotProd = boundary_x[kk] * x + boundary_y[kk] * y;
                if (dotProd > max_val)
                {
                    max_val = dotProd;
                    maxi = kk;
                }
                else if (-dotProd > max_val)
                {
                    max_val = -dotProd;
                    maxi = kk + NUM_SECTOR;
                }
            }
            alfa[j * width * 2 + i * 2] = maxi % NUM_SECTOR;
            alfa[j * width * 2 + i * 2 + 1] = maxi;
        }
    }

    int* nearest = (int*)malloc(sizeof(int) * k);
    float* w = (float*)malloc(sizeof(float) * (k * 2));
    
    for (i = 0; i < k / 2; i++) nearest[i] = -1;
    for (i = k / 2; i < k; i++) nearest[i] = 1;

    for (j = 0; j < k / 2; j++)
    {
        float b_x = k / 2 + j + 0.5f;
        float a_x = k / 2 - j - 0.5f;
        w[j * 2] = 1.0f / a_x * ((a_x * b_x) / (a_x + b_x));
        w[j * 2 + 1] = 1.0f / b_x * ((a_x * b_x) / (a_x + b_x));
    }
    for (j = k / 2; j < k; j++)
    {
        float a_x = j - k / 2 + 0.5f;
        float b_x = -j + k / 2 - 0.5f + k;
        w[j * 2] = 1.0f / a_x * ((a_x * b_x) / (a_x + b_x));
        w[j * 2 + 1] = 1.0f / b_x * ((a_x * b_x) / (a_x + b_x));
    }

    for (i = 0; i < sizeY; i++)
    {
        for (j = 0; j < sizeX; j++)
        {
            for (ii = 0; ii < k; ii++)
            {
                for (jj = 0; jj < k; jj++)
                {
                    if ((i * k + ii > 0) && (i * k + ii < height - 1) && 
                        (j * k + jj > 0) && (j * k + jj < width - 1))
                    {
                        d = (k * i + ii) * width + (j * k + jj);
                        (*map)->map[i * stringSize + j * (*map)->numFeatures + alfa[d * 2]] += 
                            r[d] * w[ii * 2] * w[jj * 2];
                        (*map)->map[i * stringSize + j * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] += 
                            r[d] * w[ii * 2] * w[jj * 2];
                        if ((i + nearest[ii] >= 0) && (i + nearest[ii] <= sizeY - 1))
                        {
                            (*map)->map[(i + nearest[ii]) * stringSize + j * (*map)->numFeatures + alfa[d * 2]] += 
                                r[d] * w[ii * 2 + 1] * w[jj * 2];
                            (*map)->map[(i + nearest[ii]) * stringSize + j * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] += 
                                r[d] * w[ii * 2 + 1] * w[jj * 2];
                        }
                        if ((j + nearest[jj] >= 0) && (j + nearest[jj] <= sizeX - 1))
                        {
                            (*map)->map[i * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2]] += 
                                r[d] * w[ii * 2] * w[jj * 2 + 1];
                            (*map)->map[i * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] += 
                                r[d] * w[ii * 2] * w[jj * 2 + 1];
                        }
                        if ((i + nearest[ii] >= 0) && (i + nearest[ii] <= sizeY - 1) && 
                            (j + nearest[jj] >= 0) && (j + nearest[jj] <= sizeX - 1))
                        {
                            (*map)->map[(i + nearest[ii]) * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2]] += 
                                r[d] * w[ii * 2 + 1] * w[jj * 2 + 1];
                            (*map)->map[(i + nearest[ii]) * stringSize + (j + nearest[jj]) * (*map)->numFeatures + alfa[d * 2 + 1] + NUM_SECTOR] += 
                                r[d] * w[ii * 2 + 1] * w[jj * 2 + 1];
                        }
                    }
                }
            }
        }
    }

    free(w);
    free(nearest);
    free(r);
    free(alfa);

    return LATENT_SVM_OK;
}

/*
// Feature map Normalization and Truncation 
//
// API
// int normalizeAndTruncate(featureMap *map, const float alfa);
// INPUT
// map               - feature map
// alfa              - truncation threshold
// OUTPUT
// map               - truncated and normalized feature map
// RESULT
// Error status
*/
int normalizeAndTruncate(CvLSVMFeatureMapCaskade *map, const float alfa)
{
    int i,j, ii;
    int sizeX, sizeY, p, pos, pp, xp, pos1, pos2;
    float * partOfNorm; // norm of C(i, j)
    float * newData;
    float   valOfNorm;

    sizeX     = map->sizeX;
    sizeY     = map->sizeY;
    partOfNorm = (float *)malloc (sizeof(float) * (sizeX * sizeY));

    p  = NUM_SECTOR;
    xp = NUM_SECTOR * 3;
    pp = NUM_SECTOR * 12;

    for(i = 0; i < sizeX * sizeY; i++)
    {
        valOfNorm = 0.0f;
        pos = i * map->numFeatures;
        for(j = 0; j < p; j++)
        {
            valOfNorm += map->map[pos + j] * map->map[pos + j];
        }/*for(j = 0; j < p; j++)*/
        partOfNorm[i] = valOfNorm;
    }/*for(i = 0; i < sizeX * sizeY; i++)*/
    
    sizeX -= 2;
    sizeY -= 2;

    newData = (float *)malloc (sizeof(float) * (sizeX * sizeY * pp));
//normalization
    for(i = 1; i <= sizeY; i++)
    {
        for(j = 1; j <= sizeX; j++)
        {
            valOfNorm = sqrtf(
                partOfNorm[(i    )*(sizeX + 2) + (j    )] +
                partOfNorm[(i    )*(sizeX + 2) + (j + 1)] +
                partOfNorm[(i + 1)*(sizeX + 2) + (j    )] +
                partOfNorm[(i + 1)*(sizeX + 2) + (j + 1)]) + FLT_EPSILON;
            pos1 = (i  ) * (sizeX + 2) * xp + (j  ) * xp;
            pos2 = (i-1) * (sizeX    ) * pp + (j-1) * pp;
            for(ii = 0; ii < p; ii++)
            {
                newData[pos2 + ii        ] = map->map[pos1 + ii    ] / valOfNorm;
            }/*for(ii = 0; ii < p; ii++)*/
            for(ii = 0; ii < 2 * p; ii++)
            {
                newData[pos2 + ii + p * 4] = map->map[pos1 + ii + p] / valOfNorm;
            }/*for(ii = 0; ii < 2 * p; ii++)*/
            valOfNorm = sqrtf(
                partOfNorm[(i    )*(sizeX + 2) + (j    )] +
                partOfNorm[(i    )*(sizeX + 2) + (j + 1)] +
                partOfNorm[(i - 1)*(sizeX + 2) + (j    )] +
                partOfNorm[(i - 1)*(sizeX + 2) + (j + 1)]) + FLT_EPSILON;
            for(ii = 0; ii < p; ii++)
            {
                newData[pos2 + ii + p    ] = map->map[pos1 + ii    ] / valOfNorm;
            }/*for(ii = 0; ii < p; ii++)*/
            for(ii = 0; ii < 2 * p; ii++)
            {
                newData[pos2 + ii + p * 6] = map->map[pos1 + ii + p] / valOfNorm;
            }/*for(ii = 0; ii < 2 * p; ii++)*/
            valOfNorm = sqrtf(
                partOfNorm[(i    )*(sizeX + 2) + (j    )] +
                partOfNorm[(i    )*(sizeX + 2) + (j - 1)] +
                partOfNorm[(i + 1)*(sizeX + 2) + (j    )] +
                partOfNorm[(i + 1)*(sizeX + 2) + (j - 1)]) + FLT_EPSILON;
            for(ii = 0; ii < p; ii++)
            {
                newData[pos2 + ii + p * 2] = map->map[pos1 + ii    ] / valOfNorm;
            }/*for(ii = 0; ii < p; ii++)*/
            for(ii = 0; ii < 2 * p; ii++)
            {
                newData[pos2 + ii + p * 8] = map->map[pos1 + ii + p] / valOfNorm;
            }/*for(ii = 0; ii < 2 * p; ii++)*/
            valOfNorm = sqrtf(
                partOfNorm[(i    )*(sizeX + 2) + (j    )] +
                partOfNorm[(i    )*(sizeX + 2) + (j - 1)] +
                partOfNorm[(i - 1)*(sizeX + 2) + (j    )] +
                partOfNorm[(i - 1)*(sizeX + 2) + (j - 1)]) + FLT_EPSILON;
            for(ii = 0; ii < p; ii++)
            {
                newData[pos2 + ii + p * 3 ] = map->map[pos1 + ii    ] / valOfNorm;
            }/*for(ii = 0; ii < p; ii++)*/
            for(ii = 0; ii < 2 * p; ii++)
            {
                newData[pos2 + ii + p * 10] = map->map[pos1 + ii + p] / valOfNorm;
            }/*for(ii = 0; ii < 2 * p; ii++)*/
        }/*for(j = 1; j <= sizeX; j++)*/
    }/*for(i = 1; i <= sizeY; i++)*/
//truncation
    for(i = 0; i < sizeX * sizeY * pp; i++)
    {
        if(newData [i] > alfa) newData [i] = alfa;
    }/*for(i = 0; i < sizeX * sizeY * pp; i++)*/
//swop data

    map->numFeatures  = pp;
    map->sizeX = sizeX;
    map->sizeY = sizeY;

    free (map->map);
    free (partOfNorm);

    map->map = newData;

    return LATENT_SVM_OK;
}
/*
// Feature map reduction
// In each cell we reduce dimension of the feature vector
// according to original paper special procedure
//
// API
// int PCAFeatureMaps(featureMap *map)
// INPUT
// map               - feature map
// OUTPUT
// map               - feature map
// RESULT
// Error status
*/
int PCAFeatureMaps(CvLSVMFeatureMapCaskade *map)
{ 
    int i,j, ii, jj, k;
    int sizeX, sizeY, p,  pp, xp, yp, pos1, pos2;
    float * newData;
    float val;
    float nx, ny;
    
    sizeX = map->sizeX;
    sizeY = map->sizeY;
    p     = map->numFeatures;
    pp    = NUM_SECTOR * 3 + 4;
    yp    = 4;
    xp    = NUM_SECTOR;

    nx    = 1.0f / sqrtf((float)(xp * 2));
    ny    = 1.0f / sqrtf((float)(yp    ));

    newData = (float *)malloc (sizeof(float) * (sizeX * sizeY * pp));

    for(i = 0; i < sizeY; i++)
    {
        for(j = 0; j < sizeX; j++)
        {
            pos1 = ((i)*sizeX + j)*p;
            pos2 = ((i)*sizeX + j)*pp;
            k = 0;
            for(jj = 0; jj < xp * 2; jj++)
            {
                val = 0;
                for(ii = 0; ii < yp; ii++)
                {
                    val += map->map[pos1 + yp * xp + ii * xp * 2 + jj];
                }/*for(ii = 0; ii < yp; ii++)*/
                newData[pos2 + k] = val * ny;
                k++;
            }/*for(jj = 0; jj < xp * 2; jj++)*/
            for(jj = 0; jj < xp; jj++)
            {
                val = 0;
                for(ii = 0; ii < yp; ii++)
                {
                    val += map->map[pos1 + ii * xp + jj];
                }/*for(ii = 0; ii < yp; ii++)*/
                newData[pos2 + k] = val * ny;
                k++;
            }/*for(jj = 0; jj < xp; jj++)*/
            for(ii = 0; ii < yp; ii++)
            {
                val = 0;
                for(jj = 0; jj < 2 * xp; jj++)
                {
                    val += map->map[pos1 + yp * xp + ii * xp * 2 + jj];
                }/*for(jj = 0; jj < xp; jj++)*/
                newData[pos2 + k] = val * nx;
                k++;
            } /*for(ii = 0; ii < yp; ii++)*/           
        }/*for(j = 0; j < sizeX; j++)*/
    }/*for(i = 0; i < sizeY; i++)*/
//swop data

    map->numFeatures = pp;

    free (map->map);

    map->map = newData;

    return LATENT_SVM_OK;
}


//modified from "lsvmc_routine.cpp"

int allocFeatureMapObject(CvLSVMFeatureMapCaskade **obj, const int sizeX, 
                          const int sizeY, const int numFeatures)
{
    int i;
    (*obj) = (CvLSVMFeatureMapCaskade *)malloc(sizeof(CvLSVMFeatureMapCaskade));
    (*obj)->sizeX       = sizeX;
    (*obj)->sizeY       = sizeY;
    (*obj)->numFeatures = numFeatures;
    (*obj)->map = (float *) malloc(sizeof (float) * 
                                  (sizeX * sizeY  * numFeatures));
    for(i = 0; i < sizeX * sizeY * numFeatures; i++)
    {
        (*obj)->map[i] = 0.0f;
    }
    return LATENT_SVM_OK;
}

int freeFeatureMapObject (CvLSVMFeatureMapCaskade **obj)
{
    if(*obj == NULL) return LATENT_SVM_MEM_NULL;
    free((*obj)->map);
    free(*obj);
    (*obj) = NULL;
    return LATENT_SVM_OK;
}
