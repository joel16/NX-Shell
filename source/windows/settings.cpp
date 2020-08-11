#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "windows.h"

namespace Windows {
    void SettingsWindow(MenuItem *item) {
		Windows::SetupWindow();
		
		if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode("Sort Settings")) {
				ImGui::RadioButton(" By name (ascending)", &config.sort, 0);
				ImGui::RadioButton(" By name (descending)", &config.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
				ImGui::RadioButton(" By size (largest first)", &config.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
				ImGui::RadioButton(" By size (smallest first)", &config.sort, 3);
				ImGui::TreePop();
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::TreeNode("About")) {
				ImGui::Text("NX-Shell Version: v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("ImGui Version: %s",  ImGui::GetVersion());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Author: Joel16");
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Banner: Preetisketch");
				ImGui::TreePop();
			}
		}
		
		ImGui::End();
		ImGui::PopStyleVar();
		Config::Save(config);
		item->file_count = FS::RefreshEntries(&item->entries, item->file_count);
	}
}
