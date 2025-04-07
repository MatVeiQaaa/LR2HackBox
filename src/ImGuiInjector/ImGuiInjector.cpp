#include "ImGuiInjector/ImGuiInjector.hpp"

#include <iostream>
#include <windows.h>

#include "Helpers/Helpers.hpp"
#include "imgui/imgui.h"
#include "kiero/kiero.h"
#include "minhook/include/MinHook.h"

#pragma comment (lib, "BaseModels.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

#if KIERO_INCLUDE_D3D9
# include "kiero/examples/imgui/impl/d3d9_impl.h"
#endif

#if KIERO_INCLUDE_D3D10
# include "kiero/examples/imgui/impl/d3d10_impl.h"
#endif

#if KIERO_INCLUDE_D3D11
# include "kiero/examples/imgui/impl/d3d11_impl.h"
#endif

#if KIERO_INCLUDE_D3D12
#endif

#if KIERO_INCLUDE_OPENGL
#endif

#if KIERO_INCLUDE_VULKAN
#endif

#if !KIERO_USE_MINHOOK
# error "The example requires that minhook be enabled!"
#endif

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static UINT lastMsg = 0;
const static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
   /* if (lastMsg != msg) {
        std::cout << "Message: " << msg << std::endl;
        lastMsg = msg;
    }*/
    if (ImGuiInjector::Get().WndProcHandler(hWnd, msg, wParam, lParam)) {
    	return true;
    }
    return CallWindowProc(ImGuiInjector::Get().GetPreviousWndProc(), hWnd, msg, wParam, lParam);
};

ImGuiInjector& ImGuiInjector::Get() {
    static ImGuiInjector instance;
    return instance;
}

ImGuiInjector::ImGuiInjector() {
    ImGuiInjector::mMenus.clear();
    ImGuiInjector::Init();
}

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

typedef HRESULT(__stdcall* tGetDeviceState)(IDirectInputDevice7* pThis, DWORD cbData, LPVOID lpvData);
tGetDeviceState GetDeviceState = nullptr;
HRESULT __stdcall OnGetDeviceState(IDirectInputDevice7* pThis, DWORD cbData, LPVOID lpvData) {
    HRESULT result = GetDeviceState(pThis, cbData, lpvData);
    if (result == DI_OK) {
        if (ImGuiInjector::Get().WantsMouseInput() && cbData == sizeof(DIMOUSESTATE2)) { // Mouse device
            ((LPDIMOUSESTATE2)lpvData)->rgbButtons[0] = 0;
            ((LPDIMOUSESTATE2)lpvData)->rgbButtons[1] = 0;
        }
        else if (ImGuiInjector::Get().WantsKeyboardInput() && cbData == 256) { // Keyboard device
            memset(lpvData, 0, 256);
        }
        //else if (cbData == sizeof(DIJOYSTATE)) { // Controller device
        //    memset(lpvData, 0, cbData);
        //}
    }
    return result;
}

bool HookDinput7(HMODULE hModule) {
    IDirectInput7* pDirectInput = NULL;
    typedef HRESULT(__stdcall* tDirectInputCreateEx)(HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN punkOuter);
    tDirectInputCreateEx DirectInputCreateEx = (tDirectInputCreateEx)GetProcAddress(hModule, "DirectInputCreateEx");
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

    if (MH_CreateHookEx((LPVOID)GetDeviceState, &OnGetDeviceState, &GetDeviceState) != MH_OK)
    {
        std::cout << "Couldn't hook GetDeviceState" << std::endl;
        return false;
    }

    if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
    {
        std::cout << ("Couldn't enable dinput hooks");
        return 1;
    }

    lpdiMouse->Release();
    pDirectInput->Release();

    return true;
}

void ImGuiInjector::HookDinput() {
    HMODULE dinputHandle7 = GetModuleHandle(L"dinput.dll");
    if (dinputHandle7) mDinputVer = 7;
    HMODULE dinputHandle8 = GetModuleHandle(L"dinput8.dll");
    if (dinputHandle8) mDinputVer = 8;

    switch (mDinputVer) {
    case 7: {
        HookDinput7(dinputHandle7);
        break;
    }
    case 8: std::cout << "Dinput8 hooking is unimplemented" << std::endl; break;
    default: break;
    }
}

int ImGuiInjector::Init() {
    if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success)
    {
        switch (kiero::getRenderType())
        {
#if KIERO_INCLUDE_D3D9
        case kiero::RenderType::D3D9:
            std::cout << "Kiero DX9 initialised\n";
            impl::d3d9::init();
            break;
#endif
#if KIERO_INCLUDE_D3D10
        case kiero::RenderType::D3D10:
            std::cout << "Kiero DX10 initialised\n";
            impl::d3d10::init();
            break;
#endif
#if KIERO_INCLUDE_D3D11
        case kiero::RenderType::D3D11:
            std::cout << "Kiero DX11 initialised\n";
            impl::d3d11::init();
            break;
#endif
        case kiero::RenderType::D3D12:
            // TODO: D3D12 implementation?
            break;
        case kiero::RenderType::OpenGL:
            // TODO: OpenGL implementation?
            break;
        case kiero::RenderType::Vulkan:
            // TODO: Vulkan implementation?
            break;
        }
        HookDinput();
        return 1;
    }
    
    mLogger.LogOut("Failed to initialize ImGui");

    return 0;
}

void ImGuiInjector::SetWndProcHook(HWND hWnd) {
    ImGuiInjector::mWindowHandle = hWnd;
    ImGuiInjector::mWndProcPtr = reinterpret_cast<LONG_PTR>(WndProc);
    ImGuiInjector::mPreviousWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(ImGuiInjector::mWindowHandle, GWLP_WNDPROC, ImGuiInjector::mWndProcPtr));
}

HWND ImGuiInjector::GetWindowHandle() {
    return ImGuiInjector::mWindowHandle;
}

WNDPROC ImGuiInjector::GetPreviousWndProc() {
    return ImGuiInjector::mPreviousWndProc;
}

bool ImGuiInjector::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    for (auto it = mMenus.begin(); it != mMenus.end(); it++) {
        ImGuiMenu* menu = *(it._Ptr);
        menu->MessageHandler(hWnd, msg, wParam, lParam);
    }
    //if (IsMenuRunning()) ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

    if (ImGuiInjector::WantsMouseInput() && msg == WM_MOUSEWHEEL) return true;

    return false;
}

void ImGuiInjector::AddMenu(ImGuiMenu* pMenu) {
    ImGuiInjector::mMenus.push_back(pMenu);
}

void ImGuiInjector::RunMenus() {
    bool drawImGuiCursor = false;
    for (auto it = mMenus.begin(); it != mMenus.end(); it++) {
        ImGuiMenu* menu = *(it._Ptr);
        if (menu->IsOpen()) {
            drawImGuiCursor = true;
            menu->Loop();
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = drawImGuiCursor;
}

bool ImGuiInjector::IsMenuRunning() {
    bool isMenuRunning = false;
    for (auto it = mMenus.begin(); it != mMenus.end(); it++) {
        ImGuiMenu* menu = *(it._Ptr);
        isMenuRunning |= menu->IsOpen();
    }
    return isMenuRunning;
}

bool ImGuiInjector::WantsMouseInput() {
    return mWantsMouseInput;
}

bool ImGuiInjector::WantsKeyboardInput() {
    return mWantsKeyboardInput;
}

void ImGuiInjector::ResetInput() {
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureMouse = false;
    io.WantCaptureKeyboard = false;
    mWantsMouseInput = false;
    mWantsKeyboardInput = false;
}

void ImGuiInjector::UpdateInput() {
    ImGuiIO& io = ImGui::GetIO();
    mWantsMouseInput = io.WantCaptureMouse;
    mWantsKeyboardInput = io.WantCaptureKeyboard;
}

void ImGuiInjector::LoadJapaneseFont() {
    ImGuiIO& io = ImGui::GetIO();
    try {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    }
    catch (...) {
        std::cout << "Couldn't load font at path C:\\Windows\\Fonts\\msgothic.ttc" << std::endl;
    }
}