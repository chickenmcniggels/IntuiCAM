#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

PartingOperation::PartingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Parting, name, tool) {
}

std::unique_ptr<Toolpath> PartingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    double startRadius = params_.partingDiameter / 2.0;
    double endRadius = params_.centerHoleDiameter / 2.0;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(startRadius + 5.0, 0, params_.partingZ + 2.0));
    toolpath->addRapidMove(Geometry::Point3D(startRadius, 0, params_.partingZ + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(startRadius, 0, params_.partingZ), params_.feedRate);
    
    // Part off from outside to center
    toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, params_.partingZ), params_.feedRate);
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(endRadius, 0, params_.partingZ + params_.retractDistance));
    toolpath->addRapidMove(Geometry::Point3D(startRadius + 10.0, 0, params_.partingZ + params_.retractDistance));
    
    return toolpath;
}

bool PartingOperation::validate() const {
    return params_.partingDiameter > params_.centerHoleDiameter && 
           params_.feedRate > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 