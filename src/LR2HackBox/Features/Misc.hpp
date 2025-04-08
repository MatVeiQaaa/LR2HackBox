#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>

class Misc : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

private:
	static void OnSetRetryFlag(SafetyHookContext& regs);
	static void OnPlayISetSelecter(SafetyHookContext& regs);
	void OnInit(SafetyHookContext& regs);
	static void OnInitPlay(SafetyHookContext& regs);
	static void OnInitRetry(SafetyHookContext& regs);

	int mOrigGaugeType = 0;

	static void OnRandomMixInput(SafetyHookContext& regs);
	static void OnSceneInitSwitch(SafetyHookContext& regs);
	static void OnOpenFolderPlaySound(SafetyHookContext& regs);

	bool mIsRandomSelectEntry = false;

	static void OnAddToAvgBpmSum(SafetyHookContext& regs);
	static void OnCalcAvgSpeedmult(SafetyHookContext& regs);

	void StartRandomFromFolder();

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsRetryTweaks = false;
	bool mIsRandomSelect = false;
	bool mIsMainBPM = false;
};