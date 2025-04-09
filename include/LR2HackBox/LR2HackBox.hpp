#include "BaseModels/ModBody.hpp"
#include "BaseModels/ImGuiMenu.hpp"
#include "BaseModels/ModFeature.hpp"
#include "BaseModels/ConfigManager.hpp"

#include "LR2Mem/LR2Bindings.hpp"

class LR2HackBoxMenu : public ImGuiMenu {
public:
	void Loop();
	void MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
};

class LR2HackBox : public ModBody {
public:

	static LR2HackBox& Get();
	bool Hook();
	bool Unhook();

	LR2::game* GetGame();

	LR2HackBoxMenu mMenu;

	ModFeature* mUnrandomizer = nullptr;
	ModFeature* mFunny = nullptr;
	ModFeature* mMisc = nullptr;

	ConfigManager* mConfig = nullptr;
private:
	LR2HackBox() = default;
	~LR2HackBox() = default;
	LR2HackBox(const LR2HackBox&) = delete;
	LR2HackBox& operator=(const LR2HackBox&) = delete;
};