#ifndef NX_SHELL_FS_H
#define NX_SHELL_FS_H

#include <string>
#include <switch.h>

extern FsFileSystem *fs;
extern FsFileSystem devices[4];

typedef enum FileType {
    FileTypeNone,
    FileTypeArchive,
    FileTypeAudio,
    FileTypeImage,
    FileTypeText
} FileType;

namespace FS {
    bool FileExists(const char path[FS_MAX_PATH]);
    bool DirExists(const char path[FS_MAX_PATH]);
    std::string GetFileExt(const std::string &filename);
    FileType GetFileType(const std::string &filename);
    s64 GetDirList(char path[FS_MAX_PATH], FsDirectoryEntry **entriesp);
    void FreeDirEntries(FsDirectoryEntry **entries, s64 entry_count);
    s64 RefreshEntries(FsDirectoryEntry **entries, s64 entry_count);
    s64 ChangeDirNext(const char path[FS_MAX_PATH], FsDirectoryEntry **entries, s64 entry_count);
    s64 ChangeDirPrev(FsDirectoryEntry **entries, s64 entry_count);
    Result GetTimeStamp(FsDirectoryEntry *entry, FsTimeStampRaw *timestamp);
    Result Rename(FsDirectoryEntry *entry, const char filename[FS_MAX_PATH]);
    Result Delete(FsDirectoryEntry *entry);
    Result SetArchiveBit(FsDirectoryEntry *entry);
    void Copy(FsDirectoryEntry *entry, const std::string &path);
    Result Paste(void);
    Result Move(void);
    Result GetFileSize(const char path[FS_MAX_PATH], s64 *size);
    Result WriteFile(const char path[FS_MAX_PATH], const void *buf, u64 size);
    Result GetFreeStorageSpace(s64 *size);
    Result GetTotalStorageSpace(s64 *size);
    Result GetUsedStorageSpace(s64 *size);
}

#endif
