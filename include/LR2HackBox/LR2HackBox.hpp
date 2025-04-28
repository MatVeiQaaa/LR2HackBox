#include "BaseModels/ModBody.hpp"
#include "BaseModels/ImGuiMenu.hpp"
#include "BaseModels/ModFeature.hpp"
#include "BaseModels/ConfigManager.hpp"

#include "LR2Mem/LR2Bindings.hpp"

#include <unordered_map>
#include <string>

#ifndef NDEBUG
#define IFDEBUG(x) x
//#define MEMORYTRACKER
#else
#define IFDEBUG(x)
#endif

#ifdef MEMORYTRACKER
#define IFMEMORYTRACKER(x) x
#else
#define IFMEMORYTRACKER(x)
#endif

class LR2HackBoxMenu : public ImGuiMenu {
public:
	void Loop();
	void MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void InitBindings();
private:
	void BindingsMenu();

	bool mIsRebindMenu = false;

	std::unordered_map<std::string, int> mMenuBindings;
	std::pair<std::string, bool> mMenuBindingAwaitsRebind{ "NONE", false };
};

class LR2HackBox : public ModBody {
public:

	static LR2HackBox& Get();
	bool Hook();
	bool Unhook();

	LR2::game* GetGame();
	void* GetSqlite();

	LR2HackBoxMenu mMenu;

	ModFeature* mUnrandomizer = nullptr;
	ModFeature* mFunny = nullptr;
	ModFeature* mMisc = nullptr;
	ModFeature* mAnalogInput = nullptr;
	IFMEMORYTRACKER(ModFeature* mMemoryTracker = nullptr);

	ConfigManager* mConfig = nullptr;
private:
	LR2HackBox() = default;
	~LR2HackBox() = default;
	LR2HackBox(const LR2HackBox&) = delete;
	LR2HackBox& operator=(const LR2HackBox&) = delete;
};