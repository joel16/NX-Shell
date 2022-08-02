#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "config.hpp"
#include "fs.hpp"
#include "language.hpp"
#include "log.hpp"
#include "popups.hpp"

// Global vars
FsFileSystem *fs;
FsFileSystem devices[4];

namespace FS {
    static int PREVIOUS_BROWSE_STATE = 0;

    typedef struct {
        char copy_path[FS_MAX_PATH];
        char copy_filename[FS_MAX_PATH];
        bool is_dir = false;
    } FS_Copy_Struct;
    
    FS_Copy_Struct fs_copy_struct;

    bool FileExists(const char path[FS_MAX_PATH]) {
        FsFile file;
        if (R_SUCCEEDED(fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
            fsFileClose(&file);
            return true;
        }
        
        return false;
    }
    
    bool DirExists(const char path[FS_MAX_PATH]) {
        FsDir dir;
        if (R_SUCCEEDED(fsFsOpenDirectory(fs, path, FsDirOpenMode_ReadDirs, &dir))) {
            fsDirClose(&dir);
            return true;
        }
        
        return false;
    }

    Result GetFileSize(const char path[FS_MAX_PATH], s64 &size) {
        Result ret = 0;
        
        FsFile file;
        if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", path, ret);
            return ret;
        }
        
        if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
            Log::Error("fsFileGetSize(%s) failed: 0x%x\n", path, ret);
            fsFileClose(&file);
            return ret;
        }
        
        fsFileClose(&file);
        return 0;
    }
    
    std::string GetFileExt(const std::string &filename) {
        std::string ext = std::filesystem::path(filename).extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }
    
    FileType GetFileType(const std::string &filename) {
        std::string ext = FS::GetFileExt(filename);
        
        if ((!ext.compare(".ZIP")) || (!ext.compare(".RAR")) || (!ext.compare(".7Z")))
            return FileTypeArchive;
        else if ((!ext.compare(".BMP")) || (!ext.compare(".GIF")) || (!ext.compare(".JPG")) || (!ext.compare(".JPEG")) || (!ext.compare(".PGM"))
            || (!ext.compare(".PPM")) || (!ext.compare(".PNG")) || (!ext.compare(".PSD")) || (!ext.compare(".TGA")) || (!ext.compare(".WEBP")))
            return FileTypeImage;
        else if ((!ext.compare(".JSON")) || (!ext.compare(".LOG")) || (!ext.compare(".TXT")) || (!ext.compare(".CFG")) || (!ext.compare(".INI")))
            return FileTypeText;
            
        return FileTypeNone;
    }
    
    Result GetDirList(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries) {
        FsDir dir;
        Result ret = 0;
        s64 read_entries = 0;
        const std::string cwd = path;
        entries.clear();

        // Create ".." entry
        FsDirectoryEntry entry;
        std::strncpy(entry.name, "..", 3);
        entry.type = FsDirEntryType_Dir;
        entry.file_size = 0;
        entries.push_back(entry);
        
        if (R_FAILED(ret = fsFsOpenDirectory(fs, path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir))) {
            Log::Error("GetDirListfsFsOpenDirectory(%s) failed: 0x%x\n", path, ret);
            return ret;
        }
        
        while (true) {
            FsDirectoryEntry entry;
            if (R_FAILED(ret = fsDirRead(&dir, &read_entries, 1, &entry))) {
                fsDirClose(&dir);
                Log::Error("fsDirRead(%s) failed: 0x%x\n", path, ret);
                return ret;
            }
            
            if (read_entries != 1)
                break;
            
            entries.push_back(entry);
        }

        fsDirClose(&dir);
        return 0;
    }
    
    static Result ChangeDir(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries) {
        Result ret = 0;
        std::vector<FsDirectoryEntry> new_entries;
        
        if (R_FAILED(ret = FS::GetDirList(path, new_entries)))
            return ret;
            
        // Apply cd after successfully listing new directory
        entries.clear();
        std::strncpy(cfg.cwd, path, FS_MAX_PATH);
        Config::Save(cfg);
        entries = new_entries;
        return 0;
    }
    
    static int GetPrevPath(char path[FS_MAX_PATH]) {
        if (std::strlen(cfg.cwd) <= 1 && cfg.cwd[0] == '/')
            return -1;
            
        // Remove upmost directory
        bool copy = false;
        int len = 0;
        for (ssize_t i = std::strlen(cfg.cwd); i >= 0; i--) {
            if (cfg.cwd[i] == '/')
                copy = true;
            if (copy) {
                path[i] = cfg.cwd[i];
                len++;
            }
        }
        
        // remove trailing slash
        if (len > 1 && path[len - 1] == '/')
            len--;
            
        path[len] = '\0';
        return 0;
    }
    
    Result ChangeDirNext(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries) {
        char new_cwd[FS_MAX_PATH];
        const char *sep = (std::strncmp(cfg.cwd, "/", 2) == 0)?  "" : "/"; // Don't append / if at /
        
        if ((std::snprintf(new_cwd, FS_MAX_PATH, "%s%s%s", cfg.cwd, sep, path)) > 0)
            return FS::ChangeDir(new_cwd, entries);
            
        return 0;
    }
    
    Result ChangeDirPrev(std::vector<FsDirectoryEntry> &entries) {
        char new_cwd[FS_MAX_PATH];
        if (FS::GetPrevPath(new_cwd) < 0)
            return -1;
            
        return FS::ChangeDir(new_cwd, entries);
    }

    static int BuildPath(FsDirectoryEntry &entry, char path[FS_MAX_PATH], const char filename[FS_MAX_PATH]) {
        if ((std::snprintf(path, FS_MAX_PATH, "%s%s%s", cfg.cwd, (std::strncmp(cfg.cwd, "/", 2) == 0)?  "" : "/", entry.name)) > 0)
            return 0;
            
        return -1;
    }

    static int BuildPath(char path[FS_MAX_PATH], const char filename[FS_MAX_PATH]) {
        if ((std::snprintf(path, FS_MAX_PATH, "%s%s%s", cfg.cwd, (std::strncmp(cfg.cwd, "/", 2) == 0)?  "" : "/", filename[0] != '\0'? filename : "")) > 0)
            return 0;
        
        return -1;
    }

    Result GetTimeStamp(FsDirectoryEntry &entry, FsTimeStampRaw &timestamp) {
        Result ret = 0;
        
        char path[FS_MAX_PATH];
        if (R_FAILED(FS::BuildPath(entry, path, "")))
            return -1;
            
        if (R_FAILED(ret = fsFsGetFileTimeStampRaw(fs, path, &timestamp))) {
            Log::Error("fsFsGetFileTimeStampRaw(%s) failed: 0x%x\n", path, ret);
            return ret;
        }
            
        return 0;
    }

    Result Rename(FsDirectoryEntry &entry, const char filename[FS_MAX_PATH]) {
        Result ret = 0;
        
        char path[FS_MAX_PATH];
        if (FS::BuildPath(entry, path, "") < 0)
            return -1;
            
        char new_path[FS_MAX_PATH];
        if (FS::BuildPath(new_path, filename) < 0)
            return -1;
        
        if (entry.type == FsDirEntryType_Dir) {
            if (R_FAILED(ret = fsFsRenameDirectory(fs, path, new_path))) {
                Log::Error("fsFsRenameDirectory(%s, %s) failed: 0x%x\n", path, new_path, ret);
                return ret;
            }
        }
        else {
            if (R_FAILED(ret = fsFsRenameFile(fs, path, new_path))) {
                Log::Error("fsFsRenameFile(%s, %s) failed: 0x%x\n", path, new_path, ret);
                return ret;
            }
        }
        
        return 0;
    }
    
    Result Delete(FsDirectoryEntry &entry) {
        Result ret = 0;
        
        char path[FS_MAX_PATH];
        if (FS::BuildPath(entry, path, "") < 0)
            return -1;
            
        if (entry.type == FsDirEntryType_Dir) {
            if (R_FAILED(ret = fsFsDeleteDirectoryRecursively(fs, path))) {
                Log::Error("fsFsDeleteDirectoryRecursively(%s) failed: 0x%x\n", path, ret);
                return ret;
            }
        }
        else {
            if (R_FAILED(ret = fsFsDeleteFile(fs, path))) {
                Log::Error("fsFsDeleteFile(%s) failed: 0x%x\n", path, ret);
                return ret;
            }
        }
        
        return 0;
    }

    Result SetArchiveBit(FsDirectoryEntry &entry) {
        Result ret = 0;
        
        char path[FS_MAX_PATH];
        if (FS::BuildPath(entry, path, "") < 0)
            return -1;
            
        if (R_FAILED(ret = fsFsSetConcatenationFileAttribute(fs, path))) {
            Log::Error("fsFsSetConcatenationFileAttribute(%s) failed: 0x%x\n", path, ret);
            return ret;
        }
            
        return 0;
    }
    
    static Result CopyFile(const char src_path[FS_MAX_PATH], const char dest_path[FS_MAX_PATH]) {
        Result ret = 0;
        FsFile src_handle, dest_handle;
        
        if (R_FAILED(ret = fsFsOpenFile(&devices[PREVIOUS_BROWSE_STATE], src_path, FsOpenMode_Read, &src_handle))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", src_path, ret);
            return ret;
        }
        
        s64 size = 0;
        if (R_FAILED(ret = fsFileGetSize(&src_handle, &size))) {
            Log::Error("fsFileGetSize(%s) failed: 0x%x\n", src_path, ret);
            fsFileClose(&src_handle);
            return ret;
        }

        // This may fail or not, but we don't care -> create the file if it doesn't exist, otherwise continue.
        fsFsCreateFile(fs, dest_path, size, 0);
            
        if (R_FAILED(ret = fsFsOpenFile(fs, dest_path, FsOpenMode_Write, &dest_handle))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", dest_path, ret);
            fsFileClose(&src_handle);
            return ret;
        }
        
        u64 bytes_read = 0;
        const u64 buf_size = 0x10000;
        s64 offset = 0;
        unsigned char *buf = new unsigned char[buf_size];
        std::string filename = std::filesystem::path(src_path).filename();
        
        do {
            std::memset(buf, 0, buf_size);
            
            if (R_FAILED(ret = fsFileRead(&src_handle, offset, buf, buf_size, FsReadOption_None, &bytes_read))) {
                Log::Error("fsFileRead(%s) failed: 0x%x\n", src_path, ret);
                delete[] buf;
                fsFileClose(&src_handle);
                fsFileClose(&dest_handle);
                return ret;
            }
            
            if (R_FAILED(ret = fsFileWrite(&dest_handle, offset, buf, bytes_read, FsWriteOption_Flush))) {
                Log::Error("fsFileWrite(%s) failed: 0x%x\n", dest_path, ret);
                delete[] buf;
                fsFileClose(&src_handle);
                fsFileClose(&dest_handle);
                return ret;
            }
            
            offset += bytes_read;
            Popups::ProgressBar(static_cast<float>(offset), static_cast<float>(size), strings[cfg.lang][Lang::OptionsCopying], filename.c_str());
        } while(offset < size);
        
        delete[] buf;
        fsFileClose(&src_handle);
        fsFileClose(&dest_handle);
        return 0;
    }

    static Result CopyDir(const char src_path[FS_MAX_PATH], const char dest_path[FS_MAX_PATH]) {
        Result ret = 0;
        FsDir dir;
        
        if (R_FAILED(ret = fsFsOpenDirectory(&devices[PREVIOUS_BROWSE_STATE], src_path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir))) {
            Log::Error("fsFsOpenDirectory(%s) failed: 0x%x\n", src_path, ret);
            return ret;
        }
            
        // This may fail or not, but we don't care -> make the dir if it doesn't exist, otherwise continue.
        fsFsCreateDirectory(fs, dest_path);
        
        s64 entry_count = 0;
        if (R_FAILED(ret = fsDirGetEntryCount(&dir, &entry_count))) {
            Log::Error("fsDirGetEntryCount(%s) failed: 0x%x\n", src_path, ret);
            return ret;
        }
            
        FsDirectoryEntry *entries = new FsDirectoryEntry[entry_count * sizeof(*entries)];
        if (R_FAILED(ret = fsDirRead(&dir, nullptr, static_cast<size_t>(entry_count), entries))) {
            Log::Error("fsDirRead(%s) failed: 0x%x\n", src_path, ret);
            delete[] entries;
            return ret;
        }
        
        for (s64 i = 0; i < entry_count; i++) {
            std::string filename = entries[i].name;
            if (!filename.empty()) {
                if ((!filename.compare(".")) || (!filename.compare("..")))
                    continue;
                
                std::string src = src_path;
                src.append("/");
                src.append(filename);

                std::string dest = dest_path;
                dest.append("/");
                dest.append(filename);
                
                if (entries[i].type == FsDirEntryType_Dir)
                    FS::CopyDir(src.c_str(), dest.c_str()); // Copy Folder (via recursion)
                else
                    FS::CopyFile(src.c_str(), dest.c_str()); // Copy File
            }
        }
        
        delete[] entries;
        fsDirClose(&dir);
        return 0;
    }

    void Copy(FsDirectoryEntry &entry, const std::string &path) {
        std::string full_path = path;
        full_path.append("/");
        full_path.append(entry.name);
        
        if ((std::strncmp(entry.name, "..", 2)) != 0) {
            std::strcpy(fs_copy_struct.copy_path, full_path.c_str());
            std::strcpy(fs_copy_struct.copy_filename, entry.name);
            
            if (entry.type == FsDirEntryType_Dir)
                fs_copy_struct.is_dir = true;
        }
    }

    Result Paste(void) {
        Result ret = 0;
        
        char path[FS_MAX_PATH];
        FS::BuildPath(path, fs_copy_struct.copy_filename);
        
        if (fs_copy_struct.is_dir) // Copy folder recursively
            ret = FS::CopyDir(fs_copy_struct.copy_path, path);
        else // Copy file
            ret = FS::CopyFile(fs_copy_struct.copy_path, path);
            
        std::memset(fs_copy_struct.copy_path, 0, FS_MAX_PATH);
        std::memset(fs_copy_struct.copy_filename, 0, FS_MAX_PATH);
        fs_copy_struct.is_dir = false;
        return ret;
    }

    Result Move(void) {
        Result ret = 0;

        char path[FS_MAX_PATH];
        FS::BuildPath(path, fs_copy_struct.copy_filename);
        
        if (fs_copy_struct.is_dir) {
            if (R_FAILED(ret = fsFsRenameDirectory(fs, fs_copy_struct.copy_path, path))) {
                Log::Error("fsFsRenameDirectory(%s, %s) failed: 0x%x\n", path, fs_copy_struct.copy_filename, ret);
                return ret;
            }
        }
        else {
            if (R_FAILED(ret = fsFsRenameFile(fs, fs_copy_struct.copy_path, path))) {
                Log::Error("fsFsRenameFile(%s, %s) failed: 0x%x\n", path, fs_copy_struct.copy_filename, ret);
                return ret;
            }
        }
        
        std::memset(fs_copy_struct.copy_path, 0, FS_MAX_PATH);
        std::memset(fs_copy_struct.copy_filename, 0, FS_MAX_PATH);
        fs_copy_struct.is_dir = false;
        return 0;
    }
    
    Result GetFreeStorageSpace(s64 &size) {
        Result ret = 0;
        
        if (R_FAILED(ret = fsFsGetFreeSpace(fs, "/", &size))) {
            Log::Error("fsFsGetFreeSpace() failed: 0x%x\n", ret);
            return ret;
        }
        
        return 0;
    }
    
    Result GetTotalStorageSpace(s64 &size) {
        Result ret = 0;
        
        if (R_FAILED(ret = fsFsGetTotalSpace(fs, "/", &size))) {
            Log::Error("fsFsGetTotalSpace() failed: 0x%x\n", ret);
            return ret;
        }
        
        return 0;
    }
    
    Result GetUsedStorageSpace(s64 &size) {
        Result ret = 0;
        s64 free_size = 0, total_size = 0;
        
        if (R_FAILED(ret = FS::GetFreeStorageSpace(free_size)))
            return ret;
            
        if (R_FAILED(ret = FS::GetTotalStorageSpace(total_size)))
            return ret;
        
        size = (total_size - free_size);
        return 0;
    }
}
