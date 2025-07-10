#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>
#include <memory>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace Toolpath {

FacingOperation::FacingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) {
    // Initialize with default parameters
}

std::string FacingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate Z coordinates
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z (facing direction). ";
    }
    
    if (std::abs(params.startZ - params.endZ) < 0.001) {
        errors << "Insufficient facing depth (< 0.001mm). ";
    }
    
    // Validate radius constraints
    if (params.maxRadius <= 0.0) {
        errors << "Maximum radius must be positive. ";
    }
    
    if (params.minRadius < 0.0) {
        errors << "Minimum radius cannot be negative. ";
    }
    
    if (params.maxRadius <= params.minRadius) {
        errors << "Maximum radius must be greater than minimum radius. ";
    }
    
    // Validate stock allowances
    if (params.stockAllowance < 0.0) {
        errors << "Stock allowance cannot be negative. ";
    }
    
    if (params.finalStockAllowance < 0.0) {
        errors << "Final stock allowance cannot be negative. ";
    }
    
    if (params.stockAllowance < params.finalStockAllowance) {
        errors << "Stock allowance must be greater than or equal to final stock allowance. ";
    }
    
    // Validate cutting parameters
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.depthOfCut > 5.0) {
        errors << "Depth of cut too large (>5mm) for facing operation. ";
    }
    
    if (params.radialStepover <= 0.0) {
        errors << "Radial stepover must be positive. ";
    }
    
    if (params.radialStepover > (params.maxRadius - params.minRadius)) {
        errors << "Radial stepover too large for radius range. ";
    }
    
    if (params.axialStepover <= 0.0) {
        errors << "Axial stepover must be positive. ";
    }
    
    // Validate feed rates
    if (params.feedRate <= 0.0) {
        errors << "Feed rate must be positive. ";
    }
    
    if (params.feedRate > 1.0) {
        errors << "Feed rate seems excessive (>1.0 mm/rev) for facing. ";
    }
    
    if (params.finishingFeedRate <= 0.0) {
        errors << "Finishing feed rate must be positive. ";
    }
    
    if (params.roughingFeedRate <= 0.0) {
        errors << "Roughing feed rate must be positive. ";
    }
    
    if (params.springPassFeedRate <= 0.0) {
        errors << "Spring pass feed rate must be positive. ";
    }
    
    // Validate surface speed and spindle limits
    if (params.surfaceSpeed <= 0.0) {
        errors << "Surface speed must be positive. ";
    }
    
    if (params.surfaceSpeed > 1000.0) {
        errors << "Surface speed seems excessive (>1000 m/min). ";
    }
    
    if (params.minSpindleSpeed <= 0.0) {
        errors << "Minimum spindle speed must be positive. ";
    }
    
    if (params.maxSpindleSpeed <= params.minSpindleSpeed) {
        errors << "Maximum spindle speed must be greater than minimum. ";
    }
    
    // Validate pass management
    if (params.numberOfRoughingPasses < 1) {
        errors << "Number of roughing passes must be at least 1. ";
    }
    
    if (params.numberOfRoughingPasses > 20) {
        errors << "Number of roughing passes seems excessive (>20). ";
    }
    
    // Validate tolerance parameters
    if (params.profileTolerance <= 0.0) {
        errors << "Profile tolerance must be positive. ";
    }
    
    if (params.dimensionalTolerance <= 0.0) {
        errors << "Dimensional tolerance must be positive. ";
    }
    
    if (params.surfaceRoughnessTolerance <= 0.0) {
        errors << "Surface roughness tolerance must be positive. ";
    }
    
    // Validate chip control parameters
    if (params.chipBreakFrequency <= 0.0) {
        errors << "Chip break frequency must be positive. ";
    }
    
    if (params.chipBreakRetract < 0.0) {
        errors << "Chip break retract cannot be negative. ";
    }
    
    // Validate counter boring parameters if enabled
    if (params.enableCounterBoring) {
        if (params.counterBoreDepth <= 0.0) {
            errors << "Counter bore depth must be positive. ";
        }
        
        if (params.counterBoreDiameter <= 0.0) {
            errors << "Counter bore diameter must be positive. ";
        }
        
        if (params.counterBoreDiameter > params.maxRadius * 2.0) {
            errors << "Counter bore diameter exceeds maximum facing diameter. ";
        }
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> FacingOperation::generateToolpath(const Geometry::Part& part) {
    // Extract 2D profile from part for tool-agnostic implementation
    ProfileExtractor::ExtractionParameters extractParams;
    extractParams.tolerance = params_.profileTolerance;     // High precision for facing
    extractParams.minSegmentLength = 0.0001;               // Very fine segments
    extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Standard lathe Z-axis
    extractParams.sortSegments = true;                     // Ensure proper ordering
    
    // Get part shape for profile extraction
    // Note: In real implementation, this would be extracted from Geometry::Part
    TopoDS_Shape partShape; // This would come from part.getShape() or similar
    auto profile = ProfileExtractor::extractProfile(partShape, extractParams);
    
    if (profile.isEmpty()) {
        // Create basic facing based on parameters if no profile available
        auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
        
        // Create simple facing from maxRadius to minRadius
        LatheProfile::Profile2D basicProfile;
        LatheProfile::ProfileSegment segment;
        segment.start = IntuiCAM::Geometry::Point2D(params_.maxRadius, params_.startZ);
        segment.end = IntuiCAM::Geometry::Point2D(params_.minRadius, params_.startZ);
        segment.isLinear = true;
        segment.length = params_.maxRadius - params_.minRadius;
        basicProfile.segments.push_back(segment);
        
        return generateProfileBasedFacing(basicProfile);
    }
    
    // Generate facing toolpath based on extracted profile
    return generateProfileBasedFacing(profile);
}

std::unique_ptr<Toolpath> FacingOperation::generateProfileBasedFacing(const LatheProfile::Profile2D& profile) {
    // Extract facing boundary from profile
    auto facingBoundary = extractFacingBoundary(profile);
    
    if (facingBoundary.empty()) {
        // Return empty toolpath if no valid boundary
        auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
        return toolpath;
    }
    
    // Generate toolpath based on facing strategy
    switch (params_.strategy) {
        case FacingStrategy::InsideOut:
            return generateInsideOutFacing(profile);
            
        case FacingStrategy::OutsideIn:
            return generateOutsideInFacing(profile);
            
        case FacingStrategy::Spiral:
            return generateSpiralFacing(profile);
            
        case FacingStrategy::AdaptiveRoughing:
            return generateAdaptiveFacing(profile);
            
        case FacingStrategy::Conventional:
        case FacingStrategy::Climb:
        case FacingStrategy::HighSpeedFacing:
            // For now, use outside-in facing
            return generateOutsideInFacing(profile);
            
        default:
            return generateOutsideInFacing(profile);
    }
}

std::unique_ptr<Toolpath> FacingOperation::generateInsideOutFacing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    auto facingBoundary = extractFacingBoundary(profile);
    if (facingBoundary.empty()) {
        return toolpath;
    }
    
    double facingZ = params_.startZ;
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Calculate axial steps for multi-pass facing
    auto axialSteps = calculateOptimalAxialSteps(params_.startZ, params_.endZ);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.maxRadius + params_.clearanceDistance));
    
    // Generate facing passes from inside out
    for (double currentZ : axialSteps) {
        // Calculate optimal radial steps for this Z level
        auto radialSteps = calculateOptimalRadialSteps(params_.minRadius, params_.maxRadius);
        
        // Face from center outward
        for (size_t i = 0; i < radialSteps.size() - 1; ++i) {
            double startRadius = radialSteps[i];
            double endRadius = radialSteps[i + 1];
            
            // Calculate feed rate for this pass
            double feedRate = params_.roughingFeedRate;
            if (i == radialSteps.size() - 2 && params_.enableFinishingPass) {
                feedRate = params_.finishingFeedRate; // Last pass is finishing
            }
            
            // Add facing pass
            addFacingPass(toolpath.get(), currentZ, startRadius, endRadius, feedRate * 60.0, 
                         "Inside-out facing pass");
            
            // Add chip break if enabled
            if (params_.chipControl == ChipControl::ChipBreaking && i % 3 == 2) {
                addChipBreak(toolpath.get(), Geometry::Point3D(currentZ, 0.0, endRadius));
            }
        }
    }
    
    // Add spring pass if enabled
    if (params_.enableSpringPass) {
        addSpringPass(toolpath.get(), params_.endZ, params_.minRadius, params_.maxRadius);
    }
    
    // Return to safe position
    addSafetyMove(toolpath.get(), Geometry::Point3D(safeZ, 0.0, params_.maxRadius));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FacingOperation::generateOutsideInFacing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    auto facingBoundary = extractFacingBoundary(profile);
    if (facingBoundary.empty()) {
        return toolpath;
    }
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Calculate axial steps for multi-pass facing
    auto axialSteps = calculateOptimalAxialSteps(params_.startZ, params_.endZ);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.maxRadius + params_.clearanceDistance));
    
    // Generate facing passes from outside in
    for (double currentZ : axialSteps) {
        // Calculate optimal radial steps for this Z level
        auto radialSteps = calculateOptimalRadialSteps(params_.minRadius, params_.maxRadius);
        
        // Face from outside inward
        for (int i = static_cast<int>(radialSteps.size()) - 1; i > 0; --i) {
            double startRadius = radialSteps[i];
            double endRadius = radialSteps[i - 1];
            
            // Calculate feed rate for this pass
            double feedRate = params_.roughingFeedRate;
            if (i == 1 && params_.enableFinishingPass) {
                feedRate = params_.finishingFeedRate; // Last pass is finishing
            }
            
            // Add facing pass
            addFacingPass(toolpath.get(), currentZ, startRadius, endRadius, feedRate * 60.0, 
                         "Outside-in facing pass");
            
            // Add chip break if enabled
            if (params_.chipControl == ChipControl::ChipBreaking && (radialSteps.size() - i) % 3 == 0) {
                addChipBreak(toolpath.get(), Geometry::Point3D(currentZ, 0.0, endRadius));
            }
        }
    }
    
    // Add spring pass if enabled
    if (params_.enableSpringPass) {
        addSpringPass(toolpath.get(), params_.endZ, params_.maxRadius, params_.minRadius);
    }
    
    // Return to safe position
    addSafetyMove(toolpath.get(), Geometry::Point3D(safeZ, 0.0, params_.maxRadius));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FacingOperation::generateSpiralFacing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Calculate axial steps for multi-pass facing
    auto axialSteps = calculateOptimalAxialSteps(params_.startZ, params_.endZ);
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.maxRadius + params_.clearanceDistance));
    
    // Generate spiral facing passes
    for (double currentZ : axialSteps) {
        // Calculate number of spiral turns based on stepover
        int spiralTurns = std::max(1, static_cast<int>((params_.maxRadius - params_.minRadius) / params_.radialStepover));
        
        // Calculate feed rate
        double feedRate = (currentZ == axialSteps.back() && params_.enableFinishingPass) 
                         ? params_.finishingFeedRate : params_.roughingFeedRate;
        
        // Add spiral pass
        addSpiralPass(toolpath.get(), currentZ, params_.maxRadius, params_.minRadius, 
                     feedRate * 60.0, spiralTurns);
    }
    
    // Add spring pass if enabled
    if (params_.enableSpringPass) {
        addSpringPass(toolpath.get(), params_.endZ, params_.maxRadius, params_.minRadius);
    }
    
    // Return to safe position
    addSafetyMove(toolpath.get(), Geometry::Point3D(safeZ, 0.0, params_.maxRadius));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FacingOperation::generateAdaptiveFacing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Rapid to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, params_.maxRadius + params_.clearanceDistance));
    
    // Calculate adaptive axial steps based on material removal
    auto axialSteps = calculateOptimalAxialSteps(params_.startZ, params_.endZ);
    
    // Generate adaptive facing passes
    for (size_t axialIndex = 0; axialIndex < axialSteps.size(); ++axialIndex) {
        double currentZ = axialSteps[axialIndex];
        
        // Calculate adaptive radial steps (finer near center, coarser at edge)
        std::vector<double> adaptiveRadialSteps;
        double radius = params_.maxRadius;
        
        while (radius > params_.minRadius) {
            adaptiveRadialSteps.push_back(radius);
            
            // Adaptive stepover: smaller steps near center for better surface finish
            double adaptiveFactor = radius / params_.maxRadius; // 1.0 at edge, approaches 0 at center
            double currentStepover = params_.radialStepover * (0.5 + 0.5 * adaptiveFactor);
            
            radius -= currentStepover;
        }
        adaptiveRadialSteps.push_back(params_.minRadius);
        
        // Face with adaptive stepover
        for (size_t i = 0; i < adaptiveRadialSteps.size() - 1; ++i) {
            double startRadius = adaptiveRadialSteps[i];
            double endRadius = adaptiveRadialSteps[i + 1];
            
            // Calculate adaptive feed rate
            double feedRate = calculateAdaptiveFeedRate(startRadius);
            if (axialIndex == axialSteps.size() - 1 && i == adaptiveRadialSteps.size() - 2 && params_.enableFinishingPass) {
                feedRate = params_.finishingFeedRate; // Final pass
            }
            
            // Add facing pass
            addFacingPass(toolpath.get(), currentZ, startRadius, endRadius, feedRate * 60.0, 
                         "Adaptive facing pass");
        }
    }
    
    // Add spring pass if enabled
    if (params_.enableSpringPass) {
        addSpringPass(toolpath.get(), params_.endZ, params_.maxRadius, params_.minRadius);
    }
    
    // Return to safe position
    addSafetyMove(toolpath.get(), Geometry::Point3D(safeZ, 0.0, params_.maxRadius));
    
    return toolpath;
}

std::vector<IntuiCAM::Geometry::Point2D> FacingOperation::extractFacingBoundary(const LatheProfile::Profile2D& profile) {
    std::vector<IntuiCAM::Geometry::Point2D> boundary;
    
    // Find the face boundary from profile
    // For facing, we're interested in the Z=startZ plane
    double targetZ = params_.startZ;
    double tolerance = params_.profileTolerance;
    
    for (const auto& segment : profile.segments) {
        // Check if segment intersects the facing plane
        if ((segment.start.z <= targetZ + tolerance && segment.end.z >= targetZ - tolerance) ||
            (segment.start.z >= targetZ - tolerance && segment.end.z <= targetZ + tolerance)) {
            
            // Add intersection points
            if (std::abs(segment.start.z - targetZ) <= tolerance) {
                boundary.push_back(segment.start);
            }
            if (std::abs(segment.end.z - targetZ) <= tolerance) {
                boundary.push_back(segment.end);
            }
            
            // For segments crossing the plane, interpolate intersection
            if ((segment.start.z < targetZ && segment.end.z > targetZ) ||
                (segment.start.z > targetZ && segment.end.z < targetZ)) {
                
                double t = (targetZ - segment.start.z) / (segment.end.z - segment.start.z);
                IntuiCAM::Geometry::Point2D intersection;
                intersection.x = segment.start.x + t * (segment.end.x - segment.start.x);
                intersection.z = targetZ;
                boundary.push_back(intersection);
            }
        }
    }
    
    // Sort by radius (x-coordinate)
    std::sort(boundary.begin(), boundary.end(), 
              [](const auto& a, const auto& b) { return a.x > b.x; });
    
    // Remove duplicates
    boundary.erase(std::unique(boundary.begin(), boundary.end(),
        [tolerance](const auto& a, const auto& b) {
            return std::abs(a.x - b.x) < tolerance && std::abs(a.z - b.z) < tolerance;
        }), boundary.end());
    
    return boundary;
}

std::vector<double> FacingOperation::calculateOptimalRadialSteps(double minRadius, double maxRadius) {
    std::vector<double> steps;
    
    if (params_.enableAdaptiveStepover) {
        // Adaptive stepover: finer steps near center
        double radius = maxRadius;
        while (radius > minRadius) {
            steps.push_back(radius);
            
            // Adaptive factor: smaller steps near center
            double adaptiveFactor = radius / maxRadius;
            double stepover = params_.radialStepover * (0.4 + 0.6 * adaptiveFactor);
            
            radius -= stepover;
        }
    } else {
        // Uniform stepover
        double radius = maxRadius;
        while (radius > minRadius) {
            steps.push_back(radius);
            radius -= params_.radialStepover;
        }
    }
    
    steps.push_back(minRadius);
    return steps;
}

std::vector<double> FacingOperation::calculateOptimalAxialSteps(double startZ, double endZ) {
    std::vector<double> steps;
    
    double totalDepth = startZ - endZ;
    double stockToRemove = totalDepth - params_.finalStockAllowance;
    
    // Calculate number of passes needed
    int numPasses = std::max(1, static_cast<int>(std::ceil(stockToRemove / params_.depthOfCut)));
    numPasses = std::min(numPasses, params_.numberOfRoughingPasses);
    
    // Calculate actual depth per pass
    double depthPerPass = stockToRemove / numPasses;
    
    // Generate axial steps
    for (int i = 0; i < numPasses; ++i) {
        double currentZ = startZ - ((i + 1) * depthPerPass);
        steps.push_back(currentZ);
    }
    
    // Add final finishing step if enabled
    if (params_.enableFinishingPass && steps.back() != endZ) {
        steps.push_back(endZ);
    }
    
    return steps;
}

void FacingOperation::addFacingPass(Toolpath* toolpath, double zPosition, double startRadius, 
                                   double endRadius, double feedRate, const std::string& description) {
    // Add approach move
    addApproachMove(toolpath, Geometry::Point3D(zPosition, 0.0, startRadius));
    
    // Add facing cut
    toolpath->addLinearMove(Geometry::Point3D(zPosition, 0.0, endRadius), feedRate, 
                           OperationType::Facing, description.empty() ? "Facing cut" : description);
    
    // Add retract move
    addRetractMove(toolpath, Geometry::Point3D(zPosition, 0.0, endRadius));
}

void FacingOperation::addSpiralPass(Toolpath* toolpath, double zPosition, double startRadius, 
                                   double endRadius, double feedRate, int spiralTurns) {
    // Calculate spiral parameters
    double radiusStep = (startRadius - endRadius) / (spiralTurns * 360); // Per degree
    int totalDegrees = spiralTurns * 360;
    
    // Add approach move
    addApproachMove(toolpath, Geometry::Point3D(zPosition, 0.0, startRadius));
    
    // Generate spiral path
    for (int angle = 0; angle <= totalDegrees; angle += 5) { // 5 degree increments
        double currentRadius = startRadius - (angle * radiusStep);
        currentRadius = std::max(currentRadius, endRadius);
        
        // For lathe operations, Y remains 0, spiral effect is in radius
        toolpath->addLinearMove(Geometry::Point3D(zPosition, 0.0, currentRadius), feedRate,
                               OperationType::Facing, "Spiral facing");
        
        if (currentRadius <= endRadius) break;
    }
    
    // Add retract move
    addRetractMove(toolpath, Geometry::Point3D(zPosition, 0.0, endRadius));
}

void FacingOperation::addChipBreak(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& position) {
    // Retract for chip break
    Geometry::Point3D retractPos(position.x, position.y, position.z + params_.chipBreakRetract);
    toolpath->addRapidMove(retractPos, OperationType::Facing, "Chip break retract");
    
    // Dwell for chip break
    toolpath->addDwell(0.1); // 0.1 second dwell
    
    // Return to cutting position
    toolpath->addLinearMove(position, params_.feedRate * 60.0, OperationType::Facing, "Return from chip break");
}

double FacingOperation::calculateSpindleSpeed(double radius) const {
    if (!params_.enableConstantSurfaceSpeed || radius <= 0.0) {
        return (params_.minSpindleSpeed + params_.maxSpindleSpeed) / 2.0;
    }
    
    // Calculate spindle speed for constant surface speed: N = (1000 * V) / (π * D)
    double diameter = radius * 2.0;
    double spindleSpeed = (1000.0 * params_.surfaceSpeed) / (M_PI * diameter);
    
    // Clamp to spindle speed limits
    return std::max(params_.minSpindleSpeed, std::min(params_.maxSpindleSpeed, spindleSpeed));
}

double FacingOperation::calculateAdaptiveFeedRate(double radius, double curvature) const {
    if (!params_.adaptiveFeedRate) {
        return params_.feedRate;
    }
    
    // Base feed rate
    double baseFeedRate = params_.feedRate;
    
    // Reduce feed rate near center for better surface finish
    double radiusFactor = radius / params_.maxRadius; // 1.0 at edge, 0.0 at center
    double adaptiveFactor = 0.6 + 0.4 * radiusFactor; // 0.6-1.0 range
    
    // Additional reduction for high curvature areas
    if (curvature > 0.1) {
        adaptiveFactor *= 0.8;
    }
    
    return baseFeedRate * adaptiveFactor;
}

double FacingOperation::calculateOptimalDepthOfCut(double radius, double materialHardness) const {
    double baseDepth = params_.depthOfCut;
    
    // Adjust for radius (lighter cuts near center)
    double radiusFactor = radius / params_.maxRadius;
    double depthFactor = 0.7 + 0.3 * radiusFactor; // 0.7-1.0 range
    
    // Adjust for material hardness
    double hardnessFactor = 1.0 / materialHardness; // Lighter cuts for harder materials
    
    return baseDepth * depthFactor * hardnessFactor;
}

void FacingOperation::addApproachMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& startPoint) {
    // Calculate approach position with clearance
    double approachZ = startPoint.x + params_.clearanceDistance;
    
    // Rapid to approach position
    toolpath->addRapidMove(Geometry::Point3D(approachZ, 0.0, startPoint.z), 
                          OperationType::Facing, "Approach to facing position");
    
    // Feed to actual start position
    toolpath->addLinearMove(startPoint, params_.feedRate * 60.0, 
                           OperationType::Facing, "Feed to facing start");
}

void FacingOperation::addRetractMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& endPoint) {
    // Calculate retract position
    double retractZ = endPoint.x + params_.retractDistance;
    
    // Retract from end position
    toolpath->addRapidMove(Geometry::Point3D(retractZ, 0.0, endPoint.z), 
                          OperationType::Facing, "Retract from facing");
}

void FacingOperation::addSafetyMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& position) {
    toolpath->addRapidMove(position, OperationType::Facing, "Safety move");
}

void FacingOperation::optimizeForSurfaceFinish(Toolpath* toolpath) {
    // Implementation would optimize existing toolpath for surface finish
    // This is a placeholder for future optimization algorithms
}

void FacingOperation::optimizeForCycleTime(Toolpath* toolpath) {
    // Implementation would optimize existing toolpath for cycle time
    // This is a placeholder for future optimization algorithms
}

void FacingOperation::addFinishingPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius) {
    addFacingPass(toolpath, zPosition, startRadius, endRadius, params_.finishingFeedRate * 60.0, "Finishing pass");
}

void FacingOperation::addSpringPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius) {
    addFacingPass(toolpath, zPosition, startRadius, endRadius, params_.springPassFeedRate * 60.0, "Spring pass");
    
    // Add dwell for spring pass
    if (params_.enableDwells) {
        toolpath->addDwell(params_.dwellTime);
    }
}

bool FacingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM
