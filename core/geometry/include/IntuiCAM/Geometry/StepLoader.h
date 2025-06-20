#pragma once

#include <string>
#include <memory>
#include <vector>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Geometry {

// STEP file import/export functionality
class StepLoader {
public:
    struct ImportResult {
        std::vector<std::unique_ptr<Part>> parts;
        bool success = false;
        std::string errorMessage;
    };
    
    struct ExportOptions;

    // Import operations
    static ImportResult importStepFile(const std::string& filePath);
    static ImportResult importStepFromString(const std::string& stepData);
    
    // Export operations
    static bool exportStepFile(const std::string& filePath,
                               const std::vector<const Part*>& parts,
                               const ExportOptions& options);
    static bool exportStepFile(const std::string& filePath,
                               const std::vector<const Part*>& parts);
    
    // Validation
    static bool validateStepFile(const std::string& filePath);
    static std::vector<std::string> getSupportedFormats();
    
private:
    StepLoader() = default;
};

struct StepLoader::ExportOptions {
    double tolerance = 0.01;
    bool writeUnits = true;
    std::string units = "mm";
};

// OpenCASCADE integration utilities
class OCCTAdapter {
public:
    // Convert between IntuiCAM and OpenCASCADE types
    static std::unique_ptr<Part> convertFromOCCT(const void* occtShape);
    static void* convertToOCCT(const Part& part);
    
    // Mesh generation
    static std::unique_ptr<Mesh> generateMeshFromOCCT(const void* occtShape, double tolerance);
    
    // Geometric analysis
    static std::vector<Point3D> detectCylindersInOCCT(const void* occtShape);
    static std::optional<double> getLargestCylinderDiameter(const void* occtShape);
};

} // namespace Geometry
} // namespace IntuiCAM 