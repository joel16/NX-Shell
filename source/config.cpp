#include <cstdio>
#include <cstring>
#include <jansson.h>

#include "config.hpp"
#include "fs.hpp"
#include "log.hpp"

#define CONFIG_VERSION 4

config_t cfg;

namespace Config {
    static const char *config_file = "{\n\t\"config_version\": %d,\n\t\"language\": %d,\n\t\"dev_options\": %d,\n\t\"image_filename\": %d\n}";
    static int config_version_holder = 0;
    static const int buf_size = 128;
    
    int Save(config_t &config) {
        Result ret = 0;
        char *buf = new char[buf_size];
        u64 len = std::snprintf(buf, buf_size, config_file, CONFIG_VERSION, config.lang, config.dev_options, config.image_filename);
        
        // Delete and re-create the file, we don't care about the return value here.
        fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
        fsFsCreateFile(fs, "/switch/NX-Shell/config.json", len, 0);
        
        FsFile file;
        if (R_FAILED(ret = fsFsOpenFile(fs, "/switch/NX-Shell/config.json", FsOpenMode_Write, std::addressof(file)))) {
            Log::Error("fsFsOpenFile(/switch/NX-Shell/config.json) failed: 0x%x\n", ret);
            delete[] buf;
            return ret;
        }
        
        if (R_FAILED(ret = fsFileWrite(std::addressof(file), 0, buf, len, FsWriteOption_Flush))) {
            Log::Error("fsFileWrite(/switch/NX-Shell/config.json) failed: 0x%x\n", ret);
            delete[] buf;
            fsFileClose(std::addressof(file));
            return ret;
        }
        
        fsFileClose(std::addressof(file));
        delete[] buf;
        return 0;
    }
    
    static void SetDefault(config_t &config) {
        config.lang = 1;
        config.dev_options = false;
        config.image_filename = false;
    }
    
    int Load(void) {
        Result ret = 0;
        
        if (!FS::DirExists("/switch/"))
            fsFsCreateDirectory(fs, "/switch");
        if (!FS::DirExists("/switch/NX-Shell/"))
            fsFsCreateDirectory(fs, "/switch/NX-Shell");
            
        if (!FS::FileExists("/switch/NX-Shell/config.json")) {
            Config::SetDefault(cfg);
            return Config::Save(cfg);
        }
        
        FsFile file;
        if (R_FAILED(ret = fsFsOpenFile(fs, "/switch/NX-Shell/config.json", FsOpenMode_Read, std::addressof(file))))
            return ret;
        
        s64 size = 0;
        if (R_FAILED(ret = fsFileGetSize(std::addressof(file), std::addressof(size)))) {
            fsFileClose(std::addressof(file));
            return ret;
        }

        char *buf =  new char[size + 1];
        if (R_FAILED(ret = fsFileRead(std::addressof(file), 0, buf, static_cast<u64>(size) + 1, FsReadOption_None, nullptr))) {
            delete[] buf;
            fsFileClose(std::addressof(file));
            return ret;
        }
        
        fsFileClose(std::addressof(file));
        
        json_t *root;
        json_error_t error;
        root = json_loads(buf, 0, std::addressof(error));
        delete[] buf;
        
        if (!root) {
            printf("error: on line %d: %s\n", error.line, error.text);
            return -1;
        }
        
        json_t *config_ver = json_object_get(root, "config_version");
        config_version_holder = json_integer_value(config_ver);

        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder < CONFIG_VERSION) {
            fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
            Config::SetDefault(cfg);
            return Config::Save(cfg);
        }

        json_t *language = json_object_get(root, "language");
        cfg.lang = json_integer_value(language);
        
        json_t *dev_options = json_object_get(root, "dev_options");
        cfg.dev_options = json_integer_value(dev_options);

        json_t *image_filename = json_object_get(root, "image_filename");
        cfg.image_filename = json_integer_value(image_filename);

        json_decref(root);
        return 0;
    }
}
