#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Toolpath/ContouringOperation.h>
#include <IntuiCAM/Toolpath/ThreadingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>

#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>

// OpenCASCADE includes for display object creation
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <Geom_Line.hxx>
#include <gp_Lin.hxx>
#include <Quantity_Color.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Precision.hxx>

namespace IntuiCAM {
namespace Toolpath {

ToolpathGenerationPipeline::ToolpathGenerationPipeline() {
    // Constructor implementation
}

ToolpathGenerationPipeline::GenerationResult 
ToolpathGenerationPipeline::generateToolpaths(const GenerationRequest& request) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    GenerationResult result;
    result.success = false;
    
    // Mark as generating
    m_isGenerating = true;
    m_cancelRequested = false;
    
    try {
        // Step 1: Validate request
        reportProgress(0.0, "Validating request...", request);
        std::string validationError = validateRequest(request);
        if (!validationError.empty()) {
            result.errorMessage = "Request validation failed: " + validationError;
            m_isGenerating = false;
            return result;
        }

        // Step 2: Validate and fill missing parameters
        reportProgress(0.1, "Validating operation parameters...", request);
        auto materialProps = OperationParameterManager::getMaterialProperties(
            request.globalParams.materialType);
        
        auto validatedOps = validateAndFillParameters(request.enabledOps, request.globalParams);
        if (m_cancelRequested) {
            result.errorMessage = "Generation cancelled by user";
            m_isGenerating = false;
            return result;
        }

        // Step 3: Extract 2D profile
        reportProgress(0.2, "Extracting 2D profile from part geometry...", request);
        result.extractedProfile = extractProfile(request.partGeometry, request.globalParams);
        if (result.extractedProfile.empty()) {
            result.errorMessage = "Failed to extract valid 2D profile from part geometry";
            m_isGenerating = false;
            return result;
        }

        // Step 4: Generate toolpaths for each enabled operation
        double progressPerOp = 0.6 / validatedOps.size(); // 60% for toolpath generation
        double currentProgress = 0.3;
        
        for (const auto& operation : validatedOps) {
            if (m_cancelRequested) {
                result.errorMessage = "Generation cancelled by user";
                m_isGenerating = false;
                return result;
            }
            
            std::string progressMsg = "Generating " + operation.operationType + " toolpath...";
            reportProgress(currentProgress, progressMsg, request);
            
            try {
                auto toolpath = generateOperationToolpath(
                    operation, 
                    result.extractedProfile, 
                    request.primaryTool, 
                    request.globalParams
                );
                
                if (toolpath) {
                    result.generatedToolpaths.push_back(std::move(toolpath));
                } else {
                    result.warnings.push_back(
                        "Failed to generate toolpath for " + operation.operationType + " operation");
                }
            } catch (const std::exception& e) {
                result.warnings.push_back(
                    "Error generating " + operation.operationType + " toolpath: " + e.what());
            }
            
            currentProgress += progressPerOp;
        }

        // Step 5: Create display objects
        reportProgress(0.9, "Creating display objects...", request);
        result.toolpathDisplayObjects = createToolpathDisplayObjects(result.generatedToolpaths);
        result.profileDisplayObject = createProfileDisplayObject(
            result.extractedProfile, request.globalParams.turningAxis);

        // Step 6: Calculate statistics
        reportProgress(0.95, "Calculating statistics...", request);
        result.statistics = calculateStatistics(result.generatedToolpaths, result.extractedProfile);

        // Complete
        auto endTime = std::chrono::high_resolution_clock::now();
        result.processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);
        
        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        result.generationTimestamp = ss.str();

        result.success = true;
        reportProgress(1.0, "Toolpath generation completed successfully", request);
        
    } catch (const std::exception& e) {
        result.errorMessage = "Unexpected error during generation: " + std::string(e.what());
    } catch (...) {
        result.errorMessage = "Unknown error during toolpath generation";
    }
    
    m_isGenerating = false;
    return result;
}

std::future<ToolpathGenerationPipeline::GenerationResult> 
ToolpathGenerationPipeline::generateToolpathsAsync(const GenerationRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return generateToolpaths(request);
    });
}

std::string ToolpathGenerationPipeline::validateRequest(const GenerationRequest& request) {
    // Validate part geometry
    if (request.partGeometry.IsNull()) {
        return "Part geometry is null or invalid";
    }
    
    // Validate turning axis - gp_Dir is always unit length, just check if it's valid
    try {
        gp_Dir testDir = request.globalParams.turningAxis.Direction();
        (void)testDir; // Suppress unused variable warning
    } catch (const Standard_Failure&) {
        return "Invalid turning axis direction";
    }
    
    // Validate enabled operations
    if (request.enabledOps.empty()) {
        return "No operations enabled for toolpath generation";
    }
    
    bool hasEnabledOp = false;
    for (const auto& op : request.enabledOps) {
        if (op.enabled) {
            hasEnabledOp = true;
            break;
        }
    }
    
    if (!hasEnabledOp) {
        return "No enabled operations found";
    }
    
    // Validate primary tool
    if (!request.primaryTool) {
        return "Primary tool is required for toolpath generation";
    }
    
    // Validate global parameters
    if (request.globalParams.profileTolerance <= 0.0) {
        return "Profile tolerance must be positive";
    }
    
    if (request.globalParams.profileSections < 10) {
        return "Profile sections must be at least 10";
    }
    
    return ""; // Valid
}

double ToolpathGenerationPipeline::estimateProcessingTime(const GenerationRequest& request) {
    double estimatedTime = 1.0; // Base time in seconds
    
    // Add time based on number of enabled operations
    for (const auto& op : request.enabledOps) {
        if (op.enabled) {
            if (op.operationType == "Contouring") {
                estimatedTime += 3.0; // Complex operation
            } else if (op.operationType == "Threading") {
                estimatedTime += 2.0; // Moderate complexity
            } else {
                estimatedTime += 1.0; // Simple operations
            }
        }
    }
    
    // Add time based on profile complexity
    estimatedTime += request.globalParams.profileSections / 100.0;
    
    return estimatedTime;
}

void ToolpathGenerationPipeline::cancelGeneration() {
    m_cancelRequested = true;
}

std::vector<ToolpathGenerationPipeline::EnabledOperation> 
ToolpathGenerationPipeline::validateAndFillParameters(
    const std::vector<EnabledOperation>& operations,
    const ToolpathGenerationParameters& globalParams) {
    
    std::vector<EnabledOperation> validatedOps;
    auto materialProps = OperationParameterManager::getMaterialProperties(globalParams.materialType);
    
    for (const auto& op : operations) {
        if (!op.enabled) continue;
        
        EnabledOperation validatedOp = op;
        
        // Convert to OperationParameterManager format
        OperationParameterManager::OperationConfig config;
        config.operationType = op.operationType;
        config.enabled = op.enabled;
        config.numericParams = op.numericParams;
        config.stringParams = op.stringParams;
        config.booleanParams = op.booleanParams;
        
        // Fill missing parameters
        auto filledConfig = OperationParameterManager::fillMissingParameters(
            op.operationType, config, materialProps);
        
        // Convert back
        validatedOp.numericParams = filledConfig.numericParams;
        validatedOp.stringParams = filledConfig.stringParams;
        validatedOp.booleanParams = filledConfig.booleanParams;
        
        validatedOps.push_back(validatedOp);
    }
    
    return validatedOps;
}

LatheProfile::Profile2D ToolpathGenerationPipeline::extractProfile(
    const TopoDS_Shape& partGeometry,
    const ToolpathGenerationParameters& params) {
    
    ProfileExtractor::ExtractionParameters extractParams;
    extractParams.turningAxis = params.turningAxis;
    extractParams.profileTolerance = params.profileTolerance;
    extractParams.profileSections = params.profileSections;
    extractParams.includeInternalFeatures = true;
    extractParams.autoDetectFeatures = true;
    
    return ProfileExtractor::extractProfile(partGeometry, extractParams);
}

std::unique_ptr<Toolpath> ToolpathGenerationPipeline::generateOperationToolpath(
    const EnabledOperation& operation,
    const LatheProfile::Profile2D& profile,
    std::shared_ptr<Tool> tool,
    const ToolpathGenerationParameters& globalParams) {
    
    // This is a simplified implementation - in practice, each operation would have
    // its own sophisticated generation logic
    
    if (operation.operationType == "Contouring") {
        // Use enhanced ContouringOperation
        ContouringOperation contouringOp;
        ContouringOperation::Parameters params;
        
        // Fill parameters from operation config
        params.safetyHeight = globalParams.safetyHeight;
        params.clearanceDistance = globalParams.clearanceDistance;
        params.profileTolerance = globalParams.profileTolerance;
        
        // In real implementation, the part geometry would come from workspace
        // For now, skip the contouring operation implementation
        // auto result = contouringOp.generateToolpaths(partGeometry, tool, params);
        // if (result.success && result.facingToolpath) {
        //     return std::move(result.facingToolpath);
        // }
    }
    
    // For now, create a simple dummy toolpath for other operations
    auto toolpath = std::make_unique<Toolpath>(operation.operationType + "_Toolpath", tool);
    
    // Add some basic moves based on the profile
    if (!profile.empty()) {
        // Add rapid to start
        gp_Pnt startPoint(profile.front().x, 0.0, profile.front().z);
        startPoint.Translate(gp_Vec(0, 0, globalParams.safetyHeight));
        toolpath->addRapidMove(Geometry::Point3D(startPoint.X(), startPoint.Y(), startPoint.Z()));
        
        // Add feed moves along profile
        for (const auto& point : profile) {
            gp_Pnt profilePoint(point.x, 0.0, point.z);
            toolpath->addLinearMove(
                Geometry::Point3D(profilePoint.X(), profilePoint.Y(), profilePoint.Z()),
                100.0 // feed rate
            );
        }
        
        // Add rapid to safe height
        toolpath->addRapidMove(Geometry::Point3D(startPoint.X(), startPoint.Y(), startPoint.Z()));
    }
    
    return toolpath;
}

std::vector<Handle(AIS_InteractiveObject)> 
ToolpathGenerationPipeline::createToolpathDisplayObjects(
    const std::vector<std::unique_ptr<Toolpath>>& toolpaths) {
    
    std::vector<Handle(AIS_InteractiveObject)> displayObjects;
    
    // Color scheme for different operation types
    std::map<std::string, Quantity_Color> operationColors = {
        {"Contouring", Quantity_Color(0.0, 0.8, 0.0, Quantity_TOC_RGB)}, // Green
        {"Threading", Quantity_Color(0.8, 0.0, 0.8, Quantity_TOC_RGB)}, // Magenta
        {"Chamfering", Quantity_Color(0.0, 0.0, 0.8, Quantity_TOC_RGB)}, // Blue
        {"Parting", Quantity_Color(0.8, 0.8, 0.0, Quantity_TOC_RGB)}     // Yellow
    };
    
    for (const auto& toolpath : toolpaths) {
        try {
            BRepBuilderAPI_MakeWire wireBuilder;
            const auto& moves = toolpath->getMovements();
            
            for (size_t i = 1; i < moves.size(); ++i) {
                const auto& prevMove = moves[i-1];
                const auto& currentMove = moves[i];
                
                gp_Pnt p1(prevMove.position.x, prevMove.position.y, prevMove.position.z);
                gp_Pnt p2(currentMove.position.x, currentMove.position.y, currentMove.position.z);
                
                if (p1.Distance(p2) > Precision::Confusion()) {
                    BRepBuilderAPI_MakeEdge edgeBuilder(p1, p2);
                    if (edgeBuilder.IsDone()) {
                        wireBuilder.Add(edgeBuilder.Edge());
                    }
                }
            }
            
            if (wireBuilder.IsDone()) {
                TopoDS_Shape wireShape = wireBuilder.Wire();
                Handle(AIS_Shape) aisShape = new AIS_Shape(wireShape);
                
                // Set color based on toolpath name
                Quantity_Color color(0.8, 0.0, 0.0, Quantity_TOC_RGB); // Default red
                for (const auto& pair : operationColors) {
                    if (toolpath->getName().find(pair.first) != std::string::npos) {
                        color = pair.second;
                        break;
                    }
                }
                
                aisShape->SetColor(color);
                aisShape->SetWidth(2.0);
                
                displayObjects.push_back(aisShape);
            }
        } catch (const std::exception& e) {
            // Log error but continue with other toolpaths
            // In practice, this would use proper logging
        }
    }
    
    return displayObjects;
}

Handle(AIS_InteractiveObject) ToolpathGenerationPipeline::createProfileDisplayObject(
    const LatheProfile::Profile2D& profile,
    const gp_Ax1& turningAxis) {
    
    if (profile.empty()) {
        return Handle(AIS_InteractiveObject)();
    }
    
    try {
        BRepBuilderAPI_MakeWire wireBuilder;
        
        for (size_t i = 1; i < profile.size(); ++i) {
            const IntuiCAM::Geometry::Point2D& p1 = profile[i-1];
            const IntuiCAM::Geometry::Point2D& p2 = profile[i];
            
            // Convert 2D profile points to 3D using turning axis
            gp_Pnt point1 = turningAxis.Location().Translated(
                gp_Vec(turningAxis.Direction()) * p1.x + 
                gp_Vec(0, 0, p1.z));
            gp_Pnt point2 = turningAxis.Location().Translated(
                gp_Vec(turningAxis.Direction()) * p2.x + 
                gp_Vec(0, 0, p2.z));
            
            if (point1.Distance(point2) > Precision::Confusion()) {
                BRepBuilderAPI_MakeEdge edgeBuilder(point1, point2);
                if (edgeBuilder.IsDone()) {
                    wireBuilder.Add(edgeBuilder.Edge());
                }
            }
        }
        
        if (wireBuilder.IsDone()) {
            TopoDS_Shape profileWireShape = wireBuilder.Wire();
            Handle(AIS_Shape) profileShape = new AIS_Shape(profileWireShape);
            profileShape->SetColor(Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB)); // Orange
            profileShape->SetWidth(3.0);
            return profileShape;
        }
    } catch (const std::exception& e) {
        // Log error and return null handle
    }
    
    return Handle(AIS_InteractiveObject)();
}

ToolpathGenerationPipeline::ToolpathStatistics 
ToolpathGenerationPipeline::calculateStatistics(
    const std::vector<std::unique_ptr<Toolpath>>& toolpaths,
    const LatheProfile::Profile2D& profile) {
    
    ToolpathStatistics stats;
    
    for (const auto& toolpath : toolpaths) {
        double operationTime = toolpath->estimateMachiningTime();
        stats.totalMachiningTime += operationTime;
        
        int moveCount = static_cast<int>(toolpath->getMovementCount());
        stats.totalMovements += moveCount;
        
        // Store per-operation statistics
        stats.operationTimes[toolpath->getName()] = operationTime;
        stats.operationMoves[toolpath->getName()] = moveCount;
    }
    
    // Estimate material removal volume based on profile
    if (profile.size() >= 2) {
        double totalVolume = 0.0;
        for (size_t i = 1; i < profile.size(); ++i) {
            const IntuiCAM::Geometry::Point2D& p1 = profile[i-1];
            const IntuiCAM::Geometry::Point2D& p2 = profile[i];
            
            double height = std::abs(p2.x - p1.x);
            double avgRadius = (p1.z + p2.z) / 2.0;
            double volume = M_PI * avgRadius * avgRadius * height;
            totalVolume += volume;
        }
        stats.materialRemovalVolume = totalVolume;
    }
    
    return stats;
}

void ToolpathGenerationPipeline::reportProgress(
    double progress, 
    const std::string& status,
    const GenerationRequest& request) {
    
    if (request.progressCallback) {
        request.progressCallback(progress, status);
    }
}

} // namespace Toolpath
} // namespace IntuiCAM 