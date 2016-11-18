/*!
 * \file
 *   Shows how to asynchronously transfer the framebuffer while rendering the next
 *   frame. The framerate is printed to the window. Async transfers can be toggled
 *   by pressing 'a'.
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
#include <time.h>

#include "GL/glew.h"
#include "GL/freeglut_std.h"
#include "GL/freeglut_ext.h"
#include "GL/glext.h"

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#include "CommandLine.h"
#include "Timer.h"

// MS did not follow the naming convention
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
//! Version number of an application
#define APP_VERSION 2

//! the NvIFR API function list
static NvIFRAPI nvIFR;
//! the NvIFR session handle
static NV_IFROGL_SESSION_HANDLE sessionHandle;
//! the NvIFR transfer object handle
static NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle[2];

//! the width and height of the window
static unsigned int width = 256, height = 256;
//! the width and height of the framebuffer
static unsigned int fboWidth = 1024, fboHeight = 1024;

//! if set then use asyonchronous transfers
static bool useAsyncTransfer = true;
//! visibility state of the window
static int visibilityState = GLUT_VISIBLE;
//! frame rate measure start time
static timerValue startTime = 0;
//! frame counter
static unsigned int frameCounter = 0;
//! frame rate
static float frameRate = 0.;

//! time duration for which application to be run
static long duration = 0;
//! to track the start and end time of the app
static time_t start = 0;
static time_t end = 0;
//! total number of frames to be encoded
static long numFrames = 30;
//! The deinit callback after application window is closed.
static void deinit();

//! OpenGL object ID's
static GLuint fboID[2], texID[4], vboID, iboID;

/*!
 * The vertex data for a colored cube. Position data is interleved with
 * color data.
 */
static const GLfloat vertexData[] =
{
    // left-bottom-front
    -1.f, -1.f, -1.f,
     0.f,  0.f,  1.f,

    // right-bottom-front
     1.f, -1.f, -1.f,
     0.f,  0.f,  0.f,

    // right-top-front
     1.f,  1.f, -1.f,
     0.f,  1.f,  1.f,

    // left-top-front
    -1.f,  1.f, -1.f,
     0.f,  1.f,  0.f,

    // left-bottom-back
    -1.f, -1.f,  1.f,
     1.f,  0.f,  1.f,

    // right-bottom-back
     1.f, -1.f,  1.f,
     1.f,  0.f,  0.f,

    // right-top-back
     1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,

    // left-top-back
    -1.f,  1.f,  1.f,
     1.f,  1.f,  0.f,
};

/*!
 * The index data for a cube drawn using GL_QUADS.
 */
static const GLushort indexData[] =
{
     0, 1, 2, 3,    // front
     1, 5, 6, 2,    // right
     3, 2, 6, 7,    // top
     4, 0, 3, 7,    // left
     4, 5, 1, 0,    // bottom
     5, 4, 7, 6,    // back
};

/*
 * Call this function to have some text drawn at given coordinates
 *
 * \param [in] x,y
 *   Window coordinate where the text starts
 * \param [in] text
 */
static void printText(int x, int y, const char *text)
{
    // Prepare the OpenGL state
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0., width, 0., height, -1., 1.);

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
static void display()
{
    char textBuffer[32];
    float elapsedTime;
    int index;

    // if the window is not visible then do nothing
    if (visibilityState != GLUT_VISIBLE)
        return;

    if (startTime == 0)
        startTime = getTimeInuS();

    // Draw a colored cube to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fboID[frameCounter & 1]);
    glViewport(0, 0, fboWidth, fboHeight);

    glClearColor(0.2f, 0.4f, 0.7f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.,1., -1.,1., 1.,5.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.f, 0.f, -3.f);
    glRotatef(19.f, 0.f, 0.f, 1.f);
    glRotatef(50.f, 0.f, 1.f, 0.f);
    glRotatef(20.f, 1.f, 0.f, 0.f);
    glDrawElements(GL_QUADS, sizeof(indexData)/sizeof(indexData[0]), GL_UNSIGNED_SHORT, 0);

    // Start the transfer. This is non-blocking.
    if (nvIFR.nvIFROGLTransferFramebufferToSys(transferObjectHandle[frameCounter & 1], fboID[frameCounter & 1], GL_COLOR_ATTACHMENT0,
        NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_NONE, 0,0, 0,0) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to transfer data from the framebuffer.\n");
        exit(-1);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (useAsyncTransfer)
    {
        // async usage, map the previous transfer object. Note that there is
        // a one frame delay until the first frame data is available
        if (frameCounter > 0)
            index = (frameCounter - 1) & 1;
        else
            index = -1;
    }
    else
    {
        // synchronous usage, map the current transfer object
        index = frameCounter & 1;
    }

    if (index >= 0)
    {
        uintptr_t dataSize;
        const void *data;

        // lock the previously transferred data
        if (nvIFR.nvIFROGLLockTransferData(transferObjectHandle[index], &dataSize, &data) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to lock the transferred data.\n");
            exit(-1);
        }

        // here the data can be used to be saved on disk or streamed over the network

        // release the data buffer
        if (nvIFR.nvIFROGLReleaseTransferData(transferObjectHandle[index]) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to release the transferred data.\n");
            exit(-1);
        }
    }

    frameCounter++;
    elapsedTime = (float)(getTimeInuS() - startTime) / 1000000;

    // print the frame rate every second
    if (elapsedTime > 1.f)
    {
        frameRate = (float)frameCounter / elapsedTime;
        frameCounter = 0;
        startTime = 0;

        // draw the frame rate to the window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        if (useAsyncTransfer)
            printText(0, height - 12, "Asynchronous transfers enabled");
        else
            printText(0, height - 12, "Synchronous transfers enabled");
        snprintf(textBuffer, sizeof(textBuffer), "%4.0f fps\n", frameRate);
        printText(0, height - 2 * 12, textBuffer);

        glutSwapBuffers();
    }
    time(&end);
    if ((duration > 0) && ((end - start) > duration))
    {
        deinit();
        exit(0);
    }
    else if ((numFrames > 0) && (frameCounter >= (unsigned int)numFrames))
    {
        deinit();
        exit(0);
    }
}

/*!
 * The reshape callback for the window.
 *
 * \param [in] newWidth, newHeight
 *   Specify the new window size in pixels.
 */
static void reshape(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
}

/*!
 * The idle callback is continuously called when events are not being received.
 */
static void idle()
{
    // call the display function to get a constant animation
    display();
}

/*!
 * The visibility callback for a window is called when the visibility of a window changes.
 */
static void visibility(int state)
{
    visibilityState = state;
}

/*!
 * The keyboard callback for the current window.
 *
 * \param [in] key
 *   the ASCII character
 * \param [in] x, y
 *    mouse location in window relative coordinates when the key was pressed
 */
static void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'a':
        useAsyncTransfer = !useAsyncTransfer;
        break;
    }
}

/*!
 * The deinit callback after application window is closed.
 */
static void deinit()
{
    unsigned int i;

    // clean up
    for (i = 0; i < 2; i++)
    {
        if (nvIFR.nvIFROGLDestroyTransferObject(transferObjectHandle[i]) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to destroy the NvIFROGL transfer object.\n");
            return;
        }
    }

    if (nvIFR.nvIFROGLDestroySession(sessionHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to destroy the NvIFROGL session.\n");
        return;
    }

    glDeleteBuffers(1, &iboID);
    glDeleteBuffers(1, &vboID);
    glDeleteTextures(4, texID);
    glDeleteFramebuffers(2, fboID);
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
    NV_IFROGL_TO_SYS_CONFIG toSysConfig;
    unsigned int i;
    printf("Version number of an application: %d\n\n", APP_VERSION);
    // Parsing Command line arguments
    if (!commandline_parser(argc, argv, &duration, NULL, &fboWidth, &fboHeight, &numFrames, NULL, NULL, NULL, NULL))
    {
        return -1;
    }

    // Intialize glut, create the window and setup the callbacks
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCloseFunc(deinit);
    glutCreateWindow("NvIFROpenGLAsync");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutVisibilityFunc(visibility);
    glutKeyboardFunc(keyboard);

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

    printf(
        "This sample shows how to asynchronously transfer the framebuffer while\n"
        "rendering the next frame. The framerate is printed to the window.\n"
        "Async transfers can be toggled by pressing 'a'..\n");

    // Set up the frame buffer
    glGenTextures(4, texID);
    glGenFramebuffers(2, fboID);

    for (i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_RECTANGLE, texID[i * 2 + 0]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, fboWidth, fboHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindTexture(GL_TEXTURE_RECTANGLE, texID[i * 2 + 1]);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, fboWidth, fboHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        glBindTexture(GL_TEXTURE_RECTANGLE, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, fboID[i]);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texID[i * 2 + 0], 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texID[i * 2 + 1], 0);

        if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            fprintf(stderr, "Framebuffer is incomplete.\n");
            return -1;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Set up the geometry data
    glGenBuffers(1, &vboID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData, GL_STATIC_DRAW);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), 0);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    glGenBuffers(1, &iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), &indexData, GL_STATIC_DRAW);

    if (nvIFR.initialize() == false)
    {
        fprintf(stderr, "Failed to create a NvIFROGL instance.\n");
        return -1;
    }
   	printf("NvIFROpenGL API version number: %d.%d\n\n", ENCODEAPI_MAJOR(nvIFR), ENCODEAPI_MINOR(nvIFR));  
    // A session is required. The session is associated with the current OpenGL context.
    if (nvIFR.nvIFROGLCreateSession(&sessionHandle, NULL) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to create a NvIFROGL session.\n");
        return -1;
    }

    toSysConfig.format = NV_IFROGL_TARGET_FORMAT_CUSTOM;
    toSysConfig.flags = NV_IFROGL_TRANSFER_OBJECT_FLAG_NONE;
    toSysConfig.customFormat = GL_BGRA;
    toSysConfig.customType = GL_UNSIGNED_BYTE;

    for (i = 0; i < 2; i++)
    {
        // A transfer object defines the mode and format of a transfer. Here we
        // transfer to system memory because we need to access the data with the
        // CPU. We also use a custom target format.
        if (nvIFR.nvIFROGLCreateTransferToSysObject(sessionHandle, &toSysConfig, &transferObjectHandle[i]) != NV_IFROGL_SUCCESS)
        {
            fprintf(stderr, "Failed to create a NvIFROGL transfer object.\n");
            return -1;
        }
    }

    time(&start);
    // start
    glutMainLoop();

    return 0;
}
