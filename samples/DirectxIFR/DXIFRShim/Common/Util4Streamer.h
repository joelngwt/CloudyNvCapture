#include "StreamerFile.h"

class Util4Streamer
{
public:
	static Streamer *GetStreamer(AppParam *pAppParam, int width, int height)
	{
		HMODULE hDll = LoadLibrary("Streaming.dll");
		if (!hDll) {
			return new StreamerFile(pAppParam, width, height);
		}
		typedef Streamer *(*GetStreamerHttp_Type)(AppParam *);
		GetStreamerHttp_Type GetStreamerHttp_Proc = (GetStreamerHttp_Type)GetProcAddress(hDll, "GetStreamerHttp");
		if (!GetStreamerHttp_Proc) {
			return new StreamerFile(pAppParam, width, height);
		}
		return GetStreamerHttp_Proc(pAppParam);
	}
};
