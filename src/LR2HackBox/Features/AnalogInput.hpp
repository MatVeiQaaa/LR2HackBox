#include "BaseModels/ModFeature.hpp"
#include "safetyhook/safetyhook.hpp"

#include <unordered_map>
#include <stdint.h>
class AnalogInput : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

	void SetEnabled(bool value);

private:
	static HRESULT __stdcall OnGetDeviceState(void* pThis, DWORD cbData, LPVOID lpvData);
	safetyhook::InlineHook oGetDeviceState;
	static int __cdecl OnInputToButton(void* is, void* cfg_input, int player, int isReplay);
	safetyhook::InlineHook oInputToButton;

	enum Axis {
		X,
		Y,
		Z,
		NONE
	};

	struct InputDevice {
		void* pDevice = nullptr;
		std::string name = "UNDEFINED";
		Axis axis = NONE;
		bool enabled = false;
		bool axisInverted = false;
		int boundDeviceIdx = -1;
		int boundAxisIdx = -1;
		int prevValue = MININT;
		int timeout = 0;
		int timeoutDuration = 100;
		int delta = 0;
		int threshold = 8;
		int upState = 0;
		int downState = 0;

		void Reset() { prevValue = MININT; timeout = 0; delta = 0; upState = 0; downState = 0; }
	};
	std::unordered_map<void*, std::string> devicesMap;
	std::vector<void*> devices;
	std::vector<const char*> deviceNames;
	InputDevice boundDevices[2];

	bool mIsEnabled = false;
};