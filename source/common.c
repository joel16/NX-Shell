#include "common.h"

jmp_buf exitJmp;

SDL_Window *WINDOW;
SDL_Surface *WINDOW_SURFACE;
SDL_Renderer *RENDERER;
TTF_Font *Roboto_large, *Roboto, *Roboto_small, *Roboto_OSK;

int MENU_DEFAULT_STATE;
int BROWSE_STATE;

char cwd[512];
