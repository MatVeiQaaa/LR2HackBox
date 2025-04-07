#include "BaseModels/ModFeature.hpp"

#include "safetyhook/safetyhook.hpp"

#include <stdint.h>
class AprilFools : public ModFeature {
public:
	bool Init(uintptr_t moduleBase);
	bool Deinit();

	bool GetEnabled();
	void SetEnabled(bool value);
	void ToggleEnabled();

	void Menu();

private:
	static void OnDrawNote(SafetyHookContext& regs);

	std::vector<SafetyHookMid> mMidHooks;

	bool mIsEnabled = false;
};