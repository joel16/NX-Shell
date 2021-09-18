#include <algorithm>
#include <cstring>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "imgui_deko3d.h"
#include "imgui_internal.h"
#include "tabs.h"
#include "textures.h"
#include "utils.h"

namespace Tabs {
    static const u32 sampler_id = 1;
    static const ImVec2 tex_size = ImVec2(25, 25);

    static bool Sort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB) {
        bool descending = false;
        ImGuiTableSortSpecs *table_sort_specs = ImGui::TableGetSortSpecs();
        
        for (int i = 0; i < table_sort_specs->SpecsCount; ++i) {
            const ImGuiTableColumnSortSpecs *column_sort_spec = &table_sort_specs->Specs[i];
            descending = (column_sort_spec->SortDirection == ImGuiSortDirection_Descending);

            // Make sure ".." stays at the top regardless of sort direction
            if (strcasecmp(entryA.name, "..") == 0)
                return true;
            if(strcasecmp(entryB.name, "..") == 0)
                return false;
            
            if ((entryA.type == FsDirEntryType_Dir) && !(entryB.type == FsDirEntryType_Dir))
                return true;
            else if (!(entryA.type == FsDirEntryType_Dir) && (entryB.type == FsDirEntryType_Dir))
                return false;
            else {
                switch (column_sort_spec->ColumnIndex) {
                    case 1: // filename
                        return descending? (strcasecmp(entryB.name, entryA.name) < 0) : (strcasecmp(entryA.name, entryB.name) < 0);
                        break;
                        
                    case 2: // Size
                        return descending? (entryB.file_size < entryA.file_size) : (entryA.file_size < entryB.file_size);
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        return false;
    }

    void FileBrowser(WindowData &data) {
        if (ImGui::BeginTabItem("File Browser")) {
            // Display current working directory
            ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), cfg.cwd);
            
            // Draw storage bar
            ImGui::ProgressBar(static_cast<float>(data.used_storage) / static_cast<float>(data.total_storage), ImVec2(1265.0f, 6.0f), "");
            ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Spacing

            ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersInner |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
            
            if (ImGui::BeginTable("Directory List", 3, tableFlags)) {
                // Make header always visible
                ImGui::TableSetupScrollFreeze(0, 1);

                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Filename", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs *sorts_specs = ImGui::TableGetSortSpecs()) {
                    if (sorts_specs->SpecsDirty) {
                        std::sort(data.entries.begin(), data.entries.end(), Sort);
                        sorts_specs->SpecsDirty = false;
                    }
                }

                for (u64 i = 0; i < data.entries.size(); i++) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    
                    if ((data.checkbox_data.checked.at(i)) && (!data.checkbox_data.cwd.compare(cfg.cwd)))
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(check_icon.image_id, sampler_id)), tex_size);
                    else
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(uncheck_icon.image_id, sampler_id)), tex_size);
                    
                    ImGui::PopID();

                    ImGui::TableNextColumn();
                    FileType file_type = FS::GetFileType(data.entries[i].name);
                    
                    if (data.entries[i].type == FsDirEntryType_Dir)
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(folder_icon.image_id, sampler_id)), tex_size);
                    else
                        ImGui::Image(imgui::deko3d::makeTextureID(dkMakeTextureHandle(file_icons[file_type].image_id, sampler_id)), tex_size);
                    
                    ImGui::SameLine();

                    if (ImGui::Selectable(data.entries[i].name, false)) {
                        if (data.entries[i].type == FsDirEntryType_Dir) {
                            if (std::strncmp(data.entries[i].name, "..", 2) == 0) {
                                if (R_SUCCEEDED(FS::ChangeDirPrev(data.entries))) {
                                    if ((data.checkbox_data.count > 1) && (data.checkbox_data.checked_copy.empty()))
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

                            // Reapply sort
                            ImGuiTableSortSpecs *sorts_specs = ImGui::TableGetSortSpecs();
                            sorts_specs->SpecsDirty = true;
                        }
                        else {
                            
                        }
                    }

                    if (ImGui::IsItemHovered())
                        data.selected = i;

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
