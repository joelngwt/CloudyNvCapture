/*!
 * \file
 *
 * Implementation of OpenGLDisplay and OpenGLWin for unix.
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


#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else // !_WIN32
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif // !_WIN32

#include "OpenGLWin.h"
#include "Helper.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifndef _WIN32
class OpenGLDisplayPriv
{
public:
    Display *m_dpy;
    Window   m_root;
};
#endif

/*!
 * OpenGLDisplay constructor.
 */
OpenGLDisplay::OpenGLDisplay(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *privData;

    privData = new OpenGLDisplayPriv();
    if (privData != NULL)
    {
        memset(privData, 0, sizeof(*privData));
    }
    else
    {
        PRINT_ERROR("Insufficient resource\n");
    }

    m_privData = privData;
#endif
}

/*!
 * OpenGLDisplay destructor.
 */
OpenGLDisplay::~OpenGLDisplay(void)
{
#ifndef _WIN32
    close();

    delete (OpenGLDisplayPriv*)m_privData;
#endif
}

/*!
 * Returns 'TRUE' is OpenGLDisplay object is valid.
 */
inline bool OpenGLDisplay::initialize(void) const
{
#ifdef _WIN32
    return true;
#else
    return (m_privData != NULL);
#endif
}

/*!
 * Open logical OpenGL display.
 */
bool OpenGLDisplay::open(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *privData = (OpenGLDisplayPriv*)m_privData;

    if (!initialize() || (privData->m_dpy != NULL))
    {
        PRINT_ERROR("OpenGLDisplay is already opened\n");
        return false;
    }

    /* Open X-server connection */
    privData->m_dpy = XOpenDisplay(NULL);
    if (privData->m_dpy == NULL)
    {
        PRINT_ERROR("Cannot connect to X server\n");
        return false;
    }

    /* Get pointer to default root window */
    privData->m_root = DefaultRootWindow(privData->m_dpy);
#endif

    return true;
}

/*!
 * Close logical OpenGL display.
 */
void OpenGLDisplay::close(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *privData = (OpenGLDisplayPriv*)m_privData;

    if (!initialize() || (privData->m_dpy == NULL))
    {
        return;
    }

    /* Close X-server connection */
    XCloseDisplay(privData->m_dpy);

    privData->m_dpy = NULL;
#endif
}

class OpenGLWinPriv
{
public:
#ifdef _WIN32
    HWND m_hWnd;
    HDC m_hDC;
    HGLRC m_hGLRC;
#else // !_WIN32
    GLXFBConfig  m_fbc;
    XVisualInfo *m_vi;

    Window m_win;
    XSetWindowAttributes m_swa;

    GLXWindow    m_gwin;
    GLXContext   m_glc;
#endif // !_WIN32
    const OpenGLWinPriv *m_sharedWin;
};

/*!
 * OpenGLWin constructor.
 */
OpenGLWin::OpenGLWin(void)
{
    OpenGLWinPriv *privData;

    privData = new OpenGLWinPriv();
    if (privData != NULL)
    {
        memset(privData, 0, sizeof(*privData));
    }
    else
    {
        PRINT_ERROR("Insufficient resource\n");
    }

    m_privData = privData;
    m_dpy = NULL;
}

/*!
 * OpenGLWin destructor.
 */
OpenGLWin::~OpenGLWin(void)
{
    close();

    delete (OpenGLWinPriv*)m_privData;
}

/*!
 * Returns 'TRUE' if OpenGLWin object is valid.
 */
inline bool OpenGLWin::initialize(void) const
{
    return (m_privData != NULL);
}

/*!
 * Open OpenGL window on given OpenGL display.
 */
bool OpenGLWin::open(const OpenGLDisplay *dpy, const char *title,
                     unsigned int width, unsigned int height, const OpenGLWin *win)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData = (dpy == NULL) ? NULL : (OpenGLDisplayPriv*)dpy->m_privData;
#endif
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;

    if (!initialize())
    {
        PRINT_ERROR("OpenGLWin Object is not initialized\n");
        return false;
    }

    if (m_dpy != NULL)
    {
        PRINT_ERROR("OpenGLWin is already opened\n");
        return false;
    }

#ifdef _WIN32
    if (!dpy->initialize())
    {
        PRINT_ERROR("OpenGLDisplay is closed\n");
        return false;
    }

    int iPixelFormat;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };

    winPrivData->m_hWnd = CreateWindow("static", title, WS_OVERLAPPEDWINDOW,
                        0, 0, width, height, NULL, NULL, NULL, NULL);
    if (!winPrivData->m_hWnd)
    {
        PRINT_ERROR("Failed to create window\n");
        return false;
    }

    winPrivData->m_hDC = GetDC(winPrivData->m_hWnd);

    iPixelFormat = ChoosePixelFormat(winPrivData->m_hDC, &pfd);
    if (iPixelFormat == 0)
    {
        PRINT_ERROR("Unable to find pixel format.\n");
        return false;
    }

    if (SetPixelFormat(winPrivData->m_hDC, iPixelFormat, &pfd) != TRUE)
    {
        PRINT_ERROR("Failed to set pixel format.\n");
        return false;
    }

    winPrivData->m_hGLRC = wglCreateContext(winPrivData->m_hDC);
    if (winPrivData->m_hGLRC == 0)
    {
        PRINT_ERROR("Failed to create context.\n");
        return false;
    }

    if (win != NULL)
    {
        if (wglShareLists(((OpenGLWinPriv*)win->m_privData)->m_hGLRC, winPrivData->m_hGLRC) != TRUE)
        {
            PRINT_ERROR("Failed to share contexts.\n");
            return false;
        }
    }
#else
    if (!dpy->initialize() || (dpyPrivData->m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLDisplay is closed\n");
        return false;
    }

    /* Find a best FBConfig */
    {
        GLXFBConfig *fbc;
        int nelements;

        fbc = glXChooseFBConfig(dpyPrivData->m_dpy,
                                DefaultScreen(dpyPrivData->m_dpy), 0, &nelements);
        if (fbc == NULL)
        {
            PRINT_ERROR("Faile to get best FBConfig\n");
            return false;
        }
        winPrivData->m_fbc = fbc[0];
        free(fbc);

        winPrivData->m_vi = glXGetVisualFromFBConfig(dpyPrivData->m_dpy, winPrivData->m_fbc);
        if(winPrivData->m_vi == NULL)
        {
            PRINT_ERROR("\nNo appropriate visual found\n");
            return false;
        }
    }

    /* Create X and GLX windows */
    {
        Colormap cmap;

        cmap = XCreateColormap(dpyPrivData->m_dpy, dpyPrivData->m_root,
                               winPrivData->m_vi->visual,
                               AllocNone);

        winPrivData->m_swa.colormap   = cmap;
        winPrivData->m_swa.event_mask = ExposureMask | KeyPressMask;

        winPrivData->m_win = XCreateWindow(dpyPrivData->m_dpy, dpyPrivData->m_root,
                                         0, 0, width, height, 0, winPrivData->m_vi->depth,
                                         InputOutput, winPrivData->m_vi->visual,
                                         CWColormap | CWEventMask, &(winPrivData->m_swa));

        if (winPrivData->m_win == None)
        {
            PRINT_ERROR("Failed to create Window\n");
            return false;
        }

        XStoreName(dpyPrivData->m_dpy, winPrivData->m_win, (const char*)title);

        winPrivData->m_gwin = glXCreateWindow(dpyPrivData->m_dpy,
                                            winPrivData->m_fbc, winPrivData->m_win, 0);

        if (winPrivData->m_gwin == None)
        {
            PRINT_ERROR("Failed to create GLX Window\n");
            XDestroyWindow(dpyPrivData->m_dpy, winPrivData->m_win);
            return false;
        }
    }

    /* Create GLX context */
    {
        GLXContext sharedContext = 0;

        if (win != NULL && win->m_dpy != NULL)
        {
            sharedContext = ((OpenGLWinPriv*)win->m_privData)->m_glc;
            winPrivData->m_sharedWin = win != NULL ? ((OpenGLWinPriv*)win->m_privData) : NULL;
        }

        winPrivData->m_glc = glXCreateNewContext(dpyPrivData->m_dpy, winPrivData->m_fbc,
                                               GLX_RGBA_TYPE, sharedContext, GL_TRUE);

        if (winPrivData->m_glc ==  None)
        {
            PRINT_ERROR("Failed to create GLX context\n");
            glXDestroyWindow(dpyPrivData->m_dpy, winPrivData->m_gwin);
            XDestroyWindow(dpyPrivData->m_dpy, winPrivData->m_win);
            return false;
        }
    }
#endif // !_WIN32

    m_dpy = dpy;

    m_title = strdup(title);
    m_width = width;
    m_height = height;

    return true;
}

/*!
 * Close OpenGL window.
 */
void OpenGLWin::close(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData;
#endif
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;

    if (!initialize() || (m_dpy == NULL))
    {
        return;
    }

#ifdef _WIN32
    if (winPrivData->m_hGLRC != 0)
    {
        wglDeleteContext(winPrivData->m_hGLRC);
        winPrivData->m_hGLRC = 0;
    }
    if (winPrivData->m_hDC != 0)
    {
        ReleaseDC(winPrivData->m_hWnd, winPrivData->m_hDC);
        winPrivData->m_hDC = 0;
    }
    if (winPrivData->m_hWnd != 0)
    {
        CloseWindow(winPrivData->m_hWnd);
        winPrivData->m_hDC = 0;
    }
#else // !_WIN32
    dpyPrivData = (OpenGLDisplayPriv*)m_dpy->m_privData;

    if (!m_dpy->initialize() || dpyPrivData->m_dpy == NULL)
    {
        m_dpy = NULL;
        return;
    }

    /* Destroy GLX context */
    if (winPrivData->m_glc != None)
    {
        glXDestroyContext(dpyPrivData->m_dpy, winPrivData->m_glc);
    }

    /* Destroy GLX window */
    if (winPrivData->m_gwin != None)
    {
        glXDestroyWindow(dpyPrivData->m_dpy, winPrivData->m_gwin);
    }

    /* Destroy X window */
    if (winPrivData->m_win != None)
    {
        XDestroyWindow(dpyPrivData->m_dpy, winPrivData->m_win);
    }
#endif // !_WIN32

    m_dpy = NULL;
    delete m_title;
}

/*!
 * Show OpenGL window on display.
 */
void OpenGLWin::show(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData;
#endif
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;

    if (!initialize() || (m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLWin is already closed");
        return;
    }

#ifdef _WIN32
    ShowWindow(winPrivData->m_hWnd, SW_SHOW);
#else
    dpyPrivData = (OpenGLDisplayPriv*)m_dpy->m_privData;

    if (!m_dpy->initialize() || (dpyPrivData->m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLDisplay is already closed");
        m_dpy = NULL;
        return;
    }

    XMapWindow(dpyPrivData->m_dpy, winPrivData->m_win);
#endif
}

/*!
 * Bind OpenGL window with current thread.
 */
bool OpenGLWin::makeCurrent(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData;
#endif
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;

    if (!initialize() || (m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLWin is already closed");
        return false;
    }

#ifdef _WIN32
    if (!wglMakeCurrent(winPrivData->m_hDC, winPrivData->m_hGLRC))
#else
    dpyPrivData = (OpenGLDisplayPriv*)m_dpy->m_privData;

    if (!m_dpy->initialize() || (dpyPrivData->m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLDisplay is already closed");
        m_dpy = NULL;
        return false;
    }

    if (!glXMakeContextCurrent(dpyPrivData->m_dpy, winPrivData->m_gwin, winPrivData->m_gwin,
                               winPrivData->m_glc))
#endif
    {
        return false;
    }

    return true;
}

/*!
 * Unbind OpenGL window with current thread.
 */
bool OpenGLWin::loseCurrent(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData;
#else
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;
#endif

    if (!initialize() || (m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLWin is already closed");
        return false;
    }

#ifdef _WIN32
    if (!wglMakeCurrent(winPrivData->m_hDC, NULL))
#else
    dpyPrivData = (OpenGLDisplayPriv*)m_dpy->m_privData;

    if (!m_dpy->initialize() || (dpyPrivData->m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLDisplay is already closed");
        m_dpy = NULL;
        return false;
    }

    if (!glXMakeCurrent(dpyPrivData->m_dpy, None, NULL))
#endif
    {
        return false;
    }

    return true;
}

/*!
 * Swap OpenGL buffer.
 */
void OpenGLWin::swapBuffers(void)
{
#ifndef _WIN32
    OpenGLDisplayPriv *dpyPrivData;
#endif
    OpenGLWinPriv *winPrivData = (OpenGLWinPriv*)m_privData;

    if (!initialize() || (m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLWin is already closed");
        return;
    }

#ifdef _WIN32
    SwapBuffers(winPrivData->m_hDC);
#else
    dpyPrivData = (OpenGLDisplayPriv*)m_dpy->m_privData;

    if (!m_dpy->initialize() || (dpyPrivData->m_dpy == NULL))
    {
        PRINT_ERROR("OpenGLWin is already closed");
        m_dpy = NULL;
        return;
    }

    glXSwapBuffers(dpyPrivData->m_dpy, winPrivData->m_gwin);
#endif
}
