#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "config.h"
#include "fs.h"
#include "log.h"

// Global vars
FsFileSystem *fs;
FsFileSystem devices[4];

namespace FS {
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

    static bool Sort(const FsDirectoryEntry &entryA, const FsDirectoryEntry &entryB) {
		if ((entryA.type == FsDirEntryType_Dir) && !(entryB.type == FsDirEntryType_Dir))
			return true;
		else if (!(entryA.type == FsDirEntryType_Dir) && (entryB.type == FsDirEntryType_Dir))
			return false;
		else {
			switch(cfg.sort) {
				case 0: // Sort alphabetically (ascending - A to Z)
					if (strcasecmp(entryA.name, entryB.name) < 0)
						return true;
					
					break;
				
				case 1: // Sort alphabetically (descending - Z to A)
					if (strcasecmp(entryB.name, entryA.name) < 0)
						return true;
					
					break;
				
				case 2: // Sort by file size (largest first)
					if (entryB.file_size < entryA.file_size)
						return true;
					
					break;
					
				case 3: // Sort by file size (smallest first)
					if (entryA.file_size < entryB.file_size)
						return true;
					
					break;
			}
		}
		
		return false;
	}

	Result GetDirList(const char path[FS_MAX_PATH], std::vector<FsDirectoryEntry> &entries) {
		FsDir dir;
		Result ret = 0;
        s64 read_entries = 0;
        const std::string cwd = path;

        if (strncmp(path, "/", std::strlen(path))) {
            FsDirectoryEntry entry;
            std::strncpy(entry.name, "..", 3);
            entry.type = FsDirEntryType_Dir;
            entry.file_size = 0;
            entries.push_back(entry);
        }
		
		if (R_FAILED(ret = fsFsOpenDirectory(fs, path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir))) {
			Log::Error("fsFsOpenDirectory(%s) failed: 0x%x\n", path, ret);
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

		std::sort(entries.begin(), entries.end(), FS::Sort);
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
		char new_cwd[FS_MAX_PATH + 1];
		const char *sep = !std::strncmp(cfg.cwd, "/", 2) ? "" : "/"; // Don't append / if at /
		
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
