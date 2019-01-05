#include <malloc.h>
#include <minizip/unzip.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "archive.h"
#include "common.h"
#include "progress_bar.h"
#include "fs.h"
#include "utils.h"

#include "dmc_unrar.c"

static char *Archive_GetDirPath(char *path) {
	char *e = strrchr(path, '/');

	if (!e) {
		char* buf = strdup(path);
		return buf;
	}

	int index = (int)(e - path);
	char *str = malloc(sizeof(char) * (index + 1));
	strncpy(str, path, index);
	str[index] = '\0';

	return str;
}

static char *Archive_GetFilename(dmc_unrar_archive *archive, size_t i) {
	size_t size = dmc_unrar_get_filename(archive, i, 0, 0);
	if (!size)
		return 0;

	char *filename = malloc(size);
	if (!filename)
		return 0;

	size = dmc_unrar_get_filename(archive, i, filename, size);
	if (!size) {
		free(filename);
		return 0;
	}

	dmc_unrar_unicode_make_valid_utf8(filename);
	if (filename[0] == '\0') {
		free(filename);
		return 0;
	}

	return filename;
}

static char *Archive_RemoveFileExt(char *filename) {
	char *ret, *lastdot;

   	if (filename == NULL)
   		return NULL;
   	if ((ret = malloc(strlen(filename) + 1)) == NULL)
   		return NULL;

   	strcpy(ret, filename);
   	lastdot = strrchr(ret, '.');

   	if (lastdot != NULL)
   		*lastdot = '\0';

   	return ret;
}

static const char *Archive_GetFileExt(const char *filename) {
	const char *ext = strrchr(filename, '.');
	return (ext && ext != filename) ? ext : (filename + strlen(filename));
}

static Result unzExtractCurrentFile(unzFile *unzHandle, int *path) {
	Result res = 0;
	char filename[256];
	unsigned int bufsize = (64 * 1024);

	unz_file_info file_info;
	if ((res = unzGetCurrentFileInfo(unzHandle, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0)) != 0) {
		unzClose(unzHandle);
		return -1;
	}

	void *buf = malloc(bufsize);
	if (!buf)
		return -2;

	char *filenameWithoutPath = Utils_Basename(filename);

	if ((*filenameWithoutPath) == '\0') {
		if ((*path) == 0)
			mkdir(filename, 0777);
	}
	else {
		const char *write;

		if ((*path) == 0)
			write = filename;
		else
			write = filenameWithoutPath;
		
		if ((res = unzOpenCurrentFile(unzHandle)) != UNZ_OK) {
			unzClose(unzHandle);
			free(buf);
			return res;
		}

		FILE *out = fopen(write, "wb");

		if ((out == NULL) && ((*path) == 0) && (filenameWithoutPath != (char *)filename)) {
			char c = *(filenameWithoutPath - 1);
			*(filenameWithoutPath - 1) = '\0';
			mkdir(write, 0777);
			*(filenameWithoutPath - 1) = c;
			out = fopen(write, "wb");
		}

		do {
			res = unzReadCurrentFile(unzHandle, buf, bufsize);

			if (res < 0)
				break;

			if (res > 0)
				fwrite(buf, 1, res, out);
		} 
		while (res > 0);

		fclose(out);

		res = unzCloseCurrentFile(unzHandle);
	}
	
	if (buf)
		free(buf);
	
	return res;
}

static Result unzExtractAll(const char *src, unzFile *unzHandle) {
	Result res = 0;
	int path = 0;
	char *filename = Utils_Basename(src);
	
	unz_global_info global_info;
	memset(&global_info, 0, sizeof(unz_global_info));
	
	if ((res = unzGetGlobalInfo(unzHandle, &global_info)) != UNZ_OK) { // Get info about the zip file.
		unzClose(unzHandle);
		return res;
	}

	for (unsigned int i = 0; i < global_info.number_entry; i++) {
		ProgressBar_DisplayProgress("Extracting", filename, i, global_info.number_entry);

		if ((res = unzExtractCurrentFile(unzHandle, &path)) != UNZ_OK)
			break;

		if ((i + 1) < global_info.number_entry) {
			if ((res = unzGoToNextFile(unzHandle)) != UNZ_OK) { // Could not read next file.
				unzClose(unzHandle);
				return res;
			}
		}
	}

	return res;
}

Result Archive_ExtractZIP(const char *src)
{
	char *path = malloc(256);
	char *dirname_without_ext = Archive_RemoveFileExt((char *)src);

	snprintf(path, 512, "%s/", dirname_without_ext);
	FS_MakeDir(fs, path);
	chdir(path);

	unzFile *unzHandle = unzOpen(src); // Open zip file

	if (unzHandle == NULL) {// not found
		free(path);
		free(dirname_without_ext);
		return -1;
	}

	Result res = unzExtractAll(src, unzHandle);
	res = unzClose(unzHandle);

	return res;
}

Result Archive_ExtractRAR(const char *src) {
	char *path = malloc(256);
	char *dirname_without_ext = Archive_RemoveFileExt((char *)src);

	snprintf(path, 512, "%s/", dirname_without_ext);
	FS_MakeDir(fs, path);
	chdir(path);

	dmc_unrar_archive rar_archive;
	dmc_unrar_return ret;

	ret = dmc_unrar_archive_init(&rar_archive);
	if (ret != DMC_UNRAR_OK) {
		free(path);
		free(dirname_without_ext);
		return -1;
	}

	ret = dmc_unrar_archive_open_path(&rar_archive, src);
	if (ret != DMC_UNRAR_OK) {
		free(path);
		free(dirname_without_ext);
		return -1;
	}

	size_t count = dmc_unrar_get_file_count(&rar_archive);

	for (size_t i = 0; i < count; i++) {
		char *filename = Archive_GetFilename(&rar_archive, i);

		char unrar_path[512];
		snprintf(unrar_path, 512, "%s%s", path, Archive_GetDirPath(filename));

		ProgressBar_DisplayProgress("Extracting", Utils_Basename(filename), i, count);

		if (!FS_DirExists(fs, unrar_path)) {
			if ((strcmp(Archive_GetFileExt(unrar_path), "") == 0) || (dmc_unrar_file_is_directory(&rar_archive, i)))
				FS_MakeDir(fs, unrar_path);
		}
		if (filename && !dmc_unrar_file_is_directory(&rar_archive, i)) {
			dmc_unrar_return supported = dmc_unrar_file_is_supported(&rar_archive, i);
			
			if (supported == DMC_UNRAR_OK) {
				dmc_unrar_return extracted = dmc_unrar_extract_file_to_path(&rar_archive, i, filename, NULL, true);
				
				if (extracted != DMC_UNRAR_OK) {
					free(filename);
					free(path);
					free(dirname_without_ext);
					return -1;
				}

			}
			else {
				free(filename);
				free(path);
				free(dirname_without_ext);
				return -1;
			}
		}

		free(filename);
	}

	free(path);
	free(dirname_without_ext);
	dmc_unrar_archive_close(&rar_archive);
	return 0;
}
