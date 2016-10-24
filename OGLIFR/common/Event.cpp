/*!
 * \file
 * Event implementation
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

#include "Event.h"

/*!
 * Constructor.
 */
Event::Event()
{
#ifdef _WIN32
    m_eventHandle = 0;
#else
    pthread_cond_init(&m_cond, NULL);
    pthread_mutex_init(&m_mutex, NULL);
    m_value = false;
#endif
}

/*!
 * Destructor.
 */
Event::~Event()
{
}

/*!
 * Init.
 *
 * \return false if failed
 */
bool Event::init()
{
#ifdef _WIN32
    m_eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!m_eventHandle)
    {
        return false;
    }
#else
    if (pthread_cond_init(&m_cond, NULL) != 0)
    {
        return false;
    }
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
    {
        return false;
    }
#endif

    return true;
}

/*!
 * Cleanup.
 */
void Event::cleanup()
{
#ifdef _WIN32
    if (m_eventHandle)
    {
        CloseHandle(m_eventHandle);
        m_eventHandle = 0;
    }
#else
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
#endif
}

/*!
 * Wait for an event to be signalled.
 */
void Event::wait()
{
#ifdef _WIN32
    WaitForSingleObject(m_eventHandle, INFINITE);
#else
    pthread_mutex_lock(&m_mutex);
    while (m_value == false)
    {
        pthread_cond_wait(&m_cond, &m_mutex);
    }
    m_value = false;
    pthread_mutex_unlock(&m_mutex);
#endif
}

/*!
 * Signal an event.
 */
void Event::signal()
{
#ifdef _WIN32
    SetEvent(m_eventHandle);
#else
    pthread_mutex_lock(&m_mutex);
    m_value = true;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
#endif
}
