#include <cstdio>
#include <cstring>
#include <jansson.h>

#include "config.h"
#include "fs.h"
#include "log.h"

#define CONFIG_VERSION 3

config_t cfg;

namespace Config {
    static const char *config_file = "{\n\t\"config_ver\": %d,\n\t\"lang\": %d,\n\t\"dev_options\": %d,\n\t\"image_filename\": %d,\n\t\"last_dir\": \"%s\"\n}";
    static int config_version_holder = 0;
    static const int buf_size = 128 + FS_MAX_PATH;
    
    int Save(config_t &config) {
        Result ret = 0;
        char *buf = new char[buf_size];
        u64 len = std::snprintf(buf, buf_size, config_file, CONFIG_VERSION, config.lang, config.dev_options, config.image_filename, config.cwd);
        
        // Delete and re-create the file, we don't care about the return value here.
        fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
        fsFsCreateFile(fs, "/switch/NX-Shell/config.json", len, 0);
        
        FsFile file;
        if (R_FAILED(ret = fsFsOpenFile(fs, "/switch/NX-Shell/config.json", FsOpenMode_Write, &file))) {
            Log::Error("fsFsOpenFile(/switch/NX-Shell/config.json) failed: 0x%x\n", ret);
            delete[] buf;
            return ret;
        }
        
        if (R_FAILED(ret = fsFileWrite(&file, 0, buf, len, FsWriteOption_Flush))) {
            Log::Error("fsFileWrite(/switch/NX-Shell/config.json) failed: 0x%x\n", ret);
            delete[] buf;
            fsFileClose(&file);
            return ret;
        }
        
        fsFileClose(&file);
        delete[] buf;
        return 0;
    }
    
    static void SetDefault(config_t &config) {
        config.lang = 1;
        config.dev_options = false;
        config.image_filename = false;
        std::strncpy(config.cwd, "/", 2);
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
        if (R_FAILED(ret = fsFsOpenFile(fs, "/switch/NX-Shell/config.json", FsOpenMode_Read, &file)))
            return ret;
        
        s64 size = 0;
        if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
            fsFileClose(&file);
            return ret;
        }

        char *buf =  new char[size + 1];
        if (R_FAILED(ret = fsFileRead(&file, 0, buf, static_cast<u64>(size) + 1, FsReadOption_None, nullptr))) {
            delete[] buf;
            fsFileClose(&file);
            return ret;
        }
        
        fsFileClose(&file);
        
        json_t *root;
        json_error_t error;
        root = json_loads(buf, 0, &error);
        delete[] buf;
        
        if (!root) {
            printf("error: on line %d: %s\n", error.line, error.text);
            return -1;
        }
        
        json_t *config_ver = json_object_get(root, "config_ver");
        config_version_holder = json_integer_value(config_ver);

        json_t *lang = json_object_get(root, "lang");
        cfg.lang = json_integer_value(lang);
        
        json_t *dev_options = json_object_get(root, "dev_options");
        cfg.dev_options = json_integer_value(dev_options);

        json_t *image_filename = json_object_get(root, "image_filename");
        cfg.image_filename = json_integer_value(image_filename);
        
        json_t *last_dir = json_object_get(root, "last_dir");
        std::strcpy(cfg.cwd, json_string_value(last_dir));
        
        if (!FS::DirExists(cfg.cwd))
            std::strcpy(cfg.cwd, "/");
            
        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder < CONFIG_VERSION) {
            fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
            Config::SetDefault(cfg);
            return Config::Save(cfg);
        }
        
        json_decref(root);
        return 0;
    }
}
