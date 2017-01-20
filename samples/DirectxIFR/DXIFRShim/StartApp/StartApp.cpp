#include <windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <iostream>
#include <signal.h>
#include "Logger.h"
#include "AppParam.h"
#include "Util4Streamer.h"

using namespace std;

simplelogger::Logger *logger 
	= simplelogger::LoggerFactory::CreateConsoleLogger(simplelogger::DEBUG);

void ShowUsageAndExit(char *szExeName) 
{
	printf(
		"Usage: %s -r <WxH> -gpu <gpu number> -audio <audio number> -hevc <application command line> -players <number of players> " \
		"-rows <number of split screen rows> -cols <number of split screen columns> -width <width of a single split screen> " \
		"-height <height of a single split screen>\n"
		"-hevc is optional\n"
		"-width and -height seems broken. Avoid for now.\n", szExeName);
	exit(0);
}

static POINT aRes[] = {{640,480}, {800,600}, {720,576}, {1024,768}, {1280,720}, {1680,1050}, {1920,1080}, {2560,1600}};

static int FindMatchedResolution(char *szResolution) 
{
	char buf[10];
	for (int i = 0; i < sizeof(aRes) / sizeof(aRes[0]); ++i) {
		sprintf(buf, "%dx%d", aRes[i].x, aRes[i].y);
		if (!strcmp(buf, szResolution)) {
			return i;
		}
	}
	return -1;
}

void ParseArgs(int argc, char *argv[], int &iArg, int &iResolution, int &iGpu, int &iAudio, 
			   int &iNumPlayers, int &iCols, int &iRows, int &iSplitWidth, int &iSplitHeight, BOOL &bHEVC)
{
	char *str, *pEnd;
	for (iArg = 1; iArg < argc; iArg++) {
		if (!_stricmp(argv[iArg], "-r")) {
			if (iArg + 1 < argc && 
				(iResolution = FindMatchedResolution(argv[++iArg])) != -1) {
					continue;
			}
			ShowUsageAndExit(argv[0]);
		}

		if (!_stricmp(argv[iArg], "-gpu")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iGpu = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iGpu < 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-audio")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iAudio = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iAudio < 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-players")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];                                                                                                                         
			iNumPlayers = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iNumPlayers <= 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-cols")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iCols = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iCols <= 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-rows")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iRows = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iRows <= 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-width")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iSplitWidth = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iSplitWidth <= 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-height")) {
			if (iArg + 1 >= argc) {
				ShowUsageAndExit(argv[0]);
			}
			str = argv[++iArg];
			iSplitHeight = strtol(str, &pEnd, 10);
			if (pEnd == str || *pEnd != '\0' || iSplitHeight <= 0) {
				ShowUsageAndExit(argv[0]);
			}
			continue;
		}

		if (!_stricmp(argv[iArg], "-hevc")) {
			bHEVC = true;
			continue;
		}

		/*When control flow reaches here, no valid option is parsed. 
		  The rest are application command line.*/
		break;
	}

	if (iArg == argc) {
		//No application program command line
		ShowUsageAndExit(argv[0]);
	}
}

BOOL TrimPathToDirectory(char *szPath) 
{
	std::string strIniPath(szPath);
	size_t i = strIniPath.rfind("\\");
	if (i != std::string::npos) {
		szPath[i] = '\0';
		return TRUE;
	}
	return FALSE;
}

void PrintWaitingMessage(char *msg) {
	static int n = 0;
	static char *sign = "-\\|/";
	cout << msg << sign[n++%4] << '\r';
	cout.flush();
}

BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) 
{
	LUID luid;
	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
		LOG_INFO(logger, "LookupPrivilegeValue error: " << GetLastError());
		return FALSE; 
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD)NULL)) { 
		LOG_INFO(logger, "AdjustTokenPrivileges error: " << GetLastError());
		return FALSE; 
	}

	return GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

BOOL ElevatePrivilege() {
	HANDLE hProcessCurrent = GetCurrentProcess(), hTokenCurrent;
	BOOL bSuccess = FALSE;
	if (OpenProcessToken(hProcessCurrent, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenCurrent)) {
		bSuccess = SetPrivilege(hTokenCurrent, SE_DEBUG_NAME, TRUE);
		CloseHandle(hTokenCurrent);
	}
	return bSuccess;
}

BOOL bFullStop = FALSE;
void SignalHandler_FullStop(int signal)
{
	bFullStop = TRUE;
}

int main(int argc, char *argv[]) 
{
	if (argc == 1) {
		ShowUsageAndExit(argv[0]);
	}

	ElevatePrivilege();

	int iArg;
	int iRes = 1;
	int iGpu = 0;
	int iAudio = 0;
	int iNumPlayers = 1;
	int iCols = 1;
	int iRows = 1;
	int iSplitWidth = 0;
	int iSplitHeight = 0;
	BOOL bHEVC = FALSE;
	ParseArgs(argc, argv, iArg, iRes, iGpu, iAudio, iNumPlayers, iCols, iRows, iSplitWidth, iSplitHeight, bHEVC);

	ULONGLONG pid = GetCurrentProcessId();
	AppParamManager appParamManger(&pid);
	AppParam *pAppParam = appParamManger.GetAppParam();
	if (!pAppParam) {
		printf("Unable to setup shared memory. Program will exit.\n");
		return 1;
	}
	pAppParam->iGpu = iGpu;

	if (iAudio >= 0) {
		sprintf(pAppParam->szAudioKeyword, "Nvidia Capture:%02d", iAudio);
		printf("pAppParam->szAudioKeyword=%s\n", pAppParam->szAudioKeyword);
	} else {
		*pAppParam->szAudioKeyword = '\0';
	}

	pAppParam->numPlayers = iNumPlayers;
	pAppParam->cols = iCols;
	pAppParam->rows = iRows;
	if (iSplitWidth == 0 || iSplitHeight == 0)
	{
		pAppParam->splitHeight = aRes[iRes].y / iRows;
		pAppParam->splitWidth = aRes[iRes].x / iCols;;
		pAppParam->height = aRes[iRes].y;
		pAppParam->width = aRes[iRes].x;
	}
	else
	{
		pAppParam->splitHeight = iSplitHeight;
		pAppParam->splitWidth = iSplitWidth;
	}
	pAppParam->cxEncoding = aRes[iRes].x;
	pAppParam->cyEncoding = aRes[iRes].y;
	pAppParam->bHEVC = bHEVC;

	char szAppDir[MAX_PATH];
	strcpy_s(szAppDir, argv[iArg]);
	if (!TrimPathToDirectory(szAppDir)) {
		strcpy_s(szAppDir, ".");
	}
	std::ostringstream ossCmdLine;
	for (; iArg < argc; iArg++) {
		ossCmdLine << argv[iArg] << " ";
	}
	char szCmdLine[32 * 1024];
	sprintf_s(szCmdLine, sizeof(szCmdLine), "%s", ossCmdLine.str().c_str());

	printf(
		"GPU number: %d\n"
		"Audio number: %d\n"
		"Codec: %s\n"
		"Number of players: %d\n"
		"Rows x Columns: %d x %d\n"
		"Width x height: %d x %d\n"
		"Starting application: %s\n"
		"Working directory: %s\n"
		, iGpu, iAudio, bHEVC ? "H265" : "H264", pAppParam->numPlayers, pAppParam->cols, pAppParam->rows, 
		pAppParam->splitWidth, pAppParam->splitHeight, szCmdLine, szAppDir);

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, szAppDir, &si, &pi)) {
		printf("Failed to start process: %s\n", szCmdLine);
		return 0;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	while (appParamManger.IsAppUninitialized()) {
		Sleep(100);
	}
	return 0;
}
