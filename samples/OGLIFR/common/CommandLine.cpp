/*!
 * \file
 * Command line parser routines for NvIFR samples.
 *
 * \copyright
 * Copyright 2013-2016 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#include "CommandLine.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

void print_help(long *duration, char *output,
	unsigned int *width, unsigned int *height,
	long *frames, int *loops, int *threads,
	NV_IFROGL_HW_ENC_TYPE *codecType)
{
    printf("\n\nThe following command line arguments can be used with this app.\n");
	if (duration) {
		printf(
			" -duration : The time period in seconds for which the application captures\n"
			"             the frames and exit.\n");
	}
	if (output) {
		printf(
			" -output   : Name of the captured output file. If set to 'none', output file\n"
			"             is not generated.\n");
	}
	if (width) {
		printf(
			" -width    : Set the width of the frame buffer object used by NvIFROpenGL.\n");
	}      
	if (height) {
		printf(
			" -height   : Set the height of the frame buffer object used by NvIFROpenGL.\n");
	}
	if (frames) {
		printf(
			" -frames   : Set total number of frames application captures and encodes\n"
			"             before exit.\n");
	}      
	if (loops) {
		printf(
			" -loops    : Determines how may times to launch a window and capture the frames.\n"
			"             Should be used along with '-duration' or '-frames'.\n"
			"             If the '-output' is not set to 'none', '_<itteration_count> will be\n"
			"             appended to the generated bitstream.\n");
	}
	if (threads) {
		printf(
			" -threads  : Number of concurrent NvIFR threads to launch.\n");
	}
	if (codecType) {
		printf(
			" -codec    : Set the codec to use. Either 'h264' or 'h265'\n");
	}
    printf(" -help     : Print this help.\n");
}

bool commandline_parser(int argc, char *argv[], long *duration, char *output,
                        unsigned int *width, unsigned int *height,
                        long *frames, bool *isOutFile, int *loops, int *threads,
                        NV_IFROGL_HW_ENC_TYPE *codecType)
{
	long nDuration = 0, nFrames = 0;

    // Setting default values for command line parameters
    if (codecType != NULL)
    {
        *codecType = NV_IFROGL_HW_ENC_H264;
    }
    // Parsing Command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "-duration", strlen("-duration")) == 0)
        {
            if (duration == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter %s\n", argv[i-1]);
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            nDuration = atoi(argv[i]);
            if (nFrames > 0)
            {
                fprintf(stderr, "-duration can not be used with -frames.\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
        }
        else if (strncmp(argv[i], "-output", strlen("-output")) == 0)
        {
            if (output == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter '-output'\n");
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter %s\n", argv[i-1]);
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }

            if (strncmp(argv[i], "none", strlen("none")) == 0)
            {
                *isOutFile = false;
            }
            else
            {
                strcpy(output, argv[i]);
            }
        }
        else if (strncmp(argv[i], "-width", strlen("-width")) == 0)
        {
            if (width == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter %s\n", argv[i-1]);
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            *width = atoi(argv[i]);
        }
        else if (strncmp(argv[i], "-height", strlen("-height")) == 0)
        {
            if (height == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter %s\n", argv[i-1]);
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            *height = atoi(argv[i]);
        }
        else if (strncmp(argv[i], "-frames", strlen("-frames")) == 0)
        {
            if (frames == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter %s\n", argv[i-1]);
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            nFrames = atoi(argv[i]);
            if (nDuration > 0)
            {
                fprintf(stderr, "-frames can not be used with -duration.\n\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
        }
        else if (strncmp(argv[i], "-loops", strlen("-loops")) == 0)
        {
            if (loops == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter '-loops'\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            *loops = atoi(argv[i]);
            fprintf(stdout, "\nUsing 'loops': Either -frames or -duration should be specified with this option.\n\n");
        }
        else if (strncmp(argv[i], "-threads", strlen("-threads")) == 0)
        {
            if (threads == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter '-threads'\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            *threads = atoi(argv[i]);
            fprintf(stdout, "\nUsing 'threads': Either -frames or -duration should be specified with this option.\n\n");
        }
        else if (strncmp(argv[i], "-codec", strlen("-codec")) == 0)
        {
            if (codecType == NULL) {
                fprintf(stderr, "Invalid/Unsupported parameter %s\n", argv[i-1]);
                return false;
            }
            if (++i >= argc)
            {
                fprintf(stderr, "Invalid use of parameter '-codec'\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
            if (strncmp(argv[i], "h264", strlen("h264")) == 0)
            {
                *codecType = NV_IFROGL_HW_ENC_H264;
            }
            else if (strncmp(argv[i], "h265", strlen("h265")) == 0)
            {
                *codecType = NV_IFROGL_HW_ENC_H265;
            }
            else
            {
                fprintf(stderr, "Invalid parameter set for '-codec'\n");
                print_help(duration, output, width, height, frames, loops, threads, codecType);
                return false;
            }
        }
        else if (strncmp(argv[i], "-help", strlen("-help")) == 0)
        {
            print_help(duration, output, width, height, frames, loops, threads, codecType);
            return false;
        }
        else
        {
            fprintf(stderr, "Invalid argument.\n\n");
            print_help(duration, output, width, height, frames, loops, threads, codecType);
            return false;
        }
    }

	if (duration && nDuration) {
		*duration = nDuration;
		if (frames) {
			*frames = 0;
		}
	}

	if (frames && nFrames) {
		*frames = nFrames;
		if (duration) {
			*duration = 0;
		}
	}

    return true;
}

