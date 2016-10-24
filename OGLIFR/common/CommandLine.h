/*!
 * \file
 * Command line parser routines for NvIFR samples.
 *
 * \copyright
 * Copyright 2013 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#if !defined __COMMANDLINE_H__
#define __COMMANDLINE_H__

#include "NvIFR/NvIFROpenGL.h"

#define MAX_OUTPUT_FILENAME_LENGTH (60)
#define RESOLUTION_720P            (1280*720)
#define BITRATE_720P               (5000000)

extern void print_help(void);
extern bool commandline_parser(int argc, char *argv[], long *duration, char *output,
                        unsigned int *width, unsigned int *height,
                        long *frames, bool *isOutFile, int *loops, int *threads,
                        NV_IFROGL_HW_ENC_TYPE *codecType);

#endif
