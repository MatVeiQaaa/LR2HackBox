#ifndef NDEBUG
#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>
#include <unordered_map>

class MemoryTracker : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

private:
	static LPVOID __stdcall OnHeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
	static LPVOID __stdcall  OnHeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
	static BOOL __stdcall OnHeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);

	size_t mCurrentAllocated = 0;
	unsigned int mRefCount = 0;
	std::unordered_map<void*, size_t> mAllocatedRef;
};
#endif