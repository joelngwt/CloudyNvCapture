/*!
 * \brief
 * Demonstrates using the NvFBCToCuda interface to copy the desktop
 * into a CUDA buffer and then send it to NVENC for hardware encode
 * 
 * \file
 * This sample demonstrates using the NvFBCToCuda interface to copy the desktop 
 * into a CUDA buffer.  From the CUDA Buffer, this can be transferred to NVENC
 * where the H.264 video encoder can encode the stream.
 * 
 * Encoder.h: Declares the Encoder class, this class is simply a wrapper around 
 * the CUDA video encoder API.
 * 
 * Encoder.cpp: Defines the Encoder class, wraps up the details when using the 
 * CUDA video encoder so they don't detract from the NvFBCCuda example.
 *
 * NvFBCCudaNvEnc.cpp: Demonstrates usage of the NvFBCToCuda interface.  
 * It demonstrates how to load vFBC.dll, initialize NvFBC function pointers, 
 * create an instance of NvFBCCuda and using NvFBCCuda to copy the frame buffer 
 * into a CUDA device pointer, where is then passed to NVENC to generate a H.264 
 * video.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#include <windows.h>
//#include <atltime.h>
#include <stdio.h> 

#include <string>

#include <cuda.h>
#include "helper_cuda_drvapi.h"

#include <Timer.h>

#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCCuda.h>

#include <cuda_runtime_api.h>

#include "Encoder.h"
#include "bitmap.h"

//! Number of frames to record.
#define FRAME_COUNT 30

//! Starting encoder bitrate
#define ENCODER_BITRATE 8000000

//! Command line arguments
typedef struct
{
    int         iFrameCnt; // Number of frames to encode
    int         iBitrate; // Bitrate to use
    std::string sBaseName; // Basename for the output stream
}AppArguments;

FILE *GetOutputFile(std::string baseName, DWORD width, DWORD height)
{
    FILE *output = NULL;

    char baseFileName[256];

    sprintf(baseFileName, "%s-%dx%d.h264", baseName.c_str(), width, height);

    output = fopen(baseFileName, "wb");

    if(NULL == output)
    {
        fprintf(stderr, "Failed to open the file %s for writing.\n", baseFileName);
        return NULL;
    }

    return output;
}

void printHelp()
{
    printf("Usage NvFBCCudaNvEnc: [-bitrate bitrate] [-frames framecnt] [-output filename]\n");
    printf("  -bitrate bitrate      The bitrate to encode at.\n");
    printf("  -frames framecnt      The number of frames to encode.\n");
    printf("  -output filename      The base name for output stream.\n");
}

bool parseArgs(int argc, char **argv, AppArguments &args)
{
    args.iBitrate = ENCODER_BITRATE;
    args.iFrameCnt = FRAME_COUNT;
    args.sBaseName = "NvFBCCudaNvEnc";

    for(int cnt = 1; cnt < argc; ++cnt)
    {
        if(0 == STRCASECMP(argv[cnt], "-bitrate"))
        {
            ++cnt;

            if(cnt < argc)
            {
                args.iBitrate = atoi(argv[cnt]);
            }
            else
            {
                printf("Missing bitrate argument\n");
                printHelp();
                return false;
            }
        }
        else if(0 == STRCASECMP(argv[cnt], "-frames"))
        {
            ++cnt;

            if(cnt < argc)
            {
                args.iFrameCnt = atoi(argv[cnt]);
            }
            else
            {
                printf("Missing frame count argument\n");
                printHelp();
                return false;
            }
        }
        else if(0 == STRCASECMP(argv[cnt], "-output"))
        {
            ++cnt;

            if(cnt < argc)
            {
                args.sBaseName = argv[cnt];
            }
            else
            {
                printf("Missing output argument\n");
                printHelp();
                return false;
            }
        }
        else
        {
            printf("Unexpected argument: %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }

    return true;
}


// Function used to launch the CUDA post-processing
extern "C" cudaError launch_CudaARGB2NV12Process(int w, int h, CUdeviceptr pARGBImage, CUdeviceptr pNV12Image);

/*!
 * Main program
 */
int main(int argc, char *argv[])
{
    AppArguments args;

    FILE *fOut=NULL;
    NvFBCLibrary nvfbc;
    NvFBCCuda *nvfbcCuda = NULL;

    //! Timers
    Timer grabTimer;
    Timer frameTimer;

    DWORD currentWidth = 0, currentHeight = 0;
    DWORD maxDisplayWidth = -1, maxDisplayHeight = -1;
    DWORD maxBufferSize = -1;

    NvFBCFrameGrabInfo frameGrabInfo;

    //! CUDA resources
    CUcontext cudaContext = NULL;
    CUdeviceptr argbBuffer = NULL;
    CUdeviceptr nv12Buffer = NULL;
    //BYTE *nv12sysbuf = NULL;

    //! Encoder
    Encoder encoder;

    if(!parseArgs(argc, argv, args))
        return -1;

    //! Load the nvfbc Library
    if(!nvfbc.load())
    {
        fprintf(stderr, "Unable to load the NvFBC library\n");
        return -1;
    }

    //! Create the CUDA context, this context will be used by NvFBC and must be created
    
    //! Create an instance of the NvFBCCuda class, the first argument specifies the frame buffer
    nvfbcCuda = (NvFBCCuda *)nvfbc.create(NVFBC_SHARED_CUDA, &maxDisplayWidth, &maxDisplayHeight);

    checkCudaErrors(cuCtxPopCurrent(&cudaContext));
    checkCudaErrors(cuCtxPushCurrent(cudaContext));

    if (!nvfbcCuda)
    {
        fprintf(stderr, "Failed to create an instance of NvFBCCuda\n");
        return -1;
    }

    //! Get the max buffer size from NvFBCCuda
    nvfbcCuda->NvFBCCudaGetMaxBufferSize(&maxBufferSize);

    //! Allocate memory on the CUDA device to store the framebuffer. 
    checkCudaErrors(cuMemAlloc(&argbBuffer, maxBufferSize));
    checkCudaErrors(cuMemAlloc(&nv12Buffer, 2*(maxBufferSize/3)));  // no need for full size
    //checkCudaErrors(cuMemAllocHost((void **)&nv12sysbuf, 2*(maxBufferSize/3)));

    if (S_OK != encoder.Init(cudaContext, maxBufferSize))
    {
        fprintf(stderr, "Failed to initialize encoder\n");
        return -1;
    }

    frameTimer.reset();
    for(int frameCnt = 0; frameCnt < args.iFrameCnt; ++frameCnt)
    {
        grabTimer.reset();

        NVFBCRESULT fbcRes = NVFBC_SUCCESS;
        {
            NVFBC_CUDA_GRAB_FRAME_PARAMS fbcCudaGrabParams = {0};
            fbcCudaGrabParams.dwVersion = NVFBC_CUDA_GRAB_FRAME_PARAMS_VER;
            fbcCudaGrabParams.pCUDADeviceBuffer = (void *)argbBuffer;
            fbcCudaGrabParams.pNvFBCFrameGrabInfo = &frameGrabInfo;
            fbcCudaGrabParams.dwFlags = NVFBC_TOCUDA_NOWAIT;
            
            fbcRes = nvfbcCuda->NvFBCCudaGrabFrame(&fbcCudaGrabParams);
        }

        if(fbcRes == NVFBC_SUCCESS)
        {
            if (frameCnt == 0)
            {
                if (S_OK != encoder.SetupEncoder(frameGrabInfo.dwWidth, frameGrabInfo.dwHeight, frameGrabInfo.dwBufferWidth, args.iBitrate))
                {
                    fprintf(stderr, "Failed to set up encoder.\n");
                    break;
                }
                currentWidth = frameGrabInfo.dwBufferWidth;
                currentHeight = frameGrabInfo.dwHeight;
                fOut = GetOutputFile(args.sBaseName, currentWidth, currentHeight);
            }
            //! The frame number will only increment if the frame has changed
            double grabTime = grabTimer.now();
            double encodeTime = frameTimer.now() - grabTime;
            
            printf("Grab %d: frame %d, grab time %.2f, encode time %.2f ms, overlay %s\n", 
                frameCnt, frameCnt, grabTime, encodeTime, 
                frameGrabInfo.bOverlayActive?"active":"in-active");

            frameTimer.reset();

            //! If the grab resolution is different then the current resolution the encoder must be re-initialized
            if((currentWidth != frameGrabInfo.dwBufferWidth) || (currentHeight != frameGrabInfo.dwHeight))
            {
                //! Save the height and width so we can determine if it has changed.
                currentWidth = frameGrabInfo.dwBufferWidth;
                currentHeight = frameGrabInfo.dwHeight;

                encoder.Reconfigure(frameGrabInfo.dwWidth, frameGrabInfo.dwHeight, frameGrabInfo.dwBufferWidth, args.iBitrate);


                //! Close the output file
                if(NULL != fOut)
                {
                    fclose(fOut);
                    fOut = NULL;
                }

                //! Get a new file pointer to write the stream to.
                fOut = GetOutputFile(args.sBaseName+"_1", currentWidth, currentHeight);

                if(!fOut)
                    return -1;
            }

            launch_CudaARGB2NV12Process(frameGrabInfo.dwBufferWidth, frameGrabInfo.dwHeight, argbBuffer, nv12Buffer);       // this can write directly into the host mapped buffer
            //checkCudaErrors(cuMemcpyDtoH(nv12sysbuf, nv12Buffer, 2*(maxBufferSize/3)));
            //SaveYUV("Dump.bmp", nv12sysbuf, frameGrabInfo.dwWidth, frameGrabInfo.dwHeight);
            unsigned int frameIDX = frameCnt%MAX_BUF_QUEUE;
            if(S_OK != encoder.LaunchEncode(frameIDX, nv12Buffer))
            {
                fprintf(stderr, "Failed encoding frame %d\n", frameCnt);
                return -1;
            }
            if(S_OK != encoder.GetBitstream(frameIDX, fOut))
            {
                fprintf(stderr, "Failed encoding frame %d\n", frameCnt);
                return -1;
            }
        }
        else
        {
            fprintf(stderr, "Grab frame failed. NvFBCCudaGrabFrame returned: %d\n", fbcRes );
            return -1;
        }
    }

    //! Terminate the encoder
    encoder.TearDown();

    //! Close the output file
    if(NULL != fOut)
        fclose(fOut);

    //! Release the NvFBCCuda instance
    nvfbcCuda->NvFBCCudaRelease();

    return 0;
}
