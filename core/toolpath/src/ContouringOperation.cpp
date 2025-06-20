#include <IntuiCAM/Toolpath/ContouringOperation.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <limits>

namespace IntuiCAM {
namespace Toolpath {

ContouringOperation::Result ContouringOperation::generateToolpaths(
    const IntuiCAM::Geometry::Part& part,
    std::shared_ptr<Tool> tool,
    const Parameters& params)
{
    Result result;
    
    // Validate parameters first
    std::string validationError = validateParameters(params);
    if (!validationError.empty()) {
        result.success = false;
        result.errorMessage = "Parameter validation failed: " + validationError;
        return result;
    }
    
    try {
        // Extract 2D profile from the part
        result.extractedProfile = extractProfile(part, params);
        
        if (result.extractedProfile.empty()) {
            result.success = false;
            result.errorMessage = "Failed to extract valid profile from part geometry";
            return result;
        }
        
        // Plan the operation sequence based on enabled operations and profile
        auto operationSequence = planOperationSequence(result.extractedProfile, params);
        
        // Generate toolpaths for each enabled operation in sequence
        for (const auto& operation : operationSequence) {
            if (operation == "facing" && params.enableFacing) {
                result.facingToolpath = generateFacingPass(result.extractedProfile, tool, params);
                if (!result.facingToolpath) {
                    result.success = false;
                    result.errorMessage = "Failed to generate facing toolpath";
                    return result;
                }
            }
            else if (operation == "roughing" && params.enableRoughing) {
                result.roughingToolpath = generateRoughingPass(result.extractedProfile, tool, params);
                if (!result.roughingToolpath) {
                    result.success = false;
                    result.errorMessage = "Failed to generate roughing toolpath";
                    return result;
                }
            }
            else if (operation == "finishing" && params.enableFinishing) {
                result.finishingToolpath = generateFinishingPass(result.extractedProfile, tool, params);
                if (!result.finishingToolpath) {
                    result.success = false;
                    result.errorMessage = "Failed to generate finishing toolpath";
                    return result;
                }
            }
        }
        
        // Calculate statistics
        result.estimatedTime = estimateTotalTime(result, tool);
        result.materialRemoved = calculateMaterialRemoval(result.extractedProfile, params);
        
        // Count total moves across all toolpaths
        result.totalMoves = 0;
        if (result.facingToolpath) result.totalMoves += result.facingToolpath->getMovements().size();
        if (result.roughingToolpath) result.totalMoves += result.roughingToolpath->getMovements().size();
        if (result.finishingToolpath) result.totalMoves += result.finishingToolpath->getMovements().size();
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception during toolpath generation: " + std::string(e.what());
    }
    
    return result;
}

LatheProfile::Profile2D ContouringOperation::extractProfile(
    const IntuiCAM::Geometry::Part& part,
    const Parameters& params)
{
    return LatheProfile::extract(part, params.profileSections, params.profileTolerance);
}

std::string ContouringOperation::validateParameters(const Parameters& params)
{
    std::vector<std::string> errors;
    
    // Validate safety and clearance
    if (params.safetyHeight <= 0.0) {
        errors.push_back("Safety height must be positive");
    }
    if (params.clearanceDistance <= 0.0) {
        errors.push_back("Clearance distance must be positive");
    }
    
    // Validate profile extraction parameters
    if (params.profileTolerance <= 0.0) {
        errors.push_back("Profile tolerance must be positive");
    }
    if (params.profileSections < 10) {
        errors.push_back("Profile sections must be at least 10 for reasonable accuracy");
    }
    if (params.profileSections > 1000) {
        errors.push_back("Profile sections should not exceed 1000 for performance reasons");
    }
    
    // Check that at least one operation is enabled
    if (!params.enableFacing && !params.enableRoughing && !params.enableFinishing) {
        errors.push_back("At least one operation (facing, roughing, or finishing) must be enabled");
    }
    
    // Combine all errors into a single message
    if (errors.empty()) {
        return "";
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < errors.size(); ++i) {
        if (i > 0) ss << "; ";
        ss << errors[i];
    }
    return ss.str();
}

ContouringOperation::Parameters ContouringOperation::getDefaultParameters(
    const std::string& materialType,
    const std::string& partComplexity)
{
    Parameters params;
    
    // Adjust parameters based on material type (using actual parameter names)
    if (materialType == "aluminum") {
        // FacingOperation parameters use stepover, not feedRate in params
        params.roughingParams.depthOfCut = 3.0;   // mm - can be more aggressive  
        params.finishingParams.feedRate = 0.1;    // mm/rev - faster for aluminum
    }
    else if (materialType == "steel") {
        params.roughingParams.depthOfCut = 2.0;   // mm - moderate cut
        params.finishingParams.feedRate = 0.05;   // mm/rev - conservative for steel
    }
    else if (materialType == "stainless") {
        params.roughingParams.depthOfCut = 1.5;   // mm - very conservative
        params.finishingParams.feedRate = 0.03;   // mm/rev - slow for work hardening
    }
    
    // Adjust based on part complexity
    if (partComplexity == "simple") {
        params.profileSections = 50;               // Fewer sections for simple parts
        params.roughingParams.depthOfCut = 2.0;   // mm - larger cuts
    }
    else if (partComplexity == "complex") {
        params.profileSections = 200;              // More sections for complex geometry
        params.roughingParams.depthOfCut = 0.5;   // mm - smaller cuts for precision
        params.finishingParams.feedRate = params.finishingParams.feedRate * 0.5; // Slower finish
    }
    
    return params;
}

std::unique_ptr<Toolpath> ContouringOperation::generateFacingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params)
{
    if (profile.empty()) {
        return nullptr;
    }
    
    // Create facing operation and generate toolpath
    FacingOperation facingOp("Facing", tool);
    
    // Find the maximum radius and front face Z position from profile
    double maxRadius = 0.0;
    double frontZ = profile.front().z; // Assuming profile is sorted by Z
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.x); // x is radius in Point2D
    }
    
    // Set up facing parameters based on profile geometry
    FacingOperation::Parameters facingParams;
    facingParams.startDiameter = (maxRadius + params.clearanceDistance) * 2.0;
    facingParams.endDiameter = 0.0; // Face to center
    facingParams.stepover = 0.5;
    facingParams.stockAllowance = 0.2;
    facingParams.roughingOnly = false;
    
    facingOp.setParameters(facingParams);
    
    // Create a dummy part for the operation
    auto bbox = Geometry::BoundingBox(
        Geometry::Point3D(-maxRadius, -maxRadius, frontZ - 1),
        Geometry::Point3D(maxRadius, maxRadius, frontZ + 1)
    );
    
    // We need to create a Part object, but for now generate a basic facing toolpath
    auto toolpath = std::make_unique<Toolpath>("Facing", tool);
    
    // Generate basic facing moves
    double currentRadius = maxRadius + params.clearanceDistance;
    double endRadius = 0.0;
    double z = frontZ;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(currentRadius + 5.0, 0, z + 5.0));
    toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, z + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, z), tool->getCuttingParameters().feedRate);
    
    // Face from outside to center
    while (currentRadius > endRadius) {
        toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, z), tool->getCuttingParameters().feedRate);
        
        // Retract and move to next pass
        toolpath->addRapidMove(Geometry::Point3D(endRadius, 0, z + 2.0));
        currentRadius -= facingParams.stepover;
        
        if (currentRadius > endRadius) {
            toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, z + 2.0));
            toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, z), tool->getCuttingParameters().feedRate);
        }
    }
    
    // Final retract
    toolpath->addRapidMove(Geometry::Point3D(endRadius, 0, z + 10.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ContouringOperation::generateRoughingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params)
{
    if (profile.empty()) {
        return nullptr;
    }
    
    // Create roughing operation and generate toolpath
    RoughingOperation roughingOp("Roughing", tool);
    
    // Set profile bounds
    double minZ = profile.front().z;
    double maxZ = profile.back().z;
    double maxRadius = 0.0;
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.x); // x is radius in Point2D
    }
    
    // Convert profile to roughing operation format
    RoughingOperation::Parameters roughingParams;
    roughingParams.startDiameter = (maxRadius + params.clearanceDistance) * 2.0;
    roughingParams.endDiameter = 20.0; // Conservative end diameter
    roughingParams.startZ = minZ;
    roughingParams.endZ = maxZ;
    roughingParams.depthOfCut = 2.0;
    roughingParams.stockAllowance = 0.5;
    
    roughingOp.setParameters(roughingParams);
    
    // Generate basic roughing toolpath
    auto toolpath = std::make_unique<Toolpath>("Roughing", tool);
    
    double currentZ = minZ;
    double startRadius = maxRadius + params.clearanceDistance;
    double endRadius = 10.0;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(startRadius + 5.0, 0, currentZ + 5.0));
    
    while (currentZ < maxZ) {
        // Rapid to start of cut
        toolpath->addRapidMove(Geometry::Point3D(startRadius + 2.0, 0, currentZ + 2.0));
        toolpath->addRapidMove(Geometry::Point3D(startRadius, 0, currentZ + 2.0));
        toolpath->addLinearMove(Geometry::Point3D(startRadius, 0, currentZ), tool->getCuttingParameters().feedRate);
        
        // Cut from outside to inside
        toolpath->addLinearMove(Geometry::Point3D(endRadius, 0, currentZ), tool->getCuttingParameters().feedRate);
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(endRadius, 0, currentZ + 2.0));
        
        currentZ += roughingParams.depthOfCut;
    }
    
    // Final retract
    toolpath->addRapidMove(Geometry::Point3D(endRadius, 0, maxZ + 10.0));
    
    return toolpath;
}

std::unique_ptr<Toolpath> ContouringOperation::generateFinishingPass(
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const Parameters& params)
{
    if (profile.empty()) {
        return nullptr;
    }
    
    // Create finishing operation and generate toolpath
    FinishingOperation finishingOp("Finishing", tool);
    
    // Find profile bounds
    double minZ = profile.front().z;
    double maxZ = profile.back().z;
    double maxRadius = 0.0;
    
    for (const auto& point : profile) {
        maxRadius = std::max(maxRadius, point.x); // x is radius in Point2D
    }
    
    // Set up finishing parameters to follow the exact profile
    FinishingOperation::Parameters finishingParams;
    finishingParams.targetDiameter = maxRadius * 2.0;
    finishingParams.startZ = minZ;
    finishingParams.endZ = maxZ;
    finishingParams.surfaceSpeed = 150.0;
    finishingParams.feedRate = 0.05;
    
    finishingOp.setParameters(finishingParams);
    
    // Generate basic finishing toolpath
    auto toolpath = std::make_unique<Toolpath>("Finishing", tool);
    
    double radius = maxRadius;
    
    // Rapid to start position
    toolpath->addRapidMove(Geometry::Point3D(radius + 5.0, 0, minZ + 5.0));
    toolpath->addRapidMove(Geometry::Point3D(radius + 2.0, 0, minZ + 2.0));
    toolpath->addLinearMove(Geometry::Point3D(radius, 0, minZ), tool->getCuttingParameters().feedRate);
    
    // Single finishing pass along profile
    toolpath->addLinearMove(Geometry::Point3D(radius, 0, maxZ), finishingParams.feedRate);
    
    // Final retract
    toolpath->addRapidMove(Geometry::Point3D(radius + 5.0, 0, maxZ + 10.0));
    
    return toolpath;
}

std::vector<std::string> ContouringOperation::planOperationSequence(
    const LatheProfile::Profile2D& profile,
    const Parameters& params)
{
    std::vector<std::string> sequence;
    
    // Optimal sequence for contouring: facing first, then roughing, then finishing
    if (params.enableFacing) {
        sequence.push_back("facing");
    }
    if (params.enableRoughing) {
        sequence.push_back("roughing");
    }
    if (params.enableFinishing) {
        sequence.push_back("finishing");
    }
    
    return sequence;
}

double ContouringOperation::estimateTotalTime(const Result& result, std::shared_ptr<Tool> tool)
{
    double totalTime = 0.0;
    
    // Estimate time for each toolpath based on move count and typical feeds/speeds
    if (result.facingToolpath) {
        // Facing typically uses higher feed rates
        totalTime += result.facingToolpath->getMovements().size() * 0.1; // 0.1 min per move estimate
    }
    
    if (result.roughingToolpath) {
        // Roughing takes longer due to material removal
        totalTime += result.roughingToolpath->getMovements().size() * 0.2; // 0.2 min per move estimate
    }
    
    if (result.finishingToolpath) {
        // Finishing uses slower feeds for surface quality
        totalTime += result.finishingToolpath->getMovements().size() * 0.15; // 0.15 min per move estimate
    }
    
    // Add setup time
    totalTime += 2.0; // 2 minutes setup time
    
    return totalTime;
}

double ContouringOperation::calculateMaterialRemoval(
    const LatheProfile::Profile2D& profile,
    const Parameters& params)
{
    if (profile.size() < 2) {
        return 0.0;
    }
    
    double volume = 0.0;
    
    // Calculate volume of revolution using trapezoidal rule
    for (size_t i = 1; i < profile.size(); ++i) {
        const auto& p1 = profile[i-1];
        const auto& p2 = profile[i];
        
        double dz = std::abs(p2.z - p1.z);
        double avgRadius = (p1.x + p2.x) * 0.5; // x is radius in Point2D
        
        // Volume of cylindrical segment
        volume += 3.14159265359 * avgRadius * avgRadius * dz;
    }
    
    return volume;
}

} // namespace Toolpath
} // namespace IntuiCAM 