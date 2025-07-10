#include <IntuiCAM/Toolpath/ExternalRoughingOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace IntuiCAM {
namespace Toolpath {

ExternalRoughingOperation::ExternalRoughingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Roughing, name, tool) {
    // Initialize with default parameters
}

std::string ExternalRoughingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate diameter constraints for external roughing
    if (params.startDiameter <= 0.0) {
        errors << "Start diameter must be positive. ";
    }
    
    if (params.endDiameter <= 0.0) {
        errors << "End diameter must be positive. ";
    }
    
    if (params.startDiameter <= params.endDiameter) {
        errors << "For external roughing, start diameter must be greater than end diameter. ";
    }
    
    // Validate Z positions
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z. ";
    }
    
    // Validate cutting parameters
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.stepover <= 0.0) {
        errors << "Stepover must be positive. ";
    }
    
    if (params.stockAllowance < 0.0) {
        errors << "Stock allowance cannot be negative. ";
    }
    
    // Validate that there's material to remove
    double materialToRemove = (params.startDiameter - params.endDiameter) / 2.0;
    if (materialToRemove <= params.stockAllowance) {
        errors << "Stock allowance exceeds material to be removed. ";
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

std::unique_ptr<Toolpath> ExternalRoughingOperation::generateToolpath(const Geometry::Part& part) {
    // Extract 2D profile from part for tool-agnostic implementation
    ProfileExtractor::ExtractionParameters extractParams;
    extractParams.tolerance = 0.01;                              // 0.01mm tolerance for roughing
    extractParams.minSegmentLength = 0.001;                     // Filter tiny segments
    extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Standard lathe Z-axis
    extractParams.sortSegments = true;                          // Ensure proper ordering
    
    // Get part shape for profile extraction
    // Note: In real implementation, this would be extracted from Geometry::Part
    TopoDS_Shape partShape; // This would come from part.getShape() or similar
    auto profile = ProfileExtractor::extractProfile(partShape, extractParams);
    
    // Choose strategy based on parameters and extracted profile
    if (params_.useProfileFollowing && !profile.isEmpty()) {
        return generateProfileFollowingRoughing(profile);
    } else {
        // Choose between axial and radial based on aspect ratio
        double axialDepth = abs(params_.startZ - params_.endZ);
        double radialRemoval = (params_.startDiameter - params_.endDiameter) / 2.0;
        
        if (axialDepth > radialRemoval * 3.0) {
            return generateAxialRoughing();
        } else {
            return generateRadialRoughing();
        }
    }
}

std::unique_ptr<Toolpath> ExternalRoughingOperation::generateAxialRoughing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentZ = params_.startZ;
    double targetZ = params_.endZ;
    
    // Calculate roughing diameter (leave stock allowance)
    double roughingDiameter = params_.endDiameter + (2.0 * params_.stockAllowance);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.startDiameter / 2.0 + 5.0));
    
    // Axial roughing passes
    bool reverse = false;
    while (currentZ > targetZ) {
        double nextZ = std::max(targetZ, currentZ - params_.depthOfCut);
        
        addRoughingPass(toolpath.get(), nextZ, roughingDiameter, reverse);
        
        currentZ = nextZ;
        
        // Alternate direction if enabled
        if (params_.reversePass) {
            reverse = !reverse;
        }
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, roughingDiameter / 2.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ExternalRoughingOperation::generateRadialRoughing() {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double currentDiameter = params_.startDiameter;
    double targetDiameter = params_.endDiameter + (2.0 * params_.stockAllowance);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, currentDiameter / 2.0 + 5.0));
    
    // Radial roughing passes
    bool reverse = false;
    while (currentDiameter > targetDiameter) {
        double nextDiameter = std::max(targetDiameter, currentDiameter - (2.0 * params_.stepover));
        
        // Position to start of cut
        if (!reverse) {
            toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, nextDiameter / 2.0));
            toolpath->addLinearMove(Geometry::Point3D(params_.startZ, 0.0, nextDiameter / 2.0), params_.feedRate);
            toolpath->addLinearMove(Geometry::Point3D(params_.endZ, 0.0, nextDiameter / 2.0), params_.feedRate);
        } else {
            toolpath->addRapidMove(Geometry::Point3D(params_.endZ - 1.0, 0.0, nextDiameter / 2.0));
            toolpath->addLinearMove(Geometry::Point3D(params_.endZ, 0.0, nextDiameter / 2.0), params_.feedRate);
            toolpath->addLinearMove(Geometry::Point3D(params_.startZ, 0.0, nextDiameter / 2.0), params_.feedRate);
        }
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, nextDiameter / 2.0));
        
        // Chip breaking if enabled
        if (params_.enableChipBreaking && nextDiameter > targetDiameter) {
            toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0 + params_.chipBreakDistance, 0.0, nextDiameter / 2.0));
            toolpath->addDwell(0.2);
        }
        
        currentDiameter = nextDiameter;
        
        // Alternate direction if enabled
        if (params_.reversePass) {
            reverse = !reverse;
        }
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, targetDiameter / 2.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ExternalRoughingOperation::generateProfileFollowingRoughing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    if (profile.isEmpty()) {
        // Fallback to radial roughing if no profile available
        return generateRadialRoughing();
    }
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Extract profile bounds for analysis
    double minZ, maxZ, minRadius, maxRadius;
    profile.getBounds(minZ, maxZ, minRadius, maxRadius);
    
    // Calculate roughing boundaries from profile (only positive X side already extracted)
    double profileStartZ = std::max(minZ, params_.startZ);
    double profileEndZ = std::min(maxZ, params_.endZ);
    double roughingStockRadius = params_.stockAllowance;  // Leave stock for finishing
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, maxRadius + 5.0));
    
    // Generate roughing passes by stepping radially inward
    double currentRadius = maxRadius;
    double targetRadius = minRadius + roughingStockRadius;
    
    int passCount = 0;
    bool reverse = false;
    
    while (currentRadius > targetRadius && passCount < 100) { // Safety limit
        double nextRadius = std::max(targetRadius, currentRadius - params_.stepover);
        
        // Generate profile-following pass at current radius
        generateProfileFollowingPass(toolpath.get(), profile, nextRadius, reverse);
        
        currentRadius = nextRadius;
        passCount++;
        
        // Alternate direction if enabled
        if (params_.reversePass) {
            reverse = !reverse;
        }
        
        // Chip breaking if enabled
        if (params_.enableChipBreaking && currentRadius > targetRadius) {
            toolpath->addRapidMove(Geometry::Point3D(profileStartZ + params_.chipBreakDistance, 0.0, currentRadius));
            toolpath->addDwell(0.2);
        }
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, targetRadius));
    
    return toolpath;
}

void ExternalRoughingOperation::generateProfileFollowingPass(Toolpath* toolpath, 
                                                           const LatheProfile::Profile2D& profile, 
                                                           double targetRadius, 
                                                           bool reverse) {
    
    // Find profile points/segments at target radius with stock allowance
    std::vector<std::pair<double, double>> cuttingPoints; // (z, radius) pairs
    
    // Interpolate cutting points from profile segments
    for (const auto& segment : profile.segments) {
        // Check if this segment intersects our target radius
        double segmentMinRadius = std::min(segment.start.x, segment.end.x);
        double segmentMaxRadius = std::max(segment.start.x, segment.end.x);
        
        if (targetRadius >= segmentMinRadius && targetRadius <= segmentMaxRadius) {
            // Linear interpolation to find Z position at target radius
            double t = (targetRadius - segment.start.x) / (segment.end.x - segment.start.x);
            if (std::abs(segment.end.x - segment.start.x) < 1e-6) {
                t = 0.0; // Vertical segment
            }
            t = std::max(0.0, std::min(1.0, t)); // Clamp to [0,1]
            
            double z = segment.start.z + t * (segment.end.z - segment.start.z);
            cuttingPoints.push_back({z, targetRadius});
        }
    }
    
    // Sort cutting points by Z coordinate
    std::sort(cuttingPoints.begin(), cuttingPoints.end());
    
    if (cuttingPoints.empty()) {
        // No intersection found, use straight cut between start and end Z
        cuttingPoints.push_back({params_.startZ, targetRadius});
        cuttingPoints.push_back({params_.endZ, targetRadius});
    }
    
    // Generate toolpath moves following the profile
    if (!reverse) {
        // Normal direction: start to end Z
        // Rapid to start position
        toolpath->addRapidMove(Geometry::Point3D(cuttingPoints.front().first + 1.0, 0.0, targetRadius));
        
        // Feed to first cutting point
        toolpath->addLinearMove(Geometry::Point3D(cuttingPoints.front().first, 0.0, cuttingPoints.front().second), 
                               params_.feedRate);
        
        // Follow profile segments
        for (size_t i = 1; i < cuttingPoints.size(); ++i) {
            toolpath->addLinearMove(Geometry::Point3D(cuttingPoints[i].first, 0.0, cuttingPoints[i].second), 
                                   params_.feedRate);
        }
    } else {
        // Reverse direction: end to start Z
        // Rapid to end position
        toolpath->addRapidMove(Geometry::Point3D(cuttingPoints.back().first - 1.0, 0.0, targetRadius));
        
        // Feed to last cutting point
        toolpath->addLinearMove(Geometry::Point3D(cuttingPoints.back().first, 0.0, cuttingPoints.back().second), 
                               params_.feedRate);
        
        // Follow profile segments in reverse
        for (int i = static_cast<int>(cuttingPoints.size()) - 2; i >= 0; --i) {
            toolpath->addLinearMove(Geometry::Point3D(cuttingPoints[i].first, 0.0, cuttingPoints[i].second), 
                                   params_.feedRate);
        }
    }
    
    // Retract to clearance position
    double retractZ = reverse ? cuttingPoints.front().first + 1.0 : cuttingPoints.back().first + 1.0;
    toolpath->addRapidMove(Geometry::Point3D(retractZ, 0.0, targetRadius));
}

void ExternalRoughingOperation::addRoughingPass(Toolpath* toolpath, double currentZ, double currentDiameter, bool reverse) {
    if (!reverse) {
        // Normal direction: start to end
        toolpath->addRapidMove(Geometry::Point3D(params_.startZ + 1.0, 0.0, params_.startDiameter / 2.0));
        toolpath->addLinearMove(Geometry::Point3D(params_.startZ, 0.0, params_.startDiameter / 2.0), params_.feedRate);
        toolpath->addLinearMove(Geometry::Point3D(currentZ, 0.0, currentDiameter / 2.0), params_.feedRate);
    } else {
        // Reverse direction: end to start
        toolpath->addRapidMove(Geometry::Point3D(currentZ - 1.0, 0.0, params_.startDiameter / 2.0));
        toolpath->addLinearMove(Geometry::Point3D(currentZ, 0.0, params_.startDiameter / 2.0), params_.feedRate);
        toolpath->addLinearMove(Geometry::Point3D(params_.startZ, 0.0, currentDiameter / 2.0), params_.feedRate);
    }
    
    // Retract
    toolpath->addRapidMove(Geometry::Point3D(currentZ + 1.0, 0.0, currentDiameter / 2.0));
}

bool ExternalRoughingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM 