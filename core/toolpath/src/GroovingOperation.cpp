#include <IntuiCAM/Toolpath/GroovingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

GroovingOperation::GroovingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Grooving, name, tool) {
}

std::unique_ptr<Toolpath> GroovingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    double startRadius = params_.grooveDiameter / 2.0;
    double endRadius = startRadius - params_.grooveDepth;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(startRadius + 5.0, 0, params_.grooveZ + 2.0));
    toolpath->addRapidMove(Geometry::Point3D(startRadius, 0, params_.grooveZ + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(startRadius, 0, params_.grooveZ), params_.feedRate);
    
    // Plunge to groove depth
    toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, params_.grooveZ), params_.feedRate);
    
    // Move along groove width if needed
    if (params_.grooveWidth > 0.1) {
        double halfWidth = params_.grooveWidth / 2.0;
        toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, params_.grooveZ + halfWidth), params_.feedRate);
        toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, params_.grooveZ - halfWidth), params_.feedRate);
        toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, params_.grooveZ), params_.feedRate);
    }
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(startRadius, 0, params_.grooveZ));
    toolpath->addRapidMove(Geometry::Point3D(startRadius + 10.0, 0, params_.grooveZ + 5.0));
    
    return toolpath;
}

bool GroovingOperation::validate() const {
    return params_.grooveDiameter > 0.0 && 
           params_.grooveDepth > 0.0 && 
           params_.feedRate > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 