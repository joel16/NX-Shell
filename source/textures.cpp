#include <cstring>

// JPEG
#include <turbojpeg.h>

// STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_JPEG
#define STBI_NO_PIC
#define STBI_NO_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_PNM
#define STBI_ONLY_PSD
#define STBI_ONLY_TGA
#include <stb_image.h>

// PNG
#include <png.h>

// WEBP
#include <webp/decode.h>

#include <switch.h>

#include "fs.h"
#include "imgui.h"
#include "textures.h"

Tex folder_icon, file_icons[5], check_icon, uncheck_icon;

namespace Textures {
	static Result ReadFile(const std::string &path, unsigned char **buffer, s64 *size) {
		Result ret = 0;
		FsFile file;

		char new_path[FS_MAX_PATH + 1];
		if (R_FAILED(fsdevTranslatePath(path.c_str(), &fs, new_path)))
			return -1;
		
		if (R_FAILED(ret = fsFsOpenFile(fs, new_path, FsOpenMode_Read, &file))) {
			fsFileClose(&file);
			return ret;
		}

		if (R_FAILED(ret = fsFileGetSize(&file, size))) {
			fsFileClose(&file);
			return ret;
		}

		*buffer = new unsigned char[*size];

		u64 bytes_read = 0;
		if (R_FAILED(ret = fsFileRead(&file, 0, *buffer, static_cast<u64>(*size), FsReadOption_None, &bytes_read))) {
			fsFileClose(&file);
			return ret;
		}
		
		if (bytes_read != static_cast<u64>(*size)) {
			fsFileClose(&file);
			return -1;
		}

		fsFileClose(&file);
		return 0;
	}

	static bool LoadImage(unsigned char *data, GLint format, Tex *texture, void (*free_func)(void *)) {
		// Create a OpenGL texture identifier
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		
		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		// Upload pixels into texture
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);
		
		if (*free_func)
			free_func(data);
		
		return true;
	}
	
	static bool LoadImageRomfs(const std::string &path, Tex *texture) {
		bool ret = false;
		png_image image;
		memset(&image, 0, (sizeof image));
		image.version = PNG_IMAGE_VERSION;

		if (png_image_begin_read_from_file(&image, path.c_str()) != 0) {
			png_bytep buffer;
			image.format = PNG_FORMAT_RGBA;
			buffer = new png_byte[PNG_IMAGE_SIZE(image)];

			if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
				texture->width = image.width;
				texture->height = image.height;
				ret = Textures::LoadImage(buffer, GL_RGBA, texture, nullptr);
				delete[] buffer;
				png_image_free(&image);
			}
			else {
				if (buffer == nullptr)
					png_image_free(&image);
				else
					delete[] buffer;
			}
		}

		return ret;
	}

	static bool LoadImageJPEG(unsigned char **data, s64 *size, Tex *texture) {
		tjhandle jpeg = tjInitDecompress();
		int jpegsubsamp = 0;
		tjDecompressHeader2(jpeg, *data, *size, &texture->width, &texture->height, &jpegsubsamp);
		unsigned char *buffer = new unsigned char[texture->width * texture->height * 3];
		tjDecompress2(jpeg, *data, *size, buffer, texture->width, 0, texture->height, TJPF_RGB, TJFLAG_FASTDCT);
		bool ret = LoadImage(buffer, GL_RGB, texture, nullptr);
		tjDestroy(jpeg);
		delete[] buffer;
		return ret;
	}

	static bool LoadImageOther(unsigned char **data, s64 *size, Tex *texture) {
		unsigned char *image = stbi_load_from_memory(*data, *size, &texture->width, &texture->height, nullptr, STBI_rgb_alpha);
		bool ret = Textures::LoadImage(image, GL_RGBA, texture, nullptr);
		return ret;
	}

	static bool LoadImagePNG(unsigned char **data, s64 *size, Tex *texture) {
		bool ret = false;
		png_image image;
		memset(&image, 0, (sizeof image));
		image.version = PNG_IMAGE_VERSION;

		if (png_image_begin_read_from_memory(&image, *data, *size) != 0) {
			png_bytep buffer;
			image.format = PNG_FORMAT_RGBA;
			buffer = new png_byte[PNG_IMAGE_SIZE(image)];

			if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
				texture->width = image.width;
				texture->height = image.height;
				ret = Textures::LoadImage(buffer, GL_RGBA, texture, nullptr);
				delete[] buffer;
				png_image_free(&image);
			}
			else {
				if (buffer == nullptr)
					png_image_free(&image);
				else
					delete[] buffer;
			}
		}

		return ret;
	}

	static bool LoadImageWEBP(unsigned char **data, s64 *size, Tex *texture) {
		*data = WebPDecodeRGBA(*data, *size, &texture->width, &texture->height);
		bool ret = Textures::LoadImage(*data, GL_RGBA, texture, nullptr);
		return ret;
	}

	ImageType GetImageType(const std::string &filename) {
		std::string ext = FS::GetFileExt(filename);
		
		if ((!ext.compare(".JPG")) || (!ext.compare(".JPEG")))
			return ImageTypeJPEG;
		else if (!ext.compare(".PNG"))
			return ImageTypePNG;
		else if (!ext.compare(".WEBP"))
			return ImageTypeWEBP;
		
		return ImageTypeOther;
	}

	bool LoadImageFile(const std::string &path, ImageType type, Tex *texture) {
		unsigned char *data = nullptr;
		s64 size = 0;

		if (R_FAILED(Textures::ReadFile(path, &data, &size))) {
			delete[] data;
			return false;
		}
		
		bool ret = false;
		switch(type) {
			case ImageTypeJPEG:
				ret = Textures::LoadImageJPEG(&data, &size, texture);
				break;

			case ImageTypePNG:
				ret = Textures::LoadImagePNG(&data, &size, texture);
				break;

			case ImageTypeWEBP:
				ret = Textures::LoadImageWEBP(&data, &size, texture);
				break;

			default:
				ret = Textures::LoadImageOther(&data, &size, texture);
				break;
		}

		delete[] data;
		return ret;
	}
	
	void Init(void) {
		bool image_ret = Textures::LoadImageRomfs("romfs:/folder.png", &folder_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageRomfs("romfs:/check.png", &check_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageRomfs("romfs:/uncheck.png", &uncheck_icon);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageRomfs("romfs:/file.png", &file_icons[FileTypeNone]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageRomfs("romfs:/archive.png", &file_icons[FileTypeArchive]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageRomfs("romfs:/audio.png", &file_icons[FileTypeAudio]);
		IM_ASSERT(image_ret);
		
		image_ret = Textures::LoadImageRomfs("romfs:/image.png", &file_icons[FileTypeImage]);
		IM_ASSERT(image_ret);

		image_ret = Textures::LoadImageRomfs("romfs:/text.png", &file_icons[FileTypeText]);
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
