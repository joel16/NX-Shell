#include <cstring>
#include <filesystem>
#include <zzip/zzip.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "log.h"
#include "popups.h"
#include "lang.hpp"
using namespace lang::literals;
namespace ArchiveHelper {
    std::string ConstructPath(const char path[FS_MAX_PATH]) {
        std::string new_path = config.cwd;
        new_path.append("/");
        new_path.append(std::filesystem::path(path).stem());
        new_path.append("/");
        return new_path;
    }
    
    std::string ConstructDirname(const char path[FS_MAX_PATH], char *dirname) {
        std::string new_dirname = ArchiveHelper::ConstructPath(path);
        new_dirname.append(std::filesystem::path(dirname).parent_path());
        return new_dirname;
    }
    
    std::string ConstructFilename(const char path[FS_MAX_PATH], char *filename) {
        std::string new_filename = ArchiveHelper::ConstructPath(path);
        new_filename.append(filename);
        return new_filename;
    }
    
    Result RecursiveMakeDir(const std::string &path) {
        Result ret = 0;
        char buf[FS_MAX_PATH + 1];
        char *p = nullptr;
        
        int length = std::snprintf(buf, sizeof(buf), path.c_str());
        if (buf[length - 1] == '/')
            buf[length - 1] = 0;
            
        for (p = buf + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                ret = fsFsCreateDirectory(fs, buf);
                *p = '/';
            }
            
            ret = fsFsCreateDirectory(fs, buf);
        }
        
        return ret;
    }
    
    Result ExtractFile(ZZIP_DIR *dir, const ZZIP_DIRENT &entry, const std::string &path) {
        Result ret = 0;
        ZZIP_FILE *src_handle = zzip_file_open(dir, entry.d_name, O_RDONLY);
        if (!src_handle) {
            Log::Error("zzip_file_open(%s) failed\n", path.c_str());
            return -1;
        }
            
        char dest_path[FS_MAX_PATH + 1];
        std::snprintf(dest_path, FS_MAX_PATH, path.c_str());
        
        if (!FS::FileExists(dest_path))
            fsFsCreateFile(fs, dest_path, entry.st_size, 0);
            
        FsFile dest_handle;
        if (R_FAILED(ret = fsFsOpenFile(fs, dest_path, FsOpenMode_Write, &dest_handle))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", path.c_str(), ret);
            zzip_file_close(src_handle);
            return ret;
        }
        
        const u64 buf_size = 0x10000;
        s64 offset = 0;
        unsigned char *buf = new unsigned char[buf_size];
        
        zzip_ssize_t bytes_read = 0;
        std::string filename = std::filesystem::path(entry.d_name).filename();
        while (0 < (bytes_read = zzip_read(src_handle, buf, buf_size - 1))) {
            if (R_FAILED(ret = fsFileWrite(&dest_handle, offset, buf, bytes_read, FsWriteOption_Flush))) {
                Log::Error("fsFileWrite(%s) failed: 0x%x\n", path.c_str(), ret);
                delete[] buf;
                zzip_file_close(src_handle);
                fsFileClose(&dest_handle);
                return ret;
            }
            
            offset += bytes_read;
            std::memset(buf, 0, buf_size);
            Popups::ProgressPopup(static_cast<float>(offset), static_cast<float>(entry.st_size), "Extracting:", filename.c_str());
        }
        
        delete[] buf;
        fsFileClose(&dest_handle);
        zzip_file_close(src_handle);
        return 0; 
    }
    
    void Extract(const char path[FS_MAX_PATH]) {
        ZZIP_DIR *dir;
        ZZIP_DIRENT entry;
        zzip_error_t error;
        
        dir = zzip_dir_open(path, &error);
        if (!dir) {
            Log::Error("zzip_dir_open(%s) failed: 0x%x\n", path, error);
            return;
        }
            
        while (zzip_dir_read(dir, &entry)) {
            std::string pathname = ArchiveHelper::ConstructDirname(path, entry.d_name);
            ArchiveHelper::RecursiveMakeDir(pathname);
            
            std::string filename = ArchiveHelper::ConstructFilename(path, entry.d_name);
            ArchiveHelper::ExtractFile(dir, entry, filename);
        }
        
        zzip_dir_close(dir);
    }
}

namespace Popups {
    void ArchivePopup(void) {
        Popups::SetupPopup("Archive"_lang.c_str());
        
        if (ImGui::BeginPopupModal("Archive"_lang.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Text_This"_lang.c_str());
            std::string text = "Text_Do"_lang + item.selected_filename + "?";
            ImGui::Text(text.c_str());
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("OK"_lang.c_str(), ImVec2(120, 0))) {
                ImGui::EndPopup();
                ImGui::PopStyleVar();
                ImGui::Render();
                
                char path[FS_MAX_PATH + 1];
                if ((std::snprintf(path, FS_MAX_PATH, "%s/%s", config.cwd, item.selected_filename.c_str())) > 0) {
                    ArchiveHelper::Extract(path);
                    item.file_count = FS::RefreshEntries(&item.entries, item.file_count);
                    GUI::ResetCheckbox();
                }
                
                ImGui::CloseCurrentPopup();
                item.state = MENU_STATE_HOME;
                return;
            }
            
            ImGui::SameLine(0.0f, 15.0f);
            
            if (ImGui::Button("Cancel"_lang.c_str(), ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                item.state = MENU_STATE_HOME;
            }
        }
        
        Popups::ExitPopup();
    }
}
