#include <cstring>
#include <string>
#include <memory>

// BMP
#include "libnsbmp.h"

// GIF
#include <gif_lib.h>

// JPEG
#include <turbojpeg.h>

// STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_NO_HDR
#define STBI_NO_JPEG
#define STBI_NO_PIC
#define STBI_NO_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_PNM
#define STBI_ONLY_PSD
#define STBI_ONLY_TGA
#include "stb_image.h"

// PNG
#include <png.h>

// WEBP
#include <webp/decode.h>

#include <switch.h>

#include "fs.hpp"
#include "gui.hpp"
#include "imgui_impl_switch.hpp"
#include "log.hpp"
#include "textures.hpp"
#include "windows.hpp"

#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

std::vector<Tex> file_icons;
Tex folder_icon, check_icon, uncheck_icon;

namespace BMP {
    static void *bitmap_create(int width, int height, [[maybe_unused]] unsigned int state) {
        /* ensure a stupidly large (>50Megs or so) bitmap is not created */
        if ((static_cast<long long>(width) * static_cast<long long>(height)) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
            
        return std::calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char *>(bitmap);
    }
    
    static size_t bitmap_get_bpp([[maybe_unused]] void *bitmap) {
        return BYTES_PER_PIXEL;
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        std::free(bitmap);
    }
}

namespace Textures {
    typedef enum ImageType {
        ImageTypeBMP,
        ImageTypeGIF,
        ImageTypeJPEG,
        ImageTypePNG,
        ImageTypeWEBP,
        ImageTypeOther
    } ImageType;
    
    static Result ReadFile(const char path[FS_MAX_PATH], unsigned char **buffer, s64 &size) {
        Result ret = 0;
        FsFile file;
        
        if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", path, ret);
            return ret;
        }
        
        if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
            Log::Error("fsFileGetSize(%s) failed: 0x%x\n", path, ret);
            fsFileClose(&file);
            return ret;
        }

        *buffer = new unsigned char[size];

        u64 bytes_read = 0;
        if (R_FAILED(ret = fsFileRead(&file, 0, *buffer, static_cast<u64>(size), FsReadOption_None, &bytes_read))) {
            Log::Error("fsFileRead(%s) failed: 0x%x\n", path, ret);
            fsFileClose(&file);
            return ret;
        }
        
        if (bytes_read != static_cast<u64>(size)) {
            Log::Error("bytes_read(%llu) does not match file size(%llu)\n", bytes_read, size);
            fsFileClose(&file);
            return -1;
        }

        fsFileClose(&file);
        return 0;
    }

    static bool Create(unsigned char *data, GLint format, Tex &texture) {
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
        
        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, data);
        return true;
    }
    
    static bool LoadImageRomfs(const std::string &path, Tex &texture) {
        bool ret = false;
        png_image image;
        std::memset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;

        if (png_image_begin_read_from_file(&image, path.c_str()) != 0) {
            png_bytep buffer;
            image.format = PNG_FORMAT_RGBA;
            buffer = new png_byte[PNG_IMAGE_SIZE(image)];

            if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
                texture.width = image.width;
                texture.height = image.height;
                ret = Textures::Create(buffer, GL_RGBA, texture);
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
    
    static bool LoadImageBMP(unsigned char **data, s64 &size, Tex &texture) {
        bmp_bitmap_callback_vt bitmap_callbacks = {
            BMP::bitmap_create,
            BMP::bitmap_destroy,
            BMP::bitmap_get_buffer,
            BMP::bitmap_get_bpp
        };
        
        bmp_result code = BMP_OK;
        bmp_image bmp;
        bmp_create(&bmp, &bitmap_callbacks);
        
        code = bmp_analyse(&bmp, size, *data);
        if (code != BMP_OK) {
            bmp_finalise(&bmp);
            return false;
        }
        
        code = bmp_decode(&bmp);
        if (code != BMP_OK) {
            if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
                bmp_finalise(&bmp);
                return false;
            }
            
            /* skip if the decoded image would be ridiculously large */
            if ((bmp.width * bmp.height) > 200000) {
                bmp_finalise(&bmp);
                return false;
            }
        }
        
        texture.width = bmp.width;
        texture.height = bmp.height;
        bool ret = Textures::Create(static_cast<unsigned char *>(bmp.bitmap), GL_RGBA, texture);
        bmp_finalise(&bmp);
        return ret;
    }

    static bool LoadImageGIF(const std::string &path, std::vector<Tex> &textures) {
        bool ret = false;
        int error = 0;
        GifFileType *gif = DGifOpenFileName(path.c_str(), &error);

        if (!gif) {
            Log::Error("DGifOpenFileName failed: %d\n", error);
            return ret;
        }

        if (DGifSlurp(gif) != GIF_OK) {
            Log::Error("DGifSlurp failed: %d\n", gif->Error);
            return ret;
        }

        if (gif->ImageCount <= 0) {
            Log::Error("Gif does not contain any images.\n");
            return ret;
        }
        
        textures.resize(gif->ImageCount);
        
        // seiken's example code from:
        // https://forums.somethingawful.com/showthread.php?threadid=2773485&userid=0&perpage=40&pagenumber=487#post465199820
        textures[0].width = gif->SWidth;
        textures[0].height = gif->SHeight;
        std::unique_ptr<u32[]> pixels(new u32[textures[0].width * textures[0].height]);
        
        for (int i = 0; i < textures[0].width * textures[0].height; ++i)
            pixels[i] = gif->SBackGroundColor;
            
        for (int i = 0; i < gif->ImageCount; ++i) {
            const SavedImage &frame = gif->SavedImages[i];
            bool transparency = false;
            unsigned char transparency_byte = 0;
            
            // Delay time in hundredths of a second.
            int delay_time = 1;
            for (int j = 0; j < frame.ExtensionBlockCount; ++j) {
                const ExtensionBlock &block = frame.ExtensionBlocks[j];
                
                if (block.Function != GRAPHICS_EXT_FUNC_CODE)
                    continue;
                    
                // Here's the metadata for this frame.
                char dispose = (block.Bytes[0] >> 2) & 7;
                transparency = block.Bytes[0] & 1;
                delay_time = block.Bytes[1] + (block.Bytes[2] << 8);
                transparency_byte = block.Bytes[3];
                
                if (dispose == 2) {
                    // Clear the canvas.
                    for (int k = 0; k < textures[0].width * textures[0].height; ++k)
                        pixels[k] = gif->SBackGroundColor;
                }
            }
            
            // Colour map for this frame.
            ColorMapObject *map = frame.ImageDesc.ColorMap ? frame.ImageDesc.ColorMap : gif->SColorMap;
            
            // Region this frame draws to.
            int fw = frame.ImageDesc.Width;
            int fh = frame.ImageDesc.Height;
            int fl = frame.ImageDesc.Left;
            int ft = frame.ImageDesc.Top;
            
            for (int y = 0; y < std::min(textures[0].height, fh); ++y) {
                for (int x = 0; x < std::min(textures[0].width, fw); ++x) {
                    unsigned char byte = frame.RasterBits[x + y * fw];

                    // Transparent pixel.
                    if (transparency && byte == transparency_byte)
                        continue;
                        
                    // Draw to canvas.
                    const GifColorType &c = map->Colors[byte];
                    pixels[fl + x + (ft + y) * textures[0].width] = c.Red | (c.Green << 8) | (c.Blue << 16) | (0xff << 24);
                }
            }

            textures[i].delay = delay_time * 10000000;
            textures[i].width = textures[0].width;
            textures[i].height = textures[0].height;
            
            // Here's the actual frame, pixels.get() is now a pointer to the 32-bit RGBA
            // data for this frame you might expect.
            ret = Textures::Create(reinterpret_cast<unsigned char*>(pixels.get()), GL_RGBA, textures[i]);
        }
        
        if (DGifCloseFile(gif, &error) != GIF_OK) {
            Log::Error("DGifCloseFile failed: %d\n", error);
            return false;
        }

        return true;
    }
    
    static bool LoadImageJPEG(unsigned char **data, s64 &size, Tex &texture) {
        tjhandle jpeg = tjInitDecompress();
        int jpegsubsamp = 0;
        tjDecompressHeader2(jpeg, *data, size, &texture.width, &texture.height, &jpegsubsamp);
        unsigned char *buffer = new unsigned char[texture.width * texture.height * 3];
        tjDecompress2(jpeg, *data, size, buffer, texture.width, 0, texture.height, TJPF_RGB, TJFLAG_FASTDCT);
        bool ret = Textures::Create(buffer, GL_RGB, texture);
        tjDestroy(jpeg);
        delete[] buffer;
        return ret;
    }

    static bool LoadImageOther(unsigned char **data, s64 &size, Tex &texture) {
        unsigned char *image = stbi_load_from_memory(*data, size, &texture.width, &texture.height, nullptr, STBI_rgb_alpha);
        bool ret = Textures::Create(image, GL_RGBA, texture);
        return ret;
    }

    static bool LoadImagePNG(unsigned char **data, s64 &size, Tex &texture) {
        bool ret = false;
        png_image image;
        std::memset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;

        if (png_image_begin_read_from_memory(&image, *data, size) != 0) {
            png_bytep buffer;
            image.format = PNG_FORMAT_RGBA;
            buffer = new png_byte[PNG_IMAGE_SIZE(image)];

            if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
                texture.width = image.width;
                texture.height = image.height;
                ret = Textures::Create(buffer, GL_RGBA, texture);
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

    static bool LoadImageWEBP(unsigned char **data, s64 &size, Tex &texture) {
        *data = WebPDecodeRGBA(*data, size, &texture.width, &texture.height);
        bool ret = Textures::Create(*data, GL_RGBA, texture);
        return ret;
    }

    ImageType GetImageType(const std::string &filename) {
        std::string ext = FS::GetFileExt(filename);
        
        if (!ext.compare(".BMP"))
            return ImageTypeBMP;
        else if (!ext.compare(".GIF"))
            return ImageTypeGIF;
        else if ((!ext.compare(".JPG")) || (!ext.compare(".JPEG")))
            return ImageTypeJPEG;
        else if (!ext.compare(".PNG"))
            return ImageTypePNG;
        else if (!ext.compare(".WEBP"))
            return ImageTypeWEBP;
        
        return ImageTypeOther;
    }

    bool LoadImageFile(const char path[FS_MAX_PATH], std::vector<Tex> &textures) {
        bool ret = false;

        // Resize to 1 initially. If the file is a GIF it will be resized accordingly.
        textures.resize(1);

        ImageType type = Textures::GetImageType(path);

        if (type == ImageTypeGIF)
            ret = Textures::LoadImageGIF(path, textures);
        else {
            unsigned char *data = nullptr;
            s64 size = 0;
            
            if (R_FAILED(Textures::ReadFile(path, &data, size))) {
                delete[] data;
                return ret;
            }

            switch(type) {
                case ImageTypeBMP:
                    ret = Textures::LoadImageBMP(&data, size, textures[0]);
                    break;
                    
                case ImageTypeJPEG:
                    ret = Textures::LoadImageJPEG(&data, size, textures[0]);
                    break;
                    
                case ImageTypePNG:
                    ret = Textures::LoadImagePNG(&data, size, textures[0]);
                    break;
                    
                case ImageTypeWEBP:
                    ret = Textures::LoadImageWEBP(&data, size, textures[0]);
                    break;
                    
                default:
                    ret = Textures::LoadImageOther(&data, size, textures[0]);
                    break;
            }

            delete[] data;
        }
        return ret;
    }
    
    void Init(void) {
        const int num_icons = 4;

        const std::string paths[num_icons] {
            "romfs:/file.png",
            "romfs:/archive.png",
            "romfs:/image.png",
            "romfs:/text.png"
        };

        bool image_ret = Textures::LoadImageRomfs("romfs:/folder.png", folder_icon);
        IM_ASSERT(image_ret);

        image_ret = Textures::LoadImageRomfs("romfs:/check.png", check_icon);
        IM_ASSERT(image_ret);

        image_ret = Textures::LoadImageRomfs("romfs:/uncheck.png", uncheck_icon);
        IM_ASSERT(image_ret);
        
        file_icons.resize(num_icons);

        for (int i = 0; i < num_icons; i++) {
            bool ret = Textures::LoadImageRomfs(paths[i], file_icons[i]);
            IM_ASSERT(ret);
        }
    }
    
    void Free(Tex &texture) {
        glDeleteTextures(1, &texture.id);
    }
    
    void Exit(void) {
        for (unsigned int i = 0; i < file_icons.size(); i++)
            Textures::Free(file_icons[i]);

        Textures::Free(uncheck_icon);
        Textures::Free(check_icon);
        Textures::Free(folder_icon);

        for (unsigned int i = 0; i < data.textures.size(); i++)
            Textures::Free(data.textures[i]);
    }
}
