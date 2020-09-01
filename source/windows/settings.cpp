#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

namespace Windows {
	void Separator(void) {
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
	}

    void SettingsWindow(void) {
		Windows::SetupWindow();
		
		if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode("Sort Settings")) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::RadioButton(" By name (ascending)", &config.sort, 0);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By name (descending)", &config.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By size (largest first)", &config.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By size (smallest first)", &config.sort, 3);
				ImGui::TreePop();
			}
			
			Windows::Separator();

			if (ImGui::TreeNode("Image Viewer")) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox(" Display filename", &config.image_filename);
				ImGui::TreePop();
			}

			Windows::Separator();

			if (ImGui::TreeNode("Developer Options")) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox(" Enable logging", &config.dev_options);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode("About")) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
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
		
		Windows::ExitWindow();
		Config::Save(config);
		item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
	}
}
