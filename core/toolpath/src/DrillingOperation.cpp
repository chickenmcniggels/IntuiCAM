#include <IntuiCAM/Toolpath/DrillingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>

namespace IntuiCAM {
namespace Toolpath {

DrillingOperation::DrillingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) { // Note: Using Facing as placeholder since Drilling not in enum
    // Initialize with default parameters
}

std::string DrillingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate hole diameter
    if (params.holeDiameter <= 0.0) {
        errors << "Hole diameter must be positive. ";
    }
    
    if (params.holeDiameter > 50.0) {
        errors << "Hole diameter seems excessive (>50mm). ";
    }
    
    // Validate hole depth
    if (params.holeDepth <= 0.0) {
        errors << "Hole depth must be positive. ";
    }
    
    if (params.holeDepth > 200.0) {
        errors << "Hole depth seems excessive (>200mm). ";
    }
    
    // Validate peck depth
    if (params.peckDepth <= 0.0) {
        errors << "Peck depth must be positive. ";
    }
    
    if (params.peckDepth > params.holeDepth) {
        errors << "Peck depth cannot exceed hole depth. ";
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

std::unique_ptr<Toolpath> DrillingOperation::generateToolpath(const Geometry::Part& part) {
    // Return empty toolpath - Drilling operation not part of core focus
    // Core focus: external roughing, external finishing, facing, and parting only
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    return toolpath;
}

std::unique_ptr<Toolpath> DrillingOperation::generateSimpleDrilling() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Drilling toolpath segments
    double safeZ = params_.startZ + params_.safetyHeight;
    double targetZ = params_.startZ - params_.holeDepth;
    
    // Rapid to safe position above hole
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    // Position to hole center at safe height
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, 0.0));
    
    // Drill to depth
    toolpath->addLinearMove(Geometry::Point3D(targetZ, 0.0, 0.0), params_.feedRate);
    
    // Dwell at bottom
    if (params_.dwellTime > 0.0) {
        toolpath->addDwell(params_.dwellTime);
    }
    
    // Retract to safe height
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> DrillingOperation::generatePeckDrilling() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentZ = params_.startZ;
    double targetZ = params_.startZ - params_.holeDepth;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    // Peck drilling cycle
    while (currentZ > targetZ) {
        double nextZ = std::max(targetZ, currentZ - params_.peckDepth);
        
        // Rapid to start of next peck
        toolpath->addRapidMove(Geometry::Point3D(currentZ, 0.0, 0.0));
        
        // Drill peck
        toolpath->addLinearMove(Geometry::Point3D(nextZ, 0.0, 0.0), params_.feedRate);
        
        // Retract for chip clearing (not all the way out if not final)
        if (nextZ > targetZ) {
            double retractZ = currentZ + params_.retractHeight;
            toolpath->addRapidMove(Geometry::Point3D(retractZ, 0.0, 0.0));
            // Brief dwell for chip clearing
            toolpath->addDwell(0.2);
        }
        
        currentZ = nextZ;
    }
    
    // Final dwell at bottom
    if (params_.dwellTime > 0.0) {
        toolpath->addDwell(params_.dwellTime);
    }
    
    // Retract to safe height
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> DrillingOperation::generateDeepHoleDrilling() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentZ = params_.startZ;
    double targetZ = params_.startZ - params_.holeDepth;
    
    // For deep holes, use smaller peck depth
    double deepPeckDepth = params_.peckDepth * 0.5;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    
    // Deep hole drilling with full retracts
    while (currentZ > targetZ) {
        double nextZ = std::max(targetZ, currentZ - deepPeckDepth);
        
        // Rapid to hole start
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ, 0.0, 0.0));
        
        // Drill to next depth
        toolpath->addLinearMove(Geometry::Point3D(nextZ, 0.0, 0.0), params_.feedRate);
        
        // Full retract for chip evacuation
        toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
        
        // Longer dwell for chip clearing in deep holes
        toolpath->addDwell(0.5);
        
        currentZ = nextZ;
    }
    
    // Final dwell at bottom on last pass
    if (params_.dwellTime > 0.0) {
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ, 0.0, 0.0));
        toolpath->addLinearMove(Geometry::Point3D(targetZ, 0.0, 0.0), params_.feedRate);
        toolpath->addDwell(params_.dwellTime);
        toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, 0.0));
    }
    
    return toolpath;
}

bool DrillingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM 