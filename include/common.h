#ifndef NX_SHELL_COMMON_H
#define NX_SHELL_COMMON_H

#include <setjmp.h>
#include <SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL_ttf.h>

#define wait(msec) svcSleepThread(10000000 * (s64)msec)

extern jmp_buf exitJmp;

extern SDL_Window *WINDOW;
extern SDL_Surface *WINDOW_SURFACE;
extern SDL_Renderer *RENDERER;
extern TTF_Font *Roboto_large, *Roboto, *Roboto_small, *Roboto_OSK;

#define ROOT_PATH "/"
#define START_PATH ROOT_PATH
#define MAX_FILES 2048
#define FILES_PER_PAGE 8

extern int MENU_DEFAULT_STATE;
extern int BROWSE_STATE;

#define MENU_STATE_HOME       0
#define MENU_STATE_OPTIONS    1
#define MENU_STATE_MENUBAR    2
#define MENU_STATE_SETTINGS   3
#define MENU_STATE_FTP        4
#define MENU_STATE_SORT       5
#define MENU_STATE_THEMES     6
#define MENU_STATE_DIALOG     7
#define MENU_STATE_PROPERTIES 8

#define STATE_SD   0
#define STATE_NAND 1

extern char cwd[512];
extern char fileName[255];

#endif