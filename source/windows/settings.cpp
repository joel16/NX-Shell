#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "windows.h"

namespace Windows {
    void SettingsWindow(void) {
		Windows::SetupWindow();
		
		if (ImGui::Begin(u8"设置", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode(u8"排序设置")) {
				ImGui::RadioButton(u8" 按名称 (升序)", &config.sort, 0);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(u8" 按名称 (降序)", &config.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(u8" 按大小 (降序)", &config.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(u8" 按大小 (升序)", &config.sort, 3);
				ImGui::TreePop();
			}
			
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			
			if (ImGui::TreeNode(u8"关于")) {
				ImGui::Text(u8"NX-Shell 版本: v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text(u8"ImGui 版本: %s",  ImGui::GetVersion());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text(u8"作者: Joel16");
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text(u8"Banner: Preetisketch");
				ImGui::TreePop();
			}
		}
		
		ImGui::End();
		ImGui::PopStyleVar();
		Config::Save(config);
		item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
	}
}
