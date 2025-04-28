#ifdef MEMORYTRACKER
#include "MemoryTracker.hpp"

#include <iostream>
#include <mutex>
#include <intrin.h>
#include <dbghelp.h>

#include "LR2HackBox/LR2HackBox.hpp"
#include "Misc.hpp"

#include "imgui/imgui.h"
#include "minhook/include/MinHook.h"

#pragma intrinsic(_ReturnAddress)

#pragma comment(lib, "Dbghelp.lib")

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

void ParsePEHeader() {
	const int MAX_FILEPATH = 255;
	char fileName[MAX_FILEPATH] = { 0 };
	GetModuleFileName(GetModuleHandle(NULL), (LPSTR)&fileName, MAX_FILEPATH);
	HANDLE file = NULL;
	DWORD fileSize = NULL;
	DWORD bytesRead = NULL;
	LPVOID fileData = NULL;
	PIMAGE_DOS_HEADER dosHeader = {};
	PIMAGE_NT_HEADERS imageNTHeaders = {};
	PIMAGE_SECTION_HEADER sectionHeader = {};
	PIMAGE_SECTION_HEADER importSection = {};
	IMAGE_IMPORT_DESCRIPTOR* importDescriptor = {};
	PIMAGE_THUNK_DATA thunkData = {};
	DWORD thunk = NULL;
	DWORD rawOffset = NULL;

	fileData = GetModuleHandle(NULL);

	// IMAGE_DOS_HEADER
	dosHeader = (PIMAGE_DOS_HEADER)fileData;
	printf("******* DOS HEADER *******\n");
	printf("\t0x%x\t\tMagic number\n", dosHeader->e_magic);
	printf("\t0x%x\t\tBytes on last page of file\n", dosHeader->e_cblp);
	printf("\t0x%x\t\tPages in file\n", dosHeader->e_cp);
	printf("\t0x%x\t\tRelocations\n", dosHeader->e_crlc);
	printf("\t0x%x\t\tSize of header in paragraphs\n", dosHeader->e_cparhdr);
	printf("\t0x%x\t\tMinimum extra paragraphs needed\n", dosHeader->e_minalloc);
	printf("\t0x%x\t\tMaximum extra paragraphs needed\n", dosHeader->e_maxalloc);
	printf("\t0x%x\t\tInitial (relative) SS value\n", dosHeader->e_ss);
	printf("\t0x%x\t\tInitial SP value\n", dosHeader->e_sp);
	printf("\t0x%x\t\tInitial SP value\n", dosHeader->e_sp);
	printf("\t0x%x\t\tChecksum\n", dosHeader->e_csum);
	printf("\t0x%x\t\tInitial IP value\n", dosHeader->e_ip);
	printf("\t0x%x\t\tInitial (relative) CS value\n", dosHeader->e_cs);
	printf("\t0x%x\t\tFile address of relocation table\n", dosHeader->e_lfarlc);
	printf("\t0x%x\t\tOverlay number\n", dosHeader->e_ovno);
	printf("\t0x%x\t\tOEM identifier (for e_oeminfo)\n", dosHeader->e_oemid);
	printf("\t0x%x\t\tOEM information; e_oemid specific\n", dosHeader->e_oeminfo);
	printf("\t0x%x\t\tFile address of new exe header\n", dosHeader->e_lfanew);

	// IMAGE_NT_HEADERS
	imageNTHeaders = (PIMAGE_NT_HEADERS)((DWORD)fileData + dosHeader->e_lfanew);
	printf("\n******* NT HEADERS *******\n");
	printf("\t%x\t\tSignature\n", imageNTHeaders->Signature);

	// FILE_HEADER
	printf("\n******* FILE HEADER *******\n");
	printf("\t0x%x\t\tMachine\n", imageNTHeaders->FileHeader.Machine);
	printf("\t0x%x\t\tNumber of Sections\n", imageNTHeaders->FileHeader.NumberOfSections);
	printf("\t0x%x\tTime Stamp\n", imageNTHeaders->FileHeader.TimeDateStamp);
	printf("\t0x%x\t\tPointer to Symbol Table\n", imageNTHeaders->FileHeader.PointerToSymbolTable);
	printf("\t0x%x\t\tNumber of Symbols\n", imageNTHeaders->FileHeader.NumberOfSymbols);
	printf("\t0x%x\t\tSize of Optional Header\n", imageNTHeaders->FileHeader.SizeOfOptionalHeader);
	printf("\t0x%x\t\tCharacteristics\n", imageNTHeaders->FileHeader.Characteristics);

	// OPTIONAL_HEADER
	printf("\n******* OPTIONAL HEADER *******\n");
	printf("\t0x%x\t\tMagic\n", imageNTHeaders->OptionalHeader.Magic);
	printf("\t0x%x\t\tMajor Linker Version\n", imageNTHeaders->OptionalHeader.MajorLinkerVersion);
	printf("\t0x%x\t\tMinor Linker Version\n", imageNTHeaders->OptionalHeader.MinorLinkerVersion);
	printf("\t0x%x\t\tSize Of Code\n", imageNTHeaders->OptionalHeader.SizeOfCode);
	printf("\t0x%x\t\tSize Of Initialized Data\n", imageNTHeaders->OptionalHeader.SizeOfInitializedData);
	printf("\t0x%x\t\tSize Of UnInitialized Data\n", imageNTHeaders->OptionalHeader.SizeOfUninitializedData);
	printf("\t0x%x\t\tAddress Of Entry Point (.text)\n", imageNTHeaders->OptionalHeader.AddressOfEntryPoint);
	printf("\t0x%x\t\tBase Of Code\n", imageNTHeaders->OptionalHeader.BaseOfCode);
	//printf("\t0x%x\t\tBase Of Data\n", imageNTHeaders->OptionalHeader.BaseOfData);
	printf("\t0x%x\t\tImage Base\n", imageNTHeaders->OptionalHeader.ImageBase);
	printf("\t0x%x\t\tSection Alignment\n", imageNTHeaders->OptionalHeader.SectionAlignment);
	printf("\t0x%x\t\tFile Alignment\n", imageNTHeaders->OptionalHeader.FileAlignment);
	printf("\t0x%x\t\tMajor Operating System Version\n", imageNTHeaders->OptionalHeader.MajorOperatingSystemVersion);
	printf("\t0x%x\t\tMinor Operating System Version\n", imageNTHeaders->OptionalHeader.MinorOperatingSystemVersion);
	printf("\t0x%x\t\tMajor Image Version\n", imageNTHeaders->OptionalHeader.MajorImageVersion);
	printf("\t0x%x\t\tMinor Image Version\n", imageNTHeaders->OptionalHeader.MinorImageVersion);
	printf("\t0x%x\t\tMajor Subsystem Version\n", imageNTHeaders->OptionalHeader.MajorSubsystemVersion);
	printf("\t0x%x\t\tMinor Subsystem Version\n", imageNTHeaders->OptionalHeader.MinorSubsystemVersion);
	printf("\t0x%x\t\tWin32 Version Value\n", imageNTHeaders->OptionalHeader.Win32VersionValue);
	printf("\t0x%x\t\tSize Of Image\n", imageNTHeaders->OptionalHeader.SizeOfImage);
	printf("\t0x%x\t\tSize Of Headers\n", imageNTHeaders->OptionalHeader.SizeOfHeaders);
	printf("\t0x%x\t\tCheckSum\n", imageNTHeaders->OptionalHeader.CheckSum);
	printf("\t0x%x\t\tSubsystem\n", imageNTHeaders->OptionalHeader.Subsystem);
	printf("\t0x%x\t\tDllCharacteristics\n", imageNTHeaders->OptionalHeader.DllCharacteristics);
	printf("\t0x%x\t\tSize Of Stack Reserve\n", imageNTHeaders->OptionalHeader.SizeOfStackReserve);
	printf("\t0x%x\t\tSize Of Stack Commit\n", imageNTHeaders->OptionalHeader.SizeOfStackCommit);
	printf("\t0x%x\t\tSize Of Heap Reserve\n", imageNTHeaders->OptionalHeader.SizeOfHeapReserve);
	printf("\t0x%x\t\tSize Of Heap Commit\n", imageNTHeaders->OptionalHeader.SizeOfHeapCommit);
	printf("\t0x%x\t\tLoader Flags\n", imageNTHeaders->OptionalHeader.LoaderFlags);
	printf("\t0x%x\t\tNumber Of Rva And Sizes\n", imageNTHeaders->OptionalHeader.NumberOfRvaAndSizes);

	// DATA_DIRECTORIES
	printf("\n******* DATA DIRECTORIES *******\n");
	printf("\tExport Directory Address: 0x%x; Size: 0x%x\n", imageNTHeaders->OptionalHeader.DataDirectory[0].VirtualAddress, imageNTHeaders->OptionalHeader.DataDirectory[0].Size);
	printf("\tImport Directory Address: 0x%x; Size: 0x%x\n", imageNTHeaders->OptionalHeader.DataDirectory[1].VirtualAddress, imageNTHeaders->OptionalHeader.DataDirectory[1].Size);

	// SECTION_HEADERS
	printf("\n******* SECTION HEADERS *******\n");
	// get offset to first section headeer
	DWORD sectionLocation = (DWORD)imageNTHeaders + sizeof(DWORD) + (DWORD)(sizeof(IMAGE_FILE_HEADER)) + (DWORD)imageNTHeaders->FileHeader.SizeOfOptionalHeader;
	DWORD sectionSize = (DWORD)sizeof(IMAGE_SECTION_HEADER);

	// get offset to the import directory RVA
	DWORD importDirectoryRVA = imageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	// print section data
	for (int i = 0; i < imageNTHeaders->FileHeader.NumberOfSections; i++) {
		sectionHeader = (PIMAGE_SECTION_HEADER)sectionLocation;
		printf("\t%s\n", sectionHeader->Name);
		printf("\t\t0x%x\t\tVirtual Size\n", sectionHeader->Misc.VirtualSize);
		printf("\t\t0x%x\t\tVirtual Address\n", sectionHeader->VirtualAddress);
		printf("\t\t0x%x\t\tSize Of Raw Data\n", sectionHeader->SizeOfRawData);
		printf("\t\t0x%x\t\tPointer To Raw Data\n", sectionHeader->PointerToRawData);
		printf("\t\t0x%x\t\tPointer To Relocations\n", sectionHeader->PointerToRelocations);
		printf("\t\t0x%x\t\tPointer To Line Numbers\n", sectionHeader->PointerToLinenumbers);
		printf("\t\t0x%x\t\tNumber Of Relocations\n", sectionHeader->NumberOfRelocations);
		printf("\t\t0x%x\t\tNumber Of Line Numbers\n", sectionHeader->NumberOfLinenumbers);
		printf("\t\t0x%x\tCharacteristics\n", sectionHeader->Characteristics);

		// save section that contains import directory table
		if (importDirectoryRVA >= sectionHeader->VirtualAddress && importDirectoryRVA < sectionHeader->VirtualAddress + sectionHeader->Misc.VirtualSize) {
			importSection = sectionHeader;
		}
		sectionLocation += sectionSize;
	}

	// get file offset to import table
	rawOffset = (DWORD)fileData + importSection->VirtualAddress;

	// get pointer to import descriptor's file offset. Note that the formula for calculating file offset is: imageBaseAddress + pointerToRawDataOfTheSectionContainingRVAofInterest + (RVAofInterest - SectionContainingRVAofInterest.VirtualAddress)
	importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(rawOffset + (imageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - importSection->VirtualAddress));

	printf("\n******* DLL IMPORTS *******\n");
	for (; importDescriptor->Name != 0; importDescriptor++) {
		// imported dll modules
		printf("\t%s\n", rawOffset + (importDescriptor->Name - importSection->VirtualAddress));
		thunk = importDescriptor->OriginalFirstThunk == 0 ? importDescriptor->FirstThunk : importDescriptor->OriginalFirstThunk;
		thunkData = (PIMAGE_THUNK_DATA)(rawOffset + (thunk - importSection->VirtualAddress));

		// dll exported functions
		for (; thunkData->u1.AddressOfData != 0; thunkData++) {
			//a cheap and probably non-reliable way of checking if the function is imported via its ordinal number ¯\_(ツ)_/¯
			if (thunkData->u1.AddressOfData > 0x80000000) {
				//show lower bits of the value to get the ordinal ¯\_(ツ)_/¯
				printf("\t\tOrdinal: %x\n", (WORD)thunkData->u1.AddressOfData);
			}
			else {
				printf("\t\t%s\n", (rawOffset + (thunkData->u1.AddressOfData - importSection->VirtualAddress + 2)));
			}
		}
	}
}


bool MemoryTracker::Init(uintptr_t moduleBase) {
	MemoryTracker::mModuleBase = (uintptr_t)GetModuleHandle(NULL);

	ParsePEHeader();

	// Only main module (LR2body.exe) for now.
	ULONG64 buffer[(sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());

	SymInitialize(hProcess, NULL, FALSE);
	SymLoadModuleEx(hProcess, NULL, "LR2body.exe", NULL, (DWORD64)hProcess, 0, NULL, 0);
	SymFromName(hProcess, "HeapAlloc", pSymbol);

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