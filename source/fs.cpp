#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <filesystem>

#include "config.hpp"
#include "fs.hpp"
#include "language.hpp"
#include "log.hpp"
#include "popups.hpp"

// Global vars
FsFileSystem *fs;
FsFileSystem devices[FileSystemMax];
std::string cwd = "/";
std::string device = "sdmc:";

namespace FS {

    typedef struct {
        std::string path;
        std::string filename;
        bool is_directory = false;
    } FSCopyEntry;
    
    FSCopyEntry fs_copy_entry;

    bool FileExists(const std::string &path) {
        struct stat file_stat = { 0 };
        return (stat(path.c_str(), std::addressof(file_stat)) == 0 && S_ISREG(file_stat.st_mode));
    }
    
    bool DirExists(const std::string &path) {
        struct stat dir_stat = { 0 };
        return (stat(path.c_str(), &dir_stat) == 0);
    }

    bool GetFileSize(const std::string &path, std::size_t &size) {
        struct stat file_stat = { 0 };
        std::string full_path = FS::BuildPath(path, true);

        if (stat(full_path.c_str(), std::addressof(file_stat)) != 0) {
            Log::Error("FS::GetFileSize(%s) failed to stat file.\n", full_path.c_str());
            return false;
        }

        size = file_stat.st_size;
        return true;
    }
    
    bool GetDirList(const std::string &device, const std::string &path, std::vector<FsDirectoryEntry> &entries) {
        DIR *dir = nullptr;
        struct dirent *d_entry = nullptr;
        
        std::string full_path = device + path;
        dir = opendir(full_path.c_str());
        entries.clear();

        if (dir) {
            FsDirectoryEntry parent_entry;
            std::memset(std::addressof(parent_entry), 0, sizeof(FsDirectoryEntry));
            std::snprintf(parent_entry.name, 3, "..");
            parent_entry.type = FsDirEntryType_Dir;
            entries.push_back(parent_entry);

            while((d_entry = readdir(dir))) {
                FsDirectoryEntry entry;
                std::memset(std::addressof(entry), 0, sizeof(FsDirectoryEntry));

                std::snprintf(entry.name, FS_MAX_PATH, d_entry->d_name);
                entry.type = (d_entry->d_type & DT_DIR)? FsDirEntryType_Dir : FsDirEntryType_File;
                entry.file_size = 0;

                entries.push_back(entry);
            }

            closedir(dir);
        }
        else {
            Log::Error("FS::GetDirList(%s) to open path.\n", full_path.c_str());
            return false;
        }

        return true;
    }

    static bool ChangeDir(const std::string &path, std::vector<FsDirectoryEntry> &entries) {
        std::vector<FsDirectoryEntry> new_entries;
        const std::string new_path = path;
        cwd = path;
        
        bool ret = FS::GetDirList(device, new_path, new_entries);
        
        entries.clear();
        entries = new_entries;
        return ret;
    }
    
    bool ChangeDirNext(const std::string &path, std::vector<FsDirectoryEntry> &entries) {
        return FS::ChangeDir(FS::BuildPath(path, false), entries);
    }
    
    bool ChangeDirPrev(std::vector<FsDirectoryEntry> &entries) {
        // We are already at the root.
        if (cwd.compare("/") == 0)
            return false;
        
        std::filesystem::path path = cwd;
        std::string parent_path = path.parent_path();
        return FS::ChangeDir(parent_path.empty()? cwd : parent_path, entries);
    }

    bool GetTimeStamp(FsDirectoryEntry &entry, FsTimeStampRaw &timestamp) {
        struct stat file_stat = { 0 };
        std::string full_path = FS::BuildPath(entry);

        if (stat(full_path.c_str(), std::addressof(file_stat)) != 0) {
            Log::Error("FS::GetTimeStamp(%s) failed to stat file.\n", full_path.c_str());
            return false;
        }

        timestamp.is_valid = 1;
        timestamp.created = file_stat.st_ctime;
        timestamp.modified = file_stat.st_mtime;
        timestamp.accessed = file_stat.st_atime;
        return true;
    }

    bool Rename(FsDirectoryEntry &entry, const std::string &dest_path) {
        std::string src_path = FS::BuildPath(entry);
        std::string full_dest_path = FS::BuildPath(dest_path, true);

        if (rename(src_path.c_str(), full_dest_path.c_str()) != 0) {
            Log::Error("FS::Rename(%s, %s) failed.\n", src_path.c_str(), dest_path.c_str());
            return false;
        }
        
        return true;
    }

    bool DeleteRecursive(const std::string &path) {
        DIR *dir = nullptr;
        struct dirent *entry = nullptr;
        dir = opendir(path.c_str());

        if (dir) {
            while((entry = readdir(dir))) {
                std::string filename = entry->d_name;
                if ((filename.compare(".") == 0) || (filename.compare("..") == 0))
                    continue;

                std::string file_path = path;
                file_path.append(path.compare("/") == 0? "" : "/");
                file_path.append(filename);

                if (entry->d_type & DT_DIR) {
                    FS::DeleteRecursive(file_path);
                }
                else {
                    if (remove(file_path.c_str()) != 0) {
                        Log::Error("FS::DeleteRecursive(%s) failed to delete file.\n", file_path.c_str());
                        return false;
                    }
                }
            }

            closedir(dir);
        }
        else {
            Log::Error("FS::DeleteRecursive(%s) failed to open path.\n", path.c_str());
            return false;
        }

        return (rmdir(path.c_str()) == 0);
    }
    
    bool Delete(FsDirectoryEntry &entry) {
        std::string full_path = FS::BuildPath(entry);

        if (entry.type == FsDirEntryType_Dir) {
            if (!FS::DeleteRecursive(full_path)) {
                Log::Error("FS::Delete(%s) failed to delete folder.\n", full_path.c_str());
                return false;
            }
        }
        else {
            if (remove(full_path.c_str()) != 0) {
                Log::Error("FS::Delete(%s) failed to delete file.\n", full_path.c_str());
                return false;
            }
        }
        
        return true;
    }
    
    static bool CopyFile(const std::string &src_path, const std::string &dest_path) {
        FILE *src = fopen(src_path.c_str(), "rb");
        if (!src) {
            Log::Error("FS::CopyFile (%s) failed to open src file.\n", src_path.c_str());
            return false;
        }

        struct stat file_stat = { 0 };
        if (stat(src_path.c_str(), std::addressof(file_stat)) != 0) {
            Log::Error("FS::CopyFile (%s) failed to get src file size.\n", src_path.c_str());
            return false;
        }

        std::size_t size = file_stat.st_size;

        FILE *dest = fopen(dest_path.c_str(), "wb");
        if (!dest) {
            Log::Error("FS::CopyFile (%s) failed to open dest file.\n", dest_path.c_str());
            fclose(src);
            return false;
        }

        std::size_t bytes_read = 0, offset = 0;
        const std::size_t buf_size = 0x10000;
        unsigned char *buf = new unsigned char[buf_size];
        std::string filename = std::filesystem::path(src_path).filename();

        do {
            std::memset(buf, 0, buf_size);

            bytes_read = fread(buf, sizeof(unsigned char), buf_size, src);
            if (bytes_read < 0) {
                Log::Error("FS::CopyFile (%s) failed to read src file.\n", src_path.c_str());
                delete[] buf;
                fclose(src);
                fclose(dest);
                return false;
            }
            
            std::size_t bytes_written = fwrite(buf, sizeof(unsigned char), bytes_read, dest);
            if (bytes_written != bytes_read) {
                Log::Error("FS::CopyFile (%s) failed to write to dest file.\n", dest_path.c_str());
                delete[] buf;
                fclose(src);
                fclose(dest);
                return false;
            }
            
            offset += bytes_read;
            Popups::ProgressBar(static_cast<float>(offset), static_cast<float>(size), strings[cfg.lang][Lang::OptionsCopying], filename.c_str());
        } while (offset < size);

        delete[] buf;
        fclose(src);
        fclose(dest);
        return true;
        return 0;
    }

    static bool CopyDir(const std::string &src_path, const std::string &dest_path) {
        DIR *dir = nullptr;
        struct dirent *entry = nullptr;
        dir = opendir(src_path.c_str());

        if (dir) {
            // This may fail or not, but we don't care -> make the dir if it doesn't exist, otherwise continue.
            mkdir(dest_path.c_str(), 0700);

            while((entry = readdir(dir))) {
                std::string filename = entry->d_name;
                if ((filename.compare(".") == 0) || (filename.compare("..") == 0))
                    continue;

                std::string src = src_path;
                src.append("/");
                src.append(filename);

                std::string dest = dest_path;
                dest.append("/");
                dest.append(filename);

                if (entry->d_type & DT_DIR)
                    FS::CopyDir(src.c_str(), dest.c_str()); // Copy Folder (via recursion)
                else
                    FS::CopyFile(src.c_str(), dest.c_str()); // Copy File
            }

            closedir(dir);
        }
        else {
            Log::Error("FS::CopyDir(%s) failed to open path.\n", src_path.c_str());
            return false;
        }

        return true;
    }

    void Copy(FsDirectoryEntry &entry, const std::string &path) {
        std::string full_path = path;
        full_path.append(path.compare("/") == 0? "" : "/");
        full_path.append(entry.name);
        
        if ((std::strncmp(entry.name, "..", 2)) != 0) {
            fs_copy_entry.path = full_path;
            fs_copy_entry.filename = entry.name;
            
            if (entry.type == FsDirEntryType_Dir)
                fs_copy_entry.is_directory = true;
        }
    }

    bool Paste(void) {
        bool ret = false;
        std::string path = FS::BuildPath(fs_copy_entry.filename, true);
        
        if (fs_copy_entry.is_directory)
            ret = FS::CopyDir(fs_copy_entry.path, path);
        else
            ret = FS::CopyFile(fs_copy_entry.path, path);

        fs_copy_entry = {};
        return ret;
    }

    bool Move(void) {
        std::string path = FS::BuildPath(fs_copy_entry.filename, true);

        if (rename(fs_copy_entry.path.c_str(), path.c_str()) != 0) {
            Log::Error("FS::Move(%s, %s) failed.\n", fs_copy_entry.path.c_str(), path.c_str());
            return false;
        }

        fs_copy_entry = {};
        return true;
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
    
    Result SetArchiveBit(const std::string &path) {
        Result ret = 0;

        char fs_path[FS_MAX_PATH];
        std::snprintf(fs_path, FS_MAX_PATH, path.c_str());

        if (R_FAILED(ret = fsFsSetConcatenationFileAttribute(std::addressof(devices[FileSystemSDMC]), fs_path))) {
            Log::Error("fsFsSetConcatenationFileAttribute(%s) failed: 0x%x\n", path.c_str(), ret);
            return ret;
        }
        
        return 0;
    }
    
    Result GetFreeStorageSpace(s64 &size) {
        Result ret = 0;
        
        if (R_FAILED(ret = fsFsGetFreeSpace(fs, "/", std::addressof(size)))) {
            Log::Error("fsFsGetFreeSpace() failed: 0x%x\n", ret);
            return ret;
        }
        
        return 0;
    }
    
    Result GetTotalStorageSpace(s64 &size) {
        Result ret = 0;
        
        if (R_FAILED(ret = fsFsGetTotalSpace(fs, "/", std::addressof(size)))) {
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

    std::string GetFileExt(const std::string &filename) {
        std::string ext = std::filesystem::path(filename).extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext;
    }

    std::string BuildPath(FsDirectoryEntry &entry) {
        std::string path_next = device;
        path_next.append(cwd);
        path_next.append((cwd.compare("/") == 0)? "" : "/");
        path_next.append(entry.name);
        return path_next;
    }

    std::string BuildPath(const std::string &path, bool device_name) {
        std::string path_next = "";

        if (device_name)
            path_next.append(device);
        
        path_next.append(cwd);
        path_next.append((cwd.compare("/") == 0)? "" : "/");
        path_next.append(path);
        return path_next;
    }
}
