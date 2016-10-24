/*!
 * \file
 * Command line parser routines for NvIFR samples.
 *
 * \copyright
 * Copyright 2013-2014 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

unsigned int calculateBitrate(unsigned int width, unsigned int height)
{
    static const unsigned int resolution720p = 1280 * 720;
    static const unsigned int bitrate720p = 5 * 1024 * 1024;

    return (unsigned int)((bitrate720p * width * height) / resolution720p);
}
