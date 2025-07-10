#define _USE_MATH_DEFINES
#include <IntuiCAM/Toolpath/ChamferingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace Toolpath {

ChamferingOperation::ChamferingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Finishing, name, tool) { // Using Finishing as placeholder
    // Initialize with default parameters
}

std::string ChamferingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate chamfer size
    if (params.chamferSize <= 0.0) {
        errors << "Chamfer size must be positive. ";
    }
    
    if (params.chamferSize > 10.0) {
        errors << "Chamfer size seems excessive (>10mm). ";
    }
    
    // Validate chamfer angle
    if (params.chamferAngle <= 0.0 || params.chamferAngle >= 90.0) {
        errors << "Chamfer angle must be between 0 and 90 degrees. ";
    }
    
    // Validate diameters
    if (params.startDiameter <= 0.0 || params.endDiameter <= 0.0) {
        errors << "Diameters must be positive. ";
    }
    
    // Validate diameter relationship based on chamfer type
    if (params.isExternal && params.startDiameter <= params.endDiameter) {
        errors << "For external chamfer, start diameter must be greater than end diameter. ";
    }
    
    if (!params.isExternal && params.startDiameter >= params.endDiameter) {
        errors << "For internal chamfer, start diameter must be less than end diameter. ";
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

std::unique_ptr<Toolpath> ChamferingOperation::generateToolpath(const Geometry::Part& part) {
    // Return empty toolpath - Chamfering operation not part of core focus
    // Core focus: external roughing, external finishing, facing, and parting only
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    return toolpath;
}

std::unique_ptr<Toolpath> ChamferingOperation::generateLinearChamfer() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double startRadius = params_.startDiameter / 2.0;
    double endRadius = params_.endDiameter / 2.0;
    
    // Calculate chamfer geometry
    double angleRad = params_.chamferAngle * M_PI / 180.0;
    double deltaZ = params_.chamferSize * cos(angleRad);
    double deltaR = params_.chamferSize * sin(angleRad);
    
    double chamferStartZ, chamferEndZ, chamferStartR, chamferEndR;
    
    if (params_.isExternal) {
        // External chamfer
        chamferStartZ = params_.startZ;
        chamferEndZ = params_.startZ - deltaZ;
        chamferStartR = startRadius;
        chamferEndR = startRadius - deltaR;
    } else {
        // Internal chamfer
        chamferStartZ = params_.startZ;
        chamferEndZ = params_.startZ - deltaZ;
        chamferStartR = startRadius;
        chamferEndR = startRadius + deltaR;
    }
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, chamferStartR + 2.0));
    
    // Position to start of chamfer
    toolpath->addRapidMove(Geometry::Point3D(chamferStartZ + 1.0, 0.0, chamferStartR));
    
    // Feed to chamfer start
    toolpath->addLinearMove(Geometry::Point3D(chamferStartZ, 0.0, chamferStartR), params_.feedRate);
    
    // Cut chamfer
    toolpath->addLinearMove(Geometry::Point3D(chamferEndZ, 0.0, chamferEndR), params_.feedRate);
    
    // Retract to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, chamferEndR));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ChamferingOperation::generateRadiusChamfer() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double startRadius = params_.startDiameter / 2.0;
    
    // For radius chamfer, use multiple linear segments to approximate arc
    int segments = 8; // Number of segments to approximate the radius
    double radius = params_.chamferSize;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, startRadius + 2.0));
    
    // Generate arc segments
    for (int i = 0; i <= segments; i++) {
        double t = static_cast<double>(i) / segments;
        double angle = t * M_PI / 2.0; // Quarter circle
        
        double z = params_.startZ - radius * (1.0 - cos(angle));
        double r = startRadius - radius * sin(angle);
        
        if (i == 0) {
            // Position to start
            toolpath->addRapidMove(Geometry::Point3D(z + 1.0, 0.0, r));
            toolpath->addLinearMove(Geometry::Point3D(z, 0.0, r), params_.feedRate);
        } else {
            // Cut arc segment
            toolpath->addLinearMove(Geometry::Point3D(z, 0.0, r), params_.feedRate);
        }
    }
    
    // Retract to safe position
    double finalR = startRadius - radius;
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, finalR));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ChamferingOperation::generateCustomAngleChamfer() {
    // For now, use linear chamfer with custom angle
    return generateLinearChamfer();
}

bool ChamferingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM 