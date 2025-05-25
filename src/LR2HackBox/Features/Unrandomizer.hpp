#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>
#include <unordered_map>
#include <deque>

class Unrandomizer : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	bool GetEnabled();
	void SetEnabled(bool value);
	void ToggleEnabled();

	bool GetBWPermute();
	void SetBWPermute(bool value);
	void ToggleBWPermute();

	void Menu();

	class RandomHistoryEntry {
	private:
		std::string mTitle;
		std::string mRandom;
	public:
		RandomHistoryEntry(std::string title, std::string random);
		std::string GetTitle();
		std::string GetRandom();
	};

private:
	std::vector<SafetyHookMid> mMidHooks;

	enum UnrandomizerState {
		UnrandomizerState_P1,
		UnrandomizerState_P2,
		UnrandomizerState_DP
	};
	static void OnSetRandomSeed(SafetyHookContext& regs);
	static void OnAfterPopulateNoteMapping(SafetyHookContext& regs);
	void DragAndDropKeyDisplay(UnrandomizerState state);

	void RandomHistoryDisplay();
	const std::deque<RandomHistoryEntry>& GetRandomHistory();
	void AddToHistory(RandomHistoryEntry entry);

	bool mIsEnabled = false;
	bool mIsBWPermute = false;
	bool mIsTrackRandom = false;
	bool mIsRRandom = false;
	bool mIs5keysMode = false;
	uint32_t mLaneOrderL[7] = { 1, 2, 3, 4, 5, 6, 7 };
	int mLaneOrderNumL = 1234567;
	uint32_t mLaneOrderR[7] = { 1, 2, 3, 4, 5, 6, 7 };

	void SetOrder(const char* arrange);
	void MirrorOrder();

	std::deque<RandomHistoryEntry> mRandomHistory;
};