#include <IntuiCAM/Toolpath/Operations.h>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

// FacingOperation implementation
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

// RoughingOperation implementation
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

// FinishingOperation implementation
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

// PartingOperation implementation
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

// ThreadingOperation implementation
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

// GroovingOperation implementation
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