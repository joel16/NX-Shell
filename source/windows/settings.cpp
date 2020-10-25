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
				const char *sort_options[] = {
					strings[cfg.lang][Lang::SettingsSortNameAsc],
					strings[cfg.lang][Lang::SettingsSortNameDesc],
					strings[cfg.lang][Lang::SettingsSortSizeLarge],
					strings[cfg.lang][Lang::SettingsSortSizeSmall]
				};
				
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

				const int max_sort = 4;
				for (int i = 0; i < max_sort; i++) {
					if (ImGui::RadioButton(sort_options[i], &cfg.sort, i)) {
						Config::Save(cfg);
						FS::GetDirList(cfg.cwd, item.entries);
					}

					if (i != (max_sort - 1))
						ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				}

				ImGui::TreePop();
			}
			
			Windows::Separator();

			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsLanguageTitle])) {
				const char *languages[] = {
					" Japanese",
					" English",
					" French",
					" German",
					" Italian",
					" Spanish",
					" Simplified Chinese",
					" Korean",
					" Dutch",
					" Portuguese",
					" Russian",
					" Traditional Chinese"
				};

				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				
				const int max_lang = 12;
				for (int i = 0; i < max_lang; i++) {
					if (ImGui::RadioButton(languages[i], &cfg.lang, i))
						Config::Save(cfg);
					
					if (i != (max_lang - 1))
						ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				}

				ImGui::TreePop();
			}

			Windows::Separator();
			
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsImageViewTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

				if (ImGui::Checkbox(strings[cfg.lang][Lang::SettingsImageViewFilenameToggle], &cfg.image_filename))
					Config::Save(cfg);
				
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode(strings[cfg.lang][Lang::SettingsDevOptsTitle])) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

				if (ImGui::Checkbox(strings[cfg.lang][Lang::SettingsDevOptsLogsToggle], &cfg.dev_options))
					Config::Save(cfg);
				
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
