#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

FinishingOperation::FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Finishing, name, tool) {
}

std::unique_ptr<Toolpath> FinishingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    double radius = params_.targetDiameter / 2.0;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(radius + 5.0, 0, params_.startZ + 5.0));
    toolpath->addRapidMove(Geometry::Point3D(radius, 0, params_.startZ + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(radius, 0, params_.startZ), params_.feedRate);
    
    // Single finishing pass
    toolpath->addLinearMove(Geometry::Point3D(radius, 0, params_.endZ), params_.feedRate);
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(radius, 0, params_.endZ - 2.0));
    toolpath->addRapidMove(Geometry::Point3D(radius + 10.0, 0, params_.endZ - 2.0));
    
    return toolpath;
}

bool FinishingOperation::validate() const {
    return params_.targetDiameter > 0.0 && 
           params_.startZ > params_.endZ && 
           params_.feedRate > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 