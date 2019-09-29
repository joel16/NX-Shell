#include <archive.h>
#include <archive_entry.h>
#include <malloc.h>
#include <string.h>
#include <switch.h>

#include "common.h"
#include "dialog.h"
#include "fs.h"
#include "menu_error.h"
#include "SDL_helper.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

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

static u64 Archive_CountFiles(const char *path) {
	int ret = 0;
	u64 count = 0;
	
	struct archive *handle = archive_read_new();
	ret = archive_read_support_format_all(handle);
	ret = archive_read_open_filename(handle, path, 1024);
	
	if (ret != ARCHIVE_OK) {
		Menu_DisplayError("archive_read_open_filename :", ret);
		return 1;
	}
	
	for (;;) {
		struct archive_entry *entry = NULL;
		int ret = archive_read_next_header(handle, &entry);
		if (ret == ARCHIVE_EOF)
			break;
		
		count++;
	}
	
	ret = archive_read_free(handle);
	return count;
}

static int Archive_WriteData(struct archive *src, struct archive *dst) {
	int ret = 0;
	
	for (;;) {
		const void *chunk = NULL;
		size_t length = 0;
		s64 offset = 0;
		
		ret = archive_read_data_block(src, &chunk, &length, &offset);
		if (ret == ARCHIVE_EOF)
			return ARCHIVE_OK;
		
		if (ret != ARCHIVE_OK)
			return ret;
			
		ret = archive_write_data_block(dst, chunk, length, offset);
		if (ret != ARCHIVE_OK)
			return ret;
	}
	
	return -1;
}

int Archive_ExtractArchive(const char *path) {
	char *dest_path = malloc(256);
	char *dirname_without_ext = Archive_RemoveFileExt((char *)path);
	
	snprintf(dest_path, 512, "%s/", dirname_without_ext);
	FS_MakeDir(fs, dest_path);
	
	int count = 0;
	u64 max = Archive_CountFiles(path);
	
	int ret = 0;
	struct archive *handle = archive_read_new();
	struct archive *dst = archive_write_disk_new();
	
	ret = archive_read_support_format_all(handle);
	ret = archive_read_open_filename(handle, path, 1024);
	if (ret != ARCHIVE_OK) {
		Menu_DisplayError("archive_read_open_filename :", ret);
		return 1;
	}
	
	for (;;) {
		Dialog_DisplayProgress("Extracting", path, count, max);
		
		struct archive_entry *entry = NULL;
		ret = archive_read_next_header(handle, &entry);
		if (ret == ARCHIVE_EOF)
			break;
			
		if (ret != ARCHIVE_OK) {
			Menu_DisplayError("archive_read_next_header failed:", ret);
			
			if (ret != ARCHIVE_WARN)
				break;
		}
		
		const char *entry_path = archive_entry_pathname(entry);
		char new_path[1024];

		ret = snprintf(new_path, sizeof(new_path), "%s/%s", dest_path, entry_path);
		ret = archive_entry_update_pathname_utf8(entry, new_path);
		if (!ret) {
			Menu_DisplayError("archive_entry_update_pathname_utf8:", ret);
			break;
		}
		
		ret = archive_write_disk_set_options(dst, ARCHIVE_EXTRACT_UNLINK);
		ret = archive_write_header(dst, entry);
		if (ret != ARCHIVE_OK) {
			Menu_DisplayError("archive_write_header failed:", ret);
			break;
		}
		
		ret = Archive_WriteData(handle, dst);
		ret = archive_write_finish_entry(dst);
		count++;
	}

	appletSetMediaPlaybackState(false);
	
	ret = archive_write_free(dst);
	ret = archive_read_free(handle);
	free(dest_path);
	free(dirname_without_ext);
	return ret;
}

int Archive_ExtractFile(const char *path) {
	int dialog_selection = 1;

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	int dialog_width = 0, dialog_height = 0;
	u32 confirm_width = 0, confirm_height = 0, cancel_width = 0, cancel_height = 0;
	SDL_GetTextDimensions(25, "YES", &confirm_width, &confirm_height);
	SDL_GetTextDimensions(25, "NO", &cancel_width, &cancel_height);
	SDL_QueryTexture(dialog, NULL, NULL, &dialog_width, &dialog_height);

	while(appletMainLoop()) {
		Dialog_DisplayPrompt("Extract file", "This may take a few minutes.", "Do you want to continue?", &dialog_selection, true);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_RIGHT) || (kDown & KEY_LSTICK_RIGHT) || (kDown & KEY_RSTICK_RIGHT))
			dialog_selection++;
		else if ((kDown & KEY_LEFT) || (kDown & KEY_LSTICK_LEFT) || (kDown & KEY_RSTICK_LEFT))
			dialog_selection--;

		Utils_SetMax(&dialog_selection, 0, 1);
		Utils_SetMin(&dialog_selection, 1, 0);

		if (kDown & KEY_B)
			break;
		
		if (kDown & KEY_A) {
			if (dialog_selection == 0) {
				appletSetMediaPlaybackState(true);
				return Archive_ExtractArchive(path);
			}
			else
				break;
		}
		
		if (touchInfo.state == TouchStart) {
			// Confirm Button
			if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width, 
				(720 - dialog_height) / 2 + 265 + confirm_height))
				dialog_selection = 0;
			// Cancel Button
			else if (tapped_inside(touchInfo, 895 - confirm_width, (720 - dialog_height) / 2 + 225, 935 + confirm_width,
				(720 - dialog_height) / 2 + 265 + cancel_height))
				dialog_selection = 1;
		}
		else if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone) {
			// Touched outside
			if (tapped_outside(touchInfo, (1280 - dialog_width) / 2, (720 - dialog_height) / 2, (1280 + dialog_width) / 2,
				(720 + dialog_height) / 2))
				break;
			// Confirm Button
			else if (tapped_inside(touchInfo, 1010 - confirm_width, (720 - dialog_height) / 2 + 225, 1050 + confirm_width,
				(720 - dialog_height) / 2 + 265 + confirm_height))
				return Archive_ExtractArchive(path);
			// Cancel Button
			else if (tapped_inside(touchInfo, 895 - confirm_width, (720 - dialog_height) / 2 + 225, 935 + confirm_width,
				(720 - dialog_height) / 2 + 265 + cancel_height))
				break;
		}
	}

	return -1;
}
