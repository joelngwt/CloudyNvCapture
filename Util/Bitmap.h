/*
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#pragma once

#include <windows.h>

// Saves the RGB buffer as a bitmap
bool SaveRGB(const char *fileName, BYTE *data, int width, int height, int stride);

// Saves the BGR buffer as a bitmap
bool SaveBGR(const char *fileName, BYTE *data, int width, int height, int stride);

// Saves the ARGB buffer as a bitmap
bool SaveARGB(const char *fileName, BYTE *data, int width, int height, int stride);

// Saves the RGBPlanar buffer as three bitmaps, one bitmap for each channel
bool SaveRGBPlanar(const char *fileName, BYTE *data, int width, int height);

// Saves the YUV420p buffer as three bitmaps, one bitmap for Y', one for U and one for V
bool SaveYUV(const char *fileName, BYTE *data, int width, int height);

// Saves the YUV444 buffer as single bitmap after converting to RGB using BT709 equations
bool SaveYUV444(const char *fileName, BYTE *data, int width, int height);

// Saves the YUV420 buffer as single bitmap after converting to YUV444, then RGB using BT709 equations
bool SaveYUV420(const char *fileName, BYTE *data, int width, int height);

// Saves the YUV420 buffer as single bitmap after converting to YUV444, then RGB using BT709 equations
bool SaveNV12(const char *fileName, BYTE *data, int width, int height, int stride);

// Saves the provided buffer as a bitmap, this method assumes the data is formated as a bitmap.
bool SaveBitmap(const char *fileName, BYTE *data, int width, int height);