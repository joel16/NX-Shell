#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <switch.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "progress_bar.h"
#include "menu_options.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

/*
*	Copy Mode
*	-1 : Nothing
*	0  : Copy
*	1  : Move
*/
static int copymode = NOTHING_TO_COPY;
/*
*	Copy Move Origin
*/
static char copysource[1024];

static int delete_dialog_selection = 0, row = 0, column = 0;
static bool copy_status = false, cut_status = false;

static int FileOptions_RmdirRecursive(char *path)
{
	File *filelist = NULL;
	DIR *directory = opendir(path);

	if (directory)
	{
		struct dirent *entries;

		while ((entries = readdir(directory)) != NULL)
		{
			if (strlen(entries->d_name) > 0)
			{
				if (strcmp(entries->d_name, ".") == 0 || strcmp(entries->d_name, "..") == 0)
					continue;

				// Allocate Memory
				File *item = (File *)malloc(sizeof(File));
				memset(item, 0, sizeof(File));

				// Copy File Name
				strcpy(item->name, entries->d_name);

				// Set Folder Flag
				item->isDir = entries->d_type == DT_DIR;

				// New List
				if (filelist == NULL) 
					filelist = item;

				// Existing List
				else
				{
					File *list = filelist;

					while(list->next != NULL) 
						list = list->next;

					list->next = item;
				}
			}
		}
	}

	closedir(directory);

	File *node = filelist;

	// Iterate Files
	for(; node != NULL; node = node->next)
	{
		// Directory
		if (node->isDir)
		{
			// Required Buffer Size
			int size = strlen(path) + strlen(node->name) + 2;

			// Allocate Buffer
			char * buffer = (char *)malloc(size);

			// Combine Path
			strcpy(buffer, path);
			strcpy(buffer + strlen(buffer), node->name);
			buffer[strlen(buffer) + 1] = 0;
			buffer[strlen(buffer)] = '/';

			// Recursion Delete
			FileOptions_RmdirRecursive(buffer);

			free(buffer);
		}

		// File
		else
		{
			// Required Buffer Size
			int size = strlen(path) + strlen(node->name) + 1;

			// Allocate Buffer
			char *buffer = (char *)malloc(size);

			// Combine Path
			strcpy(buffer, path);
			strcpy(buffer + strlen(buffer), node->name);

			// Delete File
			remove(buffer);

			free(buffer);
		}
	}

	Dirbrowse_RecursiveFree(filelist);
	return rmdir(path);
}

static int FileOptions_DeleteFile(void)
{
	// Find File
	File *file = Dirbrowse_GetFileIndex(position);

	// Not found
	if(file == NULL) 
		return -1;

	if(strcmp(file->name, "..") == 0) 
		return -2;

	char path[1024];

	// Puzzle Path
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	// Delete Folder
	if (file->isDir)
	{
		// Add Trailing Slash
		path[strlen(path) + 1] = 0;
		path[strlen(path)] = '/';

		// Delete Folder
		return FileOptions_RmdirRecursive(path);
	}

	// Delete File
	else 
		return remove(path);
}

// Copy file from src to dst
static int FileOptions_CopyFile(char *src, char *dst, bool displayAnim)
{
	int chunksize = (512 * 1024); // Chunk size
	char *buffer = (char *)malloc(chunksize); // Reading buffer

	u64 totalwrite = 0; // Accumulated writing
	u64 totalread = 0; // Accumulated reading

	int result = 0; // Result

	int in = open(src, O_RDONLY, 0777); // Open file for reading
	u64 size = FS_GetFileSize(src);

	// Opened file for reading
	if (in >= 0)
	{
		if (FS_FileExists(dst))
			remove(dst); // Delete output file (if existing)

		int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0777); // Open output file for writing

		if (out >= 0) // Opened file for writing
		{
			u64 b_read = 0; // Read byte count

			// Copy loop (512KB at a time)
			while((b_read = read(in, buffer, chunksize)) > 0)
			{
				totalread += b_read; // Accumulate read data
				totalwrite += write(out, buffer, b_read); // Write data

				if (displayAnim)
					ProgressBar_DisplayProgress(copymode == 1? "Moving" : "Copying", Utils_Basename(src), totalread, size);
			}

			close(out); // Close output file
			
			if (totalread != totalwrite) // Insufficient copy
				result = -3;
		}
		
		else // Output open error
			result = -2;
			
		close(in); // Close input file
	}

	// Input open error
	else
		result = -1;
	
	free(buffer); // Free memory
	return result; // Return result
}

// Recursively copy file from src to dst
static Result FileOptions_CopyDir(char *src, char *dst)
{
	DIR *directory = opendir(src);

	if (directory)
	{
		// Create Output Directory (is allowed to fail, we can merge folders after all)
		FS_MakeDir(dst);
		
		struct dirent * entries;

		// Iterate Files
		while ((entries = readdir(directory)) != NULL)
		{
			if (strlen(entries->d_name) > 0)
			{
				// Calculate Buffer Size
				int insize = strlen(src) + strlen(entries->d_name) + 2;
				int outsize = strlen(dst) + strlen(entries->d_name) + 2;

				// Allocate Buffer
				char * inbuffer = (char *)malloc(insize);
				char * outbuffer = (char *)malloc(outsize);

				// Puzzle Input Path
				strcpy(inbuffer, src);
				inbuffer[strlen(inbuffer) + 1] = 0;
				inbuffer[strlen(inbuffer)] = '/';
				strcpy(inbuffer + strlen(inbuffer), entries->d_name);

				// Puzzle Output Path
				strcpy(outbuffer, dst);
				outbuffer[strlen(outbuffer) + 1] = 0;
				outbuffer[strlen(outbuffer)] = '/';
				strcpy(outbuffer + strlen(outbuffer), entries->d_name);

				// Another Folder
				if (entries->d_type == DT_DIR)
					FileOptions_CopyDir(inbuffer, outbuffer); // Copy Folder (via recursion)

				// Simple File
				else
					FileOptions_CopyFile(inbuffer, outbuffer, true); // Copy File

				// Free Buffer
				free(inbuffer);
				free(outbuffer);
			}
		}

		closedir(directory);
		return 0;
	}

	return -1;
}

static void FileOptions_Copy(int flag)
{
	File *file = Dirbrowse_GetFileIndex(position);
	
	if (file == NULL)
		return;

	// Copy file source
	strcpy(copysource, cwd);
	strcpy(copysource + strlen(copysource), file->name);

	if ((file->isDir) && (strncmp(file->name, "..", 2) != 0)) // If directory, add recursive folder flag
		flag |= COPY_FOLDER_RECURSIVE;

	copymode = flag; // Set copy flags
}

// Paste file or folder
static Result FileOptions_Paste(void)
{
	if (copymode == NOTHING_TO_COPY) // No copy source
		return -1;

	// Source and target folder are identical
	char * lastslash = NULL;
	int i = 0;

	for(; i < strlen(copysource); i++)
		if (copysource[i] == '/')
			lastslash = copysource + i;

	char backup = lastslash[1];
	lastslash[1] = 0;
	int identical = strcmp(copysource, cwd) == 0;
	lastslash[1] = backup;

	if (identical)
		return -2;

	char * filename = lastslash + 1; // Source filename

	int requiredlength = strlen(cwd) + strlen(filename) + 1; // Required target path buffer size
	char * copytarget = (char *)malloc(requiredlength); // Allocate target path buffer

	// Puzzle target path
	strcpy(copytarget, cwd);
	strcpy(copytarget + strlen(copytarget), filename);

	Result ret = -3; // Return result

	// Recursive folder copy
	if ((copymode & COPY_FOLDER_RECURSIVE) == COPY_FOLDER_RECURSIVE)
	{
		// Check files in current folder
		File * node = files; for(; node != NULL; node = node->next)
		{
			if ((strcmp(filename, node->name) == 0) && (!node->isDir)) // Found a file matching the name (folder = ok, file = not)
				return -4; // Error out
		}

		ret = FileOptions_CopyDir(copysource, copytarget); // Copy folder recursively

		if ((R_SUCCEEDED(ret)) && (copymode & COPY_DELETE_ON_FINISH) == COPY_DELETE_ON_FINISH)
			FileOptions_RmdirRecursive(copysource); // Delete dir
	}

	// Simple file copy
	else
	{
		ret = FileOptions_CopyFile(copysource, copytarget, true); // Copy file
		
		if ((R_SUCCEEDED(ret)) && (copymode & COPY_DELETE_ON_FINISH) == COPY_DELETE_ON_FINISH)
			remove(copysource); // Delete file
	}

	// Paste success
	if (R_SUCCEEDED(ret))
	{
		memset(copysource, 0, sizeof(copysource)); // Erase cache data
		copymode = NOTHING_TO_COPY;
	}

	free(copytarget); // Free target path buffer
	return ret; // Return result
}

void Menu_ControlDeleteDialog(u64 input)
{
	if (input & KEY_RIGHT)
		delete_dialog_selection++;
	else if (input & KEY_LEFT)
		delete_dialog_selection--;

	if (delete_dialog_selection > 1)
		delete_dialog_selection = 0;
	else if (delete_dialog_selection < 0)
		delete_dialog_selection = 1;

	if (input & KEY_B)
	{
		delete_dialog_selection = 0;
		MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
	}

	if (input & KEY_A)
	{
		if (delete_dialog_selection == 0)
		{
			if (FileOptions_DeleteFile() == 0)
				Dirbrowse_PopulateFiles(true);
			
			MENU_DEFAULT_STATE = MENU_STATE_HOME;

		}
		else
			MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;

		delete_dialog_selection = 0;
	}
}

void Menu_DisplayDeleteDialog(void)
{
	int text_width = 0;
	TTF_SizeText(Roboto, "Do you want to continue?", &text_width, NULL);

	int confirm_width = 0, confirm_height = 0;
	TTF_SizeText(Roboto, "YES", &confirm_width, &confirm_height);

	int cancel_width = 0, cancel_height = 0;
	TTF_SizeText(Roboto, "NO", &cancel_width, &cancel_height);

	int width = 0, height = 0;
	SDL_QueryTexture(dialog, NULL, NULL, &width, &height);

	SDL_DrawImage(RENDERER, config_dark_theme? dialog_dark : dialog, ((1280 - (width)) / 2), ((720 - (height)) / 2), 880, 320);
	SDL_DrawText(Roboto, ((1280 - (width)) / 2) + 80, ((720 - (height)) / 2) + 45, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "Confirm deletion");
	SDL_DrawText(Roboto, ((1280 - (text_width)) / 2), ((720 - (height)) / 2) + 130, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Do you wish to continue?");

	if (delete_dialog_selection == 0)
		SDL_DrawRect(RENDERER, (1030 - (confirm_width)) - 20, (((720 - (height)) / 2) + 245) - 20, confirm_width + 40, confirm_height + 40, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (delete_dialog_selection == 1)
		SDL_DrawRect(RENDERER, (915 - (confirm_width)) - 20, (((720 - (height)) / 2) + 245) - 20, confirm_width + 40, cancel_height + 40, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

	SDL_DrawText(Roboto, 1030 - (confirm_width), ((720 - (height)) / 2) + 245, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "YES");
	SDL_DrawText(Roboto, 910 - (cancel_width), ((720 - (height)) / 2) + 245, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "NO");
}

void Menu_ControlProperties(u64 input)
{
	if ((input & KEY_A) || (input & KEY_B))
		MENU_DEFAULT_STATE = MENU_STATE_OPTIONS;
}

void Menu_DisplayProperties(void)
{
	// Find File
	File *file = Dirbrowse_GetFileIndex(position);

	char path[1024];
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	SDL_DrawImage(RENDERER, config_dark_theme? properties_dialog_dark : properties_dialog, 350, 85, 580, 550);
	SDL_DrawText(Roboto, 370, 133, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "Actions");

	char name[50], parent[50], size[30], contains[50]; //created[50], modified[50];

	snprintf(name, 50, "Name: %s", file->name);
	snprintf(parent, 50, "Parent: %s", cwd);

	char utils_size[16];
	Utils_GetSizeString(utils_size, FS_GetFileSize(path));
	snprintf(size, 50, "Size: %s", utils_size);

	SDL_DrawText(Roboto, 390, 183, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, name);
	SDL_DrawText(Roboto, 390, 233, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, parent);
	SDL_DrawText(Roboto, 390, 283, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, size);

	if (file->isDir)
	{
		SDL_DrawText(Roboto, 390, 333, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Contains: ");
		//SDL_DrawText(Roboto, 390, 383, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Created: ");
		//SDL_DrawText(Roboto, 390, 433, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Modified: ");
	}
	/*else
	{
		SDL_DrawText(Roboto, 390, 333, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Created: ");
		SDL_DrawText(Roboto, 390, 383, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Modified: ");
	}*/

	int width = 0, height = 0;
	TTF_SizeText(Roboto, "OK", &width, &height);
	SDL_DrawRect(RENDERER, (890 - width) - 20, (595 - height) - 20, width + 40, height + 40, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	SDL_DrawText(Roboto, 890 - width, 595 - height, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "OK");
}

void Menu_ControlOptions(u64 input)
{
	if (input & KEY_RIGHT)
		row++;
	else if (input & KEY_LEFT)
		row--;

	if (input & KEY_DDOWN)
		column++;
	else if (input & KEY_DUP)
		column--;

	if (row > 1)
		row = 0;
	else if (row < 0)
		row = 1;

	if (column > 2)
		column = 0;
	else if (column < 0)
		column = 2;

	if (input & KEY_A)
	{
		if (row == 0 && column == 0)
			MENU_DEFAULT_STATE = MENU_STATE_PROPERTIES;
		if (row == 1 && column == 1)
		{
			if (copy_status == false && cut_status == false)
			{
				copy_status = true;
				FileOptions_Copy(COPY_KEEP_ON_FINISH);
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			}
			else if (copy_status == true)
			{
				if (FileOptions_Paste() == 0)
				{
					copy_status = false;
					Dirbrowse_PopulateFiles(true);
					MENU_DEFAULT_STATE = MENU_STATE_HOME;
				}
			}
		}
		else if (row == 0 && column == 2)
		{
			if (cut_status == false && copy_status == false)
			{
				cut_status = true;
				FileOptions_Copy(COPY_DELETE_ON_FINISH);
				MENU_DEFAULT_STATE = MENU_STATE_HOME;
			}
			else if (cut_status == true)
			{
				if (FileOptions_Paste() == 0)
				{
					cut_status = false;
					Dirbrowse_PopulateFiles(true);
					MENU_DEFAULT_STATE = MENU_STATE_HOME;
				}
			}
		}
		else if (row == 1 && column == 2)
			MENU_DEFAULT_STATE = MENU_STATE_DIALOG;
	}

	if (input & KEY_B)
	{
		copy_status = false;
		cut_status = false;
		row = 0;
		column = 0;
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
	}

	if (input & KEY_X)
		MENU_DEFAULT_STATE = MENU_STATE_HOME;
}

void Menu_DisplayOptions(void)
{
	SDL_DrawImage(RENDERER, config_dark_theme? options_dialog_dark : options_dialog, 350, 85, 580, 550);
	SDL_DrawText(Roboto, 370, 133, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "Actions");

	int width = 0, height = 0;
	TTF_SizeText(Roboto, "CANCEL", &width, &height);
	SDL_DrawText(Roboto, 900 - width, 605 - height, config_dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "CANCEL");
	
	if (row == 0 && column == 0)
		SDL_DrawRect(RENDERER, 354, 188, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (row == 1 && column == 0)
		SDL_DrawRect(RENDERER, 638, 188, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (row == 0 && column == 1)
		SDL_DrawRect(RENDERER, 354, 291, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (row == 1 && column == 1)
		SDL_DrawRect(RENDERER, 638, 291, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (row == 0 && column == 2)
		SDL_DrawRect(RENDERER, 354, 393, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
	else if (row == 1 && column == 2)
		SDL_DrawRect(RENDERER, 638, 393, 287, 101, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

	SDL_DrawText(Roboto, 385, 225, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Properties");
	SDL_DrawText(Roboto, 385, 327, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Rename");
	SDL_DrawText(Roboto, 385, 429, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, cut_status? "Paste" : "Move");
		
	SDL_DrawText(Roboto, 672, 225, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "New folder");
	SDL_DrawText(Roboto, 672, 327, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, copy_status? "Paste" : "Copy");
	SDL_DrawText(Roboto, 672, 429, config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "Delete");
}
