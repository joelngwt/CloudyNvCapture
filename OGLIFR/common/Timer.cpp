/*!
 * \file
 * Timer helper implementation
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

#ifdef _WIN32
#include <windows.h>
#else
#include <stddef.h>
#include <sys/time.h>
#endif

#include "Timer.h"

/*!
 * Get current system time in uS.
 */
timerValue getTimeInuS(void)
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;

    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);

    return (timerValue)(((double)counter.QuadPart / (double)frequency.QuadPart) * 1000000.);
#else
    struct timeval val;

    gettimeofday(&val, NULL);

    return ((timerValue)val.tv_usec + (timerValue)val.tv_sec * 1000000);
#endif
}
