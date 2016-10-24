#pragma once

#include <windows.h>

class Streamer 
{
public:
	virtual ~Streamer() {}
	virtual BOOL Stream(BYTE *pData, int nBytes) = 0;
	virtual BOOL IsReady() = 0;
	virtual void Delete() {delete this;};
};
