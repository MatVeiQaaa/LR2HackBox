#include "LR2HackBox/LR2HackBox.hpp"

#include "Helpers/Helpers.hpp"
#include "ImGuiInjector/ImGuiInjector.hpp"
#include "imgui/imgui.h"

#include <iostream>
#include <string>

#include "Features/Unrandomizer.hpp"
#include "Features/AprilFools.hpp"
#include "Features/Misc.hpp"

#pragma comment(lib, "Helpers.lib")
#pragma comment(lib, "ImGuiInjector.lib")
#pragma comment(lib, "BaseModels.lib")

LR2HackBox& LR2HackBox::Get() {
	static LR2HackBox instance;
	return instance;
}

bool LR2HackBox::Hook() {
	mLogger.SetPath("./LR2HackBox.log");

	LR2::Init();
	while (!LR2::isInit) Sleep(1);

	ImGuiInjector::Get().AddMenu(&(LR2HackBox::mMenu));
	
	mModuleBase = (uintptr_t)GetModuleHandle(NULL);

	mConfig = new ConfigManager("LR2HackBox.ini");

	mUnrandomizer = new Unrandomizer();
	mAprilFools = new AprilFools();
	mMisc = new Misc();

	mUnrandomizer->Init(mModuleBase);
	mAprilFools->Init(mModuleBase);
	mMisc->Init(mModuleBase);

	return true;
}

bool LR2HackBox::Unhook() {
	mUnrandomizer->Deinit();
	mAprilFools->Deinit();
	mMisc->Deinit();
	delete(mConfig);
	delete(mUnrandomizer);
	delete(mAprilFools);
	delete(mMisc);
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
		((AprilFools*)LR2HackBox::Get().mAprilFools)->Menu();
	}

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