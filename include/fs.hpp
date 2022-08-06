#pragma once

#include <string>
#include <switch.h>
#include <vector>

typedef enum FileType {
    FileTypeNone,
    FileTypeArchive,
    FileTypeImage,
    FileTypeText
} FileType;

typedef enum FileSystemDevices {
    FileSystemSDMC,
    FileSystemSafe,
    FileSystemUser,
    FileSystemSystem,
    FileSystemMax
} FileSystemDevices;

extern FsFileSystem *fs;
extern FsFileSystem devices[FileSystemMax];

namespace FS {
    bool FileExists(const std::string &path);
    bool DirExists(const std::string &path);
    bool GetFileSize(const std::string &path, std::size_t &size);
    bool GetDirList(const std::string &device, const std::string &path, std::vector<FsDirectoryEntry> &entries);
    bool ChangeDirNext(const std::string &path, std::vector<FsDirectoryEntry> &entries);
    bool ChangeDirPrev(std::vector<FsDirectoryEntry> &entries);
    bool GetTimeStamp(FsDirectoryEntry &entry, FsTimeStampRaw &timestamp);
    bool Rename(FsDirectoryEntry &entry, const std::string &dest_path);
    bool Delete(FsDirectoryEntry &entry);
    void Copy(FsDirectoryEntry &entry, const std::string &path);
    bool Paste(void);
    bool Move(void);
    FileType GetFileType(const std::string &filename);
    Result SetArchiveBit(const std::string &path);
    Result GetFreeStorageSpace(s64 &size);
    Result GetTotalStorageSpace(s64 &size);
    Result GetUsedStorageSpace(s64 &size);
    std::string BuildPath(FsDirectoryEntry &entry);
    std::string BuildPath(const std::string &path, bool device_name);
    std::string GetFileExt(const std::string &filename);
}
