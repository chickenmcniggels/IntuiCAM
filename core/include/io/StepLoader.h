#pragma once

#include <string>

// Forward declarations for OpenCASCADE classes
class TopoDS_Shape;

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
namespace IO {

/**
 * @brief The StepLoader class handles loading of STEP files.
 * 
 * This class provides functionality to read STEP files and convert
 * them to OpenCASCADE shapes that can be used within the application.
 */
class INTUICAM_API StepLoader {
public:
    /**
     * @brief Constructs a new StepLoader instance.
     */
    StepLoader();
    
    /**
     * @brief Destroys the StepLoader instance.
     */
    ~StepLoader();
    
    /**
     * @brief Loads a STEP file from the given path.
     * 
     * @param filename The path to the STEP file to load.
     * @return true if loading was successful, false otherwise.
     */
    bool loadFile(const std::string& filename);
    
    /**
     * @brief Gets the loaded model as an OpenCASCADE shape.
     * 
     * @return TopoDS_Shape The loaded model.
     */
    TopoDS_Shape getModel() const;
    
private:
    TopoDS_Shape m_model;
};

} // namespace IO
} // namespace IntuiCAM 