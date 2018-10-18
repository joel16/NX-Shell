#include "archive.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_gallery.h"
#include "menu_music.h"
#include "menu_book_reader.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

int initialPosition = 0;
int position = 0; // menu position
int fileCount = 0; // file count
File *files = NULL; // file list

void Dirbrowse_RecursiveFree(File *node) {
	if (node == NULL) // End of list
		return;
	
	Dirbrowse_RecursiveFree(node->next); // Nest further
	free(node); // Free memory
}

static int cmpstringp(const void *p1, const void *p2) {
	FsDirectoryEntry* entryA = (FsDirectoryEntry*) p1;
	FsDirectoryEntry* entryB = (FsDirectoryEntry*) p2;
	u64 sizeA = 0, sizeB = 0;
	
	if ((entryA->type == ENTRYTYPE_DIR) && !(entryB->type == ENTRYTYPE_DIR))
		return -1;
	else if (!(entryA->type == ENTRYTYPE_DIR) && (entryB->type == ENTRYTYPE_DIR))
		return 1;
	else {
		switch(config.sort) {
			case 0: // Sort alphabetically (ascending - A to Z)
				return strcasecmp(entryA->name, entryB->name);
				break;
			
			case 1: // Sort alphabetically (descending - Z to A)
				return strcasecmp(entryB->name, entryA->name);
				break;
			
			case 2: // Sort by file size (largest first)
				sizeA = entryA->fileSize;
				sizeB = entryB->fileSize;
				return sizeA > sizeB? -1 : sizeA < sizeB? 1 : 0;
				break;
			
			case 3: // Sort by file size (smallest first)
				sizeA = entryA->fileSize;
				sizeB = entryB->fileSize;
				return sizeB > sizeA? -1 : sizeB < sizeA? 1 : 0;
				break;
		}
	}
	
	return 0;
}

Result Dirbrowse_PopulateFiles(bool clear) {
	Dirbrowse_RecursiveFree(files);
	files = NULL;
	fileCount = 0;
	
	FsDir dir;
	Result ret = 0;

	if (R_SUCCEEDED(ret = fsFsOpenDirectory(&fs, cwd, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir))) {
		/* Add fake ".." entry except on root */
		if (strcmp(cwd, ROOT_PATH)) {
			files = (File *)malloc(sizeof(File)); // New list
			memset(files, 0, sizeof(File)); // Clear memory
			strcpy(files->name, ".."); // Copy file Name
			files->isDir = 1; // Set folder flag
			fileCount++;
		}
		
		u64 entryCount = 0;
		if (R_FAILED(ret = fsDirGetEntryCount(&dir, &entryCount)))
			return ret;
		
		FsDirectoryEntry *entries = (FsDirectoryEntry*)calloc(entryCount + 1, sizeof(FsDirectoryEntry));

		if (R_SUCCEEDED(ret = fsDirRead(&dir, 0, NULL, entryCount, entries))) {
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), cmpstringp);
			
			for (u32 i = 0; i < entryCount; i++) {		
				if (!strncmp(entries[i].name, ".", 1)) // Ignore "." in all Directories
					continue;
				
				if ((!strcmp(cwd, ROOT_PATH)) && (!strncmp(entries[i].name, "..", 2))) // Ignore ".." in Root Directory
					continue;
					
				File *item = (File *)malloc(sizeof(File));
				memset(item, 0, sizeof(File));

				strcpy(item->name, entries[i].name);
				strcpy(item->ext, FS_GetFileExt(item->name));

				item->size = entries[i].fileSize;
				item->isDir = entries[i].type == ENTRYTYPE_DIR;
				
				if (files == NULL) // New list
					files = item;
				
				// Existing list
				else {
					File *list = files;
					
					while(list->next != NULL)  // Append to list
						list = list->next;
					
					list->next = item; // Link item
				}
				
				fileCount++; // Increment file count
			}

			free(entries);
			fsDirClose(&dir); // Close directory
		}	
		else {
			free(entries);
			return ret;
		}
	}
	else
		return ret;
		
	// Attempt to keep index
	if (!clear) {
		if (position >= fileCount)
			position = fileCount - 1; // Fix position
	}
	else
		position = 0; // Reset position
	
	return 0;
}

void Dirbrowse_DisplayFiles(void) {
	u32 title_height = 0;
	SDL_GetTextDimensions(30, cwd, NULL, &title_height);
	SDL_DrawImage(icon_nav_drawer, 20, 58);
	SDL_DrawImage(icon_actions, (1260 - 64), 58);
	SDL_DrawText(170, 40 + ((100 - title_height) / 2), 30, WHITE, cwd);

	int i = 0, printed = 0;
	File *file = files; // Draw file list

	for(; file != NULL; file = file->next) {
		if (printed == FILES_PER_PAGE) // Limit the files per page
			break;

		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE)) {
			if (i == position)
				SDL_DrawRect(0, 140 + (73 * printed), 1280, 73, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

			if (!strcmp(multi_select_dir, cwd)) {
				multi_select[i] == true? SDL_DrawImage(config.dark_theme? icon_check_dark : icon_check, 20, 156 + (73 * printed)) : 
					SDL_DrawImage(config.dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed));
			}
			else
				SDL_DrawImage(config.dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed));

			char path[512];
			strcpy(path, cwd);
			strcpy(path + strlen(path), file->name);

			if (file->isDir)
				SDL_DrawImageScale(config.dark_theme? icon_dir_dark : icon_dir, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "nro", 3)) || (!strncasecmp(file->ext, "elf", 3)) || (!strncasecmp(file->ext, "bin", 3)))
				SDL_DrawImageScale(icon_app, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "zip", 3)) || (!strncasecmp(file->ext, "tar", 3)) || (!strncasecmp(file->ext, "lz4", 3)))
				SDL_DrawImageScale(icon_archive, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "mp3", 3)) || (!strncasecmp(file->ext, "ogg", 3)) || (!strncasecmp(file->ext, "wav", 3)) || (!strncasecmp(file->ext, "mod", 3))
					|| (!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "midi", 4)) || (!strncasecmp(file->ext, "mid", 3)))
				SDL_DrawImageScale(icon_audio, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "png", 3)) || (!strncasecmp(file->ext, "jpg", 3)) || (!strncasecmp(file->ext, "bmp", 3)) || (!strncasecmp(file->ext, "gif", 3)))
				SDL_DrawImageScale(icon_image, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "txt", 3)) || (!strncasecmp(file->ext, "lua", 3)) || (!strncasecmp(file->ext, "cfg", 3)))
				SDL_DrawImageScale(icon_text, 80, 141 + (73 * printed), 72, 72);
			else if ((!strncasecmp(file->ext, "pdf", 3)) || (!strncasecmp(file->ext, "cbz", 3)) || (!strncasecmp(file->ext, "fb2", 3)) || (!strncasecmp(file->ext, "epub", 4)))
				SDL_DrawImageScale(icon_doc, 80, 141 + (73 * printed), 72, 72);
			else
				SDL_DrawImageScale(icon_file, 80, 141 + (73 * printed), 72, 72);

			char buf[64], size[16];
			strncpy(buf, file->name, sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			if (!file->isDir) {
				Utils_GetSizeString(size, file->size);
				u32 width = 0;
				SDL_GetTextDimensions(20, size, &width, NULL);
				SDL_DrawText(1260 - width, 180 + (73 * printed), 20, config.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, size);
			}

			u32 height = 0;
			SDL_GetTextDimensions(25, buf, NULL, &height);
			
			if (strncmp(file->name, "..", 2) == 0)
				SDL_DrawText(170, 140 + ((73 - height)/2) + (73 * printed), 25, config.dark_theme? WHITE : BLACK, "Parent folder");
			else 
				SDL_DrawText(170, 140 + ((73 - height)/2) + (73 * printed), 25, config.dark_theme? WHITE : BLACK, buf);

			printed++; // Increase printed counter
		}

		i++; // Increase counter
	}
}

static Result Dirbrowse_SaveLastDirectory(void) {
	Result ret = 0;

	if (R_FAILED(ret = FS_Write("/switch/NX-Shell/lastdir.txt", cwd)))
		return ret;

	return 0;
}

// Get file index
File *Dirbrowse_GetFileIndex(int index) {
	int i = 0;
	File *file = files; // Find file Item
	
	for(; file != NULL && i != index; file = file->next)
		i++;

	return file; // Return file
}

/**
 * Executes an operation on the file depending on the filetype.
 */
void Dirbrowse_OpenFile(void) {
	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);

	if (file == NULL)
		return;
	
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->isDir) {
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(false))) {
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(true);
		}
	}
	else if ((!strncasecmp(file->ext, "png", 3)) || (!strncasecmp(file->ext, "jpg", 3)) || (!strncasecmp(file->ext, "bmp", 3)) || (!strncasecmp(file->ext, "gif", 3)))
		Gallery_DisplayImage(path);
	else if (!strncasecmp(file->ext, "zip", 3)) {
		Archive_ExtractZip(path, cwd);
		Dirbrowse_PopulateFiles(true);
	}
	else if ((!strncasecmp(file->ext, "mp3", 3)) || (!strncasecmp(file->ext, "ogg", 3)) || (!strncasecmp(file->ext, "wav", 3)) || (!strncasecmp(file->ext, "mod", 3))
			|| (!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "midi", 4)) || (!strncasecmp(file->ext, "mid", 3)))
		Menu_PlayMusic(path);
	else if ((!strncasecmp(file->ext, "pdf", 3)) || (!strncasecmp(file->ext, "cbz", 3)) || (!strncasecmp(file->ext, "fb2", 3)) || (!strncasecmp(file->ext, "epub", 4)))
		Menu_OpenBook(path);

	/*else if (!strncasecmp(file->ext, "txt", 3))
		TextViewer_DisplayText(path);*/
}

// Navigate to Folder
int Dirbrowse_Navigate(bool parent) {
	File *file = Dirbrowse_GetFileIndex(position); // Get index
	
	if (file == NULL)
		return -1;

	// Special case ".."
	if ((parent) || (!strncmp(file->name, "..", 2))) {
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--) {
			// Slash discovered
			if (cwd[i] == '/') {
				slash = cwd + i + 1; // Save pointer
				break; // Stop search
			}
		}

		slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else {
		if (file->isDir) {
			// Append folder to working directory
			strcpy(cwd + strlen(cwd), file->name);
			cwd[strlen(cwd) + 1] = 0;
			cwd[strlen(cwd)] = '/';
		}
	}

	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}
