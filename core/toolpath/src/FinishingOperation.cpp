#include <IntuiCAM/Toolpath/FinishingOperation.h>
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

FinishingOperation::FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Finishing, name, tool) {
    // Initialize with default parameters
}

std::string FinishingOperation::validateParameters(const Parameters& params) {
    std::ostringstream errors;
    
    // Validate Z coordinates
    if (params.startZ <= params.endZ) {
        errors << "Start Z must be greater than end Z (cutting direction). ";
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
    if (params.surfaceSpeed <= 0.0) {
        errors << "Surface speed must be positive. ";
    }
    
    if (params.surfaceSpeed > 1000.0) {
        errors << "Surface speed seems excessive (>1000 m/min). ";
    }
    
    if (params.feedRate <= 0.0) {
        errors << "Feed rate must be positive. ";
    }
    
    if (params.feedRate > 0.5) {
        errors << "Feed rate seems excessive (>0.5 mm/rev) for finishing. ";
    }
    
    if (params.springPassFeedRate <= 0.0) {
        errors << "Spring pass feed rate must be positive. ";
    }
    
    if (params.depthOfCut <= 0.0) {
        errors << "Depth of cut must be positive. ";
    }
    
    if (params.depthOfCut > 0.5) {
        errors << "Depth of cut too large (>0.5mm) for finishing operation. ";
    }
    
    // Validate spindle speed limits
    if (params.minSpindleSpeed <= 0.0) {
        errors << "Minimum spindle speed must be positive. ";
    }
    
    if (params.maxSpindleSpeed <= params.minSpindleSpeed) {
        errors << "Maximum spindle speed must be greater than minimum. ";
    }
    
    // Validate number of passes
    if (params.numberOfPasses < 1) {
        errors << "Number of passes must be at least 1. ";
    }
    
    if (params.numberOfPasses > 10) {
        errors << "Number of passes seems excessive (>10). ";
    }
    
    // Validate tolerance parameters
    if (params.profileTolerance <= 0.0) {
        errors << "Profile tolerance must be positive. ";
    }
    
    if (params.dimensionalTolerance <= 0.0) {
        errors << "Dimensional tolerance must be positive. ";
    }
    
    return errors.str();
}

std::unique_ptr<Toolpath> FinishingOperation::generateToolpath(const Geometry::Part& part) {
    // Extract 2D profile from part for tool-agnostic implementation
    ProfileExtractor::ExtractionParameters extractParams;
    extractParams.tolerance = params_.profileTolerance;         // High precision for finishing
    extractParams.minSegmentLength = 0.0001;                   // Very fine segments for smooth finish
    extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Standard lathe Z-axis
    extractParams.sortSegments = true;                         // Ensure proper ordering
    
    // Get part shape for profile extraction
    // Note: In real implementation, this would be extracted from Geometry::Part
    TopoDS_Shape partShape; // This would come from part.getShape() or similar
    auto profile = ProfileExtractor::extractProfile(partShape, extractParams);
    
    if (profile.isEmpty()) {
        // Return empty toolpath if no profile available
        auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
        return toolpath;
    }
    
    // Generate finishing toolpath based on strategy
    return generateProfileBasedFinishing(profile);
}

std::unique_ptr<Toolpath> FinishingOperation::generateProfileBasedFinishing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Generate toolpath based on finishing strategy
    switch (params_.strategy) {
        case FinishingStrategy::SinglePass:
            return generateSinglePassFinishing(profile);
            
        case FinishingStrategy::MultiPass:
            return generateMultiPassFinishing(profile);
            
        case FinishingStrategy::SpringPass:
            return generateSpringPassFinishing(profile);
            
        case FinishingStrategy::ClimbFinishing:
        case FinishingStrategy::ConventionalFinishing:
            // For now, use multi-pass finishing
            return generateMultiPassFinishing(profile);
            
        default:
            return generateMultiPassFinishing(profile);
    }
}

std::unique_ptr<Toolpath> FinishingOperation::generateSinglePassFinishing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Optimize profile for finishing
    auto optimizedProfile = optimizeProfileForFinishing(profile);
    
    if (optimizedProfile.empty()) {
        return toolpath; // Return empty toolpath
    }
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Rapid to safe position
    double maxRadius = 0.0;
    for (const auto& point : optimizedProfile) {
        maxRadius = std::max(maxRadius, point.x);
    }
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, maxRadius + params_.clearanceDistance));
    
    // Add approach move to start of profile
    addApproachMove(toolpath.get(), Geometry::Point3D(optimizedProfile.front().z, 0.0, optimizedProfile.front().x));
    
    // Generate single finishing pass following profile exactly
    for (size_t i = 0; i < optimizedProfile.size(); ++i) {
        const auto& point = optimizedProfile[i];
        
        // Calculate adaptive feed rate if enabled
        double feedRate = params_.feedRate;
        if (params_.adaptiveFeedRate && i < optimizedProfile.size() - 1) {
            feedRate = calculateAdaptiveFeedRate(point, optimizedProfile[i + 1]);
        }
        
        // Calculate spindle speed for constant surface speed
        double spindleSpeed = calculateSpindleSpeed(point.x * 2.0); // Convert radius to diameter
        
        // Add finishing move with optimized parameters
        Geometry::Point3D toolpathPoint(point.z, 0.0, point.x);
        addFinishingMove(toolpath.get(), toolpathPoint, feedRate * 60.0); // Convert mm/rev to mm/min
        
        // Add dwell at sharp corners if enabled
        if (params_.enableDwells && i > 0 && i < optimizedProfile.size() - 1) {
            // Calculate angle change to detect sharp corners
            const auto& prevPoint = optimizedProfile[i - 1];
            const auto& nextPoint = optimizedProfile[i + 1];
            
            double angle1 = std::atan2(point.x - prevPoint.x, point.z - prevPoint.z);
            double angle2 = std::atan2(nextPoint.x - point.x, nextPoint.z - point.z);
            double angleChange = std::abs(angle2 - angle1);
            
            if (angleChange > M_PI / 6) { // 30 degree threshold
                toolpath->addDwell(params_.dwellTime);
            }
        }
    }
    
    // Add retract move
    addRetractMove(toolpath.get(), Geometry::Point3D(optimizedProfile.back().z, 0.0, optimizedProfile.back().x));
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, optimizedProfile.back().x));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FinishingOperation::generateMultiPassFinishing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Optimize profile for finishing
    auto optimizedProfile = optimizeProfileForFinishing(profile);
    
    if (optimizedProfile.empty()) {
        return toolpath; // Return empty toolpath
    }
    
    double safeZ = params_.startZ + params_.safetyHeight;
    double totalStockToRemove = params_.stockAllowance - params_.finalStockAllowance;
    
    // Calculate depth per pass
    double depthPerPass = std::min(params_.depthOfCut, totalStockToRemove / params_.numberOfPasses);
    
    // Rapid to safe position
    double maxRadius = 0.0;
    for (const auto& point : optimizedProfile) {
        maxRadius = std::max(maxRadius, point.x);
    }
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, maxRadius + params_.clearanceDistance));
    
    // Generate multiple finishing passes
    for (int pass = 0; pass < params_.numberOfPasses; ++pass) {
        double currentStockAllowance = params_.stockAllowance - (pass + 1) * depthPerPass;
        currentStockAllowance = std::max(currentStockAllowance, params_.finalStockAllowance);
        
        // Calculate feed rate (slower for final passes)
        double passRatio = static_cast<double>(pass + 1) / params_.numberOfPasses;
        double currentFeedRate = params_.feedRate * (1.0 - passRatio * 0.3); // Reduce feed by up to 30%
        
        // Add approach move
        addApproachMove(toolpath.get(), Geometry::Point3D(optimizedProfile.front().z, 0.0, 
                                                          optimizedProfile.front().x - currentStockAllowance));
        
        // Generate pass following profile
        for (size_t i = 0; i < optimizedProfile.size(); ++i) {
            const auto& point = optimizedProfile[i];
            
            // Offset profile by current stock allowance
            double finishRadius = point.x - currentStockAllowance;
            
            // Calculate adaptive feed rate if enabled
            double feedRate = currentFeedRate;
            if (params_.adaptiveFeedRate && i < optimizedProfile.size() - 1) {
                feedRate = calculateAdaptiveFeedRate(point, optimizedProfile[i + 1]) * (1.0 - passRatio * 0.3);
            }
            
            // Add finishing move
            Geometry::Point3D toolpathPoint(point.z, 0.0, finishRadius);
            addFinishingMove(toolpath.get(), toolpathPoint, feedRate * 60.0);
        }
        
        // Add retract move
        addRetractMove(toolpath.get(), Geometry::Point3D(optimizedProfile.back().z, 0.0, 
                                                        optimizedProfile.back().x - currentStockAllowance));
    }
    
    // Generate spring pass if enabled
    if (params_.enableSpringPass) {
        auto springPassToolpath = generateSpringPassFinishing(profile);
        // Merge spring pass moves into main toolpath
        // Note: This would require a method to merge toolpaths
    }
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, optimizedProfile.back().x));
    
    return toolpath;
}

std::unique_ptr<Toolpath> FinishingOperation::generateSpringPassFinishing(const LatheProfile::Profile2D& profile) {
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Optimize profile for finishing
    auto optimizedProfile = optimizeProfileForFinishing(profile);
    
    if (optimizedProfile.empty()) {
        return toolpath; // Return empty toolpath
    }
    
    double safeZ = params_.startZ + params_.safetyHeight;
    
    // Rapid to safe position
    double maxRadius = 0.0;
    for (const auto& point : optimizedProfile) {
        maxRadius = std::max(maxRadius, point.x);
    }
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, maxRadius + params_.clearanceDistance));
    
    // Add approach move to start of profile
    addApproachMove(toolpath.get(), Geometry::Point3D(optimizedProfile.front().z, 0.0, 
                                                      optimizedProfile.front().x - params_.finalStockAllowance));
    
    // Generate spring pass at final dimension with slow feed
    for (size_t i = 0; i < optimizedProfile.size(); ++i) {
        const auto& point = optimizedProfile[i];
        
        // Use spring pass feed rate (typically slower)
        double feedRate = params_.springPassFeedRate;
        
        // Calculate final radius
        double finalRadius = point.x - params_.finalStockAllowance;
        
        // Add spring pass move
        Geometry::Point3D toolpathPoint(point.z, 0.0, finalRadius);
        addFinishingMove(toolpath.get(), toolpathPoint, feedRate * 60.0);
        
        // Add dwells at critical points for surface finish
        if (params_.enableDwells && i % 10 == 0) { // Every 10th point
            toolpath->addDwell(params_.dwellTime);
        }
    }
    
    // Add retract move
    addRetractMove(toolpath.get(), Geometry::Point3D(optimizedProfile.back().z, 0.0, 
                                                    optimizedProfile.back().x - params_.finalStockAllowance));
    
    // Return to safe position
    toolpath->addRapidMove(Geometry::Point3D(safeZ, 0.0, optimizedProfile.back().x));
    
    return toolpath;
}

std::vector<IntuiCAM::Geometry::Point2D> FinishingOperation::optimizeProfileForFinishing(const LatheProfile::Profile2D& profile) {
    std::vector<IntuiCAM::Geometry::Point2D> optimizedProfile;
    
    // Convert profile segments to points with high resolution for finishing
    for (const auto& segment : profile.segments) {
        optimizedProfile.push_back(segment.start);
        
        // Add intermediate points for curved segments
        if (!segment.isLinear) {
            // For curved segments, add more points for smooth finish
            constexpr int intermediatePoints = 5;
            for (int i = 1; i < intermediatePoints; ++i) {
                double t = static_cast<double>(i) / intermediatePoints;
                IntuiCAM::Geometry::Point2D interpPoint;
                interpPoint.x = segment.start.x + t * (segment.end.x - segment.start.x);
                interpPoint.z = segment.start.z + t * (segment.end.z - segment.start.z);
                optimizedProfile.push_back(interpPoint);
            }
        }
        
        optimizedProfile.push_back(segment.end);
    }
    
    // Remove duplicate points
    optimizedProfile.erase(std::unique(optimizedProfile.begin(), optimizedProfile.end(),
        [](const auto& a, const auto& b) {
            return std::abs(a.x - b.x) < 1e-6 && std::abs(a.z - b.z) < 1e-6;
        }), optimizedProfile.end());
    
    // Sort by Z coordinate
    std::sort(optimizedProfile.begin(), optimizedProfile.end(),
        [](const auto& a, const auto& b) {
            return a.z > b.z; // Start from larger Z (towards chuck)
        });
    
    // Filter points within specified Z range
    optimizedProfile.erase(std::remove_if(optimizedProfile.begin(), optimizedProfile.end(),
        [this](const auto& point) {
            return point.z > params_.startZ || point.z < params_.endZ;
        }), optimizedProfile.end());
    
    return optimizedProfile;
}

double FinishingOperation::calculateSpindleSpeed(double diameter) const {
    if (!params_.enableConstantSurfaceSpeed || diameter <= 0.0) {
        return params_.maxSpindleSpeed / 2.0; // Use mid-range speed
    }
    
    // Calculate spindle speed for constant surface speed: N = (1000 * V) / (π * D)
    double spindleSpeed = (1000.0 * params_.surfaceSpeed) / (M_PI * diameter);
    
    // Clamp to spindle speed limits
    return std::max(params_.minSpindleSpeed, std::min(params_.maxSpindleSpeed, spindleSpeed));
}

double FinishingOperation::calculateAdaptiveFeedRate(const IntuiCAM::Geometry::Point2D& point, 
                                                   const IntuiCAM::Geometry::Point2D& nextPoint) const {
    if (!params_.adaptiveFeedRate) {
        return params_.feedRate;
    }
    
    // Calculate curvature approximation
    double deltaZ = std::abs(nextPoint.z - point.z);
    double deltaX = std::abs(nextPoint.x - point.x);
    double segmentLength = std::sqrt(deltaZ * deltaZ + deltaX * deltaX);
    
    if (segmentLength < 1e-6) {
        return params_.feedRate;
    }
    
    // Reduce feed rate for tight curves and increase for straight sections
    double curvatureFactor = deltaZ / segmentLength; // 1.0 for axial cuts, 0.0 for radial cuts
    double adaptiveFactor = 0.7 + 0.3 * curvatureFactor; // 0.7-1.0 range
    
    return params_.feedRate * adaptiveFactor;
}

void FinishingOperation::addFinishingMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& point, double feedRate) {
    toolpath->addLinearMove(point, feedRate, OperationType::ExternalFinishing, "Finishing cut");
}

void FinishingOperation::addApproachMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& startPoint) {
    // Calculate approach position with clearance
    double approachZ = startPoint.x + params_.clearanceDistance;
    double approachRadius = startPoint.z + params_.clearanceDistance;
    
    // Rapid to approach position
    toolpath->addRapidMove(Geometry::Point3D(approachZ, 0.0, approachRadius), 
                          OperationType::ExternalFinishing, "Approach to finishing position");
    
    // Feed to actual start position
    toolpath->addLinearMove(startPoint, params_.feedRate * 60.0, 
                           OperationType::ExternalFinishing, "Feed to finishing start");
}

void FinishingOperation::addRetractMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& endPoint) {
    // Calculate retract position
    double retractZ = endPoint.x + params_.retractDistance;
    double retractRadius = endPoint.z;
    
    // Retract from end position
    toolpath->addRapidMove(Geometry::Point3D(retractZ, 0.0, retractRadius), 
                          OperationType::ExternalFinishing, "Retract from finishing");
}

bool FinishingOperation::validate() const {
    std::string error = validateParameters(params_);
    return error.empty();
}

} // namespace Toolpath
} // namespace IntuiCAM
