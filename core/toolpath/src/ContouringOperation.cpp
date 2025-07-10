#include <IntuiCAM/Toolpath/ContouringOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Geometry/Types.h>

#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

ContouringOperation::Result ContouringOperation::generateToolpaths(
    const IntuiCAM::Geometry::Part& part,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    Result result;
    
    // Validate parameters first
    std::string validationError = validateParameters(params);
    if (!validationError.empty()) {
        result.errorMessage = "Parameter validation failed: " + validationError;
        return result;
    }
    
    if (!tool) {
        result.errorMessage = "Tool is required for contouring operation";
        return result;
    }
    
    try {
        // Extract 2D profile from part geometry
        result.extractedProfile = extractProfile(part, params);
        if (result.extractedProfile.empty()) {
            result.errorMessage = "Failed to extract valid profile from part geometry";
            return result;
        }
        
        // Plan operation sequence based on profile characteristics
        auto operationSequence = planOperationSequence(result.extractedProfile, params);
        
        // Generate toolpaths for each enabled operation in sequence
        for (const auto& operation : operationSequence) {
            if (operation == "facing" && params.enableFacing) {
                result.facingToolpath = generateFacingPass(
                    result.extractedProfile, tool, params);
                if (!result.facingToolpath) {
                    result.errorMessage = "Failed to generate facing toolpath";
                    return result;
                }
                result.totalMoves += static_cast<int>(result.facingToolpath->getMovementCount());
            }
            else if (operation == "roughing" && params.enableRoughing) {
                result.roughingToolpath = generateRoughingPass(
                    result.extractedProfile, tool, params);
                if (!result.roughingToolpath) {
                    result.errorMessage = "Failed to generate roughing toolpath";
                    return result;
                }
                result.totalMoves += static_cast<int>(result.roughingToolpath->getMovementCount());
            }
            else if (operation == "finishing" && params.enableFinishing) {
                result.finishingToolpath = generateFinishingPass(
                    result.extractedProfile, tool, params);
                if (!result.finishingToolpath) {
                    result.errorMessage = "Failed to generate finishing toolpath";
                    return result;
                }
                result.totalMoves += static_cast<int>(result.finishingToolpath->getMovementCount());
            }
        }
        
        // Calculate statistics
        result.estimatedTime = estimateTotalTime(result, tool);
        result.materialRemoved = calculateMaterialRemoval(result.extractedProfile, params);
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = "Exception during contouring generation: " + std::string(e.what());
    } catch (...) {
        result.errorMessage = "Unknown error during contouring generation";
    }
    
    return result;
}

LatheProfile::Profile2D ContouringOperation::extractProfile(
    const IntuiCAM::Geometry::Part& part,
    const Parameters& params) {
    
    // Use ProfileExtractor for consistent profile extraction
    ProfileExtractor::ExtractionParameters extractParams;
    
    // Convert ContouringOperation parameters to ProfileExtractor parameters
    extractParams.profileTolerance = params.profileTolerance;
    extractParams.profileSections = params.profileSections;
    extractParams.includeInternalFeatures = true;
    extractParams.autoDetectFeatures = true;
    extractParams.optimizeProfile = true;
    
    // Get turning axis from part (this would need proper implementation)
    // For now, assume standard Z-axis turning
    extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    
    // Extract profile from part geometry
    TopoDS_Shape partShape; // This would come from the Part object
    return ProfileExtractor::extractProfile(partShape, extractParams);
}

std::string ContouringOperation::validateParameters(const Parameters& params) {
    if (params.safetyHeight <= 0.0) {
        return "Safety height must be positive";
    }
    
    if (params.clearanceDistance <= 0.0) {
        return "Clearance distance must be positive";
    }
    
    if (params.profileTolerance <= 0.0) {
        return "Profile tolerance must be positive";
    }
    
    if (params.profileSections < 10) {
        return "Profile sections must be at least 10";
    }
    
    // Check that at least one operation is enabled
    if (!params.enableFacing && !params.enableRoughing && !params.enableFinishing) {
        return "At least one sub-operation (facing, roughing, or finishing) must be enabled";
    }
    
    // Validate individual operation parameters if enabled
    if (params.enableFacing) {
        std::string facingError = FacingOperation::validateParameters(params.facingParams);
        if (!facingError.empty()) {
            return "Facing parameters invalid: " + facingError;
        }
    }
    
    if (params.enableRoughing) {
        std::string roughingError = RoughingOperation::validateParameters(params.roughingParams);
        if (!roughingError.empty()) {
            return "Roughing parameters invalid: " + roughingError;
        }
    }
    
    if (params.enableFinishing) {
        std::string finishingError = FinishingOperation::validateParameters(params.finishingParams);
        if (!finishingError.empty()) {
            return "Finishing parameters invalid: " + finishingError;
        }
    }
    
    return ""; // Valid
}

ContouringOperation::Parameters ContouringOperation::getDefaultParameters(
    const std::string& materialType,
    const std::string& partComplexity) {
    
    Parameters params; // Start with constructor defaults
    
    // Get material properties for parameter adjustment
    auto materialProps = OperationParameterManager::getMaterialProperties(materialType);
    
    // Store material properties for later use by tools
    // Note: Feed rates and speeds will be set in Tool's CuttingParameters
    // The operation parameters only contain geometric information
    
    // Adjust based on part complexity
    if (partComplexity == "simple") {
        params.profileSections = 50;
        params.profileTolerance = 0.02;
    } else if (partComplexity == "complex") {
        params.profileSections = 200;
        params.profileTolerance = 0.005;
        params.enableFacing = true; // Always face complex parts
    }
    
    // Material-specific adjustments will be applied to tool parameters during toolpath generation
    
    return params;
}

std::unique_ptr<Toolpath> ContouringOperation::generateFacingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    if (profile.empty()) {
        return nullptr;
    }
    
    // Create facing toolpath
    auto toolpath = std::make_unique<Toolpath>("Facing_Pass", tool);
    
    // Find maximum radius and face boundaries
    double maxRadius = 0.0;
    double minZ = std::numeric_limits<double>::max();
    double maxZ = std::numeric_limits<double>::lowest();
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.z);
        minZ = std::min(minZ, point.x);
        maxZ = std::max(maxZ, point.x);
    }
    
    // Generate facing passes from outside to center
    double currentRadius = maxRadius + params.clearanceDistance;
    double facingDepth = tool->getCuttingParameters().depthOfCut;
    
    // Rapid to start position (safe height above face)
    gp_Pnt startPos(maxZ + params.safetyHeight, 0.0, currentRadius);
    toolpath->addRapidMove(Geometry::Point3D(startPos.Z(), startPos.Y(), startPos.X()));
    
    // Rapid down to clearance
    gp_Pnt clearancePos(maxZ + params.clearanceDistance, 0.0, currentRadius);
    toolpath->addRapidMove(Geometry::Point3D(clearancePos.Z(), clearancePos.Y(), clearancePos.X()));
    
    // Generate facing passes
    while (currentRadius > 0.1) { // Stop near center
        // Feed to face
        gp_Pnt faceStart(maxZ, 0.0, currentRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(faceStart.Z(), faceStart.Y(), faceStart.X()),
            tool->getCuttingParameters().feedRate * 60.0); // Convert mm/rev to mm/min
        
        // Face across to center (or inner radius)
        double targetRadius = std::max(0.0, currentRadius - facingDepth);
        gp_Pnt faceEnd(maxZ, 0.0, targetRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(faceEnd.Z(), faceEnd.Y(), faceEnd.X()),
            tool->getCuttingParameters().feedRate * 60.0); // Convert mm/rev to mm/min
        
        // Rapid back to clearance
        gp_Pnt retractPos(maxZ + params.clearanceDistance, 0.0, targetRadius);
        toolpath->addRapidMove(Geometry::Point3D(retractPos.Z(), retractPos.Y(), retractPos.X()));
        
        currentRadius = targetRadius;
    }
    
    // Return to safe position
    gp_Pnt safePos(maxZ + params.safetyHeight, 0.0, 0.0);
    toolpath->addRapidMove(Geometry::Point3D(safePos.Z(), safePos.Y(), safePos.X()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ContouringOperation::generateRoughingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    if (profile.empty()) {
        return nullptr;
    }
    
    auto toolpath = std::make_unique<Toolpath>("Roughing_Pass", tool);
    
    // Calculate stock allowance for finishing
    double stockAllowance = params.enableFinishing ? 
        tool->getCuttingParameters().depthOfCut * 0.3 : 0.0; // Light finishing allowance
    
    // Generate roughing passes parallel to the profile
    double currentDepth = tool->getCuttingParameters().depthOfCut;
    
    // Find profile bounds
    double minZ = profile.front().x;
    double maxZ = profile.back().x;
    double maxRadius = 0.0;
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.z);
    }
    
    // Start from safe position
    gp_Pnt startPos(maxZ + params.safetyHeight, 0.0, maxRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(startPos.Z(), startPos.Y(), startPos.X()));
    
    // Generate roughing passes from outside radius inward
    double currentRadius = maxRadius;
    
    while (currentRadius > stockAllowance) {
        // Rapid to start of pass
        gp_Pnt passStart(maxZ + params.clearanceDistance, 0.0, currentRadius);
        toolpath->addRapidMove(Geometry::Point3D(passStart.Z(), passStart.Y(), passStart.X()));
        
        // Feed down to cutting depth
        gp_Pnt cutStart(maxZ, 0.0, currentRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(cutStart.Z(), cutStart.Y(), cutStart.X()),
            tool->getCuttingParameters().feedRate * 60.0); // Convert mm/rev to mm/min
        
        // Follow profile with offset for stock allowance
        for (const auto& point : profile) {
            double offsetRadius = point.z + stockAllowance;
            if (offsetRadius <= currentRadius) {
                gp_Pnt cutPoint(point.x, 0.0, offsetRadius);
                toolpath->addLinearMove(
                    Geometry::Point3D(cutPoint.Z(), cutPoint.Y(), cutPoint.X()),
                    tool->getCuttingParameters().feedRate * 60.0); // Convert mm/rev to mm/min
            }
        }
        
        // Move to next pass depth
        currentRadius -= currentDepth;
    }
    
    // Return to safe position
    gp_Pnt endPos(maxZ + params.safetyHeight, 0.0, maxRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(endPos.Z(), endPos.Y(), endPos.X()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ContouringOperation::generateFinishingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    if (profile.empty()) {
        return nullptr;
    }
    
    auto toolpath = std::make_unique<Toolpath>("Finishing_Pass", tool);
    
    // Find profile bounds
    double minZ = profile.front().x;
    double maxZ = profile.back().x;
    double maxRadius = 0.0;
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.z);
    }
    
    // Start from safe position
    gp_Pnt startPos(maxZ + params.safetyHeight, 0.0, maxRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(startPos.Z(), startPos.Y(), startPos.X()));
    
    // Rapid to start of finishing pass
    gp_Pnt finishStart(maxZ + params.clearanceDistance, 0.0, maxRadius);
    toolpath->addRapidMove(Geometry::Point3D(finishStart.Z(), finishStart.Y(), finishStart.X()));
    
    // Feed to first profile point
    if (!profile.empty()) {
        const auto& firstPoint = profile.front();
        gp_Pnt firstCut(firstPoint.x, 0.0, firstPoint.z);
        toolpath->addLinearMove(
            Geometry::Point3D(firstCut.Z(), firstCut.Y(), firstCut.X()),
            params.finishingParams.feedRate);
        
        // Follow the exact profile for finishing
        for (size_t i = 1; i < profile.size(); ++i) {
            const auto& point = profile[i];
            gp_Pnt cutPoint(point.x, 0.0, point.z);
            toolpath->addLinearMove(
                Geometry::Point3D(cutPoint.Z(), cutPoint.Y(), cutPoint.X()),
                params.finishingParams.feedRate);
        }
    }
    
    // Return to safe position
    gp_Pnt endPos(maxZ + params.safetyHeight, 0.0, maxRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(endPos.Z(), endPos.Y(), endPos.X()));
    
    return toolpath;
}

std::vector<std::string> ContouringOperation::planOperationSequence(
    const LatheProfile::Profile2D& profile,
    const Parameters& params) {
    
    std::vector<std::string> sequence;
    
    // Standard sequence for most parts: facing -> roughing -> finishing
    if (params.enableFacing) {
        sequence.push_back("facing");
    }
    
    if (params.enableRoughing) {
        sequence.push_back("roughing");
    }
    
    if (params.enableFinishing) {
        sequence.push_back("finishing");
    }
    
    // For complex profiles, we might want to adjust the sequence
    // This is where advanced planning logic could go
    
    return sequence;
}

double ContouringOperation::estimateTotalTime(const Result& result, std::shared_ptr<Tool> tool) {
    double totalTime = 0.0;
    
    if (result.facingToolpath) {
        totalTime += result.facingToolpath->estimateMachiningTime();
    }
    
    if (result.roughingToolpath) {
        totalTime += result.roughingToolpath->estimateMachiningTime();
    }
    
    if (result.finishingToolpath) {
        totalTime += result.finishingToolpath->estimateMachiningTime();
    }
    
    // Add setup and positioning time (estimated 10% overhead)
    totalTime *= 1.1;
    
    return totalTime;
}

double ContouringOperation::calculateMaterialRemoval(
    const LatheProfile::Profile2D& profile,
    const Parameters& params) {
    
    if (profile.size() < 2) {
        return 0.0;
    }
    
    double totalVolume = 0.0;
    
    // Calculate volume using trapezoidal rule for revolution solid
    for (size_t i = 1; i < profile.size(); ++i) {
        const auto& p1 = profile[i-1];
        const auto& p2 = profile[i];
        
        double height = std::abs(p2.x - p1.x);
        double r1 = p1.z;
        double r2 = p2.z;
        
        // Volume of truncated cone segment
        double segmentVolume = M_PI * height * (r1*r1 + r1*r2 + r2*r2) / 3.0;
        totalVolume += segmentVolume;
    }
    
    return totalVolume;
}

} // namespace Toolpath
} // namespace IntuiCAM
