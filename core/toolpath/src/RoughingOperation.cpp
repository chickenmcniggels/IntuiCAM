#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <algorithm>

namespace IntuiCAM {
namespace Toolpath {

RoughingOperation::RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Roughing, name, tool) {
    // Initialize with default parameters
}

std::string RoughingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate diameter constraints
    if (params.startDiameter <= 0.0) {
        errors << "Start diameter must be positive. ";
    }
    
    if (params.endDiameter <= 0.0) {
        errors << "End diameter must be positive. ";
    }
    
    if (params.startDiameter <= params.endDiameter) {
        errors << "Start diameter must be greater than end diameter. ";
    }
    
    // Validate Z coordinates
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z (cutting downward). ";
    }
    
    // Validate depth of cut
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.depthOfCut > (params.startDiameter - params.endDiameter) / 2.0) {
        errors << "Depth of cut too large for diameter range. ";
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

std::unique_ptr<Toolpath> RoughingOperation::generateToolpath(const Geometry::Part& part) {
    // Create a basic roughing toolpath
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Get part bounding box for basic roughing
    auto bbox = part.getBoundingBox();
    
    // Calculate roughing parameters using lathe coordinates
    // Point3D(radius, 0, axial_position) where axial is along spindle, Y=0 (constrained)
    double currentRadius = params_.startDiameter / 2.0;
    double endRadius = params_.endDiameter / 2.0;
    double currentZ = params_.startZ;  // axial start position
    double endZ = params_.endZ;        // axial end position
    double safeZ = params_.startZ + 5.0; // 5mm clearance along axial direction
    
    // Safety rapid to start position - Point3D(radius, 0, axial)
    toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0.0, safeZ),
                          OperationType::ExternalRoughing, "Rapid to start position");
    
    // Generate roughing passes - all moves with Y=0 (lathe constraint)
    while (currentRadius > endRadius + params_.stockAllowance) {
        // Rapid to cutting position - approach axially to cutting depth
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0.0, currentZ + 1.0),
                              OperationType::ExternalRoughing, "Rapid to cutting position");
        
        // Feed to cutting depth - maintain Y=0
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0.0, currentZ), 100.0,
                               OperationType::ExternalRoughing, "Feed to cutting depth");
        
        // Rough down to end Z - longitudinal cut (axial movement)
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0.0, endZ), 100.0,
                               OperationType::ExternalRoughing, "Longitudinal roughing cut");
        
        // Retract axially - stay at same radius, move back along axial direction
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0.0, endZ + 1.0),
                              OperationType::ExternalRoughing, "Axial retract");
        
        // Move to next radius (deeper cut) - reduce radius for next pass
        currentRadius -= params_.depthOfCut;
    }
    
    // Return to safe position - Point3D(radius, 0, axial)
    toolpath->addRapidMove(Geometry::Point3D(endRadius, 0.0, safeZ),
                          OperationType::ExternalRoughing, "Return to safe position");
    
    return toolpath;
}

bool RoughingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

// Placeholder implementations for helper methods
std::vector<Geometry::Point2D> RoughingOperation::extractProfile(const Geometry::Part& part) {
    // Simple implementation - return empty for now
    return std::vector<Geometry::Point2D>();
}

std::vector<Geometry::Point2D> RoughingOperation::generateSimpleProfile(const Geometry::BoundingBox& bbox) {
    // Simple implementation - return empty for now
    return std::vector<Geometry::Point2D>();
}

double RoughingOperation::findMaxRadiusFromSection(const TopoDS_Shape& sectionShape) {
    // Simple implementation - return 0 for now
    return 0.0;
}

double RoughingOperation::getProfileRadiusAtZ(const std::vector<Geometry::Point2D>& profile, double z) {
    // Simple implementation - return 0 for now
    return 0.0;
}

std::unique_ptr<Toolpath> RoughingOperation::generateBasicRoughing() {
    // Simple implementation - return nullptr for now
    return nullptr;
}

std::vector<Geometry::Point3D> RoughingOperation::simplifyPath(const std::vector<Geometry::Point3D>& points, double tolerance) {
    // Simple implementation - return copy for now
    return points;
}

} // namespace Toolpath
} // namespace IntuiCAM
