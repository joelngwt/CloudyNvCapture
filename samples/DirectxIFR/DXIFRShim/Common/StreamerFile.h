#pragma once

#include "Streamer.h"
#include <vector>

extern simplelogger::Logger *logger;

class StreamerFile : public Streamer
{
public:
	StreamerFile(AppParam *pAppParam, int width, int height)
	{
		// Open pipe to ffmpeg here
		// Need to accept another argument from the cmd: -player index, -rows, -cols, -size of split screen
		// Based on index, crop the image accordingly.
		
		int row = 0;
		int col = 0;

		for (int i = 0; i < pAppParam->numPlayers; ++i)
		{
			std::stringstream *StringStream = new std::stringstream();
			*StringStream << "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s " << width << "x" << height << \
						     " -re -i - -filter:v crop=\"" << pAppParam->splitWidth << ":" << pAppParam->splitHeight << ":" << 0 + pAppParam->splitWidth*col << ":" << 0 + pAppParam->splitHeight*row << "\" " \
							 "-listen 1 -c:v libx264 -threads 1 -preset ultrafast " \
							 "-an -tune zerolatency -x264opts crf=2:vbv-maxrate=4000:vbv-bufsize=160:intra-refresh=1:slice-max-size=1500:keyint=30:ref=1 " \
							 "-f mpegts http://172.26.186.80:" << 30000 + i << " 2> output" << i << ".txt";
			//*StringStream << "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 1920x1080 -re -i - -c copy -listen 1 " \
			//				 "-f h264 http://172.26.186.80:" << 30000 + i << " 2> output" << i << ".txt";

			//*StringStream << "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 1280x720 -re -i - output.h264 2> output" << i << ".txt";
			PipeList.push_back(_popen(StringStream->str().c_str(), "wb"));

			++col;
			if (col >= pAppParam->cols)
			{
				col = 0;
				++row;
			}
		
			if (PipeList[i] == NULL)
			{
				LOG_ERROR(logger, "Failed to create FFMPEG Pipe");
			}
		}
		
		
	}
	~StreamerFile()
	{
		for (int i = 0; i < PipeList.size(); ++i)
		{
			if (PipeList[i])
			{
				fclose(PipeList[i]);
			}
		}
	}
	BOOL Stream(BYTE *pData, int nBytes, int bufferIndex)
	{
		if (!PipeList[bufferIndex])
		{
			return FALSE;
		}
		fwrite(pData, nBytes, 1, PipeList[bufferIndex]);

		return TRUE;
	}
	BOOL IsReady() 
	{
		for (int i = 0; i < PipeList.size(); ++i)
		{
			if (PipeList[i] == NULL)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

private:
	std::vector<FILE*> PipeList;
};
