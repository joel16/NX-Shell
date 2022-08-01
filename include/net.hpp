#pragma once

#include <string>

namespace Net {
    bool GetNetworkStatus(void);
    bool GetAvailableUpdate(const std::string &tag);
    std::string GetLatestReleaseJSON(void);
    void GetLatestReleaseNRO(const std::string &tag);
}
