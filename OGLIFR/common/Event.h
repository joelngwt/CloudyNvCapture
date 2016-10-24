/*!
 * \file
 * Event declaration
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

#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#endif

/*!
 * 
 */
class Event
{
private:
#ifdef _WIN32
    HANDLE m_eventHandle;
#else
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
    bool m_value;
#endif

public:
    Event();
    ~Event();

    bool init();
    void cleanup();

    void wait();
    void signal();
};

#endif // _EVENT_H_
