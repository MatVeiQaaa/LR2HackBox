#include <vector>
#include <windows.h>
#include "BaseModels/ImGuiMenu.hpp"

class ImGuiInjector {
public:
	static ImGuiInjector& Get();
	int Init();
	void SetWndProcHook(HWND hWnd);
	HWND GetWindowHandle();
	WNDPROC GetPreviousWndProc();
	bool WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void AddMenu(ImGuiMenu* pMenu);
	void RunMenus();
	bool IsMenuRunning();
	bool WantsMouseInput();
	bool WantsKeyboardInput();
	void ResetInput();
	void UpdateInput();
	void LoadJapaneseFont();
private:
	ImGuiInjector();
	~ImGuiInjector() = default;
	ImGuiInjector(const ImGuiInjector&) = delete;
	ImGuiInjector& operator=(const ImGuiInjector&) = delete;

	void HookDinput();
	int mDinputVer = 0;

	bool mWantsMouseInput = false;
	bool mWantsKeyboardInput = false;

	HWND mWindowHandle = NULL;
	LONG_PTR mWndProcPtr = NULL;
	WNDPROC mPreviousWndProc = nullptr;

	std::vector<ImGuiMenu*> mMenus;
};