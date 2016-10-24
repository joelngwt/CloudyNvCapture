/*!
 * \file
 *
 * This sample shows how to use NvIFROGL to get encode caps of 
 * the hardware encoder for given codecType
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
#include <string.h>

#include "GL/glew.h"
#include "GL/freeglut_std.h"
#include "GL/freeglut_ext.h"
#include "GL/glext.h"

#ifdef _MSC_VER
// MS did not follow the naming convention
#define snprintf _snprintf
#endif

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#include "CommandLine.h"
#include "Timer.h"
#include "Util.h"

//! Version number of an application
#define APP_VERSION 1
#define YN(cond) ((cond) ? "Yes" : "No")
//! the NvIFR API function list
static NvIFRAPI nvIFR;
//! the NvIFR session handle
static NV_IFROGL_SESSION_HANDLE sessionHandle;
//! width and height of the window
static unsigned int winWidth = 256, winHeight = 256;
//! The deinit callback after application window is closed.
static void deinit();


/*!
 * The deinit callback after application window is closed.
 */
static void deinit()
{

    if (nvIFR.nvIFROGLDestroySession(sessionHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to destroy the NvIFROGL session.\n");
        return;
    }

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
    NV_IFROGL_HW_ENC_TYPE codecType;
    NV_HW_ENC_GET_CAPS pGetCaps;
    printf("Version number of an application: %d\n\n", APP_VERSION);
    // Parsing Command line arguments
    if (!commandline_parser(argc, argv, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &codecType))
    {
        return -1;
    }

    // Intialize glut, create the window and setup the callbacks
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("NvIFROpenGLNVENCGetCaps");
    glutCloseFunc(deinit);


    printf("This sample shows how to use NvIFROGL to get the encodecaps of \n"
           "the hardware encoder.\n");



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


    memset(&pGetCaps, 0, sizeof(pGetCaps));
    pGetCaps.ifrOGLVersion = NVIFROGL_VERSION;
    pGetCaps.eCodec = codecType;
    if (nvIFR.nvIFROGLGetHwEncCaps(&pGetCaps, sessionHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Call to NvIFROGLGetHwEncCaps failed.\n");
        return -1;
    }
    printf("Hw Encoder Capabilities for ");
    if(codecType == NV_IFROGL_HW_ENC_H264)
        printf("H.264 \n");
    else if (codecType == NV_IFROGL_HW_ENC_H265)
        printf("HEVC/H.265 \n");

    printf("- Codec support: %s\n", YN(pGetCaps.codecSupported));

    printf("- YUV444 support: %s\n", YN(pGetCaps.yuv444Supported));
    printf("- Lossless support: %s\n", YN(pGetCaps.losslessEncodingSupported));

    printf("- Maximum frame resolution: %ux%u\n",
           pGetCaps.maxEncodeWidthSupported, pGetCaps.maxEncodeHeightSupported);
    printf("- Maximum frame size: %uMB\n", pGetCaps.maxMBSupported);
    printf("- Maximum throughput: %uMB/s\n", pGetCaps.maxMBPerSecSupported);

    printf("- Rate control capabilities:\n");
    printf("  - Constant QP: %s\n", YN(pGetCaps.rcConstQPSupported));
    printf("  - Variable bitrate mode: %s\n", YN(pGetCaps.rcVbrSupported));
    printf("  - Constant bitrate mode: %s\n", YN(pGetCaps.rcCbrSupported));
    printf("  - Multi pass for image quality: %s\n", YN(pGetCaps.rc2PassQualitySupported));
    printf("  - Multi pass for constant frame size: %s\n", YN(pGetCaps.rc2PassFramesizeCapSupported));
    printf("- IntraRefreshSupported: %s\n", YN(pGetCaps.intraRefreshSupported));
    printf("- Supports Reference Picture Invalidation: %s\n", YN(pGetCaps.refPicInvalidateSupported));

    printf("- Dynamic resolution change: %s\n", YN(pGetCaps.dynamicResolutionSupported));
    printf("- Dynamic bitrate change: %s\n", YN(pGetCaps.dynamicBitRateSupported));

    printf("- Intra refresh: %s\n", YN(pGetCaps.intraRefreshSupported));
    printf("- Custom VBV buffer size: %s\n", YN(pGetCaps.customVBVBufSizeSupported));


    return 0;
}
