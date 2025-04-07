#include <vector>
#include <iostream>
#include <cstring>
#include "mem.h"
#include <windows.h>

// https://guidedhacking.com/threads/how-to-hack-any-game-first-internal-hack-dll-tutorial.12142/

// TODO: here should be a large comment explaining all of this mess.
const unsigned int CALL = 0xE8;
const unsigned int JMP = 0xE9;
const unsigned int NOP = 0x90;
const unsigned int CALL_SIZE = 5;
const unsigned int JMP_SIZE = 5;

bool mem::Detour32(void* src, void* dst, int len) {
	// Check if len is 5 or greater to make sure the JMP instruction can fit.
	if (len < CALL_SIZE) return false;

	// Setting EXECUTE+READ+WRITE permission for the bytes to alter.
	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);
	if (hResult == NULL)
	{
		std::cout << "couldn't set permission on hook source" << std::endl;
	}

	// Allocating memory for stolen bytes.
	void* gateway = VirtualAlloc(0, JMP_SIZE + JMP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (gateway == nullptr)
	{
		std::cout << "Couldn't allocate memory for stolen bytes" << std::endl;
		return false;
	}

	// Calculating relative addresses for JMP from source, JMP to target and JMP back to source.
	uintptr_t relativeAddress = ((uintptr_t)gateway - (uintptr_t)src) - JMP_SIZE;
	intptr_t  gatewayToSourceRelativeAddr = (intptr_t)src - (intptr_t)gateway - JMP_SIZE - JMP_SIZE;
	intptr_t  gatewayToDestinationRelativeAddr = (intptr_t)dst - (intptr_t)gateway - JMP_SIZE;

	// Setting the CALL to destination up.
	// TODO: use 'using' to extract the types.
	*(char*)((uintptr_t)gateway) = CALL;
	*(uintptr_t*)((uintptr_t)gateway + 1) = gatewayToDestinationRelativeAddr;


	// Filling the source with NOPs (needed for len > 5).
	std::memset(src, NOP, len);

	// Setting the JMP from source up.
	*(BYTE*)src = JMP;
	*(uintptr_t*)((uintptr_t)src + 1) = relativeAddress;

	// Restoring original page protection for source.
	DWORD temp = 0;
	hResult = VirtualProtect(src, len, curProtection, &temp);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	// Setting the JMP back from gateway up.
	*(uintptr_t*)((uintptr_t)gateway + 5) = JMP;
	*(uintptr_t*)((uintptr_t)gateway + 6) = gatewayToSourceRelativeAddr + len;

	return true;
}

bool mem::DetourAndExecuteStolen32(void* src, void* dst, int len) {
	// Check if len is 5 or greater to make sure the JMP instruction can fit.
	if (len < CALL_SIZE) return false;

	// Setting EXECUTE+READ+WRITE permission for the bytes to alter.
	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	// Allocating memory for stolen bytes.
	void* gateway = VirtualAlloc(0, len + CALL_SIZE + JMP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (gateway == nullptr)
	{
		std::cout << "Couldn't allocate memory for stolen bytes" << std::endl;
		return false;
	}

	// Calculating relative addresses for JMP from source, CALL from gateway and JMP from gateway.
	uintptr_t relativeAddress = ((uintptr_t)gateway - (uintptr_t)src) - CALL_SIZE;
	intptr_t  gatewayToSourceRelativeAddr = (intptr_t)src - (intptr_t)gateway - CALL_SIZE - JMP_SIZE;
	intptr_t  gatewayToDestinationRelativeAddr = (intptr_t)dst - (intptr_t)gateway - CALL_SIZE;

	// Setting the CALL to destination up.
	// TODO: use 'using' to extract the types.
	*(char*)((uintptr_t)gateway) = CALL;
	*(uintptr_t*)((uintptr_t)gateway + 1) = gatewayToDestinationRelativeAddr;

	// Copying stolen bytes to allocated memory.
	std::memcpy((void*)((uintptr_t)gateway + CALL_SIZE), src, len);

	// Filling the source with NOPs (needed for len > 5).
	std::memset(src, NOP, len);

	// Setting the JMP from source up.
	*(BYTE*)src = JMP;
	*(uintptr_t*)((uintptr_t)src + 1) = relativeAddress;

	// Restoring original page protection for source.
	DWORD temp = 0;
	hResult = VirtualProtect(src, len, curProtection, &temp);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	// Setting the JMP back from gateway up.
	*(uintptr_t*)((uintptr_t)gateway + len + 5) = JMP;
	*(uintptr_t*)((uintptr_t)gateway + len + 6) = gatewayToSourceRelativeAddr;

	return true;
}

bool mem::JmpDetour32(void* src, void* dst, int len) {
	// Check if len is 5 or greater to make sure the JMP instruction can fit.
	if (len < CALL_SIZE) return false;

	// Setting EXECUTE+READ+WRITE permission for the bytes to alter.
	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	// Allocating memory for stolen bytes.
	void* gateway = VirtualAlloc(0, len + CALL_SIZE + JMP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (gateway == nullptr)
	{
		std::cout << "Couldn't allocate memory for stolen bytes" << std::endl;
		return false;
	}

	// Calculating relative addresses for JMP from source, CALL from gateway and JMP from gateway.
	uintptr_t relativeAddress = ((uintptr_t)gateway - (uintptr_t)src) - CALL_SIZE;
	intptr_t  gatewayToSourceRelativeAddr = (intptr_t)src - (intptr_t)gateway - CALL_SIZE - JMP_SIZE;
	intptr_t  gatewayToDestinationRelativeAddr = (intptr_t)dst - (intptr_t)gateway - CALL_SIZE;

	// Setting the CALL to destination up.
	*(char*)((uintptr_t)gateway) = JMP;
	*(uintptr_t*)((uintptr_t)gateway + 1) = gatewayToDestinationRelativeAddr;

	// Copying stolen bytes to allocated memory.
	std::memcpy((void*)((uintptr_t)gateway + CALL_SIZE), src, len);

	// Filling the source with NOPs (needed for len > 5).
	std::memset(src, NOP, len);

	// Setting the JMP from source up.
	*(BYTE*)src = JMP;
	*(uintptr_t*)((uintptr_t)src + 1) = relativeAddress;

	// Restoring original page protection for source.
	DWORD temp = 0;
	hResult = VirtualProtect(src, len, curProtection, &temp);
	if (hResult == NULL)
	{
		// TODO: Throw an error.
		// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect#return-value
	}

	// Setting the JMP back from gateway up.
	*(uintptr_t*)((uintptr_t)gateway + len + 5) = JMP;
	*(uintptr_t*)((uintptr_t)gateway + len + 6) = gatewayToSourceRelativeAddr;

	return true;
}

uintptr_t mem::FindDMAAddy(uintptr_t ptr, const std::vector<unsigned int>& offsets) {
	uintptr_t addr = ptr;
	//std::cout << "being in FindDMAAddy loop\n";
	// No need for '&' here since 'unsigned int' is a primitive.
	for (const auto offset : offsets)
	{
		//std::cout << "In the loop " << offset << '\n';
		addr = *(uintptr_t*)addr;
		addr += offset;
		//std::cout << (int*)addr << '\n';
	}
	//std::cout << "Exit FindDMAAddy" << std::endl;
	return addr;
}