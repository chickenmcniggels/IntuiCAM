#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Geometry/Types.h>

#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

PartingOperation::PartingOperation(const std::string& name, std::shared_ptr<Tool> tool) 
    : Operation(Operation::Type::Parting, name, tool) {
    // Initialize with default parameters
    params_ = Parameters();
}

std::unique_ptr<Toolpath> PartingOperation::generateToolpath(const Geometry::Part& part) {
    // Use the advanced interface with stored parameters and tool
    auto result = generateToolpaths(part, getTool(), params_);
    
    // Return the main parting toolpath (standard interface expects single toolpath)
    if (result.success && result.partingToolpath) {
        return std::move(result.partingToolpath);
    }
    
    return nullptr;
}

bool PartingOperation::validate() const {
    // Validate that we have a tool and parameters are reasonable
    if (!getTool()) {
        return false;
    }
    
    // Validate parameters using static method
    std::string validationError = validateParameters(params_);
    return validationError.empty();
}

PartingOperation::Result PartingOperation::generateToolpaths(
    const IntuiCAM::Geometry::Part& part,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    Result result;
    result.usedParameters = params;
    
    // Validate parameters
    std::string validationError = validateParameters(params);
    if (!validationError.empty()) {
        result.errorMessage = "Parameter validation failed: " + validationError;
        return result;
    }
    
    if (!tool) {
        result.errorMessage = "Parting tool is required";
        return result;
    }
    
    // Validate tool compatibility
    if (!validateToolCompatibility(tool, params)) {
        result.errorMessage = "Tool is not compatible with parting operation";
        return result;
    }
    
    try {
        // Extract 2D profile for parting position analysis
        ProfileExtractor::ExtractionParameters extractParams;
        extractParams.tolerance = 0.01;
        extractParams.minSegmentLength = 0.001;
        extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        extractParams.sortSegments = true;
        
        // Use the actual part geometry for profile extraction
        TopoDS_Shape partShape;
        const Geometry::OCCTPart* occtPart = dynamic_cast<const Geometry::OCCTPart*>(&part);
        if (occtPart) {
            partShape = occtPart->getOCCTShape();
        }
        
        if (!partShape.IsNull()) {
            LatheProfile::Profile2D profile = ProfileExtractor::extractProfile(partShape, extractParams);
            
            // Detect potential parting positions from profile
            result.detectedPositions = detectPartingPositions(profile, params);
            
            if (!result.detectedPositions.empty()) {
                result.selectedPosition = selectOptimalPosition(result.detectedPositions, params);
                
                // Update parameters with detected position if preference is high
                if (result.selectedPosition.preference > 0.7) {
                    result.usedParameters.partingZ = result.selectedPosition.zPosition;
                    result.usedParameters.partingDiameter = result.selectedPosition.diameter;
                }
            }
        }
        
        // Generate relief groove if strategy requires it
        if (params.strategy == PartingStrategy::Groove || params.enableRoughingGroove) {
            result.grooveToolpath = generateGrooveToolpath(params, tool);
        }
        
        // Generate main parting toolpath
        result.partingToolpath = generateMainPartingToolpath(params, tool);
        
        // Generate finishing pass if enabled
        if (params.enableFinishingPass) {
            result.finishingToolpath = generateFinishingToolpath(params, tool);
        }
        
        // Calculate statistics
        result.estimatedTime = estimatePartingTime(params, tool);
        result.totalPasses = calculateTotalPasses(params);
        result.materialRemoved = calculateMaterialRemoval(params);
        result.partLength = std::abs(params.partingZ);
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = "Error during parting operation: " + std::string(e.what());
        result.success = false;
    }
    
    return result;
}

std::vector<PartingOperation::PartingPosition> PartingOperation::detectPartingPositions(
    const LatheProfile::Profile2D& profile,
    const Parameters& params) {
    
    std::vector<PartingPosition> positions;
    
    if (profile.isEmpty()) {
        // Create a default position based on parameters
        PartingPosition defaultPos;
        defaultPos.zPosition = params.partingZ;
        defaultPos.diameter = params.partingDiameter;
        defaultPos.accessibility = 1.0;
        defaultPos.preference = 0.8;
        defaultPos.description = "Manual position";
        defaultPos.requiresSpecialTool = false;
        
        positions.push_back(defaultPos);
        return positions;
    }
    
    // Analyze profile for potential parting positions
    auto points = profile.toPointArray(0.5); // Sample every 0.5mm
    
    if (points.size() < 3) {
        return positions; // Not enough points for analysis
    }
    
    // Look for straight sections that would be good for parting
    for (size_t i = 1; i < points.size() - 1; ++i) {
        const auto& point = points[i];
        
        // Check if this is a straight vertical section
        bool isStraight = true;
        double radiusTolerance = 0.2; // mm
        
        // Check consistency over several points
        for (size_t j = std::max(1, (int)i - 2); j < std::min(points.size() - 1, i + 3); ++j) {
            if (std::abs(points[j].x - point.x) > radiusTolerance) {
                isStraight = false;
                break;
            }
        }
        
        if (isStraight && point.x > 1.0) { // Must have reasonable diameter
            PartingPosition pos;
            pos.zPosition = point.z;
            pos.diameter = point.x * 2.0; // Convert radius to diameter
            pos.accessibility = 1.0; // Straight section is accessible
            pos.preference = 0.9; // High preference for straight sections
            pos.description = "Straight section";
            pos.requiresSpecialTool = false;
            
            // Prefer positions near the end of the part
            double endBonus = 1.0 - std::abs(point.z) / 100.0; // Closer to end gets bonus
            pos.preference = std::min(1.0, pos.preference + endBonus * 0.1);
            
            positions.push_back(pos);
        }
    }
    
    // If no good positions found, create some default candidates
    if (positions.empty()) {
        // End of part
        PartingPosition endPos;
        endPos.zPosition = params.partingZ;
        endPos.diameter = params.partingDiameter;
        endPos.accessibility = 0.9;
        endPos.preference = 0.8;
        endPos.description = "End position";
        endPos.requiresSpecialTool = false;
        positions.push_back(endPos);
        
        // Middle of part (if long enough)
        if (std::abs(params.partingZ) > 20.0) {
            PartingPosition midPos;
            midPos.zPosition = params.partingZ / 2.0;
            midPos.diameter = params.partingDiameter;
            midPos.accessibility = 0.7;
            midPos.preference = 0.6;
            midPos.description = "Middle position";
            midPos.requiresSpecialTool = false;
            positions.push_back(midPos);
        }
    }
    
    // Sort by preference (highest first)
    std::sort(positions.begin(), positions.end(), 
              [](const PartingPosition& a, const PartingPosition& b) {
                  return a.preference > b.preference;
              });
    
    return positions;
}

PartingOperation::PartingPosition PartingOperation::selectOptimalPosition(
    const std::vector<PartingPosition>& positions,
    const Parameters& params) {
    
    if (positions.empty()) {
        // Return default position
        PartingPosition defaultPos;
        defaultPos.zPosition = params.partingZ;
        defaultPos.diameter = params.partingDiameter;
        defaultPos.accessibility = 1.0;
        defaultPos.preference = 0.5;
        defaultPos.description = "Default position";
        defaultPos.requiresSpecialTool = false;
        return defaultPos;
    }
    
    // Find the position with highest preference that doesn't require special tooling
    for (const auto& pos : positions) {
        if (!pos.requiresSpecialTool) {
            return pos;
        }
    }
    
    // If all require special tooling, return the best one
    return positions[0];
}

std::string PartingOperation::validateParameters(const Parameters& params) {
    if (params.partingDiameter <= 0.0) {
        return "Parting diameter must be positive";
    }
    
    if (params.partingWidth <= 0.0) {
        return "Parting width must be positive";
    }
    
    if (params.feedRate <= 0.0) {
        return "Feed rate must be positive";
    }
    
    if (params.spindleSpeed <= 0.0) {
        return "Spindle speed must be positive";
    }
    
    if (params.depthOfCut <= 0.0) {
        return "Depth of cut must be positive";
    }
    
    if (params.numberOfPasses < 1) {
        return "Number of passes must be at least 1";
    }
    
    return ""; // Valid
}

PartingOperation::Parameters PartingOperation::getDefaultParameters(
    double diameter,
    const std::string& materialType,
    const std::string& partType) {
    
    Parameters params;
    params.partingDiameter = diameter;
    
    // Material-specific defaults
    if (materialType == "aluminum") {
        params.feedRate = 50.0;
        params.spindleSpeed = 1200.0;
        params.depthOfCut = 0.8;
    } else if (materialType == "stainless_steel") {
        params.feedRate = 20.0;
        params.spindleSpeed = 600.0;
        params.depthOfCut = 0.3;
    } else { // steel
        params.feedRate = 30.0;
        params.spindleSpeed = 800.0;
        params.depthOfCut = 0.5;
    }
    
    // Part type adjustments
    if (partType == "thin_wall") {
        params.feedRate *= 0.7;
        params.depthOfCut *= 0.8;
        params.enableFinishingPass = true;
        params.enableCoolant = true;
    }
    
    return params;
}

// Private helper methods
std::unique_ptr<Toolpath> PartingOperation::generateGrooveToolpath(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Parting Groove", tool, OperationType::Parting);
    
    // Simple groove generation - placeholder
    IntuiCAM::Geometry::Point3D grooveStart(params.partingZ + params.grooveWidth, 0.0, params.partingDiameter / 2.0);
    IntuiCAM::Geometry::Point3D grooveEnd(params.partingZ + params.grooveWidth, 0.0, params.partingDiameter / 2.0 - params.grooveDepth);
    
    toolpath->addRapidMove(grooveStart);
    toolpath->addLinearMove(grooveEnd, params.feedRate);
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateMainPartingToolpath(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Main Parting", tool, OperationType::Parting);
    
    // Generate parting passes
    double currentRadius = params.partingDiameter / 2.0;
    double targetRadius = params.centerHoleDiameter / 2.0;
    
    for (int pass = 0; pass < params.numberOfPasses; ++pass) {
        double passRadius = currentRadius - (pass + 1) * params.depthOfCut;
        passRadius = std::max(passRadius, targetRadius);
        
        IntuiCAM::Geometry::Point3D startPos(params.partingZ, 0.0, currentRadius);
        IntuiCAM::Geometry::Point3D endPos(params.partingZ, 0.0, passRadius);
        
        toolpath->addRapidMove(startPos);
        toolpath->addLinearMove(endPos, params.feedRate);
        
        if (passRadius <= targetRadius) break;
    }
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateFinishingToolpath(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Parting Finish", tool, OperationType::Parting);
    
    // Final finishing pass
    IntuiCAM::Geometry::Point3D startPos(params.partingZ, 0.0, params.partingDiameter / 2.0);
    IntuiCAM::Geometry::Point3D endPos(params.partingZ, 0.0, params.centerHoleDiameter / 2.0);
    
    toolpath->addRapidMove(startPos);
    toolpath->addLinearMove(endPos, params.finishingFeedRate);
    
    return toolpath;
}

double PartingOperation::estimatePartingTime(const Parameters& params, std::shared_ptr<Tool> tool) {
    // Simple time estimation
    double cuttingDistance = (params.partingDiameter - params.centerHoleDiameter) / 2.0;
    double timePerPass = cuttingDistance / (params.feedRate / 60.0); // Convert to seconds
    double totalTime = timePerPass * params.numberOfPasses;
    
    if (params.enableFinishingPass) {
        totalTime += timePerPass * 0.5; // Finishing pass
    }
    
    return totalTime / 60.0; // Return in minutes
}

int PartingOperation::calculateTotalPasses(const Parameters& params) {
    int passes = params.numberOfPasses;
    if (params.enableFinishingPass) passes++;
    if (params.enableRoughingGroove) passes++;
    return passes;
}

double PartingOperation::calculateMaterialRemoval(const Parameters& params) {
    // Calculate volume of material removed
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double volume = M_PI * (outerRadius * outerRadius - innerRadius * innerRadius) * params.partingWidth;
    return volume;
}

bool PartingOperation::validateToolCompatibility(std::shared_ptr<Tool> tool, const Parameters& params) {
    if (!tool) return false;
    
    // Check if tool is suitable for parting
    return (tool->getType() == Tool::Type::Parting || tool->getType() == Tool::Type::Grooving);
}

} // namespace Toolpath
} // namespace IntuiCAM
