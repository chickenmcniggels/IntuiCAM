#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

FacingOperation::FacingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) {
}

std::unique_ptr<Toolpath> FacingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    // Simple facing operation - move from outside to center
    double currentDiameter = params_.startDiameter;
    double z = 0.0; // Face at Z=0
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(currentDiameter/2.0 + 5.0, 0, z + 2.0));
    toolpath->addRapidMove(Geometry::Point3D(currentDiameter/2.0, 0, z + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(currentDiameter/2.0, 0, z), tool_->getCuttingParameters().feedRate);
    
    // Face from outside to center
    while (currentDiameter > params_.endDiameter) {
        toolpath->addLinearMove(Geometry::Point3D(params_.endDiameter/2.0, 0, z), tool_->getCuttingParameters().feedRate);
        
        // Retract and move to next pass
        toolpath->addRapidMove(Geometry::Point3D(params_.endDiameter/2.0, 0, z + 2.0));
        currentDiameter -= params_.stepover * 2.0; // Diameter reduction
        
        if (currentDiameter > params_.endDiameter) {
            toolpath->addRapidMove(Geometry::Point3D(currentDiameter/2.0, 0, z + 2.0));
            toolpath->addLinearMove(Geometry::Point3D(currentDiameter/2.0, 0, z), tool_->getCuttingParameters().feedRate);
        }
    }
    
    // Final retract
    toolpath->addRapidMove(Geometry::Point3D(params_.endDiameter/2.0, 0, z + 10.0));
    
    return toolpath;
}

bool FacingOperation::validate() const {
    return params_.startDiameter > params_.endDiameter && params_.stepover > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 