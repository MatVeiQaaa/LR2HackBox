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

	ImGui::End();
}

void LR2HackBoxMenu::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN: {
		switch (wParam) {
			case VK_INSERT: {
				LR2HackBoxMenu::ToggleOpen();
				break;
			}
		}
		break;
	}
	}
}