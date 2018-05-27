#ifndef NX_SHELL_TEXTURES_H
#define NX_SHELL_TEXTURES_H

SDL_Texture *icon_app, *icon_archive, *icon_audio, *icon_dir, *icon_file, *icon_image, *icon_text, *icon_dir_dark;
SDL_Texture *icon_check, *icon_uncheck, *icon_check_dark, *icon_uncheck_dark;
SDL_Texture *dialog, *options_dialog, *properties_dialog, *dialog_dark, *options_dialog_dark, *properties_dialog_dark;

void Textures_Load(void);
void Textures_Free(void);

#endif