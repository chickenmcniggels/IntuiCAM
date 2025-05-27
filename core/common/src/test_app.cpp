#include <Standard_Version.hxx>
#include <iostream>
#include "IntuiCAM/Common/Version.h"

int main() {
    // Print OpenCASCADE version
    std::cout << "OpenCASCADE Version: "
              << OCC_VERSION_MAJOR << "."
              << OCC_VERSION_MINOR << "."
              << OCC_VERSION_MAINTENANCE
              << std::endl;
              
    // Print IntuiCAM version
    std::cout << "IntuiCAM Version: " 
              << IntuiCAM::Common::Version::getVersionString()
              << std::endl;
              
    return 0;
} 