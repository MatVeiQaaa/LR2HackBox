#define NOMINMAX
#include "AnalogInput.hpp"

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "LR2HackBox/LR2HackBox.hpp"

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include "Helpers/Helpers.hpp"

#include "imgui/imgui.h"
#include "safetyhook/safetyhook.hpp"

#pragma comment(lib, "libSafetyhook.lib")

typedef double(__cdecl* tGetTimeWrap)();
tGetTimeWrap GetTimeWrap = (tGetTimeWrap)0x4B6890;

typedef HRESULT(__stdcall* tGetDeviceState)(IDirectInputDevice7* pThis, DWORD cbData, LPVOID lpvData);
tGetDeviceState GetDeviceState = nullptr;
HRESULT __stdcall AnalogInput::OnGetDeviceState(void* pThis, DWORD cbData, LPVOID lpvData) {
    AnalogInput& analogInput = *(AnalogInput*)(LR2HackBox::Get().mAnalogInput);

    IDirectInputDevice7& device = *(IDirectInputDevice7*)pThis;
    
    HRESULT result = analogInput.oGetDeviceState.stdcall<HRESULT>(pThis, cbData, lpvData);

    if (result != DI_OK) return result;
    if (cbData != sizeof(DIJOYSTATE)) return result;
    DIJOYSTATE& state = *(DIJOYSTATE*)lpvData;

    if (!analogInput.devicesMap.contains(pThis)) {
        DIDEVICEINSTANCE ddinst;
        ddinst.dwSize = sizeof(DIDEVICEINSTANCE);
        device.GetDeviceInfo(&ddinst);
        DIPROPDWORD prop;
        prop.diph.dwSize = sizeof(DIPROPDWORD);
        prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        prop.diph.dwObj = 0; // device property 
        prop.diph.dwHow = DIPH_DEVICE;
        prop.dwData = 0;
        HRESULT result = device.SetProperty(DIPROP_DEADZONE, &prop.diph);
        analogInput.devicesMap[pThis] = ddinst.tszProductName;
        analogInput.devices.push_back(pThis);
        analogInput.deviceNames.push_back(analogInput.devicesMap[pThis].c_str());
        for (int i = 0; i < std::size(analogInput.boundDevices); i++) {
            std::string sideNotation = "P" + std::to_string(i + 1);
            std::string savedDevice = LR2HackBox::Get().mConfig->ReadValue("sAnalogInputDevice" + sideNotation);
            if (savedDevice == ddinst.tszProductName) {
                analogInput.boundDevices[i].pDevice = pThis;
                analogInput.boundDevices[i].name = savedDevice;
                analogInput.boundDevices[i].boundDeviceIdx = analogInput.devices.size() - 1;
            }
        }
        
    }
    if (!analogInput.mIsEnabled) return result;
    for (int i = 0; i < std::size(analogInput.boundDevices); i++) {
        std::string sideNotation = "P" + std::to_string(i + 1);
        if (!analogInput.boundDevices[i].enabled) continue;
        if (pThis != analogInput.boundDevices[i].pDevice) continue;

        InputDevice& boundDevice = analogInput.boundDevices[i];

        int value = 0;
        double time = GetTimeWrap();
        switch (boundDevice.axis) {
        case X:
            value = state.lX;
            break;
        case Y:
            value = state.lY;
            break;
        case Z:
            value = state.lZ;
            break;
        case NONE:
            continue;
        }
        if (boundDevice.prevValue == MININT) {
            boundDevice.prevValue = value;
            continue;
        }

        if (boundDevice.upState == 1) boundDevice.upState = 2;
        if (boundDevice.downState == 1) boundDevice.downState = 2;

        if (boundDevice.timeout) boundDevice.timeout--;
        else {
            if (boundDevice.upState == 3) boundDevice.upState = 0;
            else if (boundDevice.upState != 0) boundDevice.upState = 3;

            if (boundDevice.downState == 3) boundDevice.downState = 0;
            else if (boundDevice.downState != 0) boundDevice.downState = 3;
        }

        if (value != boundDevice.prevValue) {
            int curDelta = value - boundDevice.prevValue;
            if (curDelta > 1000) {
                curDelta = value + boundDevice.prevValue + 1;
            }
            if (curDelta < -1000) {
                curDelta = value + boundDevice.prevValue + 1;
            }
            boundDevice.prevValue = value;

            if (boundDevice.axisInverted) curDelta = -curDelta;

            boundDevice.delta += curDelta;

            if (curDelta > 0) {
                if (boundDevice.delta >= boundDevice.threshold) {
                    boundDevice.delta = 0;
                    boundDevice.timeout = boundDevice.timeoutDuration;
                    if (boundDevice.upState == 1) boundDevice.upState = 2;
                    else if (boundDevice.upState != 2) boundDevice.upState = 1;

                    if (boundDevice.downState == 3) boundDevice.downState = 0;
                    else if (boundDevice.downState != 0) boundDevice.downState = 3;
                }
            }
            else if (curDelta < 0) {
                if (-boundDevice.delta >= boundDevice.threshold) {
                    boundDevice.delta = 0;
                    boundDevice.timeout = boundDevice.timeoutDuration;
                    if (boundDevice.downState == 1) boundDevice.downState = 2;
                    else if (boundDevice.downState != 2) boundDevice.downState = 1;

                    if (boundDevice.upState == 3) boundDevice.upState = 0;
                    else if (boundDevice.upState != 0) boundDevice.upState = 3;
                }
            }
        }
    }
    
    return result;
}

typedef int(__cdecl* tInputToButton)(LR2::inputStructure* is, LR2::CONFIG_INPUT* cfg_input, int player, int isReplay);
tInputToButton InputToButton = (tInputToButton)0x4BF020;
int __cdecl AnalogInput::OnInputToButton(void* is, void* cfg_input, int player, int isReplay) {
    AnalogInput& analogInput = *(AnalogInput*)(LR2HackBox::Get().mAnalogInput);
    int result = analogInput.oInputToButton.ccall<int>(is, cfg_input, player, isReplay);
    if (!analogInput.mIsEnabled) return result;

    LR2::inputStructure& inputStructure = *(LR2::inputStructure*)is;
    
    if (isReplay == 0) {
        if (player == 0) {
            unsigned char& upP1 = inputStructure.p1_buttonInput[10];
            if (upP1 < analogInput.boundDevices[0].upState) upP1 = analogInput.boundDevices[0].upState;

            unsigned char& downP1 = inputStructure.p1_buttonInput[11];
            if (downP1 < analogInput.boundDevices[0].downState) downP1 = analogInput.boundDevices[0].downState;

            unsigned char& upP2 = inputStructure.p2_buttonInput[10];
            if (upP2 < analogInput.boundDevices[1].upState) upP2 = analogInput.boundDevices[1].upState;

            unsigned char& downP2 = inputStructure.p2_buttonInput[11];
            if (downP2 < analogInput.boundDevices[1].downState) downP2 = analogInput.boundDevices[1].downState;
        }
        else if (player == 1) {
            unsigned char& upP1 = inputStructure.p1_buttonInput[10];
            if (upP1 < analogInput.boundDevices[0].upState) upP1 = analogInput.boundDevices[0].upState;

            unsigned char& downP1 = inputStructure.p1_buttonInput[11];
            if (downP1 < analogInput.boundDevices[0].downState) downP1 = analogInput.boundDevices[0].downState;

            unsigned char& upP2 = inputStructure.p1_buttonInput[10];
            if (upP2 < analogInput.boundDevices[1].upState) upP2 = analogInput.boundDevices[1].upState;

            unsigned char& downP2 = inputStructure.p1_buttonInput[11];
            if (downP2 < analogInput.boundDevices[1].downState) downP2 = analogInput.boundDevices[1].downState;
        }
    }

    if (inputStructure.p1_buttonInput[10] == 1 || inputStructure.p1_buttonInput[11] == 1) {
        inputStructure.p1_buttonInput[0] = 1;
    }
    else if (inputStructure.p1_buttonInput[10] == 2 || inputStructure.p1_buttonInput[11] == 2) {
        inputStructure.p1_buttonInput[0] = 2;
    }
    else if (inputStructure.p1_buttonInput[10] == 3 || inputStructure.p1_buttonInput[11] == 3) {
        inputStructure.p1_buttonInput[0] = 3;
    }

    if (inputStructure.p2_buttonInput[10] == 1 || inputStructure.p2_buttonInput[11] == 1) {
        inputStructure.p2_buttonInput[0] = 1;
    }
    else if (inputStructure.p2_buttonInput[10] == 2 || inputStructure.p2_buttonInput[11] == 2) {
        inputStructure.p2_buttonInput[0] = 2;
    }
    else if (inputStructure.p2_buttonInput[10] == 3 || inputStructure.p2_buttonInput[11] == 3) {
        inputStructure.p2_buttonInput[0] = 3;
    }

    return result;
}

bool AnalogInput::Init(uintptr_t moduleBase) {
	AnalogInput::mModuleBase = moduleBase;

    IDirectInput7* pDirectInput = NULL;
    typedef HRESULT(__stdcall* tDirectInputCreateEx)(HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN punkOuter);
    tDirectInputCreateEx DirectInputCreateEx = (tDirectInputCreateEx)GetProcAddress(GetModuleHandle("dinput.dll"), "DirectInputCreateEx");
    GUID IID_IDirectInput7A = { 0x9A4CB684,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE };
    if (DirectInputCreateEx(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput7A, (LPVOID*)&pDirectInput, NULL) != DI_OK) {
        std::cout << "DirectInputCreateEx failed" << std::endl;
        return false;
    }

    LPDIRECTINPUTDEVICE7 lpdiMouse;
    GUID GUID_SysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
    GUID IID_IDirectInputDevice7A = { 0x57D7C6BC,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE };
    if (pDirectInput->CreateDeviceEx(GUID_SysMouse, IID_IDirectInputDevice7A, (LPVOID*)&lpdiMouse, NULL) != DI_OK) {
        pDirectInput->Release();
        std::cout << "Error creating DirectInput device" << std::endl;
        return false;
    }

    uintptr_t vTable = mem::FindDMAAddy((uintptr_t)lpdiMouse, { 0x0 });

    GetDeviceState = (tGetDeviceState)(((char**)vTable)[9]);

    oGetDeviceState = safetyhook::create_inline(GetDeviceState, OnGetDeviceState);

    lpdiMouse->Release();
    pDirectInput->Release();

    oInputToButton = safetyhook::create_inline(InputToButton, OnInputToButton);

    for (int i = 0; i < std::size(boundDevices); i++) {
        std::string sideNotation = "P" + std::to_string(i + 1);
        boundDevices[i].enabled = LR2HackBox::Get().mConfig->ReadValue("bAnalogInputEnabled" + sideNotation) == "true" ? true : false;

        try {
            int savedAxis = std::stoi(LR2HackBox::Get().mConfig->ReadValue("iAnalogInputAxis" + sideNotation));
            boundDevices[i].axis = AnalogInput::Axis(savedAxis);
            boundDevices[i].boundAxisIdx = savedAxis;
        }
        catch (...) {}

        boundDevices[i].axisInverted = LR2HackBox::Get().mConfig->ReadValue("bAnalogInputAxisInvert" + sideNotation) == "true" ? true : false;

        try {
            int timeoutDuration = std::stoi(LR2HackBox::Get().mConfig->ReadValue("iAnalogTimeoutDuration" + sideNotation));
            boundDevices[i].timeoutDuration = timeoutDuration;
        }
        catch (...) {}

        try {
            int threshold = std::stoi(LR2HackBox::Get().mConfig->ReadValue("iAnalogThreshold" + sideNotation));
            boundDevices[i].threshold = threshold;
        }
        catch (...) {}
    }

	return true;
}

bool AnalogInput::Deinit() {
	return true;
}

void AnalogInput::SetEnabled(bool value) {
    mIsEnabled = value;
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

const char* const axisNames[3]{
    "X",
    "Y",
    "Z"
};

void AnalogInput::Menu() {
    for (int i = 0; i < std::size(boundDevices); i++) {
        std::string sideNotation = "P" + std::to_string(i + 1);
        std::string treeText = "Analog Input Settings " + sideNotation;
        if (ImGui::TreeNode(treeText.c_str())) {
            std::string enabledText = "Enable##" + sideNotation;
            if (ImGui::Checkbox(enabledText.c_str(), &boundDevices[i].enabled)) {
                LR2HackBox::Get().mConfig->WriteValue("bAnalogInputEnabled" + sideNotation, boundDevices[i].enabled ? "true" : "false");
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            float boxWidth = 150.f;
            std::string selectedDeviceText = "Selected Input Device##" + sideNotation;
            ImGui::SetNextItemWidth(boxWidth);
            if (ImGui::Combo(selectedDeviceText.c_str(), &boundDevices[i].boundDeviceIdx, deviceNames.data(), deviceNames.size())) {
                boundDevices[i].pDevice = devices[boundDevices[i].boundDeviceIdx];
                boundDevices[i].name = deviceNames[boundDevices[i].boundDeviceIdx];
                boundDevices[i].Reset();

                LR2HackBox::Get().mConfig->WriteValue("sAnalogInputDevice" + sideNotation, boundDevices[i].name);
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            std::string axisText = "Axis to Use##" + sideNotation;
            ImGui::SetNextItemWidth(boxWidth);
            if (ImGui::Combo(axisText.c_str(), &boundDevices[i].boundAxisIdx, axisNames, std::size(axisNames))) {
                boundDevices[i].axis = AnalogInput::Axis(boundDevices[i].boundAxisIdx);
                boundDevices[i].Reset();

                LR2HackBox::Get().mConfig->WriteValue("iAnalogInputAxis" + sideNotation, std::to_string(boundDevices[i].boundAxisIdx));
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            std::string invertAxisText = "Invert Axis##" + sideNotation;
            if (ImGui::Checkbox(invertAxisText.c_str(), &boundDevices[i].axisInverted)) {
                LR2HackBox::Get().mConfig->WriteValue("bAnalogInputAxisInvert" + sideNotation, boundDevices[i].axisInverted ? "true" : "false");
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            std::string timeoutText = "Hold Timeout Duration##" + sideNotation;
            ImGui::SetNextItemWidth(boxWidth);
            if (ImGui::InputInt(timeoutText.c_str(), &boundDevices[i].timeoutDuration)) {
                LR2HackBox::Get().mConfig->WriteValue("iAnalogTimeoutDuration" + sideNotation, std::to_string(boundDevices[i].timeoutDuration));
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            std::string thresholdText = "Activation Threshold##" + sideNotation;
            ImGui::SetNextItemWidth(boxWidth);
            if (ImGui::InputInt(thresholdText.c_str(), &boundDevices[i].threshold)) {
                LR2HackBox::Get().mConfig->WriteValue("iAnalogThreshold" + sideNotation, std::to_string(boundDevices[i].threshold));
                LR2HackBox::Get().mConfig->SaveConfig();
            }
            ImGui::TreePop();
        }
    }
}