#pragma once

#include <windows.h>

class ImGuiMenu {
public:
	virtual void Loop();
	virtual bool IsOpen();
	virtual bool ToggleOpen();
	virtual bool SetOpen(bool state);
	virtual void MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
protected:
	bool mIsOpen = false;
};