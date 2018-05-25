#include "common.h"
#include "SDL_helper.h"
#include "textures.h"

void Textures_Load(void)
{
	SDL_LoadImage(RENDERER, &icon_app, "romfs:/res/drawable/ic_fso_type_executable.png");
	SDL_LoadImage(RENDERER, &icon_archive, "romfs:/res/drawable/ic_fso_type_compress.png");
	SDL_LoadImage(RENDERER, &icon_audio, "romfs:/res/drawable/ic_fso_type_audio.png");
	SDL_LoadImage(RENDERER, &icon_dir, "romfs:/res/drawable/ic_fso_folder.png");
	SDL_LoadImage(RENDERER, &icon_file, "romfs:/res/drawable/ic_fso_default.png");
	SDL_LoadImage(RENDERER, &icon_image, "romfs:/res/drawable/ic_fso_type_image.png");
	SDL_LoadImage(RENDERER, &icon_text, "romfs:/res/drawable/ic_fso_type_text.png");
	SDL_LoadImage(RENDERER, &icon_check, "romfs:/res/drawable/btn_material_light_check_on_normal.png");
	SDL_LoadImage(RENDERER, &icon_uncheck, "romfs:/res/drawable/btn_material_light_check_off_normal.png");
	SDL_LoadImage(RENDERER, &dialog, "romfs:/res/drawable/ic_material_dialog.png");
	SDL_LoadImage(RENDERER, &options_dialog, "romfs:/res/drawable/ic_material_options_dialog.png");
	SDL_LoadImage(RENDERER, &properties_dialog, "romfs:/res/drawable/ic_material_properties_dialog.png");
}