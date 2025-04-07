#include "AprilFools.hpp"

#include <iostream>
#include "LR2HackBox/LR2HackBox.hpp"

#include "safetyhook/safetyhook.hpp"
#include "imgui/imgui.h"

#pragma comment(lib, "libSafetyhook.lib")

void AprilFools::OnDrawNote(SafetyHookContext& regs) {
	AprilFools& aprilFools = *(AprilFools*)(LR2HackBox::Get().mAprilFools);
	if (!aprilFools.GetEnabled()) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.gameplay.keymode != 7) return;

	if (regs.esi == 0) regs.esi++;
}

bool AprilFools::Init(uintptr_t moduleBase) {
	AprilFools::mModuleBase = moduleBase;

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x7098), OnDrawNote));

	return true;
}

bool AprilFools::Deinit() {
	return true;
}

bool AprilFools::GetEnabled() {
	return mIsEnabled;
}

void AprilFools::SetEnabled(bool value) {
	mIsEnabled = value;
}

void AprilFools::ToggleEnabled() {
	mIsEnabled = !mIsEnabled;
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

void AprilFools::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Indent();

	ImGui::Checkbox("Invisible Scratch", &mIsEnabled);
	ImGui::SameLine();
	HelpMarker("When enabled, will stop scratch notes from rendering.");

	ImGui::Unindent();
}