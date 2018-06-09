#ifndef NX_SHELL_TEXTURES_H
#define NX_SHELL_TEXTURES_H

SDL_Texture *icon_app, *icon_archive, *icon_audio, *icon_dir, *icon_file, *icon_image, *icon_text, *icon_dir_dark, \
	*icon_check, *icon_uncheck, *icon_check_dark, *icon_uncheck_dark, *icon_radio_off, *icon_radio_on, \
	*icon_radio_dark_off, *icon_radio_dark_on, *icon_toggle_on, *icon_toggle_dark_on, *icon_toggle_off, \
	*dialog, *options_dialog, *properties_dialog, *dialog_dark, *options_dialog_dark, *properties_dialog_dark, \
	*bg_header, *icon_settings, *icon_sd, *icon_secure, *icon_settings_dark, *icon_sd_dark, *icon_secure_dark, \
	*default_artwork, *btn_play, *btn_pause;

void Textures_Load(void);
void Textures_Free(void);

#endif