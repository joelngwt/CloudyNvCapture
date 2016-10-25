/*!
 * \brief
 * Demonstrates the use of NvFBCToSys to copy the desktop to system memory. 
 *
 * \file
 *
 * This sample demonstrates the use of NvFBCToSys class to record the
 * entire desktop. It covers loading the NvFBC DLL, loading the NvFBC 
 * function pointers, creating an instance of NvFBCToSys and using it 
 * to copy the full screen frame buffer into system memory.
 *
 * \copyright
 * CopyRight 1993-2016 NVIDIA Corporation.  All rights reserved.
 * NOTICE TO LICENSEE: This source code and/or documentation ("Licensed Deliverables")
 * are subject to the applicable NVIDIA license agreement
 * that governs the use of the Licensed Deliverables.
 */


#include <windows.h>
#include <stdio.h>

#include <string>

#include <Bitmap.h>

#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCToSys.h>

#include <sstream>
#include <ctime>
#include <vector>

// Structure to store the command line arguments
struct AppArguments
{
    int   iFrameCnt; // Number of frames to grab
    int   iStartX; // Grab start x coord
    int   iStartY; // Grab start y coord
    int   iWidth; // Grab width
    int   iHeight; // Grab height
    int   iSetUpFlags; // Set up flags
    bool  bHWCursor; // Grab hardware cursor
    NVFBCToSysGrabMode     gmMode; // Grab mode
    NVFBCToSysBufferFormat bfFormat; // Output buffer format
    std::string sBaseName; // Output file name
    FILE*       yuvFile;
	int   port; // Port to stream to. Defaults to 30000
	int   numPlayers; // Number of players to stream to. // Defaults to 1
	int   numRows; // Number of columns in the split screen
	int   numCols; // Number of rows in the split screen
};

// Prints the help message
void printHelp()
{
    printf("Usage: NvFBCToSys [options]\n");
    printf("  -frames frameCnt     The number of frames to grab\n");
    printf("                       , this value defaults to one.  If\n");
    printf("                       the value is greater than one only\n");
    printf("                       the final frame is saved as a bitmap.\n");
    printf("  -scale width height  Scales the grabbed frame buffer\n");
    printf("                       to the provided width and height\n");
    printf("  -crop x y width height  Crops the grabbed frame buffer to\n");
    printf("                          the provided region\n");
    printf("  -grabCursor          Grabs the hardware cursor\n");
    printf("  -format <ARGB|RGB|YUV420|YUV444|PLANAR|XOR>\n");
    printf("                       Sets the grab format\n");
    printf("  -output              Output file name\n"
           "                        *.bmp will produce multiple files,\n"
           "                        *.yuv will produce a yuv clip\n"
           "                        default is bmp output\n");
	printf("  -port                The port to stream to.\n");
	printf("                       Should be between 30000 and 30005.");
	printf("  -players             The number of players to stream to. Defaults to 30000.\n");
	printf("                       Should be between 1 and 6. Defaults to 1.");
    printf("  -nowait              Grab with the no wait flag\n");
}

// Parse the command line arguments
bool parseCmdLine(int argc, char **argv, AppArguments &args)
{
    args.iFrameCnt = 1;
    args.iStartX = 0;
    args.iStartY = 0;
    args.iWidth = 0;
    args.iHeight = 0;
    args.iSetUpFlags = NVFBC_TOSYS_NOFLAGS;
    args.bHWCursor = false;
    args.gmMode = NVFBC_TOSYS_SOURCEMODE_FULL;
    args.bfFormat = NVFBC_TOSYS_ARGB;
    args.sBaseName = "NvFBCToSys";
    args.yuvFile = NULL;
	args.port = 30000;
	args.numPlayers = 1;
	args.numRows = 1;
	args.numCols = 1;

    for(int cnt = 1; cnt < argc; ++cnt)
    {
        if(0 == _stricmp(argv[cnt], "-frames"))
        {
            ++cnt;

            if(cnt >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return false;
            }

            args.iFrameCnt = atoi(argv[cnt]);

            // Must grab at least one frame.
            if(args.iFrameCnt < 1)
                args.iFrameCnt = 1;
        }
        else if(0 == _stricmp(argv[cnt], "-scale"))
        {
            if(NVFBC_TOSYS_SOURCEMODE_FULL != args.gmMode)
            {
                printf("Both -crop and -scale cannot be used at the same time.\n");
                printHelp();
                return false;
            }

            if((cnt + 2) > argc)
            {
                printf("Missing -scale options\n");
                printHelp();
                return false;
            }

            args.iWidth = atoi(argv[cnt+1]);
            args.iHeight = atoi(argv[cnt+2]);
            args.gmMode = NVFBC_TOSYS_SOURCEMODE_SCALE;

            cnt += 2;
        }
        else if(0 == _stricmp(argv[cnt], "-crop"))
        {
            if(NVFBC_TOSYS_SOURCEMODE_FULL != args.gmMode)
            {
                printf("Both -crop and -scale cannot be used at the same time.\n");
                printHelp();
                return false;
            }

            if((cnt + 4) >= argc)
            {
                printf("Missing -crop options\n");
                printHelp();
                return false;
            }

            args.iStartX = atoi(argv[cnt+1]);
            args.iStartY = atoi(argv[cnt+2]);
            args.iWidth = atoi(argv[cnt+3]);
            args.iHeight = atoi(argv[cnt+4]);
            args.gmMode = NVFBC_TOSYS_SOURCEMODE_CROP;

            cnt += 4;
        }
        else if(0 == _stricmp(argv[cnt], "-grabCursor"))
        {
            args.bHWCursor = true;
        }
        else if(0 == _stricmp(argv[cnt], "-nowait"))
        {
            args.iSetUpFlags |= NVFBC_TOSYS_NOWAIT;
        }
        else if(0 == _stricmp(argv[cnt], "-format"))
        {
            ++cnt;

            if(cnt >= argc)
            {
                printf("Missing -format option\n");
                printHelp();
                return false;
            }

            if(0 == _stricmp(argv[cnt], "ARGB"))
                args.bfFormat = NVFBC_TOSYS_ARGB;
            else if(0 == _stricmp(argv[cnt], "RGB"))
                args.bfFormat = NVFBC_TOSYS_RGB;
            else if(0 == _stricmp(argv[cnt], "YUV420"))
                args.bfFormat = NVFBC_TOSYS_YYYYUV420p;
            else if(0 == _stricmp(argv[cnt], "YUV444"))
                args.bfFormat = NVFBC_TOSYS_YUV444p;
            else if(0 == _stricmp(argv[cnt], "PLANAR"))
                args.bfFormat = NVFBC_TOSYS_RGB_PLANAR;
            else if(0 == _stricmp(argv[cnt], "XOR"))
                args.bfFormat = NVFBC_TOSYS_XOR;
            else
            {
                printf("Unexpected -format option %s\n", argv[cnt]);
                printHelp();
                return false;
            }
        }
        else if(0 == _stricmp(argv[cnt], "-output"))
        {
            ++cnt;

            if(cnt >= argc)
            {
                printf("Missing -output option\n");
                printHelp();
                return false;
            }

            args.sBaseName = argv[cnt];

             if(std::string::npos != args.sBaseName.find(".yuv")) {                 
                 //args.yuvFile = fopen(args.sBaseName.c_str(), "wb");
                 //if(!args.yuvFile) {
                 //    printf("Unable to open output file <%s>, exiting ...\n", args.sBaseName.c_str());
                 //    exit(-1);
                 //}
             }
            else if(std::string::npos == args.sBaseName.find(".bmp")) {
                args.sBaseName += ".bmp";
            }
        }
		else if (0 == _stricmp(argv[cnt], "-port"))
		{
			++cnt;

			if (cnt >= argc)
			{
				printf("Missing -port option\n");
				printHelp();
				return false;
			}
			args.port = atoi(argv[cnt]);
		}
		else if (0 == _stricmp(argv[cnt], "-players"))
		{
			++cnt;

			if (cnt >= argc)
			{
				printf("Missing -players option\n");
				printHelp();
				return false;
			}
			args.numPlayers = atoi(argv[cnt]);
		}
		else if (0 == _stricmp(argv[cnt], "-layout"))
		{
			if ((cnt + 2) >= argc)
			{
				printf("Missing -layout options\n");
				printHelp();
				return false;
			}

			args.numRows = atoi(argv[cnt + 1]);
			args.numCols = atoi(argv[cnt + 2]);

			cnt += 2;
		}
        else
        {
            printf("Unexpected argument %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }

    if(/*args.yuvFile && */(args.bfFormat != NVFBC_TOSYS_YUV444p || args.bfFormat != NVFBC_TOSYS_YYYYUV420p)) {
        printf("YUV output must use YUV420p or YUV444p! Set YUV420p ...\n");
        args.bfFormat = NVFBC_TOSYS_YYYYUV420p;
    }

    return true;
}
/*!
 * Main program
 */
int main(int argc, char* argv[])
{
	using namespace std;
	clock_t begin = clock();

    AppArguments args;

    NvFBCLibrary nvfbcLibrary;
    NvFBCToSys *nvfbcToSys = NULL;

    DWORD maxDisplayWidth = -1, maxDisplayHeight = -1;
    BOOL bRecoveryDone = FALSE;

    NvFBCFrameGrabInfo grabInfo;
    unsigned char *frameBuffer = NULL;
    unsigned char *diffMap = NULL;
    char frameNo[10];
    std::string outName;

	std::vector<FILE*> PipeList;
    
    if(!parseCmdLine(argc, argv, args))
        return -1;

    //! Load NvFBC
    if(!nvfbcLibrary.load())
    {
        fprintf(stderr, "Unable to load the NvFBC library\n");
        return -1;
    }

    //! Create an instance of NvFBCToSys
    nvfbcToSys = (NvFBCToSys *)nvfbcLibrary.create(NVFBC_TO_SYS, &maxDisplayWidth, &maxDisplayHeight);

    NVFBCRESULT status = NVFBC_SUCCESS;
    if(!nvfbcToSys)
    {
        fprintf(stderr, "Unable to create an instance of NvFBC\n");
        return -1;
    }

    //! Setup the frame grab
    NVFBC_TOSYS_SETUP_PARAMS fbcSysSetupParams = {0};
    fbcSysSetupParams.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER;
    fbcSysSetupParams.eMode = args.bfFormat;
    fbcSysSetupParams.bWithHWCursor = args.bHWCursor;
    fbcSysSetupParams.bDiffMap = FALSE;
    fbcSysSetupParams.ppBuffer = (void **)&frameBuffer;
    fbcSysSetupParams.ppDiffMap = NULL;

	for (int i = 0; i < args.numPlayers; ++i)
	{
		std::stringstream *StringStream = new std::stringstream();
		// Writing desktop capture to local disk. FFMPEG encoding.
		//*StringStream << "ffmpeg -y -f rawvideo -pix_fmt yuv420p -r 25 -s 1024x768 -i - -r 25 -f mp4 -an foo.mp4";
		// FFMPEG Encoding
		*StringStream << "ffmpeg -y -f rawvideo -pix_fmt yuv420p -s " << args.iWidth << "x" << args.iHeight << " -re -i - -listen 1 -preset ultrafast -f flv -an -tune zerolatency http://172.26.186.80:" << args.port + i;
		// Nvidia Encoding
		//*StringStream << "ffmpeg -y -f rawvideo -pix_fmt yuv420p -s 1680x1050 -re -i - -listen 1 -c:v h264_nvenc -f avi -an -tune zerolatency http://172.26.186.80:30000";

		PipeList.push_back(_popen(StringStream->str().c_str(), "wb"));
	}

    status = nvfbcToSys->NvFBCToSysSetUp(&fbcSysSetupParams);
    if (status == NVFBC_SUCCESS)
    {
        //! Sleep so that ToSysSetUp forces a framebuffer update
        Sleep(100);
        
        NVFBC_TOSYS_GRAB_FRAME_PARAMS fbcSysGrabParams = {0};
        //! For each frame to grab..
        //for(int cnt = 0; cnt < args.iFrameCnt; ++cnt)
		int cnt = 0;
		while (true)
        {
			// Problem: How to cut out 6 buffers from the original?
			// Use original given API: fbcSysGrabParams?
			// Somehow divide the char array of frameBuffer?
			
			// Have to pass the number of rows and columns to args.
			// Then, when calling NvFBCToSys.exe, also pass the number of players.
			// Based on the number of players, rows, and columns, iStartX and iStartY will be determined.
			// iWidth and iHeight should be constant.

			++cnt;
            outName = args.sBaseName + "_" + _itoa(cnt, frameNo, 10) + ".bmp";

			int row = 0;
			int col = 0;

			for (int i = 0; i < args.numPlayers; ++i)
			{			
				fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
				fbcSysGrabParams.dwFlags = args.iSetUpFlags;
				fbcSysGrabParams.dwTargetWidth = args.iWidth;
				fbcSysGrabParams.dwTargetHeight = args.iHeight;
				fbcSysGrabParams.dwStartX = args.iStartX + args.iWidth*col;
				fbcSysGrabParams.dwStartY = args.iStartY + args.iHeight*row;
				fbcSysGrabParams.eGMode = args.gmMode;
				fbcSysGrabParams.pNvFBCFrameGrabInfo = &grabInfo;

				status = nvfbcToSys->NvFBCToSysGrabFrame(&fbcSysGrabParams);
				// Then, fwrite the frame buffer to player 1 (pipe1) (need 1 pipe per player. 1 pipe = 1 ffmpeg streamer)
				if (status == NVFBC_SUCCESS)
				{
					bRecoveryDone = FALSE;
					fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight * 3/2, 1, PipeList[i]);
					//fflush(PipeList[i]);
				}

				++col;

				if (col >= args.numCols)
				{
					col = 0;
					++row;
				}
			}

			//--------------
			// Player 1?
            //! Grab the frame.  
            // If scaling or cropping is enabled the width and height returned in the
            // NvFBCFrameGrabInfo structure reflect the current desktop resolution, not the actual grabbed size.
			/*
			if (args.numPlayers >= 1)
			{
				fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
				fbcSysGrabParams.dwFlags = args.iSetUpFlags;
				fbcSysGrabParams.dwTargetWidth = args.iWidth;
				fbcSysGrabParams.dwTargetHeight = args.iHeight;
				fbcSysGrabParams.dwStartX = args.iStartX;
				fbcSysGrabParams.dwStartY = args.iStartY;
				fbcSysGrabParams.eGMode = args.gmMode;
				fbcSysGrabParams.pNvFBCFrameGrabInfo = &grabInfo;

				status = nvfbcToSys->NvFBCToSysGrabFrame(&fbcSysGrabParams);
				// Then, fwrite the frame buffer to player 1 (pipe1) (need 1 pipe per player. 1 pipe = 1 ffmpeg streamer)
				if (status == NVFBC_SUCCESS)
				{
					bRecoveryDone = FALSE;
					fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight * 3 / 2, 1, PipeList[0]);
					fflush(PipeList[0]);
				}
			}
            
			//----------
			// Then, setup fbcSysGrabParams again, with different iStartX and iStartY.
			// Then, status = nvfbcToSys->NvFBCToSysGrabFrame(&fbcSysGrabParams); again?
			// Then, fwrite to player 2, pipe 2.
			if (args.numPlayers >= 2)
			{
				fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
				fbcSysGrabParams.dwFlags = args.iSetUpFlags;
				fbcSysGrabParams.dwTargetWidth = args.iWidth;
				fbcSysGrabParams.dwTargetHeight = args.iHeight;
				fbcSysGrabParams.dwStartX = args.iStartX + args.iWidth;
				fbcSysGrabParams.dwStartY = args.iStartY;
				fbcSysGrabParams.eGMode = args.gmMode;
				fbcSysGrabParams.pNvFBCFrameGrabInfo = &grabInfo;

				status = nvfbcToSys->NvFBCToSysGrabFrame(&fbcSysGrabParams);
				if (status == NVFBC_SUCCESS)
				{
					bRecoveryDone = FALSE;
					fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight * 3 / 2, 1, PipeList[1]);
					fflush(PipeList[1]);
				}
			}*/
			//----------
			
            if (status == NVFBC_SUCCESS)
            {
                /*bRecoveryDone = FALSE;
				
                //! Save the frame to disk
                switch(args.bfFormat)
                {
                case NVFBC_TOSYS_ARGB:
                    SaveARGB(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);
                    fprintf (stderr, "Grab succeeded. Wrote %s as ARGB.\n", outName.c_str() );
                    break;

                case NVFBC_TOSYS_RGB:
                    SaveRGB(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);

                    fprintf (stderr, "Grab succeeded. Wrote %s as RGB.\n", outName.c_str());
                    break;

                case NVFBC_TOSYS_YUV444p:
                    if(args.yuvFile) {
						//fprintf(stderr, "Making YUV444 file\n");
						fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight * 3, 1, args.yuvFile);
                    }
                    else {
                        SaveYUV444(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight);
                        fprintf (stderr, "Grab succeeded. Wrote %s as YUV444 converted to RGB.\n", outName.c_str());
                    }
                    break;

                case NVFBC_TOSYS_YYYYUV420p:
                    if(ThePipe) {//args.yuvFile) {
                        //fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight*3/2, 1, args.yuvFile); // Original code


						//fprintf(stderr, "Frame %d. Sending to ffmpeg\n", cnt);
						fwrite(frameBuffer, grabInfo.dwWidth*grabInfo.dwHeight * 3 / 2, 1, ThePipe);
						//fprintf(stderr, "Frame %d. done\n", cnt);
						fflush(ThePipe);
						//fprintf(stderr, "Frame %d. Making YUV420 file\n", cnt);
                    }
                    else {
                        SaveYUV420(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight);
                        fprintf (stderr, "Grab succeeded. Wrote %s as YYYYUV420p.\n", outName.c_str() );
                    }
                    break;

                case NVFBC_TOSYS_RGB_PLANAR:
                    SaveRGBPlanar(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight);
                    fprintf (stderr, "Grab succeeded. Wrote %s as RGB_PLANAR.\n", outName.c_str() );
                break;

                case NVFBC_TOSYS_XOR:
                    // The second grab results in the XOR of the first and second frame.
                    fbcSysGrabParams.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
                    fbcSysGrabParams.dwFlags = args.iSetUpFlags;
                    fbcSysGrabParams.dwTargetWidth = args.iWidth;
                    fbcSysGrabParams.dwTargetHeight = args.iHeight;
                    fbcSysGrabParams.dwStartX = 0;
                    fbcSysGrabParams.dwStartY = 0;
                    fbcSysGrabParams.eGMode = args.gmMode;
                    fbcSysGrabParams.pNvFBCFrameGrabInfo = &grabInfo;
                    status = nvfbcToSys->NvFBCToSysGrabFrame(&fbcSysGrabParams);
                    if (status == NVFBC_SUCCESS)
                        SaveRGB(outName.c_str(), frameBuffer, grabInfo.dwWidth, grabInfo.dwHeight, grabInfo.dwBufferWidth);

                    fprintf (stderr, "Grab succeeded. Wrote %s as XOR.\n", outName.c_str() );
                    break;

                default:
                    fprintf (stderr, "Un-expected grab format %d.", args.bfFormat);
                    break;
                }*/
            }
            else
            {
                if (bRecoveryDone == TRUE)
                {
                    fprintf(stderr, "Unable to recover from NvFBC Frame grab failure.\n");
                    //! Relase the NvFBCToSys object
                    nvfbcToSys->NvFBCToSysRelease();
                    return -1;
                }
                if (status == NVFBC_ERROR_INVALIDATED_SESSION)
                {
                    fprintf(stderr, "Session Invalidated. Attempting recovery\n");
                    nvfbcToSys->NvFBCToSysRelease();
                    nvfbcToSys = NULL;
                    //! Recover from error. Create an instance of NvFBCToSys
                    nvfbcToSys = (NvFBCToSys *)nvfbcLibrary.create(NVFBC_TO_SYS, &maxDisplayWidth, &maxDisplayHeight);
                    if(!nvfbcToSys)
                    {
                        fprintf(stderr, "Unable to create an instance of NvFBC\n");
                        return -1;
                    }
                    //! Setup the frame grab
                    NVFBC_TOSYS_SETUP_PARAMS fbcSysSetupParams = {0};
                    fbcSysSetupParams.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER;
                    fbcSysSetupParams.eMode = args.bfFormat;
                    fbcSysSetupParams.bWithHWCursor = args.bHWCursor;
                    fbcSysSetupParams.bDiffMap = FALSE;
                    fbcSysSetupParams.ppBuffer = (void **)&frameBuffer;
                    fbcSysSetupParams.ppDiffMap = NULL;
                    status = nvfbcToSys->NvFBCToSysSetUp(&fbcSysSetupParams);
                    cnt--;
                    if (status == NVFBC_SUCCESS)
                    {
                        bRecoveryDone = TRUE;
                    }
                    else
                    {
                        fprintf(stderr, "Unable to recover from NvFBC Frame grab failure.\n");
                        //! Relase the NvFBCToSys object
                        nvfbcToSys->NvFBCToSysRelease();
                        return -1;
                    }
                }
            }
        }
    }
    if (status != NVFBC_SUCCESS)
    {
        fprintf(stderr, "Unable to setup frame grab.\n");
    }
    //! Relase the NvFBCToSys object
    nvfbcToSys->NvFBCToSysRelease();

	if (args.yuvFile) {
		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		fprintf(stderr, "Time taken: %f", elapsed_secs);
		//fclose(args.yuvFile);
		for (int i = 0; i < PipeList.size(); ++i)
		{
			fflush(PipeList[i]);
			_pclose(PipeList[i]);
		}
	}

    return 0;
}