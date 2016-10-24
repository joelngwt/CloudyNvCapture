#include "StreamerFile.h"

class Util4Streamer
{
public:
	static Streamer *GetStreamer(AppParam *pAppParam)
	{
		HMODULE hDll = LoadLibrary("Streaming.dll");
		if (!hDll) {
			return new StreamerFile(pAppParam);
		}
		typedef Streamer *(*GetStreamerHttp_Type)(AppParam *);
		GetStreamerHttp_Type GetStreamerHttp_Proc = (GetStreamerHttp_Type)GetProcAddress(hDll, "GetStreamerHttp");
		if (!GetStreamerHttp_Proc) {
			return new StreamerFile(pAppParam);
		}
		return GetStreamerHttp_Proc(pAppParam);
	}
};
