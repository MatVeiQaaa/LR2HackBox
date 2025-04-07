#include "Unrandomizer.hpp"
#include "Unrandomizer_SeedMap.hpp"

#include <iostream>
#include <string>
#include <array>
#include <random>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <locale>
#include <codecvt>

#include "LR2HackBox/LR2HackBox.hpp"

#include "safetyhook/safetyhook.hpp"
#include "imgui/imgui.h"

#pragma comment(lib, "libSafetyhook.lib")

void Unrandomizer::OnSetRandomSeed(SafetyHookContext& regs) {
	Unrandomizer& unrandomizer = *(Unrandomizer*)(LR2HackBox::Get().mUnrandomizer);
	if (!unrandomizer.GetEnabled()) return;
	LR2::game& game = *LR2HackBox::Get().GetGame();

	uintptr_t* randomseed = &regs.eax;

	std::stringstream laneOrder;
	for (int i = 0; i < std::size(unrandomizer.mLaneOrderL); i++) {
		laneOrder << unrandomizer.mLaneOrderL[i];
	}
	if (unrandomizer.GetBWPermute()) {
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::vector<char> blueArray({ '2', '4', '6' });
		std::vector<char> whiteArray({ '1', '3', '5', '7' });
		std::shuffle(blueArray.begin(), blueArray.end(), std::default_random_engine(seed));
		std::shuffle(whiteArray.begin(), whiteArray.end(), std::default_random_engine(seed));
		std::string inputOrder = laneOrder.str();
		std::stringstream resultingOrder;

		for (int i = 0; i < std::size(unrandomizer.mLaneOrderL); i++) {
			char columnVal;
			if (inputOrder[i] % 2 == 0) {
				columnVal = *blueArray.begin();
				blueArray.erase(blueArray.begin());
			}
			else {
				columnVal = *whiteArray.begin();
				whiteArray.erase(whiteArray.begin());
			}
			resultingOrder << columnVal;
		}
		laneOrder.swap(resultingOrder);
	}
	uintptr_t unrandomseed = GetSeedMap(std::atoi(laneOrder.str().c_str()));
	if (unrandomseed == 0xFFFF) return;
	*randomseed = GetSeedMap(std::atoi(laneOrder.str().c_str()));
}

static std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

static std::string ws2utf(const std::wstring& str) {
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	std::string converted_str = converter.to_bytes(str);
	return converted_str;
}

void Unrandomizer::OnAfterPopulateNoteMapping(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.random[0] != 2 || regs.esi != 0x0F || game.gameplay.keymode != 7) return;

	int* noteMapping = (int*)(regs.esp + 0x1B0);
	char noteOrder[8];
	for (int i = 1; i < 8; i++) {
		noteOrder[noteMapping[i] - 1] = i + '0';
	}
	noteOrder[7] = 0;
	std::string name = ws2utf(s2ws(game.sSelect.metaSelected.title.body) + s2ws(game.sSelect.metaSelected.subtitle.body));
	Unrandomizer::RandomHistoryEntry entry(name, noteOrder);
	Unrandomizer& unrandomizer = *(Unrandomizer*)(LR2HackBox::Get().mUnrandomizer);
	unrandomizer.AddToHistory(entry);
	if (unrandomizer.mIsTrackRandom) unrandomizer.SetOrder(entry.GetRandom().c_str());
}

bool Unrandomizer::Init(uintptr_t moduleBase) {
	Unrandomizer::mModuleBase = moduleBase;

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0xB07DB), OnSetRandomSeed));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0xB483B), OnAfterPopulateNoteMapping));

	return true;
}

bool Unrandomizer::Deinit() {
	return true;
}

bool Unrandomizer::GetEnabled() {
	return mIsEnabled;
}

void Unrandomizer::SetEnabled(bool value) {
	mIsEnabled = value;
}

void Unrandomizer::ToggleEnabled() {
	mIsEnabled = !mIsEnabled;
}

bool Unrandomizer::GetBWPermute() {
	return mIsBWPermute;
}

void Unrandomizer::SetBWPermute(bool value) {
	mIsBWPermute = value;
}

void Unrandomizer::ToggleBWPermute() {
	mIsBWPermute = !mIsBWPermute;
}

static void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void Unrandomizer::RandomHistoryDisplay() {
	if (ImGui::TreeNode("Random History")) {
		ImGui::SameLine();
		HelpMarker("Double click the contents of a row to select it as the current random");
		int flags = ImGuiTableFlags(ImGuiTableFlags_ScrollY) | ImGuiTableFlags(ImGuiTableFlags_RowBg)
			| ImGuiTableFlags(ImGuiTableFlags_BordersOuter) | ImGuiTableFlags(ImGuiTableFlags_Resizable)
			| ImGuiTableFlags(ImGuiTableFlags_SizingStretchSame);

		float outer_size = ImGui::GetTextLineHeightWithSpacing() * 8;
		if (ImGui::BeginTable("RanTrainerLaneOrderHistory", 2, flags, ImVec2(0, outer_size))) {

			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Song Title");
			ImGui::TableSetupColumn("Random");
			ImGui::TableHeadersRow();

			const std::deque<RandomHistoryEntry> history = GetRandomHistory();
			for (auto entry : history) {
				ImGui::TableNextRow();
				for (int col = 0; col < 2; col++) {
					ImGui::TableSetColumnIndex(col);
					if (col % 2 == 0) {
						ImGui::Text(entry.GetTitle().c_str());
					}
					else {
						ImGui::Text(entry.GetRandom().c_str());
					}
					if (ImGui::IsItemHovered()) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget(ImGuiTableBgTarget_CellBg), IM_COL32(110, 90, 20, 255));
						if (ImGui::IsMouseDoubleClicked(0)) {
							SetOrder(entry.GetRandom().c_str());
						}
					}
				}
			}
			ImGui::EndTable();
		}

		ImGui::TreePop();
	}
}


void Unrandomizer::DragAndDropKeyDisplay(UnrandomizerState state) {
	ImGui::Text("Random Select");
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(196, 196, 196, 255));
	ImGui::Text("(drag and drop to reorder lanes)");
	ImGui::PopStyleColor();
	if (ImGui::Button("Mirror")) {
		MirrorOrder();
	}
	int sideCount = state == UnrandomizerState_DP ? 2 : 1;
	for (int side = 0; side < sideCount; side++) {
		uint32_t* laneOrder;
		std::string dragDropRefName;
		if (state == UnrandomizerState_DP)
		{
			if (side == 0) {
				laneOrder = mLaneOrderL;
				dragDropRefName = "RT_LANE_MEMBER_L";
			}
			else {
				laneOrder = mLaneOrderR;
				dragDropRefName = "RT_LANE_MEMBER_R";
			}
		}
		else {
			if (state == UnrandomizerState_P1) {
				laneOrder = mLaneOrderL;
				dragDropRefName = "RT_LANE_MEMBER_L";
			}
			else {
				laneOrder = mLaneOrderR;
				dragDropRefName = "RT_LANE_MEMBER_R";
			}
		}
		ImGui::NewLine();
		for (int i = 0; i < std::size(mLaneOrderL); i++) {
			ImGui::PushID(i + std::size(mLaneOrderL) * side);
			ImGui::SameLine();
			if (laneOrder[i] % 2 == 0) {
				ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(0, 0, 139, 255));
				ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(230, 230, 230, 255));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(230, 230, 230, 255));
				ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(49, 49, 49, 255));
			}
			if (mIsBWPermute) {
				ImGui::Button("", ImVec2(50, 80));
			}
			else {
				ImGui::Button(std::to_string(laneOrder[i]).c_str(), ImVec2(50, 80));
			}

			ImGui::PopStyleColor(2);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags(ImGuiDragDropFlags_None))) {
				
				ImGui::SetDragDropPayload(dragDropRefName.c_str(), &i, sizeof(int));
				ImGui::EndDragDropSource();
			}
			if (ImGui::BeginDragDropTarget()) {
				if (ImGui::AcceptDragDropPayload(dragDropRefName.c_str(), ImGuiDragDropFlags_None) != nullptr) {
					int payload_i = *(int*)ImGui::AcceptDragDropPayload(dragDropRefName.c_str())->Data;
					std::swap(laneOrder[i], laneOrder[payload_i]);
				}

				ImGui::EndDragDropTarget();
			}
			ImGui::PopID();
		}
		std::stringstream laneOrderString;
		for (int i = 0; i < std::size(mLaneOrderL); i++) {
			laneOrderString << mLaneOrderL[i];
		}
		mLaneOrderNumL = std::atoi(laneOrderString.str().c_str());
	}
}

void Unrandomizer::SetOrder(const char* arrange) {
	for (int i = 0; i < std::size(mLaneOrderL); i++) {
		mLaneOrderL[i] = arrange[i] - '0';
	}
}

void Unrandomizer::MirrorOrder() {
	std::reverse(std::begin(mLaneOrderL), std::end(mLaneOrderL)); 
}

void Unrandomizer::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();
	if (game->sSelect.metaSelected.keymode != 0 && game->sSelect.metaSelected.keymode != 7) {
		ImGui::Text("Only 7K mode is currently implemented.");
		mIsEnabled = false;
		return;
	}
	UnrandomizerState unrandomizerState;
	/*if (!game->gameplay.mode) {
		unrandomizerState = state.isP1 ? UnrandomizerState_P1 : UnrandomizerState_P2;
	}
	else {
		unrandomizerState = UnrandomizerState_DP;
	}*/
	unrandomizerState = UnrandomizerState_P1;
	DragAndDropKeyDisplay(unrandomizerState);

	RandomHistoryDisplay();

	ImGui::Text("Controls");

	ImGui::Indent();
	if (GetSeedMap(mLaneOrderNumL) == 0xFFFF) {
		ImGui::Text("This arrange is missing from the seed map...");
		mIsEnabled = false;
	}
	else ImGui::Checkbox("Trainer Enabled", &mIsEnabled);
	ImGui::SameLine();
	HelpMarker("When enabled the RANDOM play option will produce the selected random until disabled.\n\nThe selected random can be changed and the trainer toggled on or off between quick retries without needing to return to song select");
	ImGui::Checkbox("Track Current Random", &mIsTrackRandom);
	ImGui::SameLine();
	HelpMarker("While the trainer is disabled this option will update the key display to reflect the current random");
	ImGui::Checkbox("Black/White Random Select", &mIsBWPermute);
	ImGui::Unindent();
}

const std::deque<Unrandomizer::RandomHistoryEntry>& Unrandomizer::GetRandomHistory() {
	return mRandomHistory;
}
void Unrandomizer::AddToHistory(RandomHistoryEntry entry) {
	mRandomHistory.push_front(entry);
}

Unrandomizer::RandomHistoryEntry::RandomHistoryEntry(std::string title, std::string random) {
	mTitle = title;
	mRandom = random;
}
std::string Unrandomizer::RandomHistoryEntry::GetTitle() {
	return mTitle;
}
std::string Unrandomizer::RandomHistoryEntry::GetRandom() {
	return mRandom;
}