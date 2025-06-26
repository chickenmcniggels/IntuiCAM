#include <IntuiCAM/Toolpath/InternalRoughingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

InternalRoughingOperation::InternalRoughingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Roughing, name, tool) {
    // Initialize with default parameters
}

std::string InternalRoughingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate diameter constraints for internal roughing
    if (params.startDiameter <= 0.0) {
        errors << "Start diameter must be positive. ";
    }
    
    if (params.endDiameter <= 0.0) {
        errors << "End diameter must be positive. ";
    }
    
    if (params.endDiameter <= params.startDiameter) {
        errors << "For internal roughing, end diameter must be greater than start diameter. ";
    }
    
    // Validate Z positions
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z. ";
    }
    
    // Validate cutting parameters
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.stepover <= 0.0) {
        errors << "Stepover must be positive. ";
    }
    
    if (params.stockAllowance < 0.0) {
        errors << "Stock allowance cannot be negative. ";
    }
    
    // Validate that there's material to remove
    double materialToRemove = (params.endDiameter - params.startDiameter) / 2.0;
    if (materialToRemove <= params.stockAllowance) {
        errors << "Stock allowance exceeds material to be removed. ";
    }
    
    // Validate feed rate
    if (params.feedRate <= 0.0) {
        errors << "Feed rate must be positive. ";
    }
    
    // Validate spindle speed
    if (params.spindleSpeed <= 0.0) {
        errors << "Spindle speed must be positive. ";
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> InternalRoughingOperation::generateToolpath(const Geometry::Part& part) {
    // For internal roughing, typically use axial strategy
    double axialDepth = abs(params_.startZ - params_.endZ);
    double radialRemoval = (params_.endDiameter - params_.startDiameter) / 2.0;
    
    // Choose strategy based on geometry
    if (axialDepth > radialRemoval * 2.0) {
        return generateAxialRoughing();
    } else {
        return generateRadialRoughing();
    }
}

std::unique_ptr<Toolpath> InternalRoughingOperation::generateAxialRoughing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentZ = params_.startZ;
    double targetZ = params_.endZ;
    
    // Calculate roughing diameter (leave stock allowance)
    double roughingDiameter = params_.endDiameter - (2.0 * params_.stockAllowance);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.startDiameter / 2.0));
    
    // Axial roughing passes
    while (currentZ > targetZ) {
        double nextZ = std::max(targetZ, currentZ - params_.depthOfCut);
        
        addRoughingPass(toolpath.get(), nextZ, roughingDiameter);
        
        currentZ = nextZ;
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.startDiameter / 2.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> InternalRoughingOperation::generateRadialRoughing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentDiameter = params_.startDiameter;
    double targetDiameter = params_.endDiameter - (2.0 * params_.stockAllowance);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, currentDiameter / 2.0));
    
    // Radial roughing passes
    while (currentDiameter < targetDiameter) {
        double nextDiameter = std::min(targetDiameter, currentDiameter + (2.0 * params_.stepover));
        
        // Position to start of cut
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, nextDiameter / 2.0));
        
        // Feed to start Z
        toolpath->addLinearMove(Geometry::Point3D(params_.startZ, 0.0, nextDiameter / 2.0), params_.feedRate);
        
        // Cut to end Z
        toolpath->addLinearMove(Geometry::Point3D(params_.endZ, 0.0, nextDiameter / 2.0), params_.feedRate);
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, nextDiameter / 2.0));
        
        // Chip breaking if enabled
        if (params_.enableChipBreaking && nextDiameter < targetDiameter) {
            toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0 + params_.chipBreakDistance, 0.0, nextDiameter / 2.0));
            toolpath->addDwell(0.2);
        }
        
        currentDiameter = nextDiameter;
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, targetDiameter / 2.0));
    
    return toolpath;
}

void InternalRoughingOperation::addRoughingPass(Toolpath* toolpath, double currentZ, double currentDiameter) {
    // Position to start of cut
    toolpath->addRapidMove(Geometry::Point3D(currentZ + 1.0, 0.0, params_.startDiameter / 2.0));
    
    // Feed to start position
    toolpath->addLinearMove(Geometry::Point3D(currentZ, 0.0, params_.startDiameter / 2.0), params_.feedRate);
    
    // Cut radially outward
    toolpath->addLinearMove(Geometry::Point3D(currentZ, 0.0, currentDiameter / 2.0), params_.feedRate);
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(currentZ + 1.0, 0.0, currentDiameter / 2.0));
}

bool InternalRoughingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM 