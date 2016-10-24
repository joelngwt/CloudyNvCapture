#pragma once

#include <windows.h>
#include <string>

enum ControlInfoType {
	APP_START, APP_END, APP_INPUT
};

enum UserInputType {
	UI_WM, UI_JOY
};

struct UserInput {
	// Serial number
	DWORD sn;
	UserInputType type;

	RECT rcWindow;
	RECT rcClient;

	struct {
		UINT msg;
		DWORD wParam;
		DWORD lParam;
	} wm;

	struct {
		LONG lX;
		LONG lY;
		LONG lZ;
		LONG lRx;
		LONG lRy;
		LONG lRz;
		LONG rglSlider[2];
		DWORD rgdwPOV[4];
		BYTE rgbButtons[32];
	} joy;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & sn;
		ar & type;

		ar & rcWindow.top;
		ar & rcWindow.bottom;
		ar & rcWindow.left;
		ar & rcWindow.right;
		ar & rcClient.top;
		ar & rcClient.bottom;
		ar & rcClient.left;
		ar & rcClient.right;

		ar & wm.msg;
		ar & wm.wParam;
		ar & wm.lParam;

		ar & joy.lX;
		ar & joy.lY;
		ar & joy.lZ;
		ar & joy.lRx;
		ar & joy.lRy;
		ar & joy.lRz;
		for (int i = 0; i < sizeof(joy.rglSlider) / sizeof(joy.rglSlider[0]); i++) {
			ar & joy.rglSlider[i];
		}
		for (int i = 0; i < sizeof(joy.rgdwPOV) / sizeof(joy.rgdwPOV[0]); i++) {
			ar & joy.rgdwPOV[i];
		}
		for (int i = 0; i < sizeof(joy.rgbButtons) / sizeof(joy.rgbButtons[0]); i++) {
			ar & joy.rgbButtons[i];
		}
	}
};

struct ControlInfo {
	ControlInfoType type;

	std::string strCmd;
	std::string strWndClassKeyword;
	std::string strWndTitleKeyword;
	
	int iGpu;
	int iAudio;
	BOOL bDwm;
	
	UINT cxEncoding;
	UINT cyEncoding;

	BOOL bForceCdeclInEnumDevicesCallback;

	UserInput ui;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & type;
		
		ar & strCmd;
		ar & strWndClassKeyword;
		ar & strWndTitleKeyword;

		ar & iGpu;
		ar & iAudio;
		ar & bDwm;
		
		ar & cxEncoding;
		ar & cyEncoding;
		
		ar & bForceCdeclInEnumDevicesCallback;

		ar & ui;
	}
};
