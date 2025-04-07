#include "BaseModels/ImGuiMenu.hpp"

void ImGuiMenu::Loop() {
    // Has to be implemented in child class.
}

void ImGuiMenu::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Has to be implemented in child class.
}

bool ImGuiMenu::IsOpen() {
    return ImGuiMenu::mIsOpen;
}

bool ImGuiMenu::SetOpen(bool state) {
    ImGuiMenu::mIsOpen = state;
    return ImGuiMenu::IsOpen();
}

bool ImGuiMenu::ToggleOpen() {
    return ImGuiMenu::SetOpen(!ImGuiMenu::IsOpen());
}