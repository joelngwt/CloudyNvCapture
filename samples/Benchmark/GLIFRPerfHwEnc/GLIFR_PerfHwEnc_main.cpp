/*!
 * \file
 *  Performance test for OpenGL NvIFR capture and H264 encoding,
 *  with following parameters -
 *      Rate control mode: 2_PASS_QUALITY
 *      Preset           : LOW_LATENCY_HP
 *      Resolution       : 720p
 *      FramesPerSecond  : 30
 *      Bitrate          : 5000 * 1000
 *
 * \copyright    
 *
 * Copyright 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 *
 */


#include <getopt.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <GL/glew.h>
#include <GL/glext.h>

#include "NvIFR/NvIFROpenGL.h"
#include "NvIFR_API.h"
#include "Helper.h"
#include "OpenGLWin.h"
#include "ImageCollection.h"
#include "IFRObjects.h"
#include "Thread.h"
#include "Timer.h"

/*
 * Data need to store with each encoding thread.
 */
typedef struct {
    unsigned int streamID;
    OpenGLDisplay dpy;
    OpenGLWin win;

    unsigned int frameCounter;
    float        elapsedTime;

    float frameRate;

    Thread thread;
} EncoderData;
typedef EncoderData *EncoderDataPtr;

/*
 * Options available with this test application.
 */
static const struct option longOptions[] = {
    {"help",        0, NULL, 'h'},
    {"number",      1, NULL, 'n'},
    {"dataset",     1, NULL, 'd'},
    {"output",      0, NULL, 'o'},
    {"time",        1, NULL, 't'},
    {"yuv444",      0, NULL, 'y'},
    {"avgBitrate",  1, NULL, 'b'},
    {"codec",       1, NULL, 'c'},
    {"lowLatency",  0, NULL, 'w'},
    {"preset",      1, NULL, 'p'},
    {"rcMode",      1, NULL, 'r'},
    {NULL, 0, NULL, 0}
};
static const char* const shortOptions = "hn:d:ot:yb:c:wp:r:" ;

/*
 * Data Set directory Name
 */
static char *dataSetName = NULL;
/*
 * Flag to generate .h264 output file for each encoding thread.
 */
static bool generateOutputFiles = false;

#define WIN_TITLE "NVIFROpenGLNVENCPerf"

#define DEFAULT_WIN_WIDTH 1280
#define DEFAULT_WIN_HEIGHT 720

#define DEFAULT_BITRATE           5000
#define DEFAULT_FRAMES_PER_SECOND 30

#define DEFAULT_NUMBER_OF_ENCODERS 4
#define MAX_NUMBER_OF_ENCODERS 32
#define MAX_INPUT_SIZE         512

#define DEFAULT_TIMER 60

static unsigned int frameWidth  = DEFAULT_WIN_WIDTH,
                    frameHeight = DEFAULT_WIN_HEIGHT;

static OpenGLDisplay sharedDpy;
static OpenGLWin sharedWin;

static GLuint       sharedTexIDs[MAX_INPUT_SIZE];
static unsigned int totalTexIDs = MAX_INPUT_SIZE;

static unsigned int numberOfEncoders = DEFAULT_NUMBER_OF_ENCODERS;

static unsigned int bitrate         = DEFAULT_BITRATE;
static unsigned int framesPerSecond = DEFAULT_FRAMES_PER_SECOND;

static bool bYuv444 = false;      
static int nAvgBitrate; 
static NV_IFROGL_HW_ENC_TYPE eCodec = NV_IFROGL_HW_ENC_H264;
static bool bLowLatency = false;
static NV_IFROGL_HW_ENC_PRESET ePreset = NV_IFROGL_H264_PRESET_LOW_LATENCY_HP;     
static NV_IFROGL_HW_ENC_RATE_CONTROL eRcMode = NV_IFROGL_H264_RATE_CONTROL_2_PASS_QUALITY;

/* Encoder configuration same for all encoding threads */
static H264Config econfig;

volatile static bool stopEncoding = true;
static unsigned int  timer = DEFAULT_TIMER;

static EncoderData encoderData[MAX_NUMBER_OF_ENCODERS];

static Thread displayThread, stoperThread;

static void         printUsage(FILE *stream);
static void         parseCommandLine(int argc, char *argv[]);
static unsigned int encoderThreadFunction(void *data);
static int          encodeTextureCollection(EncoderDataPtr eData,
                                            IFRTransferObject *transferObj);
static unsigned int displyThreadFunction(void *data);
static unsigned int calculateAverageFps();
static inline bool  initGlew(void);

int main(int argc, char *argv[])
{
    ImageCollection *collection = NULL;
    ImageObject *img = NULL;

#ifdef _WIN32
    size_t dwCaptureSDKPathSize = 0;
    char *pCaptureSDKPath = NULL;
    char *pDatasetPath = NULL;
#endif
    unsigned int texIndex = 0;

    parseCommandLine(argc, argv);

    /* Calculate Frame(Texture) size. */
    {
        ImageObject *img = NULL;

        collection = new ImageCollection(IMAGE_TYPE_BMP);
        if (collection == NULL)
        {
            PRINT_ERROR_AND_EXIT("Failed to Create image collection\n");
        }

        if (dataSetName == NULL) {
#ifdef _WIN32
            if(0 == _dupenv_s(&pCaptureSDKPath, &dwCaptureSDKPathSize, "CAPTURESDK_PATH") && 0 != dwCaptureSDKPathSize) {
                dataSetName = new char[dwCaptureSDKPathSize + 20];
                sprintf(dataSetName, "%s\\datasets\\msenc", pCaptureSDKPath);
            
                // Test to see whether we have a BMP file in the "dataSetName" folder
                collection->load(dataSetName);
                img = collection->getNextImage();
                if (img == NULL) {
                    printf("DataSet is not provided, and no BMP image found in %s\n", dataSetName);
                    dataSetName = NULL;
                } else {
                    printf("DataSet is set to directory %s\n", dataSetName);
                }
            } else {
                printf("Warning: Unable to get the CAPTURESDK_PATH environment variable, thus cannot locate datasets\\msenc folder\n");
            }
#endif
            if (dataSetName == NULL) {
                dataSetName = ".";
                printf("DataSet is set to current directory \".\"\n");
            }
        }

        collection->load(dataSetName);

        img = collection->getNextImage();
        if (img == NULL)
        {
            PRINT_ERROR_AND_EXIT("No BMP image found. Please place a BMP image in directory \"%s\"\n", dataSetName);
        }

        frameWidth  = img->getWidth() / 16 * 16;
        frameHeight = img->getHeight() / 8 * 8;

        delete img;
        collection->rewind();
    }

    EXIT_IF_FAILED(sharedDpy.open());
    EXIT_IF_FAILED(sharedWin.open(&sharedDpy, WIN_TITLE, frameWidth, frameHeight, NULL));

    /* Bind OpenGL window to current thread for rendering. */
    EXIT_IF_FAILED(sharedWin.makeCurrent());

    EXIT_IF_FAILED(initGlew());

    if (collection->getCount() < totalTexIDs)
    {
        totalTexIDs = collection->getCount();
    }

    /* Limit textures to 70% of available video memroy */
    {
        #define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX   0x9048
        #define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

        GLint curAvailableVideoMemory = 0;
        /* RGBA 4 bytes per pixel */
        GLint textureSize = (frameWidth * frameHeight) * 4;

        GLfloat useableMem = 0.0f;

        glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,
                      &curAvailableVideoMemory);

        PRINT_WITH_TIMESTAMP("Available Video Memory: 0x%x KB\n", curAvailableVideoMemory);
        useableMem = curAvailableVideoMemory * (70.0f / 100.0f);

        if (((useableMem * 1024) / textureSize) < totalTexIDs)
        {
            totalTexIDs = (unsigned int)(useableMem * 1024 / textureSize);
        }

        PRINT_WITH_TIMESTAMP("Filling 0x%x KB of Video Memory with %u textures "
                "each of dimensions %dx%d and size 0x%x KB\n",
                (int)useableMem, totalTexIDs, frameWidth, frameHeight, textureSize/1024);

    }

    if (totalTexIDs <= 0)
    {
        PRINT_ERROR_AND_EXIT("No Video memory available to load textures.\n");
    }

    PRINT_WITH_TIMESTAMP("Loading images into textures...\n");

    texIndex = 0;
    collection->rewind();
    while (texIndex < totalTexIDs && (img = collection->getNextImage()) != NULL)
    {
        sharedTexIDs[texIndex] = img->loadToTexture(0);

        delete img;
        texIndex++;
    }
    collection->rewind();

    PRINT_WITH_TIMESTAMP("Done...\n");

    /* Setup encoder configuration, which is same for all encoders */
    {
        memset(&econfig, 0, sizeof(econfig));
        econfig.profile      = eCodec == NV_IFROGL_HW_ENC_H264 ? 100 : 1;
        econfig.frameRateNum = framesPerSecond;
        econfig.frameRateDen = 1;
        econfig.width        = frameWidth;
        econfig.height       = frameHeight;
        econfig.avgBitRate   = nAvgBitrate ? nAvgBitrate : (unsigned int)((bitrate * 1000) *((econfig.width * econfig.height)/ (1280.0f * 720.0f)));
        econfig.peakBitRate  = econfig.avgBitRate;
        econfig.GOPLength    = 0xFFFFFFFF;
        econfig.codecType    = eCodec;
        econfig.rateControl  = eRcMode;
        econfig.preset       = ePreset;
        econfig.hwEncInputFormat = bYuv444 ? NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV444 : NV_IFROGL_HW_ENC_INPUT_FORMAT_YUV420;
        econfig.stereoFormat = NV_IFROGL_H264_STEREO_NONE;
        econfig.VBVBufferSize   =  bLowLatency ? econfig.avgBitRate / econfig.frameRateNum : econfig.avgBitRate;
        econfig.VBVInitialDelay = econfig.VBVBufferSize;
    }


    PRINT_WITH_TIMESTAMP("Launching Encoding Threads...\n");

    if (!initilizeIFRApi())
    {
        PRINT_ERROR_AND_EXIT("Failed to initialize IFR Apis\n");
    }

    /*
     * Allow encoding threads to encode.
     */
    stopEncoding = false;

    for (unsigned int index = 0; index < numberOfEncoders; index++)
    {
        EncoderDataPtr eData = &encoderData[index];

        EXIT_IF_FAILED(eData->dpy.open());
        EXIT_IF_FAILED(eData->win.open(&eData->dpy, WIN_TITLE, frameWidth, frameHeight, &sharedWin));

        eData->streamID = index;

        PRINT_WITH_TIMESTAMP("  Launching Stream-%02d\n", eData->streamID);

        if (!eData->thread.create(encoderThreadFunction, (void*)eData))
        {
            PRINT_ERROR_AND_EXIT("Failed to create thread for Stream-%02d\n",
                                 eData->streamID);
        }
    }

    PRINT_WITH_TIMESTAMP("Done...\n");

    if (!displayThread.create(displyThreadFunction, NULL))
    {
        PRINT_ERROR_AND_EXIT("Failed to create display thread\n");
    }

    /*
     * Sleep main thread for test time suppiled through -time option.
     */
#ifdef _WIN32
    Sleep(timer * 1000);
#else
    sleep(timer);
#endif

    /*
     * Tells encoding thread to stop encoding.
     */
    stopEncoding = true;

    /*
     * Wait for display thread to exit.
     */
    if (!displayThread.waitForExit())
    {
        PRINT_ERROR_AND_EXIT("Failed to wait for display thread\n");
    }

    PRINT_WITH_TIMESTAMP("Waiting for threads to exit...\n");

    /*
     * Wait for all encoding threads to exit.
     */
    for (unsigned int index = 0; index < numberOfEncoders; index++)
    {
        EncoderDataPtr eData = &encoderData[index];

        if (!eData->thread.waitForExit())
        {
            PRINT_ERROR_AND_EXIT("Faild to wait for thread of stream-%02d\n",
                    eData->streamID);
        }

        eData->win.close();
        eData->dpy.close();
    }

    glDeleteTextures(totalTexIDs, sharedTexIDs);

    sharedWin.loseCurrent();
    sharedWin.close(); sharedDpy.close();

    delete collection;

    PRINT_WITH_TIMESTAMP("All Done Average FPS is %u fps ...\n", calculateAverageFps());

    return 0;
}

static void printUsage(FILE *stream)
{
    fprintf(stream, "Usage: %s options [inputDirectory]\n", "NVIFROpenGLNVENCPerf");
    fprintf(stream,
            "  -h --help                   Display Usage information\n"
            "  -n --number                 Number of encoding threads\n"
            "  -d --dataset directoryName  "
                "Directory which include input images\n"
            "  -o --output                 "
                "Generate .h264 output files for each stream\n"
            "  -t --time                   Test timer in mins\n"
            "  -y --yuv444                 Use YUV444 encoding\n"
            "  -b --avgBitrate             Average bit rate in bps\n"
            "  -c --codec                  0 for H264 and 1 for H265 encoding\n"
            "  -w --lowLatency             Use low latency encoding\n"
            "  -p --preset                 Preset: DEFAULT=0, HP=1, HQ=2, LOSSLESS_HP=3\n"
            "  -r --rcMode                 Rate control mode: CONSTQP=0, VBR=1, CBR=2,\n"
            "                              VBR_MINQP=3, 2_PASS_QUALITY=4,\n"
            "                              2_PASS_FRAMESIZE_CAP=5, CBR_IFRAME_2_PASS=6\n"
            );
}

static void parseCommandLine(int argc, char *argv[])
{
    int nextOption;

    do {
        nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);

        switch (nextOption)
        {
            case 'h':
                printUsage(stdout);
                exit(0);
            case 'n':
                {
                    unsigned int num = atoi(optarg);

                    if (num < MAX_NUMBER_OF_ENCODERS)
                    {
                        numberOfEncoders = num;
                    }
                }
                break;
            case 'd':
                dataSetName = optarg;
                break;
            case 'o':
                generateOutputFiles = true;
                break;
            case 't':
                {
                    unsigned int min = atoi(optarg);

                    timer = min * 60;
                    timer = (timer != 0) ? timer : DEFAULT_TIMER;
                }
                break;
            case 'y':
                bYuv444 = true;
                break;
            case 'b':
                if (atoi(optarg) > 0) {
                    nAvgBitrate = atoi(optarg);
                } else {
                    PRINT_ERROR_AND_EXIT("Bitrate must be positive\n");
                }
                break;
            case 'c' :
                if (atoi(optarg) == 0) {
                    eCodec = NV_IFROGL_HW_ENC_H264;
                } else if (atoi(optarg) == 1) {
                    eCodec = NV_IFROGL_HW_ENC_H265;
                }
                break;
            case 'w':
                bLowLatency = true;
                break;
            case 'p':
                if (atoi(optarg) >= NV_IFROGL_HW_ENC_PRESET_LOW_LATENCY_DEFAULT 
                        && atoi(optarg) <= NV_IFROGL_HW_ENC_PRESET_LOSSLESS_HP) {
                    ePreset = (NV_IFROGL_HW_ENC_PRESET)atoi(optarg);
                }
                break;
            case 'r':
                if (atoi(optarg) >= NV_IFROGL_HW_ENC_RATE_CONTROL_CONSTQP
                        && atoi(optarg) <= NV_IFROGL_HW_ENC_RATE_CONTROL_CBR_IFRAME_2_PASS) {
                    eRcMode = (NV_IFROGL_HW_ENC_RATE_CONTROL)atoi(optarg);
                }
                break;
            case '?':
                PRINT_ERROR_AND_EXIT("Unknown Option\n");
                exit(1);
        }
    } while (nextOption != -1);
}

static unsigned int encoderThreadFunction(void *data)
{
    EncoderDataPtr eData = (EncoderDataPtr)data;

    IFRSessionObject session;
    IFRTransferObject transferObj(&session);

    int ret = 1;

    /* Bind OpenGL window to current thread for rendering. */
    EXIT_IF_FAILED(eData->win.makeCurrent());

    EXIT_IF_FAILED(session.open());

    EXIT_IF_FAILED(transferObj.setEncodeConfig(econfig));

    ret = encodeTextureCollection(eData, &transferObj);
    if (ret)
    {
        PRINT_ERROR_AND_EXIT("Failed to encode texture collection\n");
    }

    EXIT_IF_FAILED(transferObj.unsetEncodeConfig());

    EXIT_IF_FAILED(session.close());

    eData->win.loseCurrent();

    return ret;
}


static int encodeTextureCollection(EncoderDataPtr eData,
                                   IFRTransferObject *transferObj)
{
    char outFileName[124];
    FILE *outFile = NULL;

    GLuint fboID = 0;

    timerValue   startTime = 0, totalTime = 0;

    unsigned int texIndex = 0;

    if (generateOutputFiles)
    {
        if (eCodec == NV_IFROGL_HW_ENC_H264) {
            sprintf(outFileName, "glIFR_Perf_stream-%05d.h264", eData->streamID);
        } else {
            sprintf(outFileName, "glIFR_Perf_stream-%05d.h265", eData->streamID);
        }
        outFile = fopen(outFileName, "wb");
        if (outFile == NULL)
        {
            perror("File Error");
            PRINT_ERROR_AND_EXIT("Failed to open output file\n");
        }
    }

    glGenFramebuffers(1, &fboID);

    while (!stopEncoding && texIndex < totalTexIDs)
    {
        H264EncodeParams eparams;

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sharedTexIDs[texIndex], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            PRINT_ERROR_AND_EXIT("Framebuffer is incomplete.\n");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glFinish();

        memset(&eparams, 0, sizeof(eparams));
        eparams.encodeParms           = NULL;
        eparams.stereoAttachmentRight = GL_NONE;

        startTime = getTimeInuS();

        if (transferObj->transferEncodeParams(fboID, GL_COLOR_ATTACHMENT0, eparams))
        {
            if (transferObj->lock())
            {
                if (generateOutputFiles)
                {
                    transferObj->writeToFile(outFile);
                }
                if (!transferObj->release())
                {
                    PRINT_ERROR_AND_EXIT("Failed to Release encoder\n");
                }
            } else {
                PRINT_ERROR_AND_EXIT("Failed to Lock Encoder\n");
            }
        } else {
            PRINT_ERROR_AND_EXIT("Failed to transfer encoder parameters\n");
        }

        eData->frameCounter++;
        totalTime += (getTimeInuS() - startTime);
        eData->elapsedTime = totalTime / 1000000.0f;

        if (eData->elapsedTime > 1.0f)
        {
            eData->frameRate = (float)eData->frameCounter / eData->elapsedTime;
        }

        texIndex++;
        if (texIndex >= totalTexIDs)
        {
            texIndex = 0;
        }
    }

    glDeleteFramebuffers(1, &fboID);

    if (generateOutputFiles)
    {
        fclose(outFile);
    }

    return 0;
}

static unsigned int displyThreadFunction(void *data)
{
    PRINT_WITH_TIMESTAMP("");
    for (unsigned int index = 0; index < numberOfEncoders; index++)
    {
        EncoderDataPtr eData = &encoderData[index];

        PRINT("[ Stream-%02d ]", eData->streamID);
    }
    PRINT("\n");

    while (!stopEncoding)
    {
        PRINT("\r");
        PRINT_WITH_TIMESTAMP("");
        for (unsigned int index = 0; index < numberOfEncoders; index++)
        {
            EncoderDataPtr eData = &encoderData[index];

            PRINT("[ %9.2f ]", eData->frameRate);
        }
#ifdef _WIN32
        Sleep(1 * 1000);
#else
        sleep(1);
#endif
    }
    PRINT("\n");

    for (unsigned int index = 0; index < numberOfEncoders; index++)
    {
        EncoderDataPtr eData = &encoderData[index];

        PRINT_WITH_TIMESTAMP("Stream-%02d - %u Frames of dimension %dx%d are captured and encoded in %f sec \n",
                             eData->streamID, eData->frameCounter, frameWidth, frameHeight, eData->elapsedTime);
    }

    return 0;
}

static unsigned int calculateAverageFps()
{
    float total = 0;

    for (unsigned int index = 0; index < numberOfEncoders; index++)
    {
        EncoderDataPtr eData = &encoderData[index];

        total += eData->frameRate;
    }

    return (unsigned int)(total / numberOfEncoders);
}

static inline bool initGlew(void)
{
    /*Initialize glew to have access to the extensions. */
    if (glewInit() != GLEW_OK)
    {
        PRINT_ERROR("Failed to initialize GLEW.\n");
        return false;
    }

    /* Check for required extensions. */
    if (!glewIsSupported("GL_EXT_framebuffer_object"))
    {
        PRINT_ERROR("The GL_EXT_framebuffer_object extension is required.\n");
        return false;
    }

    return true;
}

