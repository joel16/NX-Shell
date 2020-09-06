#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

TextReader text_reader;

namespace Windows {
    void TextReaderWindow(void) {
        Windows::SetupWindow();
        
        if (ImGui::Begin(item.selected_filename.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::SetNextWindowFocus();
            ImGui::InputTextMultiline(item.selected_filename.c_str(), text_reader.buf, text_reader.buf_size, ImVec2(1270.0f, 660.0f));
        }
        
        Windows::ExitWindow();
    }
}
