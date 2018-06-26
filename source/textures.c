#include "common.h"
#include "SDL_helper.h"
#include "textures.h"

void Textures_Load(void)
{
	SDL_LoadImage(RENDERER, &icon_app, "romfs:/res/drawable/ic_fso_type_executable.png");
	SDL_LoadImage(RENDERER, &icon_archive, "romfs:/res/drawable/ic_fso_type_compress.png");
	SDL_LoadImage(RENDERER, &icon_audio, "romfs:/res/drawable/ic_fso_type_audio.png");
	SDL_LoadImage(RENDERER, &icon_dir, "romfs:/res/drawable/ic_fso_folder.png");
	SDL_LoadImage(RENDERER, &icon_dir_dark, "romfs:/res/drawable/ic_fso_folder_dark.png");
	SDL_LoadImage(RENDERER, &icon_file, "romfs:/res/drawable/ic_fso_default.png");
	SDL_LoadImage(RENDERER, &icon_image, "romfs:/res/drawable/ic_fso_type_image.png");
	SDL_LoadImage(RENDERER, &icon_text, "romfs:/res/drawable/ic_fso_type_text.png");
	SDL_LoadImage(RENDERER, &icon_check, "romfs:/res/drawable/btn_material_light_check_on_normal.png");
	SDL_LoadImage(RENDERER, &icon_check_dark, "romfs:/res/drawable/btn_material_light_check_on_normal_dark.png");
	SDL_LoadImage(RENDERER, &icon_uncheck, "romfs:/res/drawable/btn_material_light_check_off_normal.png");
	SDL_LoadImage(RENDERER, &icon_uncheck_dark, "romfs:/res/drawable/btn_material_light_check_off_normal_dark.png");
	SDL_LoadImage(RENDERER, &dialog, "romfs:/res/drawable/ic_material_dialog.png");
	SDL_LoadImage(RENDERER, &options_dialog, "romfs:/res/drawable/ic_material_options_dialog.png");
	SDL_LoadImage(RENDERER, &properties_dialog, "romfs:/res/drawable/ic_material_properties_dialog.png");
	SDL_LoadImage(RENDERER, &dialog_dark, "romfs:/res/drawable/ic_material_dialog_dark.png");
	SDL_LoadImage(RENDERER, &options_dialog_dark, "romfs:/res/drawable/ic_material_options_dialog_dark.png");
	SDL_LoadImage(RENDERER, &properties_dialog_dark, "romfs:/res/drawable/ic_material_properties_dialog_dark.png");
	SDL_LoadImage(RENDERER, &bg_header, "romfs:/res/drawable/bg_header.png");
	SDL_LoadImage(RENDERER, &icon_settings, "romfs:/res/drawable/ic_material_light_settings.png");
	SDL_LoadImage(RENDERER, &icon_sd, "romfs:/res/drawable/ic_material_light_sdcard.png");
	SDL_LoadImage(RENDERER, &icon_secure, "romfs:/res/drawable/ic_material_light_secure.png");
	SDL_LoadImage(RENDERER, &icon_settings_dark, "romfs:/res/drawable/ic_material_light_settings_dark.png");
	SDL_LoadImage(RENDERER, &icon_sd_dark, "romfs:/res/drawable/ic_material_light_sdcard_dark.png");
	SDL_LoadImage(RENDERER, &icon_secure_dark, "romfs:/res/drawable/ic_material_light_secure_dark.png");
	SDL_LoadImage(RENDERER, &icon_radio_off, "romfs:/res/drawable/btn_material_light_radio_off_normal.png");
	SDL_LoadImage(RENDERER, &icon_radio_on, "romfs:/res/drawable/btn_material_light_radio_on_normal.png");
	SDL_LoadImage(RENDERER, &icon_radio_dark_off, "romfs:/res/drawable/btn_material_light_radio_off_normal_dark.png");
	SDL_LoadImage(RENDERER, &icon_radio_dark_on, "romfs:/res/drawable/btn_material_light_radio_on_normal_dark.png");
	SDL_LoadImage(RENDERER, &icon_toggle_on, "romfs:/res/drawable/btn_material_light_toggle_on_normal.png");
	SDL_LoadImage(RENDERER, &icon_toggle_dark_on, "romfs:/res/drawable/btn_material_light_toggle_on_normal_dark.png");
	SDL_LoadImage(RENDERER, &icon_toggle_off, "romfs:/res/drawable/btn_material_light_toggle_off_normal.png");
	SDL_LoadImage(RENDERER, &default_artwork, "romfs:/res/drawable/default_artwork.png");
	SDL_LoadImage(RENDERER, &btn_play, "romfs:/res/drawable/btn_playback_play.png");
	SDL_LoadImage(RENDERER, &btn_pause, "romfs:/res/drawable/btn_playback_pause.png");
	SDL_LoadImage(RENDERER, &btn_rewind, "romfs:/res/drawable/btn_playback_rewind.png");
	SDL_LoadImage(RENDERER, &btn_forward, "romfs:/res/drawable/btn_playback_forward.png");
	SDL_LoadImage(RENDERER, &btn_repeat, "romfs:/res/drawable/btn_playback_repeat.png");
	SDL_LoadImage(RENDERER, &btn_shuffle, "romfs:/res/drawable/btn_playback_shuffle.png");
	SDL_LoadImage(RENDERER, &btn_repeat_overlay, "romfs:/res/drawable/btn_playback_repeat_overlay.png");
	SDL_LoadImage(RENDERER, &btn_shuffle_overlay, "romfs:/res/drawable/btn_playback_shuffle_overlay.png");
	SDL_LoadImage(RENDERER, &icon_nav_drawer, "romfs:/res/drawable/ic_material_light_navigation_drawer.png");
	SDL_LoadImage(RENDERER, &icon_actions, "romfs:/res/drawable/ic_material_light_contextual_action.png");
	SDL_LoadImage(RENDERER, &icon_back, "romfs:/res/drawable/ic_arrow_back_normal.png");
	SDL_LoadImage(RENDERER, &icon_accept, "romfs:/res/drawable/ic_material_light_accept.png");
	SDL_LoadImage(RENDERER, &icon_accept_dark, "romfs:/res/drawable/ic_material_light_accept_dark.png");
	SDL_LoadImage(RENDERER, &icon_remove, "romfs:/res/drawable/ic_material_light_remove.png");
	SDL_LoadImage(RENDERER, &icon_remove_dark, "romfs:/res/drawable/ic_material_light_remove_dark.png");
}

void Textures_Free(void)
{
	SDL_DestroyTexture(icon_remove_dark);
	SDL_DestroyTexture(icon_remove);
	SDL_DestroyTexture(icon_accept_dark);
	SDL_DestroyTexture(icon_accept);
	SDL_DestroyTexture(icon_back);
	SDL_DestroyTexture(icon_actions);
	SDL_DestroyTexture(icon_nav_drawer);
	SDL_DestroyTexture(btn_shuffle_overlay);
	SDL_DestroyTexture(btn_repeat_overlay);
	SDL_DestroyTexture(btn_shuffle);
	SDL_DestroyTexture(btn_repeat);
	SDL_DestroyTexture(btn_forward);
	SDL_DestroyTexture(btn_rewind);
	SDL_DestroyTexture(btn_pause);
	SDL_DestroyTexture(btn_play);
	SDL_DestroyTexture(default_artwork);
	SDL_DestroyTexture(icon_toggle_off);
	SDL_DestroyTexture(icon_toggle_dark_on);
	SDL_DestroyTexture(icon_toggle_on);
	SDL_DestroyTexture(icon_radio_dark_on);
	SDL_DestroyTexture(icon_radio_dark_off);
	SDL_DestroyTexture(icon_radio_on);
	SDL_DestroyTexture(icon_radio_off);
	SDL_DestroyTexture(icon_secure_dark);
	SDL_DestroyTexture(icon_sd_dark);
	SDL_DestroyTexture(icon_settings_dark);
	SDL_DestroyTexture(icon_secure);
	SDL_DestroyTexture(icon_sd);
	SDL_DestroyTexture(icon_settings);
	SDL_DestroyTexture(bg_header);
	SDL_DestroyTexture(properties_dialog_dark);
	SDL_DestroyTexture(options_dialog_dark);
	SDL_DestroyTexture(dialog_dark);
	SDL_DestroyTexture(properties_dialog);
	SDL_DestroyTexture(options_dialog);
	SDL_DestroyTexture(dialog);
	SDL_DestroyTexture(icon_uncheck_dark);
	SDL_DestroyTexture(icon_uncheck);
	SDL_DestroyTexture(icon_check_dark);
	SDL_DestroyTexture(icon_check);
	SDL_DestroyTexture(icon_text);
	SDL_DestroyTexture(icon_image);
	SDL_DestroyTexture(icon_file);
	SDL_DestroyTexture(icon_dir_dark);
	SDL_DestroyTexture(icon_dir);
	SDL_DestroyTexture(icon_audio);
	SDL_DestroyTexture(icon_archive);
	SDL_DestroyTexture(icon_app);
}