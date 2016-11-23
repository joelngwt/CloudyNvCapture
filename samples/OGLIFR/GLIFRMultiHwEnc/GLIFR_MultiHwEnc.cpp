/*!
 * \file
 *
 * This sample shows how to use NvIFROGL to encode the contents of a
 * framebuffer with the hardware H.264 encoder (requires Kepler class
 * hardware) using multiple GPUs. One thread is created for each stream
 * on a GPU.
 *
 * \copyright
 * Copyright 2013-2015 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#include <stdio.h>
#include <string.h>

#include "GL/glew.h"
#include "GL/freeglut_std.h"
#include "GL/freeglut_ext.h"
#include "GL/glext.h"

#ifdef _MSC_VER
// MS did not follow the naming convention
#define snprintf _snprintf
#endif

#if defined (__linux__)
#include <X11/Xlib.h>
#endif

#include "encodingThread.h"
#include "CommandLine.h"

#include "getopt.h"

//! Version number of an application
#define APP_VERSION 2

const char *applicationName = "GLIFR_MultiHwEnc";
//! width and height of the window
unsigned int winWidth = 512, winHeight = 256;
//! width and height of the framebuffer
unsigned int fboWidth = 1280, fboHeight = 720;
//! framerate of encoded movie
unsigned int framesPerSecond = 30;
//! duration the application runs before it terminates automatically. If this is negative the application runs until the user terminates it.
long duration = 10;
//! number of encoding instances created per GPU
int instances = 1;
//! the list of all encoding threads
EncodingThread *threadList = NULL;
// Codec: H264 or H265
NV_IFROGL_HW_ENC_TYPE codecType = NV_IFROGL_HW_ENC_H264;

bool bStop = false;
void deinit();

/*
 * Call this function to have some text drawn at given coordinates
 *
 * \param [in] x,y
 *   Window coordinate where the text starts
 * \param [in] text
 */
void printText(int x, int y, const char *text)
{
    // Prepare the OpenGL state
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0., winWidth, 0., winHeight, -1., 1.);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2i(x, y);

    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)text);

    // Revert to the old matrix modes
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/*!
 * The display callback for the window.
 */
void display()
{
    EncodingThread *thread;
    char textBuffer[128];
    unsigned int textYpos = winHeight - 12;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, winWidth, winHeight);

    // print thread results
    thread = threadList;
    while (thread)
    {
        snprintf(textBuffer, sizeof(textBuffer), "Device %2d (%s) instance %2d: encoding at %4.0f fps\n",
            thread->getDeviceIndex(), thread->getDeviceString(), thread->getDeviceInstance(), thread->getFrameRate());
        printText(0, textYpos, textBuffer);
        textYpos -= 12;

        thread = thread->m_next;
    }

    glutSwapBuffers();
}

/*!
 * The timer callback.
 */
void timer(int value)
{
	if (bStop) {
		return;
	}
    glutPostRedisplay();
    glutTimerFunc(1000, timer, 0);
}

/*
 * Timer callback function which will be called when the program run duration has elapsed.
 */
void durationElapsed(int value)
{
    deinit();
    exit(0);
}

/*!
 * The reshape callback for the window.
 *
 * \param [in] newWidth, newHeight
 *   Specify the new window size in pixels.
 */
void reshape(int newWidth, int newHeight)
{
    winWidth = newWidth;
    winHeight = newHeight;
}

/*!
 * The deinit callback after application window is closed.
 */
void deinit()
{
    EncodingThread *thread, *prevThread;
    unsigned int threadCount = 0;
    float frameRate = 0, threadFrameRate;

	bStop = true;
    thread = threadList;
    while (thread)
    {
        threadFrameRate = thread->getFrameRate();

        printf("Device %2d (%s) instance %2d: encoding rate %4.0f fps\n",
            thread->getDeviceIndex(), thread->getDeviceString(), thread->getDeviceInstance(), threadFrameRate);
        frameRate += threadFrameRate;
        threadCount++;

        thread = thread->m_next;
    }
    printf("Average encoding rate %4.0f fps\n", frameRate / threadCount);

    printf("Terminating threads ");

    // terminate and delete threads
    thread = threadList;
    while (thread)
    {
        if (!thread->terminate())
            return;

        printf(".");
        prevThread = thread;
        thread = thread->m_next;
        threadList = thread;
        delete prevThread;
    }
    printf(" done\n");
}

/*!
 * For each GPU in the system create a thread and start it.
 *
 * \return false if failed.
 */
bool createThreads()
{
    UINT GPUIdx;
    GPU_DEVICE gpuDevice;
    HGPUNV hGPU;
    EncodingThread *thread;

    gpuDevice.cb = sizeof(gpuDevice);
    GPUIdx = 0;

    // Iterate through all GPUs
    while (wglEnumGpusNV(GPUIdx, &hGPU))
    {
        // Now get the detailed information about display 0 of this device
        if (wglEnumGpuDevicesNV(hGPU, 0, &gpuDevice))
        {
            for (int instance = 0; instance < instances; instance++)
            {
                thread = new EncodingThread(fboWidth, fboHeight, GPUIdx, hGPU, instance, codecType);
                if (!thread)
                {
                    printf("Out of memory.\n");
                    return false;
                }

                if (!thread->startup())
                {
                    printf("Failed to start encoding thread.\n");
                    return false;
                }

                thread->m_next = threadList;
                threadList = thread;
            }
        }

        GPUIdx++;
    }

    return true;
}

/*!
 * The starting point of execution.
 *
 * \param argc [in]
 *   An integer that contains the count of arguments that follow in argv. The argc parameter is always greater than or equal to 1.
 * \param argv [in]
 *   An array of null-terminated strings representing command-line arguments entered by the user of the program.
 *
 * \return -1 on failure, else 0
 */
int main(int argc, char *argv[])
{
    printf("Version number of an application: %d\n\n", APP_VERSION);
    if (!commandline_parser(argc, argv, &duration, NULL, &fboWidth, &fboHeight, NULL, NULL, NULL, &instances, &codecType))
        return -1;

#if defined (__linux__)
    XInitThreads();
#endif

    // Intialize glut, create the window and setup the callbacks
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow(applicationName);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutCloseFunc(deinit);
    glutTimerFunc(1000, timer, 0);

    // Initialize glew to have access to the extensions
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW.\n");
        return -1;
    }

    // Check for required extensions
    if (!glewIsSupported("GL_EXT_framebuffer_object"))
    {
        fprintf(stderr, "The GL_EXT_framebuffer_object extension is required.\n");
        return -1;
    }

    if (!wglewIsSupported("WGL_NV_gpu_affinity"))
    {
        fprintf(stderr, "The WGL_NV_gpu_affinity extension is required.\n");
        return -1;
    }

    if (!createThreads())
    {
        return -1;
    }

    if (duration >= 0)
    {
        // setup a timer to exit after duration seconds
        glutTimerFunc(duration * 1000, durationElapsed, 0);
    }

    glutMainLoop();

    return 0;
}
