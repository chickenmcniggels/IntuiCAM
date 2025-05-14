#include "Core.h"

// OpenCASCADE includes
#include <Standard_Version.hxx>
#include <TCollection_AsciiString.hxx>

namespace IntuiCAM {

Core::Core() : m_initialized(false) {
}

Core::~Core() {
    if (m_initialized) {
        shutdown();
    }
}

bool Core::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // Set up OpenCASCADE
    try {
        m_initialized = true;
        return true;
    } catch (...) {
        return false;
    }
}

void Core::shutdown() {
    // Cleanup
    m_initialized = false;
}

std::string Core::getVersion() const {
    return "1.0.0";
}

std::string Core::getOCCTVersion() const {
    TCollection_AsciiString occVersion(OCC_VERSION_COMPLETE);
    return occVersion.ToCString();
}

} // namespace IntuiCAM 