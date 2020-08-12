#include <cstring>

// STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_PNM
#define STBI_ONLY_PSD
#define STBI_ONLY_TGA
#include <stb_image.h>

#include <switch.h>

#include "fs.h"
#include "imgui.h"
#include "textures.h"

Tex folder_icon, file_icons[5], check_icon, uncheck_icon;

namespace Textures {
	static bool LoadImage(unsigned char *data, int *width, int *height, GLint format, Tex *texture, void (*free_func)(void *)) {
		// Create a OpenGL texture identifier
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		
		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		// Upload pixels into texture
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, format, *width, *height, 0, format, GL_UNSIGNED_BYTE, data);
		
		if (*free_func)
			free_func(data);
		
		texture->width = *width;
		texture->height = *height;
		return true;
	}
	
	bool LoadImageFile(const std::string &path, Tex *texture) {
		int width = 0, height = 0;
		unsigned char *data = stbi_load(path.c_str(), &width, &height, NULL, 4);
		if (!data)
			return false;
		
		bool ret = Textures::LoadImage(data, &width, &height, GL_RGBA, texture, stbi_image_free);
		return ret;
	}
	
	void Init(void) {
		bool image_ret = Textures::LoadImageFile("romfs:/folder.png", &folder_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageFile("romfs:/check.png", &check_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageFile("romfs:/uncheck.png", &uncheck_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageFile("romfs:/file.png", &file_icons[FileTypeNone]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageFile("romfs:/archive.png", &file_icons[FileTypeArchive]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageFile("romfs:/audio.png", &file_icons[FileTypeAudio]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageFile("romfs:/image.png", &file_icons[FileTypeImage]);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageFile("romfs:/text.png", &file_icons[FileTypeText]);
		IM_ASSERT(image_ret);
	}
	
	void Free(Tex *texture) {
		glDeleteTextures(1, &texture->id);
	}
	
	void Exit(void) {
		for (int i = 0; i < NUM_FILE_ICONS; i++)
			glDeleteTextures(1, &file_icons[i].id);
		
		glDeleteTextures(1, &uncheck_icon.id);
		glDeleteTextures(1, &check_icon.id);
		glDeleteTextures(1, &folder_icon.id);
	}
}
