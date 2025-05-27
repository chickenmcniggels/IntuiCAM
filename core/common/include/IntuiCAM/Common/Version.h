#ifndef INTUICAM_COMMON_VERSION_H
#define INTUICAM_COMMON_VERSION_H

#include <string>

namespace IntuiCAM {
namespace Common {
namespace Version {
    
    constexpr int MAJOR = 0;
    constexpr int MINOR = 1;
    constexpr int PATCH = 0;
    
    inline std::string getVersionString() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
    
} // namespace Version
} // namespace Common
} // namespace IntuiCAM

#endif // INTUICAM_COMMON_VERSION_H 