#pragma once

#include "Streamer.h"


extern simplelogger::Logger *logger;

class StreamerFile : public Streamer
{
public:
	

	StreamerFile(AppParam *pAppParam)
	{
		// Open pipe to ffmpeg here
		// Need to accept another argument from the cmd: -player index, -rows, -cols, -size of split screen
		// Based on index, crop the image accordingly.
		std::stringstream *StringStream = new std::stringstream();
		//*StringStream << "ffmpeg -re -i - -c copy -listen 1 -c:v libx264 -threads 1 -preset ultrafast " \
		//				 "-an -tune zerolatency -x264opts crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=1500:keyint=30:ref=1 " \
		//				 "-f mpegts http://172.26.186.80:" << "30000 2> output.txt";
		*StringStream << "ffmpeg -re -i - -c copy -listen 1 " \
						 "-f h264 http://172.26.186.80:" << "30000 2> output.txt";
		FFMPEGPipe = _popen(StringStream->str().c_str(), "wb");
		if (FFMPEGPipe == NULL)
		{
			LOG_ERROR(logger, "Failed to create FFMPEG Pipe");
		}
	}
	~StreamerFile()
	{
		if (FFMPEGPipe)
		{
			fclose(FFMPEGPipe);
		}
	}
	BOOL Stream(BYTE *pData, int nBytes)
	{
		if (!FFMPEGPipe) 
		{
			return FALSE;
		}

		fwrite(pData, nBytes, 1, FFMPEGPipe);
		return TRUE;
	}
	BOOL IsReady() 
	{
		return FFMPEGPipe != NULL;
	}

private:
	FILE* FFMPEGPipe;
};
