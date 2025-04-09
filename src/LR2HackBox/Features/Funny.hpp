#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>
class Funny : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	void Menu();

private:
	static void OnDrawNote(SafetyHookContext& regs);

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsInvisibleScratch = false;
	bool mIsMetronome = false;
};