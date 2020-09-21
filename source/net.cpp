#include <algorithm>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <jansson.h>
#include <switch.h>

#include "fs.h"
#include "log.h"
#include "net.h"

namespace Net {
    static s64 offset = 0;

    bool GetNetworkStatus(void) {
        Result ret = 0;
        NifmInternetConnectionStatus status;
        if (R_FAILED(ret = nifmGetInternetConnectionStatus(nullptr, nullptr, &status))) {
            Log::Error("nifmGetInternetConnectionStatus() failed: 0x%x\n", ret);
            return false;
        }

        return (status == NifmInternetConnectionStatus_Connected);
    }

    bool GetAvailableUpdate(const std::string &tag) {
        return true;

        if (tag.empty())
            return false;
        
        int current_ver = ((VERSION_MAJOR * 100) + (VERSION_MINOR * 10) + VERSION_MICRO);

        std::string tag_name = tag;
        tag_name.erase(std::remove_if(tag_name.begin(), tag_name.end(), [](char c) { return c == '.'; }), tag_name.end());
        int available_ver = std::stoi(tag_name);
        return (available_ver > current_ver);
    }
    
    size_t WriteJSONData(const char *ptr, size_t size, size_t nmemb, void *userdata) {
        const size_t total_size(size * nmemb);
        reinterpret_cast<std::string *>(userdata)->append(ptr, total_size);
        return total_size;
    }

    std::string GetLatestReleaseJSON(void) {
        std::string json = std::string();
        CURL *handle = curl_easy_init();
        
        curl_slist *header_data = nullptr;
        header_data = curl_slist_append(header_data, "Content-Type: application/json");
        header_data = curl_slist_append(header_data, "Accept: application/json");
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header_data);
        
        curl_easy_setopt(handle, CURLOPT_URL, "https://api.github.com/repos/joel16/NX-Shell/releases/latest");
        curl_easy_setopt(handle, CURLOPT_USERAGENT, "NX-Shell");
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, Net::WriteJSONData);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &json);
        curl_easy_perform(handle);
        curl_easy_cleanup(handle);
        
        json_t *root;
        json_error_t error;
        root = json_loads(json.data(), 0, &error);
        
        if (!root) {
            Log::Error("json_loads failed on line %d: %s\n", error.line, error.text);
            return std::string();
        }
        
        json_t *tag = json_object_get(root, "tag_name");
        std::string tag_name = json_string_value(tag);
        return tag_name;
    }

    size_t WriteNROData(const char *ptr, size_t size, size_t nmemb, void *userdata) {
        if (R_SUCCEEDED(fsFileWrite(reinterpret_cast<FsFile *>(userdata), offset, ptr, (size * nmemb), FsWriteOption_Flush)))
            offset += (size * nmemb);
            
        return (size * nmemb);
    }
    
    void GetLatestReleaseNRO(const std::string &tag) {
        Result ret = 0;
        FsFile file;
        const char path[FS_MAX_PATH] = "/switch/NX-Shell/NX-Shell.nro";

        if (!FS::FileExists(path))
            fsFsCreateFile(fs, path, 0, 0);

        if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Write | FsOpenMode_Append, &file))) {
            Log::Error("fsFsOpenFile(%s) failed: 0x%x\n", path, ret);
            return;
        }
        
        CURL *handle = curl_easy_init();
        if (handle) {
            std::string URL = "https://github.com/joel16/NX-Shell/releases/download/" + tag + "/NX-Shell.nro";
            curl_easy_setopt(handle, CURLOPT_URL, URL.c_str());
            curl_easy_setopt(handle, CURLOPT_USERAGENT, "NX-Shell");
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, Net::WriteNROData);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, &file);
            curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
            curl_easy_perform(handle);
            curl_easy_cleanup(handle);
        }
        
        fsFileClose(&file);
        offset = 0;
        return;
    }
}
