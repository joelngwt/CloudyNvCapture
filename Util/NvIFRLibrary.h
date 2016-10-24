#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <NvIFR/NvIFR.h>
#define NVIFR_LIBRARY_NAME      "NvIFR.dll"
#define NVIFR64_LIBRARY_NAME    "NvIFR64.dll"

typedef BOOL (WINAPI *pfnIsWow64Process) (HANDLE, PBOOL);

class NvIFRLibrary
{
protected:
    HMODULE m_handle;
    NvIFRLibrary(const NvIFRLibrary &);
    NvIFRLibrary &operator=(const NvIFRLibrary &);
    pfnIsWow64Process fnIsWow64Process;
    
    NvIFR_CreateFunctionExType                  NvIFR_CreateEx_fn;
#if DIRECT3D_VERSION  == 0x0900
    NvIFR_CreateSharedSurfaceEXTFunctionType    NvIFR_CreateSharedSurfaceEXT_fn;
    NvIFR_DestroySharedSurfaceEXTFunctionType   NvIFR_DestroySharedSurfaceEXT_fn;
    NvIFR_CopyToSharedSurfaceEXTFunctionType    NvIFR_CopyToSharedSurfaceEXT_fn;
    NvIFR_CopyFromSharedSurfaceEXTFunctionType  NvIFR_CopyFromSharedSurfaceEXT_fn;
#endif

    BOOL IsWow64()
    {
        BOOL bIsWow64 = FALSE;

        fnIsWow64Process = (pfnIsWow64Process) GetProcAddress(
            GetModuleHandle(TEXT("kernel32.dll")),"IsWow64Process");
      
        if (NULL != fnIsWow64Process)
        {
            if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
            {
                bIsWow64 = false;
            }
        }
        return bIsWow64;
    }

    std::string getNvIFRDefaultPath()
    {
        std::string defaultPath;

        size_t pathSize;
        char *libPath;

        if(0 != _dupenv_s(&libPath, &pathSize, "SystemRoot"))
        {
            fprintf(stderr, "Unable to get the SystemRoot environment variable\n");
            return defaultPath;
        }

        if(0 == pathSize)
        {
            fprintf(stderr, "The SystemRoot environment variable is not set\n");
            return defaultPath;
        }
#ifdef _WIN64
        defaultPath = std::string(libPath) + "\\System32\\" + NVIFR64_LIBRARY_NAME;
#else
        if (IsWow64())
        {
            defaultPath = std::string(libPath) + "\\Syswow64\\" + NVIFR_LIBRARY_NAME;
        }
        else
        {
            defaultPath = std::string(libPath) + "\\System32\\" + NVIFR_LIBRARY_NAME;            
        }
#endif
        return defaultPath;
    }
    
public:
    NvIFRLibrary()
        : m_handle(NULL)
        , fnIsWow64Process(NULL)
        , NvIFR_CreateEx_fn(NULL)
#if DIRECT3D_VERSION  == 0x0900
        , NvIFR_CreateSharedSurfaceEXT_fn(NULL)
        , NvIFR_DestroySharedSurfaceEXT_fn(NULL)
        , NvIFR_CopyToSharedSurfaceEXT_fn(NULL)
        , NvIFR_CopyFromSharedSurfaceEXT_fn(NULL)
#endif
    {
    }

    ~NvIFRLibrary()
    {
        //if(NULL != m_handle)
        //    FreeLibrary(m_handle);
    }

    HMODULE load()
    {
        if (NULL != m_handle)
        {
            return m_handle;
        }
        m_handle = LoadLibrary(getNvIFRDefaultPath().c_str());
        if (!m_handle)
        {
            fprintf(stderr, "Unable to load NvIFR.\n");
        }
        else
        {
            NvIFR_CreateEx_fn                  = (NvIFR_CreateFunctionExType) GetProcAddress(m_handle, "NvIFR_CreateEx");
#if DIRECT3D_VERSION  == 0x0900
            NvIFR_CreateSharedSurfaceEXT_fn    = (NvIFR_CreateSharedSurfaceEXTFunctionType) GetProcAddress( m_handle, "NvIFR_CreateSharedSurfaceEXT");
            NvIFR_DestroySharedSurfaceEXT_fn   = (NvIFR_DestroySharedSurfaceEXTFunctionType) GetProcAddress( m_handle, "NvIFR_DestroySharedSurfaceEXT");
            NvIFR_CopyToSharedSurfaceEXT_fn    = (NvIFR_CopyToSharedSurfaceEXTFunctionType) GetProcAddress( m_handle, "NvIFR_CopyToSharedSurfaceEXT");
            NvIFR_CopyFromSharedSurfaceEXT_fn  = (NvIFR_CopyFromSharedSurfaceEXTFunctionType) GetProcAddress( m_handle, "NvIFR_CopyFromSharedSurfaceEXT");
#endif

#if DIRECT3D_VERSION  == 0x0900
            if (NvIFR_CreateEx_fn                  == NULL ||
                NvIFR_CreateSharedSurfaceEXT_fn    == NULL ||
                NvIFR_DestroySharedSurfaceEXT_fn   == NULL ||
                NvIFR_CopyToSharedSurfaceEXT_fn    == NULL ||
                NvIFR_CopyFromSharedSurfaceEXT_fn  == NULL)
#else
            if (NvIFR_CreateEx_fn                  == NULL)
#endif                
            {
                fprintf(stderr, "Unable to load NvIFR Entrypoints.\n");
                FreeLibrary(m_handle);
                m_handle = NULL;
            }
        }
        return m_handle;
    }

    void *create(void *pDev, NvU32 dwInterfaceType)
    {
        //! Get the proc address for the NvIFR_Create function
        //! Create the NvIFR object
        NVIFR_CREATE_PARAMS params = {0};
        if (NvIFR_CreateEx_fn)
        {
            params.dwVersion = NVIFR_CREATE_PARAMS_VER;
            params.dwInterfaceType = dwInterfaceType;
            params.pDevice = pDev;
            NVIFRRESULT res = NvIFR_CreateEx_fn(&params);
            if (res != NVIFR_SUCCESS)
            {
                fprintf(stderr, "NvIFR_CreateEx failed with error %d.\n", res);
            }
        }
        else
        {
            fprintf(stderr, "Invalid call. NvIFR Library not initialized.\n");
        }
        return params.pNvIFR;
    }
    
    NVIFRRESULT createEx(NVIFR_CREATE_PARAMS *pParams)
    {
        //! Get the proc address for the NvIFR_Create function
        //! Create the NvIFRToSys object
        NVIFRRESULT res = NVIFR_ERROR_INVALID_CALL;
        if (NvIFR_CreateEx_fn)
        {
            res = NvIFR_CreateEx_fn(pParams);
        }
        else
        {
            fprintf(stderr, "Invalid call. NvIFR Library not initialized.\n");
        }
        return res;
    }
#if DIRECT3D_VERSION  == 0x0900    
    BOOL CreateSharedSurface (IDirect3DDevice9 * pDevice, NvU32 dwWidth, NvU32 dwHeight, IFRSharedSurfaceHandle * phIFRSharedSurface)
    {
        if (NvIFR_CreateSharedSurfaceEXT_fn)
        {
            return NvIFR_CreateSharedSurfaceEXT_fn(pDevice, dwWidth, dwHeight, phIFRSharedSurface);
        }
        return false;
    }
    
    BOOL DestroySharedSurface (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface)
    {
        if (NvIFR_DestroySharedSurfaceEXT_fn)
        {
            return NvIFR_DestroySharedSurfaceEXT_fn(pDevice, hIFRSharedSurface);
        }
        return false;
    }
    
    BOOL CopyToSharedSurface (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface)
    {
        if (NvIFR_CopyToSharedSurfaceEXT_fn)
        {
            return NvIFR_CopyToSharedSurfaceEXT_fn(pDevice, hIFRSharedSurface, pSurface);
        }
        return false;
    }
    
    BOOL CopyFromSharedSurface (IDirect3DDevice9 * pDevice, IFRSharedSurfaceHandle hIFRSharedSurface, IDirect3DSurface9 * pSurface)
    {
        if (NvIFR_CopyFromSharedSurfaceEXT_fn)
        {
            return NvIFR_CopyFromSharedSurfaceEXT_fn(pDevice, hIFRSharedSurface, pSurface);
        }
        return false;
    }
#endif
};
