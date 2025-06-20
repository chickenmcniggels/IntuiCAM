#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>

namespace IntuiCAM {
namespace Toolpath {

FacingOperation::FacingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) {
    // Initialize with default parameters
}

std::string FacingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate diameter constraints
    if (params.startDiameter <= 0.0) {
        errors << "Start diameter must be positive. ";
    }
    
    if (params.endDiameter < 0.0) {
        errors << "End diameter cannot be negative. ";
    }
    
    if (params.startDiameter <= params.endDiameter) {
        errors << "Start diameter must be greater than end diameter. ";
    }
    
    // Validate stepover
    if (params.stepover <= 0.0) {
        errors << "Stepover must be positive. ";
    }
    
    if (params.stepover > (params.startDiameter - params.endDiameter) / 2.0) {
        errors << "Stepover too large for diameter range. ";
    }
    
    // Validate stock allowance
    if (params.stockAllowance < 0.0) {
        errors << "Stock allowance cannot be negative. ";
    }
    
    if (params.stockAllowance > 5.0) {
        errors << "Stock allowance seems excessive (>5mm). ";
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> FacingOperation::generateToolpath(const Geometry::Part& part) {
    // Create a basic facing toolpath
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Get part bounding box for basic facing
    auto bbox = part.getBoundingBox();
    
    // Calculate facing parameters
    double currentRadius = params_.startDiameter / 2.0;
    double endRadius = params_.endDiameter / 2.0;
    double safeZ = bbox.max.z + 5.0; // 5mm clearance
    double faceZ = bbox.max.z;
    
    // Safety rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, currentRadius));
    
    // Generate facing passes
    while (currentRadius > endRadius) {
        // Rapid to cutting position
        toolpath->addRapidMove(Geometry::Point3D(faceZ + 1.0, 0.0, currentRadius));
        
        // Feed to face
        toolpath->addLinearMove(Geometry::Point3D(faceZ, 0.0, currentRadius), 100.0);
        
        // Face across
        double nextRadius = std::max(endRadius, currentRadius - params_.stepover);
        toolpath->addLinearMove(Geometry::Point3D(faceZ, 0.0, nextRadius), 100.0);
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(faceZ + 1.0, 0.0, nextRadius));
        
        currentRadius = nextRadius;
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, endRadius));
    
    return toolpath;
}

bool FacingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM
