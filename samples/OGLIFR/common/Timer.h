/*!
 * \file
 * Helper for getting system time.
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

#if !defined (__TIMER_H__)
#define __TIMER_H__

typedef  unsigned long long timerValue;

extern timerValue getTimeInuS(void);

#endif // __TIMER_H__

