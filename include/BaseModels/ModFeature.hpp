#pragma once

#include <windows.h>

class ModFeature {
public:
	virtual bool Init(uintptr_t moduleBase);
	virtual bool Deinit();
protected:
	uintptr_t mModuleBase = 0;
};