#pragma once

#include <string>
#include <switch.h>
#include <vector>

extern FsFileSystem *fs;
extern FsFileSystem devices[4];

typedef enum FileType {
    FileTypeNone,
    FileTypeArchive,
    FileTypeImage,
    FileTypeText
} FileType;

namespace FS {
    bool FileExists(const char path[FS_MAX_PATH]);
    bool DirExists(const char path[FS_MAX_PATH]);
    Result GetFileSize(const char path[FS_MAX_PATH], s64 &size);
    std::string GetFileExt(const std::string &filename);
    FileType GetFileType(const std::string &filename);
    Result GetDirList(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries);
    Result ChangeDirNext(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries);
    Result ChangeDirPrev(std::vector<FsDirectoryEntry> &entries);
    Result GetTimeStamp(FsDirectoryEntry &entry, FsTimeStampRaw &timestamp);
    Result Rename(FsDirectoryEntry &entry, const char filename[FS_MAX_PATH]);
    Result Delete(FsDirectoryEntry &entry);
    Result SetArchiveBit(FsDirectoryEntry &entry);
    void Copy(FsDirectoryEntry &entry, const std::string &path);
    Result Paste(void);
    Result Move(void);
    Result GetFreeStorageSpace(s64 &size);
    Result GetTotalStorageSpace(s64 &size);
    Result GetUsedStorageSpace(s64 &size);
}
