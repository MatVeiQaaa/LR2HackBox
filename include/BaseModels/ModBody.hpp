#pragma once

#include <windows.h>

class ModBody {
public:
	virtual bool Hook();
	virtual bool Unhook();
protected:
	uintptr_t mModuleBase = 0;
};