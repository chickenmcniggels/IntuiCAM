#include <IntuiCAM/Toolpath/ThreadingOperation.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

ThreadingOperation::ThreadingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Threading, name, tool) {
}

std::unique_ptr<Toolpath> ThreadingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    double radius = params_.majorDiameter / 2.0;
    double threadDepth = params_.isMetric ? params_.pitch * 0.613 : params_.pitch * 0.75;
    
    // Multiple threading passes
    for (int pass = 0; pass < params_.numberOfPasses; ++pass) {
        double currentDepth = threadDepth * (pass + 1) / params_.numberOfPasses;
        double currentRadius = radius - currentDepth;
        
        // Rapid to start
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 2.0, 0, params_.startZ + 2.0));
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, params_.startZ + 2.0));
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.startZ), 0.1);
        
        // Threading pass
        double endZ = params_.startZ - params_.threadLength;
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, endZ), 0.05);
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 2.0, 0, endZ));
    }
    
    // Final retract
    toolpath->addRapidMove(Geometry::Point3D(radius + 10.0, 0, params_.startZ + 5.0));
    
    return toolpath;
}

bool ThreadingOperation::validate() const {
    return params_.majorDiameter > 0.0 && 
           params_.pitch > 0.0 && 
           params_.threadLength > 0.0 && 
           params_.numberOfPasses > 0;
}

} // namespace Toolpath
} // namespace IntuiCAM 