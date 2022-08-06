#include <algorithm>
#include <cstring>

#include "config.hpp"
#include "fs.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "tabs.hpp"
#include "textures.hpp"
#include "utils.hpp"

int sort = 0;
std::vector<std::string> devices_list = { "sdmc:", "safe:", "user:", "system:" };

namespace FileBrowser {
    // Sort without using ImGuiTableSortSpecs
    bool Sort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB) {
        // Make sure ".." stays at the top regardless of sort direction
        if (strcasecmp(entryA.name, "..") == 0)
            return true;
        
        if (strcasecmp(entryB.name, "..") == 0)
            return false;
        
        if ((entryA.type == FsDirEntryType_Dir) && !(entryB.type == FsDirEntryType_Dir))
            return true;
        else if (!(entryA.type == FsDirEntryType_Dir) && (entryB.type == FsDirEntryType_Dir))
            return false;

        switch(sort) {
            case FS_SORT_ALPHA_ASC:
                return (strcasecmp(entryA.name, entryB.name) < 0);
                break;

            case FS_SORT_ALPHA_DESC:
                return (strcasecmp(entryB.name, entryA.name) < 0);
                break;

            case FS_SORT_SIZE_ASC:
                return (entryA.file_size < entryB.file_size);
                break;

            case FS_SORT_SIZE_DESC:
                return (entryB.file_size < entryA.file_size);
                break;
        }

        return false;
    }

    // Sort using ImGuiTableSortSpecs
    bool TableSort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB) {
        bool descending = false;
        ImGuiTableSortSpecs *table_sort_specs = ImGui::TableGetSortSpecs();
        
        for (int i = 0; i < table_sort_specs->SpecsCount; ++i) {
            const ImGuiTableColumnSortSpecs *column_sort_spec = std::addressof(table_sort_specs->Specs[i]);
            descending = (column_sort_spec->SortDirection == ImGuiSortDirection_Descending);

            // Make sure ".." stays at the top regardless of sort direction
            if (strcasecmp(entryA.name, "..") == 0)
                return true;
            
            if (strcasecmp(entryB.name, "..") == 0)
                return false;
            
            if ((entryA.type == FsDirEntryType_Dir) && !(entryB.type == FsDirEntryType_Dir))
                return true;
            else if (!(entryA.type == FsDirEntryType_Dir) && (entryB.type == FsDirEntryType_Dir))
                return false;
            else {
                switch (column_sort_spec->ColumnIndex) {
                    case 1: // filename
                        sort = descending? FS_SORT_ALPHA_DESC : FS_SORT_ALPHA_ASC;
                        return descending? (strcasecmp(entryB.name, entryA.name) < 0) : (strcasecmp(entryA.name, entryB.name) < 0);
                        break;
                        
                    case 2: // Size
                        sort = descending? FS_SORT_SIZE_DESC : FS_SORT_SIZE_ASC;
                        return descending? (entryB.file_size < entryA.file_size) : (entryA.file_size < entryB.file_size);
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        return false;
    }
}

namespace Tabs {
    static const ImVec2 tex_size = ImVec2(21, 21);

    void FileBrowser(WindowData &data) {
        if (ImGui::BeginTabItem("File Browser")) {
            ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Spacing

            ImGui::PushID("device_list");
            ImGui::PushItemWidth(160.f);
            if (ImGui::BeginCombo("", device.c_str())) {
                for (std::size_t i = 0; i < devices_list.size(); i++) {
                    const bool is_selected = (device == devices_list[i]);
                    
                    if (ImGui::Selectable(devices_list[i].c_str(), is_selected)) {
                        device = devices_list[i];
                        fs = std::addressof(devices[i]);
                        
                        cwd = "/";
                        data.entries.clear();
                        FS::GetDirList(device, cwd, data.entries);
                        
                        data.checkbox_data.checked.resize(data.entries.size());
                        FS::GetUsedStorageSpace(data.used_storage);
                        FS::GetTotalStorageSpace(data.total_storage);
                        sort = -1;
                    }
                        
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
            
            ImGui::SameLine();

            // Display current working directory
            ImGui::Text(cwd.c_str());
            
            // Draw storage bar
            ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Spacing
            ImGui::ProgressBar(static_cast<float>(data.used_storage) / static_cast<float>(data.total_storage), ImVec2(1265.0f, 6.0f), "");
            ImGui::Dummy(ImVec2(0.0f, 1.0f)); // Spacing

            ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersInner |
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
            
            if (ImGui::BeginTable("Directory List", 3, tableFlags)) {
                // Make header always visible
                // ImGui::TableSetupScrollFreeze(0, 1);

                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoHeaderLabel | ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Filename", ImGuiTableColumnFlags_DefaultSort);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 150.f);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs *sorts_specs = ImGui::TableGetSortSpecs()) {
                    if (sort == -1)
                        sorts_specs->SpecsDirty = true;
                    
                    if (sorts_specs->SpecsDirty) {
                        std::sort(data.entries.begin(), data.entries.end(), FileBrowser::TableSort);
                        sorts_specs->SpecsDirty = false;
                    }
                }

                for (u64 i = 0; i < data.entries.size(); i++) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    
                    if ((data.checkbox_data.checked[i]) && (data.checkbox_data.cwd.compare(cwd) == 0) && (data.checkbox_data.device.compare(device) == 0))
                        ImGui::Image(reinterpret_cast<ImTextureID>(check_icon.id), tex_size);
                    else
                        ImGui::Image(reinterpret_cast<ImTextureID>(uncheck_icon.id), tex_size);
                    
                    ImGui::PopID();

                    ImGui::TableNextColumn();
                    FileType file_type = FS::GetFileType(data.entries[i].name);
                    
                    if (data.entries[i].type == FsDirEntryType_Dir)
                        ImGui::Image(reinterpret_cast<ImTextureID>(folder_icon.id), tex_size);
                    else
                        ImGui::Image(reinterpret_cast<ImTextureID>(file_icons[file_type].id), tex_size);
                    
                    ImGui::SameLine();

                    if (ImGui::Selectable(data.entries[i].name, false)) {
                        if (data.entries[i].type == FsDirEntryType_Dir) {
                            if (std::strncmp(data.entries[i].name, "..", 2) == 0) {
                                if (FS::ChangeDirPrev(data.entries)) {
                                    if ((data.checkbox_data.count > 1) && (data.checkbox_data.checked_copy.empty()))
                                        data.checkbox_data.checked_copy = data.checkbox_data.checked;
                                        
                                    data.checkbox_data.checked.resize(data.entries.size());
                                }
                            }
                            else if (FS::ChangeDirNext(data.entries[i].name, data.entries)) {
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
                            std::string path = FS::BuildPath(data.entries[i]);
                            
                            switch (file_type) {
                                case FileTypeImage:
                                    if (Textures::LoadImageFile(path, data.textures))
                                        data.state = WINDOW_STATE_IMAGEVIEWER;
                                    break;

                                default:
                                    break;
                            }
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
