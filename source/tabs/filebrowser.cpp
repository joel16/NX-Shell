#include <cstdio>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "imgui_deko3d.h"
#include "imgui_internal.h"
#include "tabs.h"
#include "textures.h"
#include "utils.h"

namespace Tabs {
    static const ImVec2 tex_size = ImVec2(25, 25);

    void FileBrowser(WindowData &data) {
        if (ImGui::BeginTabItem("File Browser")) {
            // Display current working directory
            ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), cfg.cwd);
            
            // Draw storage bar
            ImGui::ProgressBar(static_cast<float>(data.used_storage) / static_cast<float>(data.total_storage), ImVec2(1265.0f, 6.0f), "");
            ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Spacing

            ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter |
                ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
            
            if (ImGui::BeginTable("Directory List", 3, tableFlags)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Filename");
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (u64 i = 0; i < data.entries.size(); i++) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    if (ImGui::ImageButton(imgui::deko3d::makeTextureID(
                        data.checkbox_data.checked.at(i)? dkMakeTextureHandle(check_icon.image_id, check_icon.sampler_id) : dkMakeTextureHandle(uncheck_icon.image_id, uncheck_icon.sampler_id)),
                        tex_size, ImVec2(0, 0), ImVec2(1, 1), 0)) {
                        data.checkbox_data.checked.at(i) = !data.checkbox_data.checked.at(i);
                    }
                    ImGui::PopID();

                    ImGui::TableNextColumn();
                    FileType file_type = FS::GetFileType(data.entries[i].name);
                    
                    if (data.entries[i].type == FsDirEntryType_Dir)
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(folder_icon.image_id, folder_icon.sampler_id)), tex_size);
                    else
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(file_icons[file_type].image_id, file_icons[file_type].sampler_id)), tex_size);
                    
                    ImGui::SameLine();

                    if (ImGui::Selectable(data.entries[i].name, false)) {
                        if (data.entries[i].type == FsDirEntryType_Dir) {
                            if (!strncmp(data.entries[i].name, "..", sizeof(data.entries[i].name))) {
                                if (R_SUCCEEDED(FS::ChangeDirPrev(data.entries))) {
                                    // Make a copy before resizing our vector.
                                    if (data.checkbox_data.count > 1)
                                        data.checkbox_data.checked_copy = data.checkbox_data.checked;
                                        
                                    data.checkbox_data.checked.resize(data.entries.size());
                                }
                            }
                            else if (R_SUCCEEDED(FS::ChangeDirNext(data.entries[i].name, data.entries))) {
                                if ((data.checkbox_data.count > 1) && (data.checkbox_data.checked_copy.empty()))
                                    data.checkbox_data.checked_copy = data.checkbox_data.checked;
                                
                                data.checkbox_data.checked.resize(data.entries.size());
                            }

                            // Reset navigation ID -- TODO: Scroll to top
                            ImGuiContext& g = *GImGui;
                            ImGui::SetNavID(ImGui::GetID(data.entries[0].name, 0), g.NavLayer, 0, ImRect());
                        }
                    }

                    ImGui::TableNextColumn();
                    if (data.entries[i].file_size != 0) {
                        char size[16];
                        Utils::GetSizeString(size, static_cast<double>(data.entries[i].file_size));
                        ImGui::Text(size);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }
    }
}
