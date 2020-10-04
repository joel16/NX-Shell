#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "net.h"
#include "popups.h"
#include "windows.h"
#include "lang.hpp"
using namespace lang::literals;
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
		
		if (ImGui::Begin("Settings"_lang.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			if (ImGui::TreeNode("Sort Settings"_lang.c_str())) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::RadioButton(" By name (ascending)"_lang.c_str(), &config.sort, 0);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By name (descending)"_lang.c_str(), &config.sort, 1);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By size (largest first)"_lang.c_str(), &config.sort, 2);
				ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
				ImGui::RadioButton(" By size (smallest first)"_lang.c_str(), &config.sort, 3);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode("Image_Viewer"_lang.c_str())) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox("Display_filename"_lang.c_str(), &config.image_filename);
				ImGui::TreePop();
			}
			
			Windows::Separator();
			
			if (ImGui::TreeNode("Developer_Options"_lang.c_str())) { 
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Checkbox("Enable_logs"_lang.c_str(), &config.dev_options);
				ImGui::TreePop();
			}

			Windows::Separator();
			
			if (ImGui::TreeNode("Lanuage_Options"_lang.c_str())) { 
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			    auto cur_lang = lang::get_current_language(), prev_lang = cur_lang;
				ImGui::RadioButton("English",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::English));
				ImGui::RadioButton("中文",        reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Chinese));
				/*im::RadioButton("Français",   reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::French));
				im::RadioButton("Nederlands", reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Dutch));
				im::RadioButton("Italiano",   reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Italian));
				im::RadioButton("Deutsch",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::German));
				im::RadioButton("Español",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Spanish));*/
				//change this when you add a new language :)
				if (cur_lang != prev_lang)
					lang::set_language(cur_lang);
				ImGui::TreePop();
			}

			Windows::Separator();
			
			if (ImGui::TreeNode("About"_lang.c_str())) {
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Text_version"_lang.c_str(), VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("ImGui_version"_lang.c_str(),  ImGui::GetVersion());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Author"_lang.c_str());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				ImGui::Text("Banner"_lang.c_str());
				ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
				if (ImGui::Button("Check_for_Updates"_lang.c_str(), ImVec2(250, 50))) {
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
