/*!
 * \file
 * Utility functions for NvIFR samples.
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

#ifndef UTIL_H
#define UTIL_H

/*!
 * Calculate the bitrate for an encoded stream with given width and height based on
 * 5 Mbps for 720p.
 *
 * \param width [in]
 *   encoded stream width
 * \param height [in]
 *   encoded stream height
 *
 * \return bitrate
 */
extern unsigned int calculateBitrate(unsigned int width, unsigned int height);

#endif // UTIL_H
