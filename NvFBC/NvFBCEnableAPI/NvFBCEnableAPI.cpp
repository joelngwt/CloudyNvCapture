#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <d3d9.h>
#define _D3D9_H_ 
#include "nvapi.h"
#include "windows.h"

#include "helper_string.h"
#include "NvFBCLibrary.h"
#include <setupapi.h>
#include <devguid.h>

#define NVFBC_VALUE_STRING "NVFBCEnable"
#define NVFBC_KEY_STRING "SYSTEM\\CurrentControlSet\\services\\nvlddmkm"
#define PRV_BOOTCOUNT_VALUE_STRING "Reboot"
#define PRV_BOOTCOUNT_KEY_STRING "SOFTWARE\\NVIDIA Corporation\\NvFBCEnable"
#define BOOTCOUNT_VALUE_STRING "BootId"
#define BOOTCOUNT_KEY_STRING "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"

typedef struct
{
    // Deprecated. Do Not Use.
    BOOL    bIsCapturePossible;     // OUT, tells if driver supports the NvFBC feature
    BOOL    bCurrentlyCapturing;    // OUT, tells if NVFBC is currently capturing
    BOOL    bCanCreateNow;          // OUT, can NvFBC_Create be called now?
    DWORD   dwVersion;              // OUT, get NvFBC interface version
}NvFBCStatus;

void printUsage()
{
    printf("Usage:\n\n");
    printf("  NvFBCEnable <option>\n\n");
    printf("Options:\n\n");
    printf("  -h           = Print this message.\n");
    printf("  -enable      = Enable NvFBC support.\n");
    printf("  -disable     = Disable NvFBC support.\n");
    printf("  -dynamic     = Using this mode will enable/ disable without reloading driver. \n");
    printf("  -noreset     = Using this mode requires the user to reboot for enabling or disabling.\n");
    printf("  -checkstatus = Check if NvFBC support is enabled.\n");
}

LONG setKeyValue(const char *keyString, const char *valueString, DWORD valueType, const BYTE *data, DWORD dataSize)
{
    HKEY key;
    LONG setResult = 0;
    setResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, keyString, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &key, NULL);

    if (setResult != ERROR_SUCCESS)
        return setResult;

    if(ERROR_SUCCESS != (setResult = RegSetValueEx(key, valueString, 0, valueType, data, dataSize)))
    {
        RegCloseKey(key);
        return setResult;
    }

    RegCloseKey(key);
    return setResult;
}

LONG deleteKeyValue(const char *keyString, const char *valueString)
{
    HKEY key;
    LONG deleteResult;
    deleteResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, keyString, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &key, NULL);

    if (ERROR_SUCCESS != deleteResult)
        return deleteResult;

    if(ERROR_SUCCESS != (deleteResult = RegDeleteValue(key, valueString)))
    {
        RegCloseKey(key);
        return deleteResult;
    }

    RegCloseKey(key);
    return deleteResult;
}

void nvfbcNoReset(NVFBC_STATE state)
{
    DWORD value = 1;
    DWORD valueSize = sizeof(DWORD);
    LONG result = ERROR_SUCCESS;

    // Set the NVFBCEnable key value
    if (state == NVFBC_STATE_ENABLE)
        result = setKeyValue(NVFBC_KEY_STRING, NVFBC_VALUE_STRING, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    else
        result = deleteKeyValue(NVFBC_KEY_STRING, NVFBC_VALUE_STRING);

    if (ERROR_SUCCESS != result)
    {
        printf("Insufficient privilege to set  %s regkey value\n",state?"Enable":"Disable");
        return;
    }
    else
    {
        RegGetValueA(HKEY_LOCAL_MACHINE, BOOTCOUNT_KEY_STRING, BOOTCOUNT_VALUE_STRING, RRF_RT_ANY, NULL, &value, &valueSize); //get the last boot count
        result = setKeyValue(PRV_BOOTCOUNT_KEY_STRING, PRV_BOOTCOUNT_VALUE_STRING, REG_DWORD, (BYTE *)&value, sizeof(DWORD)); //store prev boot count in regkey
        if(result != ERROR_SUCCESS)
        {
            printf("Insufficient privileges\n");
            return;
        }
        printf("%s success. Please reboot to take effect\n", state ? "Enable" : "Disable");
    }
    return;
}

void nvfbcCheckStatusEx()
{
    HRESULT hr;
    BOOL bLegacyPath = FALSE;
    DWORD NvDispCount = 0;
    DWORD enabled = 0;
    DWORD prevBootCount = 0;
    DWORD enabledSize = sizeof(DWORD);
    DWORD bootCount = 0;
    DWORD bootCountSize = sizeof(DWORD);
    NvFBCLibrary nvfbc;
    if (!nvfbc.load())
        return;

    RegGetValueA(HKEY_LOCAL_MACHINE, PRV_BOOTCOUNT_KEY_STRING, PRV_BOOTCOUNT_VALUE_STRING, RRF_RT_ANY, NULL, &prevBootCount, &enabledSize);
    RegGetValueA(HKEY_LOCAL_MACHINE, BOOTCOUNT_KEY_STRING, BOOTCOUNT_VALUE_STRING, RRF_RT_ANY, NULL, &bootCount, &bootCountSize);
    if (prevBootCount == bootCount) //check if previously we requested for a reboot from user
    {
        printf("Please reboot\n");
        return;
    }

    hr = RegGetValueA(HKEY_LOCAL_MACHINE, NVFBC_KEY_STRING, NVFBC_VALUE_STRING, RRF_RT_ANY, NULL, &enabled, &enabledSize);
    if (hr != ERROR_SUCCESS && hr != ERROR_FILE_NOT_FOUND)
    {
        printf("Insuffcient privilege to query NVFBC status\n");
        return;
    }

    printf("NvFBC is %s\n", enabled ? "enabled" : "disabled");
    if (!enabled)
        return; // no need to check status of displays if disabled

    NvAPI_Initialize();
    NvU32 vidPnSrcId = 0;
    NvDisplayHandle nvDispHandle;
    NvAPI_Status nvAPI_result;
    NVFBCRESULT nvFBC_result;

    LPDIRECT3D9 g_pD3D = NULL;
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        printf("      -> Unable to create Dx9 device. Can't proceed further\n");
        return;
    }

    DWORD totalAdapters = g_pD3D->GetAdapterCount();
    DWORD dwAdapterIdx = 0;
    D3DADAPTER_IDENTIFIER9 adapterInfo;

    NvFBCStatusEx nvFBCStatusParam;
    memset(&nvFBCStatusParam, 0, sizeof(NvFBCStatusEx));

    while (dwAdapterIdx < totalAdapters)
    {
        memset(&nvFBCStatusParam, 0, sizeof(nvFBCStatusParam));
        nvFBCStatusParam.dwAdapterIdx = dwAdapterIdx;
        g_pD3D->GetAdapterIdentifier(dwAdapterIdx, 0, &adapterInfo);
        nvAPI_result = NvAPI_GetAssociatedNvidiaDisplayHandle(adapterInfo.DeviceName, &nvDispHandle);

        if (nvAPI_result == NVAPI_OK)
        {
            ++NvDispCount; // Connected to Nvidia GPU. Increment the count by one
            nvFBC_result = nvfbc.getStatus(&nvFBCStatusParam);
            if (nvFBC_result == NVFBC_SUCCESS)
                printf("      -> %s in display: dwAdapterId = %d, displayName = %s\n", (!nvFBCStatusParam.bIsCapturePossible) ? "Unsupported" : (nvFBCStatusParam.bCurrentlyCapturing) ? "Currently capturing" : "NOT capturing", nvFBCStatusParam.dwAdapterIdx, adapterInfo.DeviceName);
            else
                printf("      -> Unable to fetch details for display: dwAdapterId = %d, displayName = %s\n", nvFBCStatusParam.dwAdapterIdx, adapterInfo.DeviceName);
        }
        
        ++dwAdapterIdx;
    }

    if (NvDispCount == 0)
    {
        nvFBC_result = nvfbc.getStatus(&nvFBCStatusParam);
        if (nvFBC_result == NVFBC_SUCCESS)
            printf("      ->  %s in default display. No NVIDIA display found\n", (!nvFBCStatusParam.bIsCapturePossible) ? "Unsupported" : (nvFBCStatusParam.bCurrentlyCapturing) ? "Currently capturing" : "NOT capturing");
        else
            return;
    }
    return;
}


int main(int argc, char *argv[])
{
    NVFBC_STATE state = NVFBC_STATE_ENABLE;
    bool bDynamic = false;
    bool bCheckStatus = true;
    bool noReset = false;
    
    for(int cnt = 1; cnt < argc; ++cnt)
    {
        if(0 == STRCASECMP(argv[cnt], "-enable"))
        {
            state = NVFBC_STATE_ENABLE;
        }
        else if(0 == STRCASECMP(argv[cnt], "-disable"))
        {
            state = NVFBC_STATE_DISABLE;
        }
        else if (0 == STRCASECMP(argv[cnt], "-dynamic"))
        {
            bDynamic = true;
        }
        else if (0 == STRCASECMP(argv[cnt], "-noreset"))
        {
            noReset = true;
        }
        else if (0 == STRCASECMP(argv[cnt], "-checkstatus"))
        {
            nvfbcCheckStatusEx();
            return 0;
        }
        else if (0 == STRCASECMP(argv[cnt], "-h"))
        {
            printUsage();
            return 0;
        }
        else
        {
            printf("Unexpected argument: %s\n", argv[cnt]);
            printUsage();
            return 0;
        }
    }

    if (noReset)
    {
        nvfbcNoReset(state);
        return 0;
    }
    else
    {
        NvFBCLibrary nvfbc;
        HRESULT result = deleteKeyValue(PRV_BOOTCOUNT_KEY_STRING, PRV_BOOTCOUNT_VALUE_STRING); //delete, because user requested enable/disable without reboot
        if (ERROR_SUCCESS != result && result != ERROR_FILE_NOT_FOUND)
        {
            printf("Insufficient privilege\n"); 
            return 0;
        }
        if (nvfbc.load())
        {
            if (bDynamic)
            {
                nvfbc.setGlobalFlags(NVFBC_GLOBAL_FLAGS_NO_DEVICE_RESET_TOGGLE);
            }
            nvfbc.enable(state);
        }
        return 0;
    }
}
