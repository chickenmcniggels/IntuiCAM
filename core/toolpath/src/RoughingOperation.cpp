#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

RoughingOperation::RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Roughing, name, tool) {
}

std::unique_ptr<Toolpath> RoughingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    double currentDiameter = params_.startDiameter;
    double targetDiameter = params_.endDiameter + params_.stockAllowance;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(currentDiameter/2.0 + 5.0, 0, params_.startZ + 5.0));
    
    // Iterate through roughing passes, reducing diameter with each pass
    while (currentDiameter > targetDiameter) {
        double currentRadius = currentDiameter / 2.0;
        
        // Move to start of cut (safe approach)
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, params_.startZ + 2.0));
        
        // Feed down to cutting depth
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.startZ), 
                               tool_->getCuttingParameters().feedRate);
        
        // Cut along Z axis (main cutting pass)
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.endZ), 
                               tool_->getCuttingParameters().feedRate);
        
        // Retract from cut
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 2.0, 0, params_.endZ));
        
        // Return to starting Z position for next pass
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 2.0, 0, params_.startZ + 2.0));
        
        // Reduce diameter for next pass
        currentDiameter -= params_.depthOfCut * 2.0; // Reduce diameter (2x radius)
    }
    
    // Final pass at target diameter (if we didn't hit it exactly with our step calculations)
    if (currentDiameter < targetDiameter) {
        double finalRadius = targetDiameter / 2.0;
        
        // Approach and cut
        toolpath->addRapidMove(Geometry::Point3D(finalRadius, 0, params_.startZ + 2.0));
        toolpath->addLinearMove(Geometry::Point3D(finalRadius, 0, params_.startZ), 
                               tool_->getCuttingParameters().feedRate);
        toolpath->addLinearMove(Geometry::Point3D(finalRadius, 0, params_.endZ), 
                               tool_->getCuttingParameters().feedRate);
        
        // Final retract
        toolpath->addRapidMove(Geometry::Point3D(finalRadius + 2.0, 0, params_.endZ));
    }
    
    // Final safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.startDiameter/2.0 + 5.0, 0, params_.startZ + 5.0));
    
    return toolpath;
}

bool RoughingOperation::validate() const {
    return params_.startDiameter > params_.endDiameter && 
           params_.startZ > params_.endZ && 
           params_.depthOfCut > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 