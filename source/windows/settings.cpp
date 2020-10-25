#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "language.h"
#include "net.h"
#include "popups.h"
#include "windows.h"

namespace Windows {
	static bool update_popup = false, network_status = false, update_available = false;
	static std::string tag_name = std::string();

	void Separator(void) {
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
	}
	
	void SettingsWindow(void) {
		Windows::SetupWindow();
		
		if (ImGui::Begin(strings[cfg.lang][Lang::SettingsTitle], nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsSortTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::RadioButton(strings[cfg.lang][Lang::SettingsSortNameAsc], &cfg.sort, 0);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(strings[cfg.lang][Lang::SettingsSortNameDesc], &cfg.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(strings[cfg.lang][Lang::SettingsSortSizeLarge], &cfg.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(strings[cfg.lang][Lang::SettingsSortSizeSmall], &cfg.sort, 3);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsImageViewTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox(strings[cfg.lang][Lang::SettingsImageViewFilenameToggle], &cfg.image_filename);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsDevOptsTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox(strings[cfg.lang][Lang::SettingsDevOptsLogsToggle], &cfg.dev_options);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsAboutTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("NX-Shell %s: v%d.%d.%d", strings[cfg.lang][Lang::SettingsAboutVersion], VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("ImGui %s: %s", strings[cfg.lang][Lang::SettingsAboutVersion], ImGui::GetVersion());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("%s: Joel16", strings[cfg.lang][Lang::SettingsAboutAuthor]);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("%s: Preetisketch", strings[cfg.lang][Lang::SettingsAboutBanner]);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				if (ImGui::Button(strings[cfg.lang][Lang::SettingsCheckForUpdates], ImVec2(250, 50))) {
					tag_name = Net::GetLatestReleaseJSON();
					network_status = Net::GetNetworkStatus();
					update_available = Net::GetAvailableUpdate(tag_name);
					update_popup = true;
				}
				ImGui::TreePop();
			}
		}

		if (update_popup)
			Popups::UpdatePopup(&update_popup, &network_status, &update_available, tag_name);
		
		Windows::ExitWindow();
	}
}
