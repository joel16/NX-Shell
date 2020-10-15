#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

TextReader text_reader;

namespace Windows {
    void TextReaderWindow(void) {
        Windows::SetupWindow();
        
        if (ImGui::Begin(item.entries[item.selected].name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::SetNextWindowFocus();
            ImGui::InputTextMultiline(item.entries[item.selected].name, text_reader.buf, text_reader.buf_size, ImVec2(1270.0f, 660.0f));
        }
        
        Windows::ExitWindow();
    }
}
