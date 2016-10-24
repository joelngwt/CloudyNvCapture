/*!
 * \file
 *
 * This sample shows how to initialize NvIFROGL and transfer the contents of a
 * framebuffer to the host and write to a TGA file. A colored 3D cube is drawn
 * to the FBO.
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

#include "GL/glew.h"
#include "GL/freeglut_std.h"
#include "GL/freeglut_ext.h"
#include "GL/glext.h"

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#include "TGA.h"
#include "CommandLine.h"

//! the NvIFR API function list
static NvIFRAPI nvIFR;
//! the NvIFR session handle
static NV_IFROGL_SESSION_HANDLE sessionHandle;
//! the NvIFR transfer object handle
static NV_IFROGL_TRANSFEROBJECT_HANDLE transferObjectHandle;

//! the width and height of the window
static unsigned int width = 256, height = 256;

//! Output file name
static char output[MAX_OUTPUT_FILENAME_LENGTH] = "glIFR_SimpleSample_out.tga";
//! set to true, if we want to create the output file, false otherwise.
static bool isOutFile = true;

//! OpenGL object ID's
static GLuint fboID, texID[2], vboID, iboID;

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

/*!
 * Draw to a FBO, transfer the image and write it out to a TGA file.
 */
static bool transfer()
{
    uintptr_t dataSize;
    const void *data;

    // Draw a colored cube to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glViewport(0, 0, width, height);

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
    glRotatef(19.f, 0.f, 0.f, 1.f);
    glRotatef(50.f, 0.f, 1.f, 0.f);
    glRotatef(20.f, 1.f, 0.f, 0.f);
    glDrawElements(GL_QUADS, sizeof(indexData)/sizeof(indexData[0]), GL_UNSIGNED_SHORT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // transfer the FBO
    if (nvIFR.nvIFROGLTransferFramebufferToSys(transferObjectHandle, fboID, GL_COLOR_ATTACHMENT0,
        NV_IFROGL_TRANSFER_FRAMEBUFFER_FLAG_NONE, 0,0, 0,0) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to transfer data from the framebuffer.\n");
        return false;
    }

    // lock the transferred data
    if (nvIFR.nvIFROGLLockTransferData(transferObjectHandle, &dataSize, &data) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to lock the transferred data.\n");
        return false;
    }

    // save it to a TGA file
    if (isOutFile)
    {
        if (!saveAsTGA(output, 8 * 4, width, height, data))
        {
            fprintf(stderr, "Failed to write the transferred data to the image file.\n");
            return false;
        }
    }

    // release the data buffer
    if (nvIFR.nvIFROGLReleaseTransferData(transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to release the transferred data.\n");
        return false;
    }

    return true;
}

/*!
 * The deinit callback after application window is closed.
 */
static void deinit()
{
    // clean up
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
    glDeleteTextures(2, texID);
    glDeleteFramebuffers(1, &fboID);
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

    // Parsing Command line arguments
    if (!commandline_parser(argc, argv, NULL, output, &width, &height, NULL, &isOutFile, NULL, NULL, NULL))
    {
        return -1;
    }

    // Intialize glut, create the window and setup the callbacks
    glutInit(&argc, argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCloseFunc(deinit);
    glutCreateWindow("NvIFROpenGLSimple");

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

    printf("This sample shows how to initialize NvIFROGL and transfer the contents of a\n"
           "framebuffer to the host and write to a TGA file. A colored 3D cube is drawn\n"
           "to the FBO.\n");

    // Set up the frame buffer
    glGenTextures(2, texID);

    glBindTexture(GL_TEXTURE_RECTANGLE, texID[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_RECTANGLE, texID[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);

    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texID[0], 0);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texID[1], 0);

    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer is incomplete.\n");
        return -1;
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

    // A transfer object defines the mode and format of a transfer. Here we
    // transfer to system memory because we need to access the data with the
    // CPU. We also use a custom target format.
    toSysConfig.format = NV_IFROGL_TARGET_FORMAT_CUSTOM;
    toSysConfig.flags = NV_IFROGL_TRANSFER_OBJECT_FLAG_NONE;
    toSysConfig.customFormat = GL_BGRA;
    toSysConfig.customType = GL_UNSIGNED_BYTE;
    if (nvIFR.nvIFROGLCreateTransferToSysObject(sessionHandle, &toSysConfig, &transferObjectHandle) != NV_IFROGL_SUCCESS)
    {
        fprintf(stderr, "Failed to create a NvIFROGL transfer object.\n");
        return -1;
    }

    // generate the image, transfer it and write it out
    transfer();

    return 0;
}
