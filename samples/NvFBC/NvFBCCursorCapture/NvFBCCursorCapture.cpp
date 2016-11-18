/*!
 * \brief
 * Demonstrates the use of NvFBCToSys and NvFBCToSysCursorCapture functions
 * to capture the desktop and also the cursor to system memory.
 *
 * \file
 *
 * This sample demonstrates the use of NvFBCToSys and NvFBCToSysCursorCapture 
 * classes to record the entire desktop and the cursor. It covers loading 
 * the NvFBC DLL, loading the NvFBC  function pointers, creating an instance 
 * of NvFBCToSys and NvFBCToSysCursorCapture and using it to copy the full 
 * screen frame buffer and cursor into system memory.
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
#include "nvFBCLibrary.h"
#include "Grabber.h"

#include <process.h>
#include <time.h>

#define IS_HANDLE_VALID(x) (x!=NULL && x!=INVALID_HANDLE_VALUE)

// Structure to store the command line arguments
enum NVFBCInterface
{
    ToSys = 0,
    ToDX9 = 1,
    ToHWEnc = 2,
    ToCuda = 3,
    Invalid
};

char *NVFBCInterfaceStr[] =
{
    "NvFBCToSys",
    "NvFBCCuda",
    "NvFBCHWEnc",
    "NvFBCToDx9Vid"
};

struct AppArguments
{
    int   iFrameCnt; // Number of frames to grab
    int   iNVFBCInterface; // The NVFBC interface to use for setup capture session
    int   iAdapter;   // The display to grab
    bool  bHWCursor; // Grab hardware cursor
    std::string sBaseName; // Output file name
};

// Prints the help message
void printHelp()
{
    printf("Usage: NvFBCToSys [options]\n");
    printf("  -frames frameCnt     The number of times to call grab()\n");
    printf("                       , this value defaults to one.\n");
    printf("  -output              Output file name\n");
    printf("  -adapter             Display to grab from\n");
    printf("  -interface           The NVFBC interface to use for setting up NVFBC session\n");
    printf("                           ToSys = 0,\n");
    printf("                           ToDX9 = 1,\n");
    printf("                           ToHWEnc = 2,\n");
    printf("                           ToCuda = 3\n");
}

// Parse the command line arguments
bool parseCmdLine(int argc, char **argv, AppArguments &args)
{
    bool bOutputNameSpecified = false; 
    
    args.iFrameCnt = 1;
    args.sBaseName = NVFBCInterfaceStr[ToSys];
    args.iNVFBCInterface = ToSys;
    args.iAdapter = 0;

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
            bOutputNameSpecified = true;
        }
        else if (0 == _stricmp(argv[cnt], "-interface"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return false;
            }

            args.iNVFBCInterface = atoi(argv[cnt]);
            if (args.iNVFBCInterface >= Invalid)
            {
                printf("Invalid argument for -interface\n");
                printHelp();
                return false;
            }
        }
        else if (0 == _stricmp(argv[cnt], "-adapter"))
        {
            ++cnt;

            if (cnt >= argc)
            {
                printf("Missing -frames option\n");
                printHelp();
                return false;
            }

            args.iAdapter = atoi(argv[cnt]);
            if (args.iAdapter < 0)
            {
                printf("Invalid argument for -adapter\n");
                printHelp();
                return false;
            }
        }
        else
        {
            printf("Unexpected argument %s\n", argv[cnt]);
            printHelp();
            return false;
        }
    }
    
    if (!bOutputNameSpecified)
    {
        args.sBaseName = NVFBCInterfaceStr[args.iNVFBCInterface];
    }

    if (std::string::npos == args.sBaseName.find(".bmp"))
    {
        args.sBaseName += ".bmp";
    }
        return true;
}

void TestCursors(DWORD dwAdapterIdx)
{
	fprintf(stderr, __FUNCTION__ ": [%d] Press 'q' to quit\n", dwAdapterIdx);
    char c = 'c';
    while(c != 'q')
    {
        scanf("%c", &c);
    }
}

void grabThread(void *pData)
{
    if (!pData)
    {
        fprintf(stderr, __FUNCTION__ ": Grab thread quitting. NULL input params passed\n");
    }

    GrabThreadParams *pParams = (GrabThreadParams *)pData;
    if (!(IS_HANDLE_VALID(pParams->hThreadCreatedEvent) && IS_HANDLE_VALID(pParams->hGrabEvent) &&
          IS_HANDLE_VALID(pParams->hGrabCompleteEvent) && IS_HANDLE_VALID(pParams->hQuitEvent)) ||
        (!pParams->pGrabber))
    {
        fprintf(stderr, __FUNCTION__ ": [%d] Grab thread quitting. Invalid handles passed\n", pParams->dwAdapterIdx);
    }

    DWORD dwWait = WAIT_OBJECT_0;
    HANDLE handle[2] = { pParams->hGrabEvent, pParams->hQuitEvent};
    SetEvent(pParams->hThreadCreatedEvent);

    fprintf (stderr, __FUNCTION__ ": [%d] Grab thread Started\n", pParams->dwAdapterIdx);
    while(1)
    {
        dwWait = WaitForMultipleObjects(2, handle, FALSE ,INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            pParams->pGrabber->grab();
            SetEvent(pParams->hGrabCompleteEvent);
        }
        else
        {
            fprintf(stderr, __FUNCTION__ ": [%d] Grab Thread Quitting\n", pParams->dwAdapterIdx);
            return;
        }
    }
    fprintf(stderr, __FUNCTION__ ": [%d] Grab Thread Quitting unexpectedly\n", pParams->dwAdapterIdx);
}

void cursorThread(void *pData)
{
    if (!pData)
    {
        fprintf(stderr, __FUNCTION__ ": Cursor thread quitting. NULL input params passed\n");
    }

    CursorThreadParams * pParams = (CursorThreadParams*)pData;
    if (!(IS_HANDLE_VALID(pParams->hThreadCreatedEvent) && IS_HANDLE_VALID(pParams->hCursorEvent) &&
          IS_HANDLE_VALID(pParams->hQuitEvent) && IS_HANDLE_VALID(pParams->hGrabEvent)) ||
        (!pParams->pGrabber))
    {
        fprintf(stderr, __FUNCTION__ ": [%d] Cursor thread quitting. Invalid handles passed\n", pParams->dwAdapterIdx);
    }

    DWORD dwWait = WAIT_OBJECT_0;
    HANDLE handle[2] = { pParams->hCursorEvent, pParams->hQuitEvent};

    SetEvent (pParams->hThreadCreatedEvent);
    fprintf (stderr, __FUNCTION__ ": [%d] Cursor thread started\n", pParams->dwAdapterIdx);
    while (1)
    {
        dwWait = WaitForMultipleObjects(2, handle, FALSE ,INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            pParams->pGrabber->cursorCapture();
            SetEvent(pParams->hGrabEvent);
        }
        else
        {
            fprintf(stderr, __FUNCTION__ ": [%d] Cursor Thread Quitting\n", pParams->dwAdapterIdx);
            return ;
        }
    }
    fprintf(stderr, __FUNCTION__ ": [%d] Cursor Thread Quitting unexpectedly\n", pParams->dwAdapterIdx);
}

/*!
 * Main program
 */
int main(int argc, char* argv[])
{
    AppArguments args;
    DWORD maxDisplayWidth = -1, maxDisplayHeight = -1;
    BOOL bRecoveryDone = FALSE;

    unsigned char *frameBuffer = NULL;
    unsigned char *diffMap = NULL;
    std::string outName;
    
    if(!parseCmdLine(argc, argv, args))
        return -1;

    HANDLE hGrabberThread = NULL;
    HANDLE hCursorThread = NULL;

    CursorThreadParams cParams = {0};
    GrabThreadParams gParams = {0};

    cParams.dwAdapterIdx = gParams.dwAdapterIdx = args.iAdapter;
    gParams.hGrabEvent   = CreateEvent(NULL, FALSE, FALSE, NULL);
    gParams.hQuitEvent   = CreateEvent(NULL, FALSE, FALSE, NULL);
    gParams.hGrabCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    gParams.hThreadCreatedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    cParams.hQuitEvent   = CreateEvent(NULL, FALSE, FALSE, NULL);
    cParams.hThreadCreatedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    cParams.hGrabEvent = gParams.hGrabEvent;

    Grabber *pGrabber = NULL;
    if (args.iNVFBCInterface == ToSys)
    {
        pGrabber = new ToSysGrabber();
    }
    else if (args.iNVFBCInterface == ToDX9)
    {
        pGrabber = new ToDX9VidGrabber();
    }
    else if (args.iNVFBCInterface == ToHWEnc)
    {
        pGrabber = new ToHWEncGrabber();
    }
    else if (args.iNVFBCInterface == ToCuda)
    {
        pGrabber = new ToCudaGrabber();
    }
    else
    {
        fprintf(stderr, __FUNCTION__": Invalid interface requested.\n");
        return -1;
    }

    if (pGrabber)
    {
        pGrabber->setAdapterIdx(args.iAdapter);
        
        if (pGrabber->init())
        {
            if (pGrabber->setup())
            {
                cParams.hCursorEvent = pGrabber->getCursorEvent();
                
                cParams.pGrabber = pGrabber;
                gParams.pGrabber = pGrabber;

                hGrabberThread = (void *)_beginthread(grabThread, 0, (void *)&gParams);
                WaitForSingleObject(gParams.hThreadCreatedEvent, INFINITE);
                
                hCursorThread = (void *)_beginthread(cursorThread, 0, (void *)&cParams);
                WaitForSingleObject(cParams.hThreadCreatedEvent, INFINITE);
                
                TestCursors(args.iAdapter);

                SetEvent(cParams.hQuitEvent);
                SetEvent(gParams.hQuitEvent);

                WaitForSingleObject(hCursorThread, INFINITE);
                WaitForSingleObject(hGrabberThread, INFINITE);

                pGrabber->ComputeResult();
            }
            else
            {
                fprintf(stderr, __FUNCTION__ ": [%d] Grabber Setup failed\n", args.iAdapter);
            }
        }
        else
        {
            fprintf(stderr, __FUNCTION__ ": [%d] Grabber Init failed\n", args.iAdapter);
        }
    }
    else
    {
        fprintf(stderr, __FUNCTION__ ": [%d] Grabber alloc failed\n", args.iAdapter);
    }

    CloseHandle(cParams.hQuitEvent);
    CloseHandle(gParams.hGrabEvent);
    CloseHandle(gParams.hQuitEvent);

    pGrabber->release();
    delete pGrabber;

    return 0;
}