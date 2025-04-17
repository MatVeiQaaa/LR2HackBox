#include "LR2HackBox/LR2HackBox.hpp"

#include "Helpers/Helpers.hpp"
#include "ImGuiInjector/ImGuiInjector.hpp"
#include "imgui/imgui.h"
#include "minhook/include/MinHook.h"

#include <iostream>
#include <string>

#include "Features/Unrandomizer.hpp"
#include "Features/Funny.hpp"
#include "Features/Misc.hpp"

#ifndef NDEBUG
#include "Features/MemoryTracker.hpp")
#endif

#pragma comment(lib, "Helpers.lib")
#pragma comment(lib, "ImGuiInjector.lib")
#pragma comment(lib, "BaseModels.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

LR2HackBox& LR2HackBox::Get() {
	static LR2HackBox instance;
	return instance;
}

bool LR2HackBox::Hook() {
	mLogger.SetPath("./LR2HackBox.log");

	MH_Initialize();

	IFDEBUG(mMemoryTracker = new MemoryTracker());
	IFDEBUG(mMemoryTracker->Init(mModuleBase));

	LR2::Init();
	while (!LR2::isInit) Sleep(1);

	ImGuiInjector::Get().AddMenu(&(LR2HackBox::mMenu));
	
	mModuleBase = (uintptr_t)GetModuleHandle(NULL);

	mConfig = new ConfigManager("LR2HackBox.ini");

	mMenu.InitBindings();

	mUnrandomizer = new Unrandomizer();
	mFunny = new Funny();
	mMisc = new Misc();

	mUnrandomizer->Init(mModuleBase);
	mFunny->Init(mModuleBase);
	mMisc->Init(mModuleBase);

	return true;
}

bool LR2HackBox::Unhook() {
	mUnrandomizer->Deinit();
	mFunny->Deinit();
	mMisc->Deinit();
	IFDEBUG(mMemoryTracker->Deinit());
	delete(mConfig);
	delete(mUnrandomizer);
	delete(mFunny);
	delete(mMisc);
	IFDEBUG(delete(mMemoryTracker));
	return true;
}

LR2::game* LR2HackBox::GetGame() {
	if (!LR2::isInit) return nullptr;
	return (LR2::game*)LR2::pGame;
}

void LR2HackBoxMenu::Loop() {
	ImGui::Begin("LR2HackBox", &(LR2HackBoxMenu::mIsOpen));

	ImGui::Text("LR2HackBox Menu");

	if (ImGui::CollapsingHeader("Unrandomizer")) {
		((Unrandomizer*)LR2HackBox::Get().mUnrandomizer)->Menu();
	}

	if (ImGui::CollapsingHeader("Miscellaneous")) {
		((Misc*)LR2HackBox::Get().mMisc)->Menu();
	}

	if (ImGui::CollapsingHeader("Funny")) {
		((Funny*)LR2HackBox::Get().mFunny)->Menu();
	}

	IFDEBUG(
		if (ImGui::CollapsingHeader("MemoryTracker")) {
			((MemoryTracker*)LR2HackBox::Get().mMemoryTracker)->Menu();
		}
	)

	ImVec2 oldCursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(-10000.f, -10000.f));
	ImGui::Button("Binds");
	ImGui::SetCursorPos(oldCursorPos);
	ImVec2 size = ImGui::GetItemRectSize();
	float cursorOffset = ImGui::GetCursorPosY() - ImGui::GetWindowHeight();
	ImVec2 pos;
	if (cursorOffset > 0) {
		pos = { ImGui::GetWindowWidth() - 5.f , ImGui::GetWindowHeight() + cursorOffset + size[1] + 12.f };
	}
	else {
		pos = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
	}
	ImGui::SetCursorPos(ImVec2(pos[0] - size[0] - 10.f, pos[1] - size[1] - 10.f));
	if (ImGui::Button("Binds")) {
		mIsRebindMenu = true;
	}
	BindingsMenu();

	ImGui::End();
}

void LR2HackBoxMenu::BindingsMenu() {
	if (!mIsRebindMenu) return;

	ImGui::Begin("Rebind Buttons", &mIsRebindMenu);
	ImGui::SetWindowFocus();
	for (auto& binding : mMenuBindings) {
		ImGui::Text(binding.first.c_str());
		ImGui::SameLine();
		std::string keycodeName;
		WORD vkeyChar = LOWORD(MapVirtualKey(binding.second, MAPVK_VK_TO_CHAR));
		if (vkeyChar) {
			keycodeName = vkeyChar;
		}
		else {
			keycodeName = std::string("VK_") + std::to_string(binding.second);
		}

		if (mMenuBindingAwaitsRebind.first == binding.first) {
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(0, 0, 210, 255));
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(230, 230, 230, 255));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Button), IM_COL32(0, 0, 139, 255));
			ImGui::PushStyleColor(ImGuiCol(ImGuiCol_Text), IM_COL32(230, 230, 230, 255));
		}
		if (ImGui::Button(mMenuBindingAwaitsRebind.first == binding.first ? "?" : keycodeName.c_str(), ImVec2(46, 23))) {
			mMenuBindingAwaitsRebind = { binding.first, true };
		}
		ImGui::PopStyleColor(2);
	}
	if (mMenuBindingAwaitsRebind.second) ImGui::SetNextFrameWantCaptureKeyboard(true);

	ImGui::End();
}

void LR2HackBoxMenu::InitBindings() {
	ConfigManager& config = *LR2HackBox::Get().mConfig;
	if (!config.ValueExists("iBindMenuOpen")) {
		config.WriteValue("iBindMenuOpen", std::to_string(VK_INSERT));
		config.SaveConfig();
	}
	mMenuBindings["Menu Open"] = std::stoi(config.ReadValue("iBindMenuOpen"));
}

void LR2HackBoxMenu::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == mMenuBindings["Menu Open"]) {
			LR2HackBoxMenu::ToggleOpen();
		}

		if (mMenuBindingAwaitsRebind.second) {
			mMenuBindings[mMenuBindingAwaitsRebind.first] = wParam;

			ConfigManager& config = *LR2HackBox::Get().mConfig;
			std::string configEntry = mMenuBindingAwaitsRebind.first;
			configEntry.erase(std::remove_if(configEntry.begin(), configEntry.end(), isspace), configEntry.end());
			configEntry = "iBind" + configEntry;
			config.WriteValue(configEntry, std::to_string(wParam));
			config.SaveConfig();

			mMenuBindingAwaitsRebind = { "NONE", false };
		}
		break;
	}
	}
}