/*!
 * \file
 *
 * This sample shows how to use NvIFROGL to encode the contents of a
 * pbuffer with the hardware H.264 encoder.
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
#define GL_GLEXT_PROTOTYPES


#include <stdio.h>
#include <time.h>
#include <string.h>

#include "GL/glew.h"
#include "GL/freeglut_std.h"
#include "GL/freeglut_ext.h"
#include "GL/glext.h"

#ifdef _WIN32
#include "GL/wglew.h"
#else
#include <X11/X.h>
#include "GL/glx.h"
#endif

#ifdef _MSC_VER
// MS did not follow the naming convention
#define snprintf _snprintf
#endif

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#include "CommandLine.h"
#include "Timer.h"
#include "Util.h"

//! the NvIFR API function list
static NvIFRAPI nvIFR;
//! the NvIFR session handle
static NV_IFROGL_SESSION_HANDLE sessionHandle;
//! the NvIFR transfer object handle
static NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle;

//! width and height of the window
static unsigned int winWidth = 512, winHeight = 512;
//! width and height of the pbubffer
static unsigned int pbufferWidth = 512, pbufferHeight = 512;
//! frame counter
static unsigned int frameCounter = 0;
//! the frame at which the last screen update had been done
static unsigned int lastUpdateFrame = 0;
//! time the first frame had been drawn
static timerValue startTime = 0;
//! time at which the last screen update had been done
static float lastUpdateTime = 0.f;
//! framerate of encoded movie
static unsigned int framesPerSecond = 30;
//! output file
FILE *outFile;
//! output file name
static char outFileName[MAX_OUTPUT_FILENAME_LENGTH] = "";
//! time duration for which application to be run
static long duration = 0;
//! to track the start and end time of the app
static time_t start, end;
//! total number of frames to be encoded
static long numFrames = 30;
//! set to true, if we want to create the output file, false otherwise.
static bool isOutFile = true;
//! OpenGL object ID's
static GLuint vboID, iboID;

#ifdef _WIN32
HDC hDC;
HGLRC hGLRC;
HPBUFFERARB hPbuffer;
HDC hDCPbuffer;
HGLRC hGLRCPbuffer;
#else
Display *dpy;
GLXContext pbufferContext;
GLXPbuffer pbuffer;
GLXContext glutContext;
GLXDrawable glutWindow;
#endif

//! The deinit callback after application window is closed.
static void deinit();

/*!
 * The vertex data for a colored cube. Position data is interleaved with
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
static void display()
{
    char textBuffer[32];
    float elapsedTime;
    uintptr_t dataSize;
    const void *data;

    if (startTime == 0)
        startTime = getTimeInuS();

    // Draw a colored cube to the pbuffer
#ifdef _WIN32
    if (!wglMakeCurrent(hDCPbuffer, hGLRCPbuffer))
    {
        fprintf(stderr, "Failed to make pbuffer context current.\n");
        exit(-1);
    }
#else
    glXMakeCurrent(dpy, pbuffer, pbufferContext);
#endif

    glViewport(0, 0, pbufferWidth, pbufferHeight);
    glClearColor(0.2f, 0.4f, 0.7f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.,1., -1.,1., 1.,5.);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.f, 0.f, -3.f);
    // rotate it, else it's boring
    glRotatef(14.f * ((float)frameCounter / (float)framesPerSecond) + 5.f, 0.f, 0.f, 1.f);
    glRotatef(20.f * ((float)frameCounter / (float)framesPerSecond) + 30.f, 0.f, 1.f, 0.f);
    glRotatef(10.f * ((float)frameCounter / (float)framesPerSecond) + 10.f, 1.f, 0.f, 0.f);
    glDrawElements(GL_QUADS, sizeof(indexData)/sizeof(indexData[0]), GL_UNSIGNED_SHORT, 0);

    // transfer the FBO
    if (nvIFR.nvIFROGLTransferFramebufferToHwEnc(transferObjectHandle, NULL, 0, GL_BACK_LEFT, GL_NONE) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to transfer data from the pbuffer.\n");
        exit(-1);
    }

    // lock the transferred data
    if (nvIFR.nvIFROGLLockTransferData(transferObjectHandle, &dataSize, &data) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to lock the transferred data.\n");
        exit(-1);
    }

    // write to the file
    if (isOutFile)
        fwrite(data, 1, dataSize, outFile);

    // release the data buffer
    if (nvIFR.nvIFROGLReleaseTransferData(transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to release the transferred data.\n");
        exit(-1);
    }

    frameCounter++;
    elapsedTime = (float)(getTimeInuS() - startTime) / 1000000;

#ifdef _WIN32
    if (!wglMakeCurrent(hDC, hGLRC))
    {
        fprintf(stderr, "Failed to make glut context current.\n");
        exit(-1);
    }
#else
    glXMakeCurrent(dpy, glutWindow, glutContext);
#endif

    // print the frame rate every second
    if (elapsedTime - lastUpdateTime > 1.f)
    {
        float frameRate;

        frameRate = (float)(frameCounter - lastUpdateFrame) / (elapsedTime - lastUpdateTime);
        lastUpdateFrame = frameCounter;
        lastUpdateTime = elapsedTime;

        // draw the frame rate to the window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, winWidth, winHeight);

        snprintf(textBuffer, sizeof(textBuffer), "Encoding %dx%d at %4.0f fps\n", pbufferWidth, pbufferHeight, frameRate);
        printText(0, winHeight - 12, textBuffer);

        glutSwapBuffers();
    }
    time(&end);
    if ((duration > 0) && ((end - start) > duration))
    {
        if (isOutFile)
            fclose(outFile);
        deinit();
        exit(0);
    }
    else if ((numFrames > 0) && (frameCounter >= (unsigned int)numFrames))
    {
        if (isOutFile)
            fclose(outFile);
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
    winWidth = newWidth;
    winHeight = newHeight;
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
 * The deinit callback after application window is closed.
 */
static void deinit()
{
    if (nvIFR.nvIFROGLDestroyTransferObject(transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to destroy the NvIFROGL transfer object.\n");
        return;
    }

    if (nvIFR.nvIFROGLDestroySession(sessionHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to destroy the NvIFROGL session.\n");
        return;
    }

    glDeleteBuffers(1, &iboID);
    glDeleteBuffers(1, &vboID);

#ifdef _WIN32
    wglMakeCurrent(hDCPbuffer, NULL);
    wglDeleteContext(hGLRCPbuffer);
    wglReleasePbufferDCARB(hPbuffer, hDCPbuffer);
    wglDestroyPbufferARB(hPbuffer);
#else
    glXDestroyPbuffer(dpy, pbuffer);
    glXDestroyContext(dpy, pbufferContext);
    glXDestroyContext(dpy, glutContext);
#endif
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
    NV_IFROGL_HW_ENC_CONFIG config;
    NV_IFROGL_HW_ENC_TYPE codecType;

    // Parsing Command line arguments
    if (!commandline_parser(argc, argv, &duration, outFileName, &pbufferWidth, &pbufferHeight, &numFrames, &isOutFile, NULL, NULL, &codecType))
    {
        return -1;
    }

    // Intialize glut, create the window and setup the callbacks
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("NvIFROpenGLNVENCPbuffer");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutCloseFunc(deinit);

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

    printf("This sample shows how to use NvIFROGL to encode the contents of a\n"
       "pbuffer with the hardware H.264 encoder.\n");

#ifdef _WIN32
    int attrib[] =
    {
        0, 0,
    };
    hDC = wglGetCurrentDC();
    if (!hDC)
    {
        fprintf(stderr, "Failed to get current OpenGL DC.\n");
        return -1;
    }
    hGLRC = wglGetCurrentContext();
    if (!hGLRC)
    {
        fprintf(stderr, "Failed to get current OpenGL context.\n");
        return -1;
    }
    hPbuffer = wglCreatePbufferARB(hDC, GetPixelFormat(hDC), pbufferWidth, pbufferHeight, attrib);
    if (!hPbuffer)
    {
        fprintf(stderr, "Failed to create pbuffer.\n");
        return -1;
    }
    hDCPbuffer = wglGetPbufferDCARB(hPbuffer);
    if (!hDCPbuffer)
    {
        fprintf(stderr, "Failed to get pbuffer DC.\n");
        return -1;
    }

    hGLRCPbuffer = wglCreateContext(hDCPbuffer);
    if (!hGLRCPbuffer)
    {
        fprintf(stderr, "Failed to create pbuffer context.\n");
        return -1;
    }
    if (!wglMakeCurrent(hDCPbuffer, hGLRCPbuffer))
    {
        fprintf(stderr, "Failed to make pbuffer context current.\n");
        return -1;
    }
#else
    dpy = glXGetCurrentDisplay();
    glutWindow = glXGetCurrentDrawable();

    int scrnum;
    GLXFBConfig *fbconfig;
    XVisualInfo *visinfo;
    int nitems;

    int attrib[] = {
        GLX_DOUBLEBUFFER,  True,
        GLX_RED_SIZE,      1,
        GLX_GREEN_SIZE,    1,
        GLX_BLUE_SIZE,     1,
        GLX_DEPTH_SIZE,    1,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
        None
    };

    int pbufAttrib[] = {
        GLX_PBUFFER_WIDTH,   pbufferWidth,
        GLX_PBUFFER_HEIGHT,  pbufferHeight,
        GLX_LARGEST_PBUFFER, False,
        None
    };

    scrnum = DefaultScreen(dpy);

    fbconfig = glXChooseFBConfig(dpy, scrnum, attrib, &nitems );
    if (NULL == fbconfig) {
        printf("Couldn't get fbconfig\n");
        exit( 1 );
    }

    pbuffer = glXCreatePbuffer(dpy, fbconfig[0], pbufAttrib);

    visinfo = glXGetVisualFromFBConfig(dpy, fbconfig[0]);

    if (!visinfo) {
        printf("Couldn't get an RGBA, double-buffered visual\n");
        exit( 1 );
    }

    glutContext = glXCreateContext(dpy, visinfo, NULL, GL_TRUE);
    glXMakeCurrent(dpy, glutWindow, glutContext);

    pbufferContext = glXCreateContext(dpy, visinfo, glutContext, GL_TRUE );

    if (!pbufferContext) {
        printf("Call to glXCreateContext failed!\n");
        exit( 1 );
    }

    XFree(fbconfig);
    XFree(visinfo);

    glXMakeCurrent(dpy, pbuffer, pbufferContext);
#endif

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

    memset(&config, 0, sizeof(config));

    config.profile = (codecType == NV_IFROGL_HW_ENC_H264 ) ? 100 : 1;
    config.frameRateNum = framesPerSecond;
    config.frameRateDen = 1;
    config.width = pbufferWidth;
    config.height = pbufferHeight;
    config.avgBitRate = calculateBitrate(pbufferWidth, pbufferHeight);
    config.GOPLength = 75;
    config.rateControl = NV_IFROGL_HW_ENC_RATE_CONTROL_CBR;
    config.stereoFormat = NV_IFROGL_HW_ENC_STEREO_NONE;
    config.preset = NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_HP;
    config.codecType = codecType;
	config.VBVBufferSize = config.avgBitRate;
	config.VBVInitialDelay = config.VBVBufferSize;

    if (nvIFR.nvIFROGLCreateTransferToHwEncObject(sessionHandle, &config, &transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to create a NvIFROGL transfer object.\n");
        return -1;
    }

#ifdef _WIN32
    if (!wglMakeCurrent(hDC, hGLRC))
    {
        fprintf(stderr, "Failed to make glut context current.\n");
        return -1;
    }
#else
    glXMakeCurrent(dpy, glutWindow, glutContext);
#endif

    if (isOutFile)
    {
		if (!*outFileName) {
			strcpy(outFileName, codecType == NV_IFROGL_HW_ENC_H265 ? "glIFR_Pbuffer_out.h265" : "glIFR_Pbuffer_out.h264");
		}
        outFile = fopen(outFileName, "wb");
        if (outFile == NULL)
        {
            fprintf(stderr, "Failed to create output file.\n");
            exit(-1);
        }
    }

    time(&start);
    glutMainLoop();

    fclose(outFile);

    return 0;
}
