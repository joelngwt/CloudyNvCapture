/*!
 * \file
 * Thread implementation
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

#include "Thread.h"

/*!
 * Constructor.
 */
Thread::Thread()
{
    m_proc = NULL;
    m_data = NULL;

#ifdef _WIN32
    m_threadHandle = 0;
#else
    m_threadID = 0;
#endif
}

/*!
 * Destructor.
 */
Thread::~Thread()
{
}

/*!
 * Thread function stub, calls the real thread function.
 *
 * \param [in] dataPtr
 *   Pointer to user data
 *
 * \return
 */
#ifdef _WIN32
static DWORD WINAPI threadFunction(LPVOID dataPtr)
#else
void *threadFunction(void *dataPtr)
#endif
{
    Thread *thread = (Thread*)dataPtr;

    thread->m_proc(thread->m_data);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/*!
 * Create a new thread.
 *
 * \param [in] proc
 *   Thread procedure.
 * \param [in] data
 *   Pointer to user data
 *
 * \return false if failed.
 */
bool Thread::create(ThreadProc proc, void *data)
{
    m_proc = proc;
    m_data = data;

#ifdef _WIN32
    m_threadHandle = CreateThread(NULL, 0, threadFunction, this, 0, NULL);
    if (m_threadHandle == NULL)
#else
    if (pthread_create(&m_threadID, NULL, &threadFunction, this) != 0)
#endif
    {
        return false;
    }

    return true;
}

/*!
 * Wait until a thread has exited.
 *
 * \return false if failed.
 */
bool Thread::waitForExit()
{
    bool success = true;

#ifdef _WIN32
   success = (WaitForSingleObject(m_threadHandle, INFINITE) == WAIT_OBJECT_0);

   CloseHandle(m_threadHandle);
#else
   success = (pthread_join(m_threadID, NULL) == 0);
#endif

   return success;
}
