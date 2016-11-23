/*!
 * \file
 *
 * Targa Image File (TGA)
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

#ifndef _TGA_H_
#define _TGA_H_

bool saveAsTGA(const char *fileName, char bits, short width, short height, const void *data);

#endif // _TGA_H_

