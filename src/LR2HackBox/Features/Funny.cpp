#include "Funny.hpp"

#include <iostream>
#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

#include "safetyhook/safetyhook.hpp"
#include "imgui/imgui.h"

#pragma comment(lib, "libSafetyhook.lib")

void Funny::OnDrawNote(SafetyHookContext& regs) {
	Funny& funny = *(Funny*)(LR2HackBox::Get().mFunny);
	if (!funny.mIsInvisibleScratch) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.gameplay.keymode != 7) return;

	if (regs.esi == 0) regs.esi++;
}

bool Funny::Init(uintptr_t moduleBase) {
	Funny::mModuleBase = moduleBase;

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x7098), OnDrawNote));

	return true;
}

bool Funny::Deinit() {
	return true;
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

void Funny::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Indent();

	ImGui::Checkbox("Invisible Scratch", &mIsInvisibleScratch);
	ImGui::SameLine();
	HelpMarker("When enabled, will stop scratch notes from rendering.");

	if (ImGui::Checkbox("Metronome", &mIsMetronome)) {
		((Misc*)LR2HackBox::Get().mMisc)->SetMetronome(mIsMetronome);
	}
	ImGui::SameLine();
	HelpMarker("Enables the metronome sound in playing.\n  Expects 'metronome-measure.wav' and 'metronome-beat.wav'\n  in LR2files\\Sound\\LR2HackBox\\ directory\n  or defaults to using f-open and f-close");

	ImGui::Unindent();
}