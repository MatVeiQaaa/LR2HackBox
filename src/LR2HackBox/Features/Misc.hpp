#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>

class Misc : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void SetMetronome(bool value);

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
	void OnDecideInit();

	bool mRandSelCustomEntry = false;

	static void OnAddToAvgBpmSum(SafetyHookContext& regs);
	static void OnCalcAvgSpeedmult(SafetyHookContext& regs);

	std::unordered_map<double, int> mMainBPMBpmRefcount;

	static void OnDrawNotesGetSongtimer(SafetyHookContext& regs);
	void OnPlayInit();

	int mMetronomeLastPlayedBeat = 0;
	int mMetronomePrevMeasureIdx = -1;

	static int OnSaveDrawScreenToPNG(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel);

	void StartRandomFromFolder();

	void MirrorGearshift(bool mirror);

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsRetryTweaks = false;
	bool mIsRandomSelect = false;
	bool mIsMainBPM = false;
	bool mIsMetronome = false;
	bool mIsRerouteScreenshots = false;
	bool mIsScreenshotsCopybuffer = false;
	bool mIsMirrorGearshift = false;
};