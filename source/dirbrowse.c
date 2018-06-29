#include <dirent.h>

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

#define TYPE_DIR(n) (n == DT_DIR ? 1 : 0)

int initialPosition = 0;
int position = 0; // menu position
int fileCount = 0; // file count
File *files = NULL; // file list

void Dirbrowse_RecursiveFree(File *node)
{
	if (node == NULL) // End of list
		return;
	
	Dirbrowse_RecursiveFree(node->next); // Nest further
	free(node); // Free memory
}

// Sort directories alphabetically. Folder first, then files.
static int cmpstringp(const void *p1, const void *p2)
{
	FsDirectoryEntry* entryA = (FsDirectoryEntry*) p1;
	FsDirectoryEntry* entryB = (FsDirectoryEntry*) p2;
	
	u64 sizeA = 0, sizeB = 0;
	
	if ((entryA->type == ENTRYTYPE_DIR) && !(entryB->type == ENTRYTYPE_DIR))
		return -1;
	else if (!(entryA->type == ENTRYTYPE_DIR) && (entryB->type == ENTRYTYPE_DIR))
		return 1;
	else
	{
		switch(config_sort_by)
		{
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

Result Dirbrowse_PopulateFiles(bool clear)
{
	Dirbrowse_RecursiveFree(files);
	files = NULL;
	fileCount = 0;
	
	FsDir dir;
	Result ret = 0;
	
	if (R_SUCCEEDED(ret = fsFsOpenDirectory(&fs, cwd, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir)))
	{
		/* Add fake ".." entry except on root */
		if (strcmp(cwd, ROOT_PATH))
		{
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
		
		if (R_SUCCEEDED(ret = fsDirRead(&dir, 0, NULL, entryCount, entries)))
		{
			qsort(entries, entryCount, sizeof(FsDirectoryEntry), cmpstringp);
			
			for (u32 i = 0; i < entryCount; i++) 
			{		
				if (strncmp(entries[i].name, ".", 1) == 0) // Ignore "." in all Directories
					continue;
				
				if (strcmp(cwd, ROOT_PATH) == 0 && strncmp(entries[i].name, "..", 2) == 0) // Ignore ".." in Root Directory
					continue;
					
				File *item = (File *)malloc(sizeof(File)); // Allocate memory
				memset(item, 0, sizeof(File)); // Clear memory
				strcpy(item->name, entries[i].name); // Copy file name
				item->size = entries[i].fileSize; // Copy file size
				item->isDir = entries[i].type == ENTRYTYPE_DIR; // Set folder flag
				
				if (files == NULL) // New list
					files = item;
				
				// Existing list
				else
				{
					File *list = files;
					
					while(list->next != NULL)  // Append to list
						list = list->next;
					
					list->next = item; // Link item
				}
				
				fileCount++; // Increment file count
			}
		}	
		else
		{
			free(entries);
			return ret;
		}
		
		free(entries);
		fsDirClose(&dir); // Close directory
	}
	else
		return ret;
		
	// Attempt to keep index
	if (!clear)
	{
		if (position >= fileCount)
			position = fileCount - 1; // Fix position
	}
	else
		position = 0; // Reset position
	
	return 0;
}

void Dirbrowse_DisplayFiles(void)
{
	int title_height = 0;
	SDL_DrawImage(RENDERER, icon_nav_drawer, 20, 58, 64, 64);
	SDL_DrawImage(RENDERER, icon_actions, (1260 - 64), 58, 64, 64);
	TTF_SizeText(Roboto_large, cwd, NULL, &title_height);
	SDL_DrawText(Roboto_large, 170, 40 + ((100 - title_height)/2), WHITE, cwd);

	int i = 0, printed = 0;
	File *file = files; // Draw file list

	for(; file != NULL; file = file->next)
	{
		if (printed == FILES_PER_PAGE) // Limit the files per page
			break;

		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE))
		{
			if (i == position)
				SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

			if (strcmp(multi_select_dir, cwd) == 0)
			{
				multi_select[i] == true? SDL_DrawImage(RENDERER, config_dark_theme? icon_check_dark : icon_check, 20, 156 + (73 * printed), 40, 40) : 
					SDL_DrawImage(RENDERER, config_dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed), 40, 40);
			}
			else
				SDL_DrawImage(RENDERER, config_dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed), 40, 40);

			char path[500];
			strcpy(path, cwd);
			strcpy(path + strlen(path), file->name);

			if (file->isDir)
				SDL_DrawImage(RENDERER, config_dark_theme? icon_dir_dark : icon_dir, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "nro", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "elf", 3) == 0)
				|| (strncasecmp(FS_GetFileExt(file->name), "bin", 3) == 0))
				SDL_DrawImage(RENDERER, icon_app, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "tar", 3) == 0)
				|| (strncasecmp(FS_GetFileExt(file->name), "lz4", 3) == 0))
				SDL_DrawImage(RENDERER, icon_archive, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "ogg", 3) == 0)
				|| (strncasecmp(FS_GetFileExt(file->name), "wav", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "mod", 3) == 0))
				SDL_DrawImage(RENDERER, icon_audio, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0) || 
				(strncasecmp(FS_GetFileExt(file->name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "gif", 3) == 0))
				SDL_DrawImage(RENDERER, icon_image, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "txt", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "lua", 3) == 0) 
                || (strncasecmp(FS_GetFileExt(file->name), "cfg", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "pdf", 3) == 0)
                || (strncasecmp(FS_GetFileExt(file->name), "cbz", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "fb2", 3) == 0)
                || (strncasecmp(FS_GetFileExt(file->name), "epub", 4) == 0))
				SDL_DrawImage(RENDERER, icon_text, 80, 141 + (73 * printed), 72, 72);
			else
				SDL_DrawImage(RENDERER, icon_file, 80, 141 + (73 * printed), 72, 72);

			char buf[64], size[16];
			strncpy(buf, file->name, sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			if (!file->isDir)
			{
				Utils_GetSizeString(size, file->size);
				int width = 0;
				TTF_SizeText(Roboto_small, size, &width, NULL);
				SDL_DrawText(Roboto_small, 1260 - width, 180 + (73 * printed), config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, size);
			}

			int height = 0;
			TTF_SizeText(Roboto, buf, NULL, &height);
			
			if (strncmp(file->name, "..", 2) == 0)
				SDL_DrawText(Roboto, 170, 140 + ((73 - height)/2) + (73 * printed), BLACK, "Parent folder");
			else 
				SDL_DrawText(Roboto, 170, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, buf);

			printed++; // Increase printed counter
		}

		i++; // Increase counter
	}
}

static void Dirbrowse_SaveLastDirectory(void)
{
	char *buf = (char *)malloc(256);
	strcpy(buf, cwd);

	FILE *write = fopen("/switch/NX-Shell/lastdir.txt", "w");
	fprintf(write, "%s", buf);
	fclose(write);
	free(buf);
}

// Get file index
File *Dirbrowse_GetFileIndex(int index)
{
	int i = 0;
	File *file = files; // Find file Item
	
	for(; file != NULL && i != index; file = file->next)
		i++;

	return file; // Return file
}

/**
 * Executes an operation on the file depending on the filetype.
 */
void Dirbrowse_OpenFile(void)
{
	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);

	if (file == NULL)
		return;

	strcpy(fileName, file->name);
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->isDir)
	{
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(0)))
		{
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(true);
		}
	}
	else if ((strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0) || 
		(strncasecmp(FS_GetFileExt(file->name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "gif", 3) == 0))
		Gallery_DisplayImage(path);
	else if (strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0)
	{
		Archive_ExtractZip(path, cwd);
		Dirbrowse_PopulateFiles(true);
	}
	else if ((strncasecmp(FS_GetFileExt(file->name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "ogg", 3) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "wav", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "mod", 3) == 0))
		Menu_PlayMusic(path);
    else if ((strncasecmp(FS_GetFileExt(file->name), "pdf", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "cbz", 3) == 0)
            || (strncasecmp(FS_GetFileExt(file->name), "fb2", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "epub", 4) == 0))
        Menu_OpenBook(path);

	/*else if (strncasecmp(file->ext, "txt", 3) == 0)
		TextViewer_DisplayText(path);*/
}

// Navigate to Folder
int Dirbrowse_Navigate(int dir)
{
	File *file = Dirbrowse_GetFileIndex(position); // Get index
	
	if ((file == NULL) || (!file->isDir)) // Not a folder
		return -1;

	// Special case ".."
	if ((dir == -1) || (strncmp(file->name, "..", 2) == 0))
	{
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--)
		{
			// Slash discovered
			if (cwd[i] == '/')
			{
				slash = cwd + i + 1; // Save pointer
				break; // Stop search
			}
		}

		slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else
	{
		// Append folder to working directory
		strcpy(cwd + strlen(cwd), file->name);
		cwd[strlen(cwd) + 1] = 0;
		cwd[strlen(cwd)] = '/';
	}

	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}