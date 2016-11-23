/*!
* \brief
* Demonstrates how to use NvFBC to grab and encode the desktop with
* the Kepler GPU hardware encoder.
*
* \file
*
* The sample demonstrates how to use NvFBC to grab and encode with the
* Kepler GPU encoder. The program show how to initialize the NvFBCHWEnc
* encoder class, set up the grab and encode, and grab the full-screen
* framebuffer, compress it to HWEnc, and copy it to system memory.
* Because of this, the NvFBCHWEnc class requires a Kepler video
* card to work.  Attempting to create an instance of that class on earlier cards
* will result in the create call failing.
*
* \copyright
* CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */

#include <windows.h>
#include <stdio.h> 

#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCHwEnc.h>

#include <ctime>
#include <sstream>

// Command line arguments
typedef struct
{
    int         iFrameCnt; // Number of frames to encode
    int         iBitrate;  // Bitrate to use
    int         iProfile;  // HWEnc encoding profile; BASELINE (66), MAIN (77) and HIGH (100)
    std::string sBaseName; // Basename for the output stream
    int         bLossless;
    int         bYUV444;
    NV_HW_ENC_CODEC eCodec;
}AppArguments;

// Prints the help message
void printHelp()
{
    printf("Usage: NvFBCHWEnc [options]\n");
    printf("  -frames framecnt           The number of frames to grab, defaults to 30.\n");
    printf("  -profile <BASE|MAIN|HIGH>  The encoding profile, defaults to MAIN.\n");
    printf("  -lossless                  The grabbed desktop images are encoded lossless if lossless encode feature is available.\n");
    printf("  -yuv444                    The grabbed desktop images are encoded without chroma subsampling.\n");
    printf("  -codec codectype           The codec to use for encode. 0 = H.264, 1 = HEVC");
    printf("  -output filename           The base name for output stream, defaults to 'NvFBCHWEnc'.\n");
}

// Parse the command line arguments
bool parseCmdLine(int argc, char **argv, AppArguments &args)
{
    args.iFrameCnt = 480;
    args.iBitrate = 8000000; // 8000000 bits per second
    args.iProfile = 77; // MAIN profile
    args.bLossless = false;
    args.bYUV444 = false;
    args.eCodec = NV_HW_ENC_H264;

    for (int cnt = 1; cnt < argc; ++cnt)
    {
        if (0 == _stricmp(argv[cnt], "-frames"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return false;
            }

            args.iFrameCnt = atoi(argv[cnt]);

            // Must grab at least one frame.
            if (args.iFrameCnt < 1)
                args.iFrameCnt = 1;
        }
        else if (0 == _stricmp(argv[cnt], "-profile"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -profile option\n");
                printHelp();
                return false;
            }

            if (0 == _stricmp(argv[cnt], "BASE"))
                args.iProfile = 66;
            else if (0 == _stricmp(argv[cnt], "MAIN"))
                args.iProfile = 77;
            else if (0 == _stricmp(argv[cnt], "HIGH"))
                args.iProfile = 100;
            else
            {
                printf("Unexpected -profile option %s\n", argv[cnt]);
                printHelp();
                return false;
            }
        }
        else if (0 == _stricmp(argv[cnt], "-lossless"))
        {
            args.bLossless = 1;
        }
        else if (0 == _stricmp(argv[cnt], "-yuv444"))
        {
            args.bYUV444 = 1;
        }
        else if (0 == _stricmp(argv[cnt], "-codec"))
        {
            ++cnt;
            if (cnt >= argc)
            {
                printf("Missing -codec option\n");
                printHelp();
                return false;
            }

            if (atoi(argv[cnt]) == 1)
            {
                args.eCodec = NV_HW_ENC_HEVC;
                printf("Using HEVC for encoding\n");
            }
            else if (atoi(argv[cnt]) == 0)
            {
                args.eCodec = NV_HW_ENC_H264;
                printf("Using H.264 for encoding\n");
            }
            else
            {
                args.eCodec = NV_HW_ENC_H264;
                printf("Unknown parameter for -codec. Using H.264 for encoding\n");
            }
        }
        else if (0 == _stricmp(argv[cnt], "-output"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return false;
            }

            args.sBaseName = argv[cnt];
        }
        else
        {
            printf("Unexpected argument %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }

    if (args.sBaseName.length() == 0)
    {
        args.sBaseName = "NvFBCHwEnc";
    }

    if (args.eCodec == NV_HW_ENC_HEVC)
    {
        if (std::string::npos == args.sBaseName.find(".h265"))
            args.sBaseName += ".h265";
    }
    else
    {
        if (std::string::npos == args.sBaseName.find(".h264"))
            args.sBaseName += ".h264";
    }

    return true;
}

/*!
* Main program
*/
int main(int argc, char **argv)
{
	std::stringstream *StringStream = new std::stringstream();
	//*StringStream << "ffmpeg -r 30 -i - -an -c copy -r 30 -listen 1 -c:v libx264 -f h264 -an -tune zerolatency http://172.26.186.80:30000";
	*StringStream << "ffmpeg -re -i - -c copy -listen 1 -f h264 -tune zerolatency http://172.26.186.80:30000";
	FILE* ThePipe = _popen(StringStream->str().c_str(), "wb");
	
	using namespace std;
	clock_t begin = clock();

    AppArguments args;
    int ret = 0;
    char fileName[255];

    DWORD maxWidth = -1, maxHeight = -1;

    NvFBCLibrary nvfbc;
    INvFBCHWEncoder *encoder = NULL;

    NV_HW_ENC_CONFIG_PARAMS encodeConfig = { 0 };
    NvFBCFrameGrabInfo grabInfo = { 0 };
    NV_HW_ENC_GET_BIT_STREAM_PARAMS frameInfo = { 0 };
    NVFBC_HW_ENC_GRAB_FRAME_PARAMS fbcHwEncGrabFrameParams = { 0 };
    NVFBC_HW_ENC_SETUP_PARAMS fbcHwEncSetupParams = { 0 };
	NV_HW_ENC_PIC_PARAMS encParams = { 0 };
    NVFBCRESULT res = NVFBC_SUCCESS;
    DWORD dwOutputBufferSize = 0;

	encParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;
	encodeConfig.bEnableIntraRefresh = 1;
	//! Start an Intra-Refresh cycle over n frames.
	encParams.bForceIDRFrame = 1;

    unsigned char *outputBuffer = NULL;

    FILE *outputFile = NULL;
    DWORD fileSuffix = 0;

    //! Parse the command line arguments
    if (!parseCmdLine(argc, argv, args))
        return -1;

    //! Load the NvFBC library.
    if (!nvfbc.load())
    {
        return -1;
    }

    //! Create the NvFBCHWEnc instance
    //! Here we specify the Hardware HWEnc Encoder (Kepler)
    encoder = (INvFBCHWEncoder *)nvfbc.create(NVFBC_TO_HW_ENCODER, &maxWidth, &maxHeight);

    if (!encoder)
    {
        fprintf(stderr, "Unable to load the HWEnc encoder\n");
        return -1;
    }
    //! Setup a buffer to put the encoded frame in
    if (args.bYUV444 || args.bLossless)
    {
        // Allocate an arbitrarily larger bitstream buffer for YUV444 and\or lossless encoding
        dwOutputBufferSize = 4 * 1024 * 1024;
    }
    else
    {
        // Default to 1MB of output bitstream buffer size.
        dwOutputBufferSize = 1024 * 1024;
    }

    //! Setup a buffer to put the encoded frame in
    outputBuffer = (unsigned char *)malloc(dwOutputBufferSize);

    //! Set the encoding parameters
    encodeConfig.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER;
    encodeConfig.eCodec         = args.eCodec;
    encodeConfig.dwProfile      = args.iProfile;
    encodeConfig.dwFrameRateNum = 30;
    encodeConfig.dwFrameRateDen = 1; // Set the target frame rate at 30
    encodeConfig.bOutBandSPSPPS = FALSE; // Use inband SPSPPS, if you need to grab headers on demand use outband SPSPPS
    encodeConfig.bRecordTimeStamps = FALSE; // Don't record timestamps
    encodeConfig.eStereoFormat  = NV_HW_ENC_STEREO_NONE; // No stereo

    if (args.bYUV444)
    {
        encodeConfig.bEnableYUV444Encoding = TRUE;
    }

    if (args.bLossless)
    {
        encodeConfig.ePresetConfig = NV_HW_ENC_PRESET_LOSSLESS_HP;
        encodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_CONSTQP;
    }
    else
    {
        encodeConfig.dwAvgBitRate = args.iBitrate;
        encodeConfig.dwPeakBitRate = (NvU32)(args.iBitrate * 1.50); // Set the peak bitrate to 150% of the average
		encodeConfig.dwGOPLength = 100; // The I-Frame frequency
		//encodeConfig.dwGOPLength = -1; // The I-Frame frequency
        encodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_VBR; // Set the rate control
        encodeConfig.ePresetConfig = NV_HW_ENC_PRESET_LOW_LATENCY_HQ;
        encodeConfig.dwQP = 26; // Quantization parameter, between 0 and 51 
		//encodeConfig.dwVBVBufferSize = encodeConfig.dwAvgBitRate / (encodeConfig.dwFrameRateNum / encodeConfig.dwFrameRateDen);
		//encodeConfig.dwVBVInitialDelay = encodeConfig.dwVBVBufferSize;
		
    }

    fbcHwEncSetupParams.dwVersion = NVFBC_HW_ENC_SETUP_PARAMS_VER;
    fbcHwEncSetupParams.bWithHWCursor = TRUE;
    fbcHwEncSetupParams.EncodeConfig = encodeConfig;
    fbcHwEncSetupParams.dwBSMaxSize = dwOutputBufferSize;
    //! Setup the grab and encode
    res = encoder->NvFBCHWEncSetUp(&fbcHwEncSetupParams);

    if (res != NVFBC_SUCCESS)
    {
        fprintf(stderr, "Failed to setup HW encoder.. exiting!\n");
        ret = -1;
        goto exit;
    }
    //! For each frame..
    for (int i = 0; i < args.iFrameCnt; ++i)
    {
        memset(&grabInfo, 0, sizeof(grabInfo));
        memset(&frameInfo, 0, sizeof(frameInfo));
        memset(&fbcHwEncGrabFrameParams, 0, sizeof(fbcHwEncGrabFrameParams));
        fbcHwEncGrabFrameParams.dwVersion = NVFBC_HW_ENC_GRAB_FRAME_PARAMS_VER;
        fbcHwEncGrabFrameParams.dwFlags = NVFBC_HW_ENC_NOWAIT;
        fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo = grabInfo;
        fbcHwEncGrabFrameParams.GetBitStreamParams = frameInfo;
        fbcHwEncGrabFrameParams.pBitStreamBuffer = outputBuffer;
        //! Grab and encode the frame
        res = encoder->NvFBCHWEncGrabFrame(&fbcHwEncGrabFrameParams);
        frameInfo = fbcHwEncGrabFrameParams.GetBitStreamParams;

        // We want to grab the resolution of the desktop from NvFBCGrabFrame before we write the data
        if (outputFile == NULL)
        {
            //sprintf(fileName, "%dx%d-%s", 
            //    fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo.dwWidth, 
            //    fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo.dwHeight,
            //    args.sBaseName.c_str());
			sprintf(fileName, "%s",
				args.sBaseName.c_str());
            args.sBaseName = fileName;

            //! Create the output file
            outputFile = fopen(args.sBaseName.c_str(), "wb");

            if (NULL == outputFile)
            {
                fprintf(stderr, "Unable to open %s for writing\n", args.sBaseName.c_str());
                ret = -1;
                goto exit;
            }
        }

        if (res == NVFBC_SUCCESS)
        {
            //fwrite(outputBuffer, frameInfo.dwByteSize, 1, outputFile);
			fwrite(outputBuffer, frameInfo.dwByteSize, 1, ThePipe);
			fflush(ThePipe);
            //fprintf(stderr, "Wrote frame %d to %s\n", i, args.sBaseName.c_str());
        }
        else
        {
            if (res == NVFBC_ERROR_INVALIDATED_SESSION)
            {
                encoder->NvFBCHWEncRelease();
                encoder = (INvFBCHWEncoder *)nvfbc.create(NVFBC_TO_HW_ENCODER, &maxWidth, &maxHeight);
                //! Setup the grab and encode
                res = encoder->NvFBCHWEncSetUp(&fbcHwEncSetupParams);
                if (res != NVFBC_SUCCESS)
                {
                    fprintf(stderr, "Failed to setup HW encoder.. exiting!\n");
                    ret = -1;
                    goto exit;
                }
                res = encoder->NvFBCHWEncGrabFrame(&fbcHwEncGrabFrameParams);
                if (res != NVFBC_SUCCESS)
                {
                    fprintf(stderr, "Error in grabbing frame.. exiting!\n");
                    ret = -1;
                    goto exit;
                }
                grabInfo  = fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo;
                frameInfo = fbcHwEncGrabFrameParams.GetBitStreamParams;
                
                fclose(outputFile);
                outputFile = NULL;

                sprintf(fileName, "%dx%d-NvFBCHWEncode_%d.%s", 
                    fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo.dwWidth,
                    fbcHwEncGrabFrameParams.NvFBCFrameGrabInfo.dwHeight,
                    ++fileSuffix, (args.eCodec == NV_HW_ENC_HEVC) ? "h265" : "h264");


                outputFile = fopen(fileName, "wb"); // Writing in a new file
                
                if (NULL == outputFile)
                {
                    fprintf(stderr, "Unable to open %s for writing\n", fileName);
                    ret = -1;
                    goto exit;
                }

                args.sBaseName = fileName;
                fwrite(outputBuffer, frameInfo.dwByteSize, 1, outputFile);
                fprintf(stderr, "Wrote frame %d to %s\n", i, args.sBaseName.c_str());

            }
        }
    }
exit:
    if (outputFile)
    {
        fclose(outputFile);
		fclose(ThePipe);
        free(outputBuffer);

		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		fprintf(stderr, "Time taken: %f", elapsed_secs);
    }
    if (encoder)
    {
        encoder->NvFBCHWEncRelease();
    }

    return ret;
}