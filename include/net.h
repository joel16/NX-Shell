#ifndef NX_SHELL_NET_H
#define NX_SHELL_NET_H

#include <string>

namespace Net {
    bool GetNetworkStatus(void);
    bool GetAvailableUpdate(const std::string &tag);
    std::string GetLatestReleaseJSON(void);
    void GetLatestReleaseNRO(const std::string &tag);
}

#endif
