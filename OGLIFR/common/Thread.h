/*!
 * \file
 * Thread declaration
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

#ifndef _THREAD_H_
#define _THREAD_H_

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
class Thread
{
public:
    typedef unsigned int (*ThreadProc)(void *data);

private:
    void *m_data;           //!<
    ThreadProc m_proc;      //!<

#ifdef _WIN32
    HANDLE m_threadHandle;  //!<

    friend DWORD WINAPI threadFunction(LPVOID dataPtr);
#else
    pthread_t m_threadID;   //!<

    friend void *threadFunction(void *dataPtr);
#endif

public:
    Thread();
    ~Thread();

    bool create(ThreadProc proc, void *data);
    bool waitForExit();
};

#endif // _THREAD_H_
