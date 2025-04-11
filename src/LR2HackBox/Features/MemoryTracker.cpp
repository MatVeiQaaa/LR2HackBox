#ifndef NDEBUG
#include "MemoryTracker.hpp"

#include <iostream>
#include <mutex>
#include <intrin.h>

#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

#include "imgui/imgui.h"
#include "minhook/include/MinHook.h"

#pragma intrinsic(_ReturnAddress)

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

static std::recursive_mutex sMutex;

typedef LPVOID(__stdcall* tHeapAlloc)(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
tHeapAlloc RtlHeapAlloc = (tHeapAlloc)GetProcAddress(GetModuleHandle("kernel32.dll"), "HeapAlloc");
LPVOID __stdcall MemoryTracker::OnHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
	MemoryTracker& memoryTracker = *(MemoryTracker*)(LR2HackBox::Get().mMemoryTracker);

	void* block = RtlHeapAlloc(hHeap, dwFlags, dwBytes);
	if (block == nullptr) {
		return block;
	}
	
	std::lock_guard mutex(sMutex);
	if (auto it = memoryTracker.mAllocatedRef.find(block); it != memoryTracker.mAllocatedRef.end()) {
		int diff = it->second - dwBytes;
		memoryTracker.mCurrentAllocated += diff;
		it->second = dwBytes;
	}
	else {
		memoryTracker.mAllocatedRef[block] = dwBytes;
		memoryTracker.mCurrentAllocated += dwBytes;
		memoryTracker.mRefCount++;
	}

	return block;
}

typedef LPVOID(__stdcall* tHeapReAlloc)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
tHeapReAlloc RtlHeapReAlloc = (tHeapReAlloc)GetProcAddress(GetModuleHandle("kernel32.dll"), "HeapReAlloc");
LPVOID __stdcall MemoryTracker::OnHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{
	MemoryTracker& memoryTracker = *(MemoryTracker*)(LR2HackBox::Get().mMemoryTracker);

	void* newBlock = RtlHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);
	if (newBlock == nullptr) {
		return newBlock;
	}
	std::lock_guard mutex(sMutex);
	if (auto it = memoryTracker.mAllocatedRef.find(lpMem); lpMem != nullptr && it != memoryTracker.mAllocatedRef.end()) {
		memoryTracker.mCurrentAllocated -= it->second;
		memoryTracker.mAllocatedRef.erase(it);
		memoryTracker.mRefCount--;
	}
	if (auto it = memoryTracker.mAllocatedRef.find(newBlock); it != memoryTracker.mAllocatedRef.end()) {
		int diff = it->second - dwBytes;
		memoryTracker.mCurrentAllocated += diff;
		it->second = dwBytes;
	}
	else {
		memoryTracker.mAllocatedRef[newBlock] = dwBytes;
		memoryTracker.mCurrentAllocated += dwBytes;
		memoryTracker.mRefCount++;
	}

	return newBlock;
}

typedef BOOL(__stdcall* tHeapFree)(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
tHeapFree RtlHeapFree = (tHeapFree)GetProcAddress(GetModuleHandle("kernel32.dll"), "HeapFree");
BOOL __stdcall MemoryTracker::OnHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
	MemoryTracker& memoryTracker = *(MemoryTracker*)(LR2HackBox::Get().mMemoryTracker);

	if (lpMem == nullptr) {
		return RtlHeapFree(hHeap, dwFlags, lpMem);
	}
	
	std::lock_guard mutex(sMutex);
	if (auto it = memoryTracker.mAllocatedRef.find(lpMem); it != memoryTracker.mAllocatedRef.end()) {
		memoryTracker.mCurrentAllocated -= it->second;
		memoryTracker.mAllocatedRef.erase(it);
		memoryTracker.mRefCount--;
	}

	return RtlHeapFree(hHeap, dwFlags, lpMem);
}

bool MemoryTracker::Init(uintptr_t moduleBase) {
	MemoryTracker::mModuleBase = moduleBase;

	// Only main module (LR2body.exe) for now.
	uintptr_t* LR2HeapAllocImprt = (uintptr_t*)0x7350D0;
	uintptr_t* LR2HeapReAllocImprt = (uintptr_t*)0x735288;
	uintptr_t* LR2HeapFreeImprt = (uintptr_t*)0x7350CC;

	DWORD curProtection = 0;
	VirtualProtect(LR2HeapAllocImprt, 4, PAGE_READWRITE, &curProtection);
	VirtualProtect(LR2HeapReAllocImprt, 4, PAGE_READWRITE, &curProtection);
	VirtualProtect(LR2HeapFreeImprt, 4, PAGE_READWRITE, &curProtection);

	*LR2HeapAllocImprt = (uintptr_t)OnHeapAlloc;
	*LR2HeapReAllocImprt = (uintptr_t)OnHeapReAlloc;
	*LR2HeapFreeImprt = (uintptr_t)OnHeapFree;

	if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		std::cout << ("Couldn't enable MemoryTracker hooks") << std::endl;
		return false;
	}

	return true;
}

bool MemoryTracker::Deinit() {
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

void MemoryTracker::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Indent();

	ImGui::Text("Currently allocated memory: %dB", mCurrentAllocated);
	ImGui::Text("References count: %d", mRefCount);

	ImGui::Unindent();
}
#endif