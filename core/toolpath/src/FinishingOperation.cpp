#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>

namespace IntuiCAM {
namespace Toolpath {

FinishingOperation::FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Finishing, name, tool) {
    // Initialize with default parameters
}

std::string FinishingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate diameter constraints
    if (params.targetDiameter <= 0.0) {
        errors << "Target diameter must be positive. ";
    }
    
    // Validate Z coordinates
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z (cutting downward). ";
    }
    
    // Validate surface speed
    if (params.surfaceSpeed <= 0.0) {
        errors << "Surface speed must be positive. ";
    }
    
    if (params.surfaceSpeed > 500.0) {
        errors << "Surface speed seems excessive (>500 m/min). ";
    }
    
    // Validate feed rate
    if (params.feedRate <= 0.0) {
        errors << "Feed rate must be positive. ";
    }
    
    if (params.feedRate > 1.0) {
        errors << "Feed rate seems excessive (>1.0 mm/rev). ";
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> FinishingOperation::generateToolpath(const Geometry::Part& part) {
    // Create a basic finishing toolpath
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Get part bounding box for basic finishing
    auto bbox = part.getBoundingBox();
    
    // Calculate finishing parameters
    double targetRadius = params_.targetDiameter / 2.0;
    double currentZ = params_.startZ;
    double endZ = params_.endZ;
    double safeZ = params_.startZ + 5.0; // 5mm clearance
    
    // Safety rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(targetRadius, 0.0, safeZ));
    
    // Rapid to cutting position
    toolpath->addRapidMove(Geometry::Point3D(targetRadius, 0.0, currentZ + 1.0));
    
    // Feed to cutting depth
    toolpath->addLinearMove(Geometry::Point3D(targetRadius, 0.0, currentZ), params_.feedRate * 60.0);
    
    // Finish down to end Z in single pass
    toolpath->addLinearMove(Geometry::Point3D(targetRadius, 0.0, endZ), params_.feedRate * 60.0);
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(targetRadius, 0.0, endZ - 1.0));
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(targetRadius, 0.0, safeZ));
    
    return toolpath;
}

bool FinishingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM
