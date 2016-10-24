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
#include <d3d9.h>
#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <cudaD3D9.h>
#include <NvFBCLibrary.h>
#include <NvFBC/nvFBCToSys.h>
#include <NvFBC/nvFBCToDx9Vid.h>
#include <NvFBC/nvFBCHWEnc.h>
#include <NvFBC/nvFBCCuda.h>

class Grabber;

#define SAFE_RELEASE_1(x) if(x){x->Release();x=NULL;}
#define SAFE_RELEASE_2(x,y) if(x){x->y();x=NULL;}

typedef struct _POINTERFLAGS {
  union {
    struct {
      UINT Monochrome  :1;
      UINT Color  :1;
      UINT MaskedColor  :1;
      UINT Reserved  :29;
    };
    UINT Value;
  };
} POINTERFLAGS;

typedef struct _grabthreadParams
{
    DWORD  dwAdapterIdx;
    HANDLE hQuitEvent;
    HANDLE hGrabEvent;
    HANDLE hGrabCompleteEvent;
    HANDLE hThreadCreatedEvent;
    Grabber *pGrabber;
} GrabThreadParams;

typedef struct _cursorParams
{
    DWORD  dwAdapterIdx;
    HANDLE hGrabEvent;
    HANDLE hCursorEvent;
    HANDLE hQuitEvent;
    HANDLE hThreadCreatedEvent;
    Grabber *pGrabber;
}CursorThreadParams;

class Grabber
{
protected:
    HANDLE m_hCursorEvent;
    void *m_pCursorBuf;
    unsigned int m_uFrameNo;
    NvFBCLibrary lib;
    DWORD m_dwMaxWidth;
    DWORD m_dwMaxHeight;
    DWORD dwFBCType;
    void *m_pvFBC;
    DWORD m_dwAdapterIdx;
        
    // DirectX resources
    IDirect3D9Ex        *m_pD3DEx;
    IDirect3DDevice9Ex  *m_pD3D9Device;
    IDirect3DSurface9   *m_pGDICursorSurf;
    IDirect3DSurface9   *m_pFBCCursorSurf;

    HRESULT InitD3D9();
    void CompareCursor(NVFBC_CURSOR_CAPTURE_PARAMS *pCursorParams, BOOL bDumpFrame=TRUE);
public:
    Grabber(DWORD id)
        : m_hCursorEvent(NULL)
        , m_pCursorBuf(NULL)
        , m_uFrameNo(0)
        , m_dwMaxWidth(0)
        , m_dwMaxHeight(0)
        , dwFBCType(id)
        , m_pD3DEx(NULL)
        , m_pD3D9Device(NULL)
        , m_pGDICursorSurf(NULL)
        , m_pFBCCursorSurf(NULL)
        , m_pvFBC(NULL)
        , m_dwAdapterIdx(0)
    {
    }
    void setAdapterIdx(DWORD dwAdapterIdx) { m_dwAdapterIdx = dwAdapterIdx; }
    DWORD getAdapterIdx() { return m_dwAdapterIdx; }
    virtual bool init();
    virtual bool setup()=0;
    virtual bool grab()=0;
    virtual bool cursorCapture()=0;
    virtual void release()=0;
    void ComputeResult();
    HANDLE getCursorEvent() { return m_hCursorEvent; }
    ~Grabber();
};

class ToSysGrabber : public Grabber
{
private:
    NvFBCToSys *m_pFBC;
    void *m_pSysMemBuf;
public:
    ToSysGrabber()
        : Grabber(NVFBC_TO_SYS)
        , m_pSysMemBuf(NULL)
        , m_pFBC(NULL)
    {
    }
    ~ToSysGrabber(){}
public:
    virtual bool init();
    virtual bool setup();
    virtual bool grab();
    virtual bool cursorCapture();
    virtual void release();
};

class ToDX9VidGrabber : public Grabber
{
private:
    NvFBCToDx9Vid *m_pFBC;
    IDirect3DSurface9 *m_pSurf;
public:
    ToDX9VidGrabber()
        : Grabber(NVFBC_TO_DX9_VID)
        , m_pFBC(NULL)
        , m_pSurf(NULL)
    {
    }
    ~ToDX9VidGrabber(){}
public:
    virtual bool init();
    virtual bool setup();
    virtual bool grab();
    virtual bool cursorCapture();
    virtual void release();
};


class ToHWEncGrabber : public Grabber
{
private:
    INvFBCHWEncoder *m_pFBC;
    void *m_pBitStreamBuf;
    FILE *m_pFout;
public:
    ToHWEncGrabber()
        : Grabber(NVFBC_TO_HW_ENCODER)
        , m_pFout(NULL)
        , m_pBitStreamBuf(NULL)
        , m_pFBC(NULL)
    {
    }
    ~ToHWEncGrabber(){}
public:
    virtual bool init();
    virtual bool setup();
    virtual bool grab();
    virtual bool cursorCapture();
    virtual void release();
};


class ToCudaGrabber : public Grabber
{
private:
    NvFBCCuda *m_pFBC;
    DWORD      m_dwMaxBufferSize;
    CUcontext  cudaCtx;
    CUdevice   cudaDevice;
    CUdeviceptr pDevGrabBuffer;
    void *      pCPUBuffer;
public:
    ToCudaGrabber ()
        : Grabber (NVFBC_SHARED_CUDA)
        , m_pFBC (NULL)
        , m_dwMaxBufferSize (0)
        , cudaCtx (NULL)
        , cudaDevice (NULL)
        , pDevGrabBuffer (NULL)
        , pCPUBuffer (NULL)
    {
    }
    ~ToCudaGrabber () {}
public:
    virtual bool init ();
    virtual bool setup ();
    virtual bool grab ();
    virtual bool cursorCapture ();
    virtual void release ();
};