#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Geometry/Types.h>

#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

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
        extractParams.profileTolerance = 0.01;
        extractParams.profileSections = 100;
        extractParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        
        TopoDS_Shape partShape; // This would come from the Part object
        auto profile = ProfileExtractor::extractProfile(partShape, extractParams);
        
        // Detect potential parting positions if not explicitly specified
        result.detectedPositions = detectPartingPositions(profile, params);
        if (!result.detectedPositions.empty()) {
            result.selectedPosition = selectOptimalPosition(result.detectedPositions, params);
            
            // Update parameters with detected position if not manually specified
            if (result.selectedPosition.isOptimal) {
                result.usedParameters.partingZ = result.selectedPosition.zPosition;
                result.usedParameters.partingDiameter = result.selectedPosition.diameter;
            }
        }
        
        // Generate relief groove if strategy requires it
        if (params.strategy == PartingStrategy::Groove) {
            result.grooveToolpath = generateGrooveRelief(result.usedParameters, tool);
            if (!result.grooveToolpath) {
                result.errorMessage = "Failed to generate groove relief toolpath";
                return result;
            }
        }
        
        // Generate main parting toolpath based on strategy
        switch (params.strategy) {
            case PartingStrategy::Straight:
                result.partingToolpath = generateStraightParting(result.usedParameters, tool);
                break;
            case PartingStrategy::Stepped:
                result.partingToolpath = generateSteppedParting(result.usedParameters, tool);
                break;
            case PartingStrategy::Groove:
            case PartingStrategy::Undercut:
                result.partingToolpath = generateUndercutParting(result.usedParameters, tool);
                break;
            case PartingStrategy::Trepanning:
                result.partingToolpath = generateTrepanningParting(result.usedParameters, tool);
                break;
        }
        
        if (!result.partingToolpath) {
            result.errorMessage = "Failed to generate parting toolpath";
            return result;
        }
        
        // Generate finishing pass if enabled
        if (params.enableFinishingPass) {
            result.finishingToolpath = generateFinishingPass(result.usedParameters, tool);
        }
        
        // Calculate statistics
        result.totalPasses = params.numberOfPasses;
        if (params.enableFinishingPass) result.totalPasses++;
        if (result.grooveToolpath) result.totalPasses++;
        
        result.estimatedTime = estimatePartingTime(result.usedParameters, tool);
        result.materialRemoved = calculateMaterialRemoval(result.usedParameters);
        result.partLength = std::abs(result.usedParameters.partingZ);
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.errorMessage = "Exception during parting generation: " + std::string(e.what());
    } catch (...) {
        result.errorMessage = "Unknown error during parting generation";
    }
    
    return result;
}

std::vector<PartingOperation::PartingPosition> PartingOperation::detectPartingPositions(
    const LatheProfile::Profile2D& profile,
    const Parameters& params) {
    
    std::vector<PartingPosition> positions;
    
    if (profile.size() < 5) {
        return positions; // Not enough points for analysis
    }
    
    // Analyze profile for potential parting positions
    for (size_t i = 1; i < profile.size() - 1; ++i) {
        const auto& prevPoint = profile[i-1];
        const auto& currentPoint = profile[i];
        const auto& nextPoint = profile[i+1];
        
        PartingPosition position;
        position.zPosition = currentPoint.x;
        position.diameter = currentPoint.z * 2.0; // Convert radius to diameter
        position.hasNeck = false;
        position.isOptimal = false;
        position.confidence = 0.0;
        
        // Look for necking features (reduced diameter sections)
        double prevDiameter = prevPoint.z * 2.0;
        double nextDiameter = nextPoint.z * 2.0;
        double currentDiameter = position.diameter;
        
        if (currentDiameter < prevDiameter && currentDiameter < nextDiameter) {
            // Found a neck - this is ideal for parting
            position.hasNeck = true;
            position.neckedDiameter = currentDiameter;
            position.confidence = 0.9;
            position.reason = "Necked section ideal for parting";
            position.isOptimal = true;
        }
        
        // Look for diameter changes (step features)
        double diameterChange = std::abs(currentDiameter - prevDiameter);
        if (diameterChange > 1.0) { // Significant diameter change
            position.confidence = 0.7;
            position.reason = "Diameter change suitable for parting";
            
            // Prefer smaller diameter positions
            if (currentDiameter < prevDiameter) {
                position.confidence += 0.1;
                position.reason += " (smaller diameter)";
            }
        }
        
        // Look for flat sections perpendicular to axis
        double flatnessThreshold = 0.1; // mm
        if (std::abs(currentPoint.z - prevPoint.z) < flatnessThreshold &&
            std::abs(currentPoint.z - nextPoint.z) < flatnessThreshold) {
            position.confidence += 0.2;
            position.reason += " (flat section)";
        }
        
        // Consider accessibility (avoid internal features)
        bool isAccessible = true;
        for (size_t j = i; j < profile.size(); ++j) {
            if (profile[j].z < currentPoint.z - 0.5) {
                isAccessible = false;
                break;
            }
        }
        
        if (!isAccessible) {
            position.confidence *= 0.5;
            position.reason += " (limited access)";
        }
        
        // Add position if it has reasonable confidence
        if (position.confidence > 0.3) {
            positions.push_back(position);
        }
    }
    
    // Sort by confidence (best first)
    std::sort(positions.begin(), positions.end(),
              [](const PartingPosition& a, const PartingPosition& b) {
                  return a.confidence > b.confidence;
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
        defaultPos.hasNeck = false;
        defaultPos.isOptimal = false;
        defaultPos.confidence = 0.1;
        defaultPos.reason = "User-specified position";
        return defaultPos;
    }
    
    // Return the highest confidence position
    auto bestPosition = positions[0];
    
    // Additional criteria for selection
    for (const auto& pos : positions) {
        // Prefer positions with necking
        if (pos.hasNeck && !bestPosition.hasNeck) {
            bestPosition = pos;
            continue;
        }
        
        // Prefer positions closer to desired parting location
        double distanceFromDesired = std::abs(pos.zPosition - params.partingZ);
        double bestDistanceFromDesired = std::abs(bestPosition.zPosition - params.partingZ);
        
        if (distanceFromDesired < bestDistanceFromDesired && 
            pos.confidence > bestPosition.confidence * 0.8) {
            bestPosition = pos;
        }
    }
    
    bestPosition.isOptimal = true;
    return bestPosition;
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
    
    if (params.centerHoleDiameter >= params.partingDiameter) {
        return "Center hole diameter must be less than parting diameter";
    }
    
    if (params.strategy == PartingStrategy::Stepped && params.stepSize <= 0.0) {
        return "Step size must be positive for stepped parting";
    }
    
    if (params.strategy == PartingStrategy::Groove && params.grooveWidth <= 0.0) {
        return "Groove width must be positive for groove parting";
    }
    
    return ""; // Valid
}

PartingOperation::Parameters PartingOperation::getDefaultParameters(
    double diameter,
    const std::string& materialType,
    const std::string& partType) {
    
    Parameters params;
    params.partingDiameter = diameter;
    
    // Get material properties for cutting parameters
    auto materialProps = OperationParameterManager::getMaterialProperties(materialType);
    
    // Set cutting parameters based on material
    params.feedRate = materialProps.recommendedFeedRate * 0.3; // Conservative for parting
    params.spindleSpeed = materialProps.recommendedSpindleSpeed * 0.8; // Lower speed for stability
    params.depthOfCut = std::min(0.5, diameter * 0.02); // Conservative depth
    
    // Adjust for part type
    if (partType == "thin_wall") {
        params.strategy = PartingStrategy::Groove;
        params.grooveWidth = std::max(params.partingWidth * 1.5, 4.0);
        params.supportPart = true;
        params.enableChipBreaking = true;
    } else if (partType == "hollow") {
        params.centerHoleDiameter = diameter * 0.3; // Estimate hole size
        params.strategy = PartingStrategy::Straight;
    } else if (diameter > 50.0) {
        params.strategy = PartingStrategy::Stepped;
        params.stepSize = std::min(5.0, diameter * 0.1);
    }
    
    // Material-specific adjustments
    if (materialType == "aluminum") {
        params.spindleSpeed *= 1.5;
        params.feedRate *= 1.2;
        params.enableCoolant = false; // Often better without coolant
    } else if (materialType == "stainless_steel") {
        params.feedRate *= 0.7;
        params.enableCoolant = true; // Essential for stainless
        params.enableChipBreaking = true;
    } else if (materialType == "brass") {
        params.spindleSpeed *= 1.3;
        params.feedRate *= 1.1;
    }
    
    return params;
}

std::unique_ptr<Toolpath> PartingOperation::generateStraightParting(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Straight_Parting", tool);
    
    // Calculate parting boundaries
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingZ = params.partingZ;
    
    // Start from safe position
    gp_Pnt safeStart(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(safeStart.X(), safeStart.Y(), safeStart.Z()));
    
    // Calculate pass depths
    double totalDepth = outerRadius - innerRadius;
    double depthPerPass = params.depthOfCut;
    int actualPasses = static_cast<int>(std::ceil(totalDepth / depthPerPass));
    
    // Adjust depth per pass to ensure even cuts
    depthPerPass = totalDepth / actualPasses;
    
    // Generate parting passes
    for (int pass = 0; pass < actualPasses; ++pass) {
        double currentRadius = outerRadius - (pass * depthPerPass);
        double targetRadius = outerRadius - ((pass + 1) * depthPerPass);
        
        // Ensure we don't go past the center hole
        targetRadius = std::max(targetRadius, innerRadius);
        
        // Rapid to start of cut
        gp_Pnt cutStart(partingZ + params.clearanceDistance, 0.0, currentRadius);
        toolpath->addRapidMove(Geometry::Point3D(cutStart.X(), cutStart.Y(), cutStart.Z()));
        
        // Feed to cutting position
        gp_Pnt feedStart(partingZ, 0.0, currentRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(feedStart.X(), feedStart.Y(), feedStart.Z()),
            params.feedRate);
        
        // Parting cut
        gp_Pnt cutEnd(partingZ, 0.0, targetRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(cutEnd.X(), cutEnd.Y(), cutEnd.Z()),
            params.feedRate);
        
        // Chip breaking if enabled
        if (params.enableChipBreaking && pass < actualPasses - 1) {
            auto chipBreakPositions = calculateChipBreakingPositions(params);
            for (double breakRadius : chipBreakPositions) {
                if (breakRadius >= targetRadius && breakRadius <= currentRadius) {
                    // Retract slightly for chip breaking
                    gp_Pnt chipBreak(partingZ, 0.0, breakRadius + params.chipBreakDistance);
                    toolpath->addLinearMove(
                        Geometry::Point3D(chipBreak.X(), chipBreak.Y(), chipBreak.Z()),
                        params.feedRate * 2.0);
                    
                    // Return to cutting
                    toolpath->addLinearMove(
                        Geometry::Point3D(cutEnd.X(), cutEnd.Y(), cutEnd.Z()),
                        params.feedRate);
                }
            }
        }
        
        // Retract to safe radius
        gp_Pnt retract(partingZ + params.retractDistance, 0.0, targetRadius);
        toolpath->addRapidMove(Geometry::Point3D(retract.X(), retract.Y(), retract.Z()));
    }
    
    // Final retract to safe position
    gp_Pnt finalSafe(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(finalSafe.X(), finalSafe.Y(), finalSafe.Z()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateSteppedParting(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Stepped_Parting", tool);
    
    // Calculate step sizes
    auto stepSizes = calculateStepSizes(params);
    
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingZ = params.partingZ;
    
    // Start from safe position
    gp_Pnt safeStart(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(safeStart.X(), safeStart.Y(), safeStart.Z()));
    
    // Generate stepped cuts
    double currentRadius = outerRadius;
    for (size_t step = 0; step < stepSizes.size(); ++step) {
        double stepDepth = stepSizes[step];
        double targetRadius = std::max(currentRadius - stepDepth, innerRadius);
        
        // Multiple passes for each step if needed
        double remainingDepth = currentRadius - targetRadius;
        int passesForStep = static_cast<int>(std::ceil(remainingDepth / params.depthOfCut));
        double depthPerPass = remainingDepth / passesForStep;
        
        for (int pass = 0; pass < passesForStep; ++pass) {
            double passStartRadius = currentRadius - (pass * depthPerPass);
            double passEndRadius = currentRadius - ((pass + 1) * depthPerPass);
            
            // Rapid to cutting position
            gp_Pnt cutStart(partingZ + params.clearanceDistance, 0.0, passStartRadius);
            toolpath->addRapidMove(Geometry::Point3D(cutStart.X(), cutStart.Y(), cutStart.Z()));
            
            // Feed to cutting depth
            gp_Pnt feedStart(partingZ, 0.0, passStartRadius);
            toolpath->addLinearMove(
                Geometry::Point3D(feedStart.X(), feedStart.Y(), feedStart.Z()),
                params.feedRate);
            
            // Cutting move
            gp_Pnt cutEnd(partingZ, 0.0, passEndRadius);
            toolpath->addLinearMove(
                Geometry::Point3D(cutEnd.X(), cutEnd.Y(), cutEnd.Z()),
                params.feedRate);
            
            // Retract
            gp_Pnt retract(partingZ + params.retractDistance, 0.0, passEndRadius);
            toolpath->addRapidMove(Geometry::Point3D(retract.X(), retract.Y(), retract.Z()));
        }
        
        currentRadius = targetRadius;
        
        if (currentRadius <= innerRadius) {
            break; // Reached center
        }
    }
    
    // Final safe position
    gp_Pnt finalSafe(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(finalSafe.X(), finalSafe.Y(), finalSafe.Z()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateGrooveRelief(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Groove_Relief", tool);
    
    double grooveZ = params.partingZ + params.grooveWidth / 2.0;
    double grooveRadius = params.partingDiameter / 2.0 - params.grooveDepth;
    
    // Create relief groove adjacent to parting position
    gp_Pnt grooveStart(grooveZ, 0.0, params.partingDiameter / 2.0);
    gp_Pnt grooveEnd(grooveZ, 0.0, grooveRadius);
    
    // Rapid to groove start
    toolpath->addRapidMove(Geometry::Point3D(grooveStart.X(), grooveStart.Y(), grooveStart.Z()));
    
    // Cut groove
    toolpath->addLinearMove(
        Geometry::Point3D(grooveEnd.X(), grooveEnd.Y(), grooveEnd.Z()),
        params.feedRate * 0.8); // Slower for grooving
    
    // Retract
    gp_Pnt retract(grooveZ + params.retractDistance, 0.0, grooveRadius);
    toolpath->addRapidMove(Geometry::Point3D(retract.X(), retract.Y(), retract.Z()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateUndercutParting(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    // Undercut parting uses an angled approach to avoid tool binding
    auto toolpath = std::make_unique<Toolpath>("Undercut_Parting", tool);
    
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingZ = params.partingZ;
    double undercutAngleRad = params.undercutAngle * M_PI / 180.0;
    
    // Calculate undercut geometry
    double undercutDepth = params.undercutDepth;
    double undercutOffset = undercutDepth * std::tan(undercutAngleRad);
    
    // Start position
    gp_Pnt safeStart(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(safeStart.X(), safeStart.Y(), safeStart.Z()));
    
    // Create undercut first
    gp_Pnt undercutStart(partingZ + undercutOffset, 0.0, outerRadius);
    gp_Pnt undercutEnd(partingZ, 0.0, outerRadius - undercutDepth);
    
    toolpath->addRapidMove(Geometry::Point3D(undercutStart.X(), undercutStart.Y(), undercutStart.Z()));
    toolpath->addLinearMove(
        Geometry::Point3D(undercutEnd.X(), undercutEnd.Y(), undercutEnd.Z()),
        params.feedRate);
    
    // Continue with normal parting from undercut position
    double currentRadius = outerRadius - undercutDepth;
    while (currentRadius > innerRadius) {
        double targetRadius = std::max(currentRadius - params.depthOfCut, innerRadius);
        
        gp_Pnt cutEnd(partingZ, 0.0, targetRadius);
        toolpath->addLinearMove(
            Geometry::Point3D(cutEnd.X(), cutEnd.Y(), cutEnd.Z()),
            params.feedRate);
        
        currentRadius = targetRadius;
    }
    
    // Final retract
    gp_Pnt finalSafe(partingZ + params.safetyHeight, 0.0, outerRadius + params.clearanceDistance);
    toolpath->addRapidMove(Geometry::Point3D(finalSafe.X(), finalSafe.Y(), finalSafe.Z()));
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateTrepanningParting(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    // Trepanning creates a groove around the part before final parting
    auto toolpath = std::make_unique<Toolpath>("Trepanning_Parting", tool);
    
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingZ = params.partingZ;
    double trepanningWidth = params.partingWidth * 2.0; // Wider groove
    
    // Create trepanning groove
    for (double z = partingZ - trepanningWidth/2; z <= partingZ + trepanningWidth/2; z += 0.5) {
        gp_Pnt grooveStart(z, 0.0, outerRadius);
        gp_Pnt grooveEnd(z, 0.0, outerRadius - params.depthOfCut);
        
        toolpath->addRapidMove(Geometry::Point3D(grooveStart.X(), grooveStart.Y(), grooveStart.Z()));
        toolpath->addLinearMove(
            Geometry::Point3D(grooveEnd.X(), grooveEnd.Y(), grooveEnd.Z()),
            params.feedRate * 0.7);
    }
    
    // Final parting cut through remaining material
    double remainingRadius = outerRadius - params.depthOfCut;
    while (remainingRadius > innerRadius) {
        double targetRadius = std::max(remainingRadius - params.depthOfCut, innerRadius);
        
        gp_Pnt cutStart(partingZ, 0.0, remainingRadius);
        gp_Pnt cutEnd(partingZ, 0.0, targetRadius);
        
        toolpath->addRapidMove(Geometry::Point3D(cutStart.X(), cutStart.Y(), cutStart.Z()));
        toolpath->addLinearMove(
            Geometry::Point3D(cutEnd.X(), cutEnd.Y(), cutEnd.Z()),
            params.feedRate);
        
        remainingRadius = targetRadius;
    }
    
    return toolpath;
}

std::unique_ptr<Toolpath> PartingOperation::generateFinishingPass(
    const Parameters& params,
    std::shared_ptr<Tool> tool) {
    
    auto toolpath = std::make_unique<Toolpath>("Parting_Finishing", tool);
    
    // Finishing pass at final dimensions
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingZ = params.partingZ;
    
    // Light finishing cut
    gp_Pnt finishStart(partingZ, 0.0, outerRadius - params.finishingAllowance);
    gp_Pnt finishEnd(partingZ, 0.0, innerRadius);
    
    toolpath->addRapidMove(Geometry::Point3D(finishStart.X(), finishStart.Y(), finishStart.Z()));
    toolpath->addLinearMove(
        Geometry::Point3D(finishEnd.X(), finishEnd.Y(), finishEnd.Z()),
        params.finishingFeedRate);
    
    return toolpath;
}

std::vector<double> PartingOperation::calculateStepSizes(const Parameters& params) {
    std::vector<double> stepSizes;
    
    double totalDepth = params.partingDiameter / 2.0 - params.centerHoleDiameter / 2.0;
    double targetStepSize = params.stepSize;
    
    // Calculate number of steps needed
    int numSteps = static_cast<int>(std::ceil(totalDepth / targetStepSize));
    
    // Distribute depth evenly across steps
    double actualStepSize = totalDepth / numSteps;
    
    for (int i = 0; i < numSteps; ++i) {
        stepSizes.push_back(actualStepSize);
    }
    
    return stepSizes;
}

double PartingOperation::estimatePartingTime(const Parameters& params, std::shared_ptr<Tool> tool) {
    double totalTime = 0.0;
    
    // Calculate cutting time
    double totalDepth = params.partingDiameter / 2.0 - params.centerHoleDiameter / 2.0;
    double cuttingLength = totalDepth * params.numberOfPasses;
    
    // Add extra time for stepped or groove strategies
    switch (params.strategy) {
        case PartingStrategy::Stepped:
            cuttingLength *= 1.5; // More complex moves
            break;
        case PartingStrategy::Groove:
            cuttingLength *= 1.3; // Additional groove time
            break;
        case PartingStrategy::Trepanning:
            cuttingLength *= 2.0; // Significantly more material removal
            break;
        default:
            break;
    }
    
    // Time based on feed rate
    double cuttingTime = cuttingLength / params.feedRate;
    
    // Add positioning and setup time
    double setupTime = 0.5; // 30 seconds
    double positioningTime = params.numberOfPasses * 0.1; // 6 seconds per pass
    
    totalTime = cuttingTime + setupTime + positioningTime;
    
    // Add finishing pass time if enabled
    if (params.enableFinishingPass) {
        double finishingTime = totalDepth / params.finishingFeedRate;
        totalTime += finishingTime;
    }
    
    return totalTime;
}

double PartingOperation::calculateMaterialRemoval(const Parameters& params) {
    // Calculate volume of material removed during parting
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    double partingWidth = params.partingWidth;
    
    // Volume of annular cylinder
    double volume = M_PI * partingWidth * (outerRadius * outerRadius - innerRadius * innerRadius);
    
    // Add volume for groove if using groove strategy
    if (params.strategy == PartingStrategy::Groove) {
        double grooveVolume = M_PI * params.grooveWidth * 
                             (outerRadius * outerRadius - (outerRadius - params.grooveDepth) * (outerRadius - params.grooveDepth));
        volume += grooveVolume;
    }
    
    return volume;
}

bool PartingOperation::validateToolCompatibility(std::shared_ptr<Tool> tool, const Parameters& params) {
    if (!tool) {
        return false;
    }
    
    // Check if tool is suitable for parting
    // This would check tool type, width, etc.
    // For now, return true if tool exists
    
    // Future implementations could check:
    // - Tool width vs parting width
    // - Tool length vs part diameter
    // - Tool material vs workpiece material
    
    return true;
}

std::vector<double> PartingOperation::calculateChipBreakingPositions(const Parameters& params) {
    std::vector<double> positions;
    
    if (!params.enableChipBreaking) {
        return positions;
    }
    
    double outerRadius = params.partingDiameter / 2.0;
    double innerRadius = params.centerHoleDiameter / 2.0;
    
    // Add chip breaking positions every few millimeters
    double chipBreakInterval = 3.0; // mm
    
    for (double radius = outerRadius - chipBreakInterval; 
         radius > innerRadius; 
         radius -= chipBreakInterval) {
        positions.push_back(radius);
    }
    
    return positions;
}

} // namespace Toolpath
} // namespace IntuiCAM
