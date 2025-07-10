#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

FacingOperation::FacingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) {
    // Initialize with default parameters
}

std::string FacingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate position constraints
    if (params.startPosition <= params.endPosition) {
        errors << "Start position must be greater than end position. ";
    }
    
    // Validate radius constraints
    if (params.startRadius <= 0.0) {
        errors << "Start radius must be positive. ";
    }
    
    if (params.endRadius < 0.0) {
        errors << "End radius cannot be negative. ";
    }
    
    if (params.startRadius <= params.endRadius) {
        errors << "Start radius must be greater than end radius. ";
    }
    
    // Validate cutting parameters
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.stepover <= 0.0) {
        errors << "Stepover must be positive. ";
    }
    
    if (params.stepover > (params.startRadius - params.endRadius)) {
        errors << "Stepover too large for radius range. ";
    }
    
    // Validate stock allowance
    if (params.stockAllowance < 0.0) {
        errors << "Stock allowance cannot be negative. ";
    }
    
    if (params.stockAllowance > abs(params.startPosition - params.endPosition)) {
        errors << "Stock allowance exceeds facing depth. ";
    }
    
    // Validate feed rates
    if (params.feedRate <= 0.0) {
        errors << "Feed rate must be positive. ";
    }
    
    if (params.finishingFeedRate <= 0.0) {
        errors << "Finishing feed rate must be positive. ";
    }
    
    // Validate spindle speed
    if (params.spindleSpeed <= 0.0) {
        errors << "Spindle speed must be positive. ";
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> FacingOperation::generateToolpath(const Geometry::Part& part) {
    // Determine facing strategy based on material removal
    double facingAllowance = abs(params_.startPosition - params_.endPosition);
    
    if (facingAllowance > params_.depthOfCut) {
        return generateMultiPassFacing();
    } else {
        return generateSinglePassFacing();
    }
}

std::unique_ptr<Toolpath> FacingOperation::generateMultiPassFacing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startPosition + params_.safetyHeight;
    double facingAllowance = abs(params_.startPosition - params_.endPosition);
    
    // Calculate number of roughing passes
    int roughingPasses = static_cast<int>(std::floor((facingAllowance - params_.stockAllowance) / params_.depthOfCut));
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.startRadius + 5.0, 0.0, safeZ));
    
    // Roughing passes
    for (int i = 0; i < roughingPasses; i++) {
        double currentZ = params_.startPosition - (i * params_.depthOfCut);
        addFacingPass(toolpath.get(), currentZ, params_.feedRate);
    }
    
    // Final roughing pass to leave stock allowance
    if (params_.stockAllowance > 0.0) {
        double roughingFinalZ = params_.endPosition + params_.stockAllowance;
        addFacingPass(toolpath.get(), roughingFinalZ, params_.feedRate);
    }
    
    // Finishing pass
    if (params_.enableFinishingPass && !params_.roughingOnly) {
        addFacingPass(toolpath.get(), params_.endPosition, params_.finishingFeedRate);
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.endRadius, 0.0, safeZ));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FacingOperation::generateSinglePassFacing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startPosition + params_.safetyHeight;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.startRadius + 5.0, 0.0, safeZ));
    
    // Single facing pass
    addFacingPass(toolpath.get(), params_.endPosition, params_.feedRate);
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.endRadius, 0.0, safeZ));
    
    return toolpath;
}

void FacingOperation::addFacingPass(Toolpath* toolpath, double zPosition, double feedRate) {
    double currentRadius = params_.startRadius;
    double endRadius = params_.endRadius;
    
    // Face from outside to center using lathe coordinates
    // Point3D(radius, 0, axial_position) where axial is along spindle axis
    while (currentRadius > endRadius) {
        // Rapid to cutting position - approach from safe distance above the work surface
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0.0, zPosition + 1.0),
                              OperationType::Facing, "Rapid to facing position");

        // Feed to face surface - maintain Y=0 for lathe constraint
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0.0, zPosition), feedRate,
                               OperationType::Facing, "Feed to face surface");
        
        // Calculate next radius
        double nextRadius = std::max(endRadius, currentRadius - params_.stepover);
        
        // Face across to next radius - cutting move across face (reducing radius)
        toolpath->addLinearMove(Geometry::Point3D(nextRadius, 0.0, zPosition), feedRate,
                               OperationType::Facing, "Face to radius " + std::to_string(nextRadius));
        
        // Retract slightly for next pass - stay in XZ plane
        if (nextRadius > endRadius) {
            toolpath->addRapidMove(Geometry::Point3D(nextRadius, 0.0, zPosition + 0.5),
                                  OperationType::Facing, "Retract for next pass");
        }
        
        currentRadius = nextRadius;
    }
}

bool FacingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM
