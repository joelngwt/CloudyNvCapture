#pragma once

#include "Streamer.h"

extern simplelogger::Logger *logger;

class StreamerFile : public Streamer
{
public:
	StreamerFile(AppParam *pAppParam) : hOutFile(NULL)
	{
		hOutFile = CreateFile(pAppParam && pAppParam->bHEVC ? "NvIFR.h265" : "NvIFR.h264", 
			GENERIC_WRITE, FILE_SHARE_READ, 
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOutFile == INVALID_HANDLE_VALUE) {
			LOG_ERROR(logger, "Failed to create file, e=" << GetLastError());
		}
	}
	~StreamerFile()
	{
		if (hOutFile) {
			CloseHandle(hOutFile);
		}
	}
	BOOL Stream(BYTE *pData, int nBytes)
	{
		if (!hOutFile) {
			return FALSE;
		}
		DWORD dwWritten = 0;
		WriteFile(hOutFile, pData, nBytes, &dwWritten, NULL);
		if (dwWritten != nBytes) {
			LOG_ERROR(logger, "Unsuccessful file writing: dwWritten=" << dwWritten << ", e=" << GetLastError());
		}
		return TRUE;
	}
	BOOL IsReady() 
	{
		return hOutFile != NULL;
	}

private:
	HANDLE hOutFile;
};
