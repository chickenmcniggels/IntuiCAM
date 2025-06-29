#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/ToolpathDisplayObject.h>

#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

// OpenCASCADE includes for display object creation
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <Precision.hxx>

namespace IntuiCAM {
namespace Toolpath {

ToolpathGenerationPipeline::ToolpathGenerationPipeline() {
    // Constructor implementation
}

ToolpathGenerationPipeline::PipelineResult 
ToolpathGenerationPipeline::executePipeline(const PipelineInputs& inputs) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PipelineResult result;
    result.success = false;
    
    // Mark as generating
    m_isGenerating = true;
    m_cancelRequested = false;
    
    try {
        reportProgress(0.0, "Starting toolpath generation pipeline...", result);

        // Initialize timeline (empty list that will store generated operations)
        result.timeline.clear();

        // ---------------------------------------------------------------------------
        // 3.1  Facing – always FIRST: establish reference surface at Z-max
        // ---------------------------------------------------------------------------
        if (inputs.facing) {
            reportProgress(0.1, "Generating facing toolpaths...", result);
            
            double depthOfCut = 1.0; // mm - placeholder
            int passes = static_cast<int>(std::floor(inputs.facingAllowance / depthOfCut));
            
            for (int i = 0; i < passes; ++i) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                // LATHE COORDINATE SYSTEM: X=axial, Y=0 (constrained), Z=radius
                IntuiCAM::Geometry::Point3D coordinates(
                    inputs.z0 - i * depthOfCut,  // X = axial position (was Z coordinate)
                    0.0,                         // Y = 0 (constrained to XZ plane)
                    inputs.rawMaterialDiameter / 2.0 + 5  // Z = radius (was X coordinate)
                );
                IntuiCAM::Geometry::Point3D startPos(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0 + 5);
                IntuiCAM::Geometry::Point3D endPos(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0 + 5);
                
                auto facingPasses = facingToolpath(coordinates, startPos, endPos, inputs.facingTool);
                for (auto& tp : facingPasses) {
                    result.timeline.push_back(std::move(tp));
                }
            }
            
            // One final facing pass to finish to dimension
            IntuiCAM::Geometry::Point3D finalCoord(
                inputs.z0 - inputs.facingAllowance,  // X = axial position
                0.0,                                 // Y = 0 (constrained)
                inputs.rawMaterialDiameter / 2.0 + 5  // Z = radius
            );
            IntuiCAM::Geometry::Point3D startPos(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0 + 5);
            IntuiCAM::Geometry::Point3D endPos(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0 + 5);
            
            auto finalFacing = facingToolpath(finalCoord, startPos, endPos, inputs.facingTool);
            for (auto& tp : finalFacing) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // ---------------------------------------------------------------------------
        // 3.2  INTERNAL FEATURES (drilling, boring, grooving, finishing) if enabled
        // ---------------------------------------------------------------------------
        if (inputs.machineInternalFeatures) {
            reportProgress(0.2, "Processing internal features...", result);
            
            // Extract internal diameters from profile
            std::vector<double> diameters;
            // This would be extracted from inputs.profile2D.internalProfile.diameters
            // For now, using placeholder values
            diameters = {6.0, 10.0, 18.0}; // Example internal diameters
            
            std::vector<double> needBoring, drillable;
            for (double d : diameters) {
                if (d > inputs.largestDrillSize) {
                    needBoring.push_back(d);
                } else {
                    drillable.push_back(d);
                }
            }
            
            // -- Drilling pre-bores
            if (inputs.drilling && !inputs.featuresToBeDrilled.empty()) {
                reportProgress(0.25, "Generating drilling toolpaths...", result);
                for (const auto& feature : inputs.featuresToBeDrilled) {
                    if (m_cancelRequested) {
                        result.errorMessage = "Generation cancelled by user";
                        m_isGenerating = false;
                        return result;
                    }
                    
                    auto drillingPaths = drillingToolpath(feature.depth, feature.tool);
                    for (auto& tp : drillingPaths) {
                        result.timeline.push_back(std::move(tp));
                    }
                }
            }
            
            // -- Optional rough-boring of oversized diameters
            if (inputs.internalRoughing && !needBoring.empty()) {
                reportProgress(0.3, "Generating internal roughing toolpaths...", result);
                // LATHE COORDINATE SYSTEM: X=axial, Y=0 (constrained), Z=radius
                IntuiCAM::Geometry::Point3D startXyz(inputs.z0, 0.0, 0.0); // X=axial, Y=0, Z=radius (center bore)
                
                auto roughingPaths = internalRoughingToolpath(
                    startXyz, 
                    inputs.internalRoughingTool, 
                    inputs.profile2D
                );
                for (auto& tp : roughingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
            
            // -- Finishing internal geometry
            if (inputs.internalFinishing) {
                reportProgress(0.35, "Generating internal finishing toolpaths...", result);
                for (int passIdx = 1; passIdx <= inputs.internalFinishingPasses; ++passIdx) {
                    if (m_cancelRequested) {
                        result.errorMessage = "Generation cancelled by user";
                        m_isGenerating = false;
                        return result;
                    }
                    
                    // LATHE COORDINATE SYSTEM: X=axial, Y=0 (constrained), Z=radius
                    IntuiCAM::Geometry::Point3D passXyz(inputs.z0, 0.0, 0.0); // Start at centerline
                    auto finishingPaths = internalFinishingToolpath(
                        passXyz, 
                        inputs.internalFinishingTool, 
                        inputs.profile2D
                    );
                    for (auto& tp : finishingPaths) {
                        result.timeline.push_back(std::move(tp));
                    }
                }
            }
            
            // -- Internal Grooving
            if (inputs.internalGrooving && !inputs.internalFeaturesToBeGrooved.empty()) {
                reportProgress(0.4, "Generating internal grooving toolpaths...", result);
                for (const auto& groove : inputs.internalFeaturesToBeGrooved) {
                    if (m_cancelRequested) {
                        result.errorMessage = "Generation cancelled by user";
                        m_isGenerating = false;
                        return result;
                    }
                    
                    auto groovingPaths = internalGroovingToolpath(
                        groove.coordinates,
                        groove.geometry,
                        groove.tool,
                        groove.chamferEdges
                    );
                    for (auto& tp : groovingPaths) {
                        result.timeline.push_back(std::move(tp));
                    }
                }
            }
        }

        // ---------------------------------------------------------------------------
        // 3.3  EXTERNAL ROUGHING
        // ---------------------------------------------------------------------------
        if (inputs.externalRoughing) {
            reportProgress(0.5, "Generating external roughing toolpaths...", result);
            
            // LATHE COORDINATE SYSTEM: X=axial, Y=0 (constrained), Z=radius
            IntuiCAM::Geometry::Point3D startXyz(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0); // Start at raw material radius
            
            auto roughingPaths = externalRoughingToolpath(
                startXyz, 
                inputs.externalRoughingTool, 
                inputs.profile2D
            );
            for (auto& tp : roughingPaths) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // ---------------------------------------------------------------------------
        // 3.4  EXTERNAL FINISHING
        // ---------------------------------------------------------------------------
        if (inputs.externalFinishing) {
            reportProgress(0.6, "Generating external finishing toolpaths...", result);
            for (int passIdx = 1; passIdx <= inputs.externalFinishingPasses; ++passIdx) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                // LATHE COORDINATE SYSTEM: X=axial, Y=0 (constrained), Z=radius
                IntuiCAM::Geometry::Point3D passXyz(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0); // Start at raw material radius
                auto finishingPaths = externalFinishingToolpath(
                    passXyz, 
                    inputs.externalFinishingTool, 
                    inputs.profile2D
                );
                for (auto& tp : finishingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // ---------------------------------------------------------------------------
        // 3.5  CHAMFERING
        // ---------------------------------------------------------------------------
        if (inputs.chamfering && !inputs.featuresToBeChamfered.empty()) {
            reportProgress(0.7, "Generating chamfering toolpaths...", result);
            for (const auto& chamfer : inputs.featuresToBeChamfered) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto chamferPaths = chamferingToolpath(
                    chamfer.coordinates,
                    chamfer.geometry,
                    chamfer.tool
                );
                for (auto& tp : chamferPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // ---------------------------------------------------------------------------
        // 3.6  THREADING
        // ---------------------------------------------------------------------------
        if (inputs.threading && !inputs.featuresToBeThreaded.empty()) {
            reportProgress(0.8, "Generating threading toolpaths...", result);
            for (const auto& thread : inputs.featuresToBeThreaded) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto threadPaths = threadingToolpath(
                    thread.coordinates,
                    thread.geometry,  // pitch, diameters, length, ...
                    thread.tool
                );
                for (auto& tp : threadPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // ---------------------------------------------------------------------------
        // 3.7  PARTING – always the LAST operation
        // ---------------------------------------------------------------------------
        if (inputs.parting) {
            reportProgress(0.9, "Generating parting toolpaths...", result);
            
            IntuiCAM::Geometry::Point3D partCoord(
                inputs.z0 - inputs.facingAllowance - inputs.partLength - inputs.partingAllowance,
                inputs.rawMaterialDiameter / 2.0 + 5,
                0.0
            );
            
            auto partingPaths = partingToolpath(partCoord, inputs.partingTool, true);
            for (auto& tp : partingPaths) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // Create display objects
        reportProgress(0.95, "Creating display objects...", result);
        result.toolpathDisplayObjects = createToolpathDisplayObjects(result.timeline);

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
        reportProgress(1.0, "Pipeline execution completed successfully - timeline contains ordered toolpaths", result);
        
    } catch (const std::exception& e) {
        result.errorMessage = "Unexpected error during pipeline execution: " + std::string(e.what());
    } catch (...) {
        result.errorMessage = "Unknown error during toolpath generation pipeline";
    }
    
    m_isGenerating = false;
    return result;
}

ToolpathGenerationPipeline::PipelineInputs 
ToolpathGenerationPipeline::extractInputsFromPart(const TopoDS_Shape& partGeometry, const gp_Ax1& turningAxis) {
    PipelineInputs inputs;
    
    // Extract 2D profile from part geometry
    ProfileExtractor::ExtractionParameters extractParams;
    extractParams.turningAxis = turningAxis;
    extractParams.profileTolerance = 0.01;
    extractParams.profileSections = 100;
    extractParams.includeInternalFeatures = true;
    extractParams.autoDetectFeatures = true;
    
    inputs.profile2D = ProfileExtractor::extractProfile(partGeometry, extractParams);
    
    // Auto-detect features from the profile
    auto detectedFeatures = detectFeatures(inputs.profile2D);
    
    // Categorize detected features
    for (const auto& feature : detectedFeatures) {
        if (feature.type == "hole" && feature.diameter <= inputs.largestDrillSize) {
            inputs.featuresToBeDrilled.push_back(feature);
        } else if (feature.type == "groove") {
            if (feature.coordinates.y > 0) { // External
                inputs.externalFeaturesToBeGrooved.push_back(feature);
            } else { // Internal
                inputs.internalFeaturesToBeGrooved.push_back(feature);
            }
        } else if (feature.type == "chamfer") {
            inputs.featuresToBeChamfered.push_back(feature);
        } else if (feature.type == "thread") {
            inputs.featuresToBeThreaded.push_back(feature);
        }
    }
    
    return inputs;
}

std::vector<ToolpathGenerationPipeline::DetectedFeature> 
ToolpathGenerationPipeline::detectFeatures(const LatheProfile::Profile2D& profile) {
    std::vector<DetectedFeature> features;
    
    // Placeholder implementation - this would analyze the profile for various features
    // For now, return empty list - features will be implemented later
    
    return features;
}

void ToolpathGenerationPipeline::cancelGeneration() {
    m_cancelRequested = true;
}

// =============================================================================
// STUB FUNCTIONS (Section 2 from pseudocode) - PLACEHOLDER IMPLEMENTATIONS
// =============================================================================

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::facingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const IntuiCAM::Geometry::Point3D& startPos,
    const IntuiCAM::Geometry::Point3D& endPos,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Facing, "Facing Tool");
    auto toolpath = std::make_unique<Toolpath>("Facing_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::Facing);
    
    // Professional facing pattern - rapid to start, face across diameter
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(startPos.x, startPos.y, startPos.z + 2.0), 
                          OperationType::Facing, "Rapid to facing start");
    toolpath->addLinearMove(startPos, 100.0, OperationType::Facing, "Position for facing");
    
    // Face from outside diameter to center
    for (double radius = startPos.z; radius >= 0.0; radius -= 0.5) {
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, radius), 
                               150.0, OperationType::Facing, "Facing pass");
        if (radius > 0.0) {
            // Retract and move to next pass
            toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 1.0, 0.0, radius - 0.5), 
                                  OperationType::Facing, "Position next pass");
        }
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::drillingToolpath(
    double depth,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "Drill"); // Using Turning type for drill
    auto toolpath = std::make_unique<Toolpath>("Drilling_depth_" + std::to_string(depth), tool, OperationType::Drilling);
    
    // Drilling pattern with pecking cycles
    IntuiCAM::Geometry::Point3D startPos(0.0, 0.0, 5.0); // Start above part
    IntuiCAM::Geometry::Point3D drillPos(depth, 0.0, 0.0); // Drill to depth
    
    toolpath->addRapidMove(startPos, OperationType::Drilling, "Rapid to drill start");
    
    // Peck drilling in steps
    double peckDepth = 2.0;
    for (double currentDepth = peckDepth; currentDepth <= depth; currentDepth += peckDepth) {
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(currentDepth, 0.0, 0.0), 
                               50.0, OperationType::Drilling, "Drill peck");
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(currentDepth - 1.0, 0.0, 0.0), 
                              OperationType::Drilling, "Retract for chip break");
    }
    
    // Final drill to depth
    toolpath->addLinearMove(drillPos, 50.0, OperationType::Drilling, "Final drill depth");
    toolpath->addRapidMove(startPos, OperationType::Drilling, "Retract from hole");
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalRoughingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "Internal Roughing Tool");
    auto toolpath = std::make_unique<Toolpath>("Internal_Roughing_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::InternalRoughing);
    
    // Internal roughing from center outward
    double currentRadius = 1.0; // Start from center
    double targetRadius = coordinates.z;
    double depthOfCut = 1.5;
    
    while (currentRadius < targetRadius) {
        // Position for internal cut
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 2.0, 0.0, currentRadius), 
                              OperationType::InternalRoughing, "Position internal roughing");
        
        // Feed to cutting position
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, currentRadius), 
                               100.0, OperationType::InternalRoughing, "Feed to internal cut");
        
        // Rough internal profile
        for (const auto& point : profile) {
            if (!profile.empty()) {
                toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(point.x, 0.0, currentRadius), 
                                       150.0, OperationType::InternalRoughing, "Internal roughing cut");
            }
        }
        
        currentRadius += depthOfCut;
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalRoughingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "External Roughing Tool");
    auto toolpath = std::make_unique<Toolpath>("External_Roughing_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::ExternalRoughing);
    
    // Roughing pattern from outside diameter working inward
    double currentRadius = coordinates.z;
    double endRadius = 5.0; // Minimum roughing radius
    double depthOfCut = 2.0;
    
    while (currentRadius > endRadius) {
        // Rapid to cutting position
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 2.0, 0.0, currentRadius), 
                              OperationType::ExternalRoughing, "Position for roughing pass");
        
        // Feed to start of cut
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, currentRadius), 
                               100.0, OperationType::ExternalRoughing, "Feed to cut start");
        
        // Rough along length
        for (const auto& point : profile) {
            if (!profile.empty()) {
                toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(point.x, 0.0, currentRadius), 
                                       200.0, OperationType::ExternalRoughing, "Roughing cut");
            }
        }
        
        currentRadius -= depthOfCut;
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalFinishingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "Internal Finishing Tool");
    auto toolpath = std::make_unique<Toolpath>("Internal_Finishing_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::InternalFinishing);
    
    // Internal finishing pattern
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 1.0, 0.0, 0.5), 
                          OperationType::InternalFinishing, "Rapid to internal finishing start");
    
    // Follow internal profile for precision finishing
    for (const auto& point : profile) {
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(point.x, 0.0, point.z), 
                               80.0, OperationType::InternalFinishing, "Internal finishing pass");
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalFinishingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "External Finishing Tool");
    auto toolpath = std::make_unique<Toolpath>("External_Finishing_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::ExternalFinishing);
    
    // Precision finishing - follow exact profile
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 1.0, 0.0, coordinates.z + 1.0), 
                          OperationType::ExternalFinishing, "Rapid to finishing start");
    
    // Feed to profile start
    if (!profile.empty()) {
        const auto& startPoint = profile.front();
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(startPoint.x, 0.0, startPoint.z), 
                               80.0, OperationType::ExternalFinishing, "Feed to profile start");
        
        // Follow exact profile for finishing
        for (size_t i = 1; i < profile.size(); ++i) {
            const auto& point = profile[i];
            toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(point.x, 0.0, point.z), 
                                   120.0, OperationType::ExternalFinishing, "Finishing pass");
        }
    }
    
    // Retract after finishing
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 2.0, 0.0, coordinates.z + 2.0), 
                          OperationType::ExternalFinishing, "Retract from finishing");
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalGroovingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& grooveGeometry,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Grooving, "External Grooving Tool");
    auto toolpath = std::make_unique<Toolpath>("External_Grooving_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::ExternalGrooving);
    
    // Professional grooving sequence
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z + 2.0), 
                          OperationType::ExternalGrooving, "Rapid to groove start");
    
    // Position for grooving
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z), 
                           100.0, OperationType::ExternalGrooving, "Position for groove");
    
    // Multiple plunge cuts for groove
    double grooveDepth = 2.0; // Default groove depth
    double currentDepth = 0.5;
    
    while (currentDepth <= grooveDepth) {
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y - currentDepth, coordinates.z), 
                               25.0, OperationType::ExternalGrooving, "Groove plunge");
        
        // Side cutting if needed
        if (currentDepth >= grooveDepth * 0.8) {
            toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x + 1.0, coordinates.y - currentDepth, coordinates.z), 
                                   15.0, OperationType::ExternalGrooving, "Groove side cut");
            toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x - 1.0, coordinates.y - currentDepth, coordinates.z), 
                                   15.0, OperationType::ExternalGrooving, "Groove side cut");
        }
        
        // Retract slightly between passes
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y - currentDepth + 0.1, coordinates.z), 
                              OperationType::ExternalGrooving, "Retract between passes");
        
        currentDepth += 0.5;
    }
    
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z + 2.0), 
                          OperationType::ExternalGrooving, "Retract from groove");
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalGroovingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& grooveGeometry,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Grooving, "Internal Grooving Tool");
    auto toolpath = std::make_unique<Toolpath>("Internal_Grooving_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::InternalGrooving);
    
    // Internal grooving pattern
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y - 1.0, coordinates.z), 
                          OperationType::InternalGrooving, "Rapid to internal groove");
    
    // Position for internal grooving
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z), 
                           80.0, OperationType::InternalGrooving, "Position for internal groove");
    
    // Internal groove cutting
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y + 1.5, coordinates.z), 
                           20.0, OperationType::InternalGrooving, "Internal groove cut");
    
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y - 1.0, coordinates.z), 
                          OperationType::InternalGrooving, "Retract from internal groove");
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::chamferingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& chamferGeometry,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "Chamfering Tool");
    auto toolpath = std::make_unique<Toolpath>("Chamfering_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::Chamfering);
    
    // Professional chamfering sequence
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 1.0, coordinates.y, coordinates.z + 1.0), 
                          OperationType::Chamfering, "Rapid to chamfer start");
    
    // Position for chamfer
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z), 
                           100.0, OperationType::Chamfering, "Position for chamfer");
    
    // 45-degree chamfer motion
    double chamferSize = 0.5; // Default chamfer size
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x + chamferSize, coordinates.y - chamferSize, coordinates.z), 
                           60.0, OperationType::Chamfering, "Chamfer cut");
    
    // Retract from chamfer
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x + 2.0, coordinates.y, coordinates.z + 2.0), 
                          OperationType::Chamfering, "Retract from chamfer");
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::threadingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& threadGeometry,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Threading, "Threading Tool");
    auto toolpath = std::make_unique<Toolpath>("Threading_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::Threading);
    
    // Professional threading sequence with multiple passes
    double threadLength = 10.0;
    double threadPitch = 1.5; // M10x1.5 example
    int numPasses = 4;
    
    for (int pass = 0; pass < numPasses; ++pass) {
        double depth = 0.15 * (pass + 1); // Increasing depth each pass
        
        // Rapid to thread start
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z + 2.0), 
                              OperationType::Threading, "Rapid to thread start pass " + std::to_string(pass + 1));
        
        // Position at cutting depth
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y - depth, coordinates.z + 2.0), 
                               100.0, OperationType::Threading, "Position thread depth");
        
        // Threading pass with synchronized feed
        toolpath->addThreadingMove(IntuiCAM::Geometry::Point3D(coordinates.x + threadLength, coordinates.y - depth, coordinates.z), 
                                  100.0, threadPitch);
        
        // Retract at angle to avoid thread damage
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x + threadLength + 1.0, coordinates.y, coordinates.z + 1.0), 
                               300.0, OperationType::Threading, "Thread retract pass " + std::to_string(pass + 1));
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::partingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Parting, "Parting Tool");
    auto toolpath = std::make_unique<Toolpath>("Parting_" + 
        std::to_string(coordinates.x) + "_" + std::to_string(coordinates.z), tool, OperationType::Parting);
    
    // Professional parting sequence - always last operation
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z + 3.0), 
                          OperationType::Parting, "Rapid to parting position");
    
    // Position for parting
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z), 
                           100.0, OperationType::Parting, "Position for parting");
    
    // Part off - feed slowly to center with multiple pecking passes
    double currentRadius = coordinates.z;
    double peckDepth = 1.0;
    
    while (currentRadius > 0.5) { // Leave small core for clean break
        toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, currentRadius - peckDepth), 
                               15.0, OperationType::Parting, "Parting peck");
        
        // Brief retract for chip break
        toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, currentRadius - peckDepth + 0.2), 
                              OperationType::Parting, "Chip break retract");
        
        currentRadius -= peckDepth;
    }
    
    // Final parting cut to center
    toolpath->addLinearMove(IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, 0.0), 
                           10.0, OperationType::Parting, "Final parting cut");
    
    // Rapid retract
    toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(coordinates.x, coordinates.y, coordinates.z + 5.0), 
                          OperationType::Parting, "Retract after parting");
    
    result.push_back(std::move(toolpath));
    return result;
}

// =============================================================================
// HELPER METHODS
// =============================================================================

void ToolpathGenerationPipeline::reportProgress(
    double progress, 
    const std::string& status,
    const PipelineResult& result) {
    
    if (result.progressCallback) {
        result.progressCallback(progress, status);
    }
}

std::vector<Handle(AIS_InteractiveObject)> 
ToolpathGenerationPipeline::createToolpathDisplayObjects(
    const std::vector<std::unique_ptr<Toolpath>>& toolpaths,
    const gp_Trsf& workpieceTransform) {
    
    std::vector<Handle(AIS_InteractiveObject)> displayObjects;
    
    // Professional CAM color scheme based on operation types - matching ToolpathDisplayObject colors
    std::map<OperationType, Quantity_Color> operationColors = {
        {OperationType::Facing, Quantity_Color(0.0, 0.8, 0.2, Quantity_TOC_RGB)},           // Bright Green
        {OperationType::ExternalRoughing, Quantity_Color(0.9, 0.1, 0.1, Quantity_TOC_RGB)}, // Red
        {OperationType::InternalRoughing, Quantity_Color(0.7, 0.0, 0.3, Quantity_TOC_RGB)}, // Dark Red
        {OperationType::ExternalFinishing, Quantity_Color(0.0, 0.4, 0.9, Quantity_TOC_RGB)},// Blue
        {OperationType::InternalFinishing, Quantity_Color(0.0, 0.6, 0.7, Quantity_TOC_RGB)},// Teal
        {OperationType::Drilling, Quantity_Color(0.9, 0.9, 0.0, Quantity_TOC_RGB)},         // Yellow
        {OperationType::Boring, Quantity_Color(0.8, 0.8, 0.2, Quantity_TOC_RGB)},           // Olive
        {OperationType::ExternalGrooving, Quantity_Color(0.9, 0.0, 0.9, Quantity_TOC_RGB)}, // Magenta
        {OperationType::InternalGrooving, Quantity_Color(0.7, 0.0, 0.7, Quantity_TOC_RGB)}, // Purple
        {OperationType::Chamfering, Quantity_Color(0.0, 0.9, 0.9, Quantity_TOC_RGB)},       // Cyan
        {OperationType::Threading, Quantity_Color(0.5, 0.0, 0.9, Quantity_TOC_RGB)},        // Purple-Blue
        {OperationType::Parting, Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB)},          // Orange
        {OperationType::Unknown, Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB)}           // Gray
    };
    
    for (const auto& toolpath : toolpaths) {
        try {
            // Create enhanced toolpath display object with operation type awareness
            auto displaySettings = ToolpathDisplayObject::VisualizationSettings{};
            displaySettings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
            displaySettings.lineWidth = 3.0;
            displaySettings.showStartPoint = true;
            displaySettings.showEndPoint = true;
            
            // Adjust line width based on operation type
            switch (toolpath->getOperationType()) {
                case OperationType::Facing:
                case OperationType::Parting:
                    displaySettings.lineWidth = 4.0; // Thicker for primary operations
                    break;
                case OperationType::ExternalRoughing:
                case OperationType::InternalRoughing:
                    displaySettings.lineWidth = 3.5; // Heavy material removal
                    break;
                case OperationType::ExternalFinishing:
                case OperationType::InternalFinishing:
                    displaySettings.lineWidth = 2.0; // Precision operations
                    displaySettings.showRapidMoves = false; // Hide rapids for cleaner view
                    break;
                case OperationType::Threading:
                    displaySettings.lineWidth = 2.5;
                    displaySettings.colorScheme = ToolpathDisplayObject::ColorScheme::Rainbow; // Show pass progression
                    break;
                default:
                    displaySettings.lineWidth = 2.5;
                    break;
            }
            
            // Create wireframe geometry for toolpath visualization
            BRepBuilderAPI_MakeWire wireBuilder;
            const auto& moves = toolpath->getMovements();
            bool hasValidGeometry = false;
            
            for (size_t i = 1; i < moves.size(); ++i) {
                const auto& prevMove = moves[i-1];
                const auto& currentMove = moves[i];
                
                // CORRECTED COORDINATE SYSTEM TRANSFORMATION
                // Toolpath coordinates are in work coordinates: Point3D(axial, 0, radius)
                // Transform to display coordinates: Point3D(radius, 0, axial) for XZ plane display
                // This matches how profiles are displayed in the workspace controller
                gp_Pnt p1(prevMove.position.z, 0.0, prevMove.position.x);      // (radius, 0, axial)
                gp_Pnt p2(currentMove.position.z, 0.0, currentMove.position.x); // (radius, 0, axial)
                
                if (p1.Distance(p2) > Precision::Confusion()) {
                    BRepBuilderAPI_MakeEdge edgeBuilder(p1, p2);
                    if (edgeBuilder.IsDone()) {
                        wireBuilder.Add(edgeBuilder.Edge());
                        hasValidGeometry = true;
                    }
                }
            }
            
            // Create wireframe display object with operation-specific color
            if (hasValidGeometry && wireBuilder.IsDone()) {
                TopoDS_Shape wireShape = wireBuilder.Wire();
                
                // Apply workpiece transformation to match part position
                if (workpieceTransform.Form() != gp_Identity) {
                    BRepBuilderAPI_Transform transformer(wireShape, workpieceTransform);
                    if (transformer.IsDone()) {
                        wireShape = transformer.Shape();
                    }
                }
                
                Handle(AIS_Shape) aisShape = new AIS_Shape(wireShape);
                
                // Set color based on operation type
                auto colorIt = operationColors.find(toolpath->getOperationType());
                Quantity_Color color = (colorIt != operationColors.end()) ? 
                    colorIt->second : Quantity_Color(0.8, 0.0, 0.0, Quantity_TOC_RGB);
                
                aisShape->SetColor(color);
                aisShape->SetWidth(displaySettings.lineWidth);
                aisShape->SetTransparency(0.0);
                
                // Set display mode for optimal visualization
                aisShape->SetDisplayMode(AIS_WireFrame);
                
                // Add to display objects list
                displayObjects.push_back(aisShape);
            }
            
        } catch (const std::exception& e) {
            // Log error but continue with other toolpaths
            // In a production system, this would use proper logging
            std::string errorMsg = "Error creating display object for toolpath " + 
                                 toolpath->getName() + ": " + e.what();
            // Could emit a warning signal here
        }
    }
    
    return displayObjects;
}

} // namespace Toolpath
} // namespace IntuiCAM
