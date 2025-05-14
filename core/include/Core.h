#pragma once

#include <string>

// Define export macros for shared libraries if needed
#if defined(_WIN32) && defined(INTUICAM_DLL)
    #ifdef INTUICAM_EXPORTS
        #define INTUICAM_API __declspec(dllexport)
    #else
        #define INTUICAM_API __declspec(dllimport)
    #endif
#else
    #define INTUICAM_API
#endif

namespace IntuiCAM {

/**
 * @brief The Core class represents the main engine of IntuiCAM.
 * 
 * This class serves as the primary interface to the IntuiCAM CAD/CAM functionality,
 * providing access to OpenCASCADE operations.
 */
class INTUICAM_API Core {
public:
    /**
     * @brief Constructs a new Core instance.
     */
    Core();
    
    /**
     * @brief Destroys the Core instance.
     */
    ~Core();
    
    /**
     * @brief Initializes the CAD/CAM engine.
     * 
     * This method must be called before using any other functionality.
     * 
     * @return true if initialization was successful, false otherwise.
     */
    bool initialize();
    
    /**
     * @brief Shuts down the CAD/CAM engine.
     */
    void shutdown();
    
    /**
     * @brief Gets the version of IntuiCAM.
     * 
     * @return std::string The version string.
     */
    std::string getVersion() const;
    
    /**
     * @brief Gets the version of OpenCASCADE used.
     * 
     * @return std::string The OpenCASCADE version string.
     */
    std::string getOCCTVersion() const;
    
private:
    bool m_initialized;
};

} // namespace IntuiCAM 