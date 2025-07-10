#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/ToolpathDisplayObject.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/ExternalRoughingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <IntuiCAM/Geometry/Types.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenCASCADE includes for display object creation
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <Quantity_Color.hxx>
#include <Precision.hxx>
#include <Prs3d_LineAspect.hxx>

namespace IntuiCAM {
namespace Toolpath {

// Helper function to create an empty part for operations that require one
namespace {
std::unique_ptr<IntuiCAM::Geometry::OCCTPart> createEmptyPart() {
    // Create an empty compound shape
    TopoDS_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    
    return std::make_unique<IntuiCAM::Geometry::OCCTPart>(&compound);
}
}

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
        
        // -----------------------------------------------------------------------
        // 3.1  Facing – always FIRST: establish reference surface at Z-max
        // -----------------------------------------------------------------------
        if (inputs.facing) {
            reportProgress(0.1, "Generating facing toolpaths...", result);
            
            // IMPROVED: Use actual profile bounds for facing positioning
            double facingStartZ, facingEndZ, facingMaxRadius;
            
            if (!inputs.profile2D.isEmpty()) {
                // Extract actual geometry from profile for accurate positioning
                double profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius;
                inputs.profile2D.getBounds(profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius);
                
                // Position facing at the front face of the part (maximum Z)
                facingStartZ = profileMaxZ + 1.0;  // Start slightly ahead of part
                facingEndZ = profileMaxZ - inputs.facingAllowance;  // End after removing allowance
                facingMaxRadius = profileMaxRadius + 2.0;  // Start slightly outside part radius
                
                std::cout << "  - Using profile-based facing coordinates: Z=" << facingStartZ 
                          << " to " << facingEndZ << ", radius=" << facingMaxRadius << std::endl;
            } else {
                // Fallback to input-based coordinates if no profile available
                facingStartZ = inputs.z0;
                facingEndZ = inputs.z0 - inputs.facingAllowance;
                facingMaxRadius = inputs.rawMaterialDiameter / 2.0;
                
                std::cout << "  - Using fallback facing coordinates (no profile): Z=" << facingStartZ 
                          << " to " << facingEndZ << ", radius=" << facingMaxRadius << std::endl;
            }
            
            double depthOfCut = 1.0; // mm - placeholder
            int passes = static_cast<int>(std::floor(inputs.facingAllowance / depthOfCut));
            
            for (int i = 0; i < passes; ++i) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                // IMPROVED POSITIONING: Use profile-based coordinates for accurate positioning
                double axialPosition = facingStartZ - i * depthOfCut;  // Start from profile front face
                
                IntuiCAM::Geometry::Point3D coordinates(axialPosition, 0.0, facingMaxRadius);
                IntuiCAM::Geometry::Point3D startPos(axialPosition, 0.0, facingMaxRadius + 2.0);  // Start slightly outside
                IntuiCAM::Geometry::Point3D endPos(axialPosition, 0.0, 0.0);                     // End at center
                
                auto facingPasses = facingToolpath(coordinates, startPos, endPos, inputs.facingTool);
                for (auto& tp : facingPasses) {
                    result.timeline.push_back(std::move(tp));
                }
            }
            
            // One final facing pass to finish to dimension
            IntuiCAM::Geometry::Point3D finalCoord(facingEndZ, 0.0, facingMaxRadius);
            IntuiCAM::Geometry::Point3D startPos(facingEndZ, 0.0, facingMaxRadius + 2.0);
            IntuiCAM::Geometry::Point3D endPos(facingEndZ, 0.0, 0.0);
            auto finalFacing = facingToolpath(finalCoord, startPos, endPos, inputs.facingTool);
            for (auto& tp : finalFacing) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // -----------------------------------------------------------------------
        // 3.2  Drilling and Boring (Internal Features)
        // -----------------------------------------------------------------------
        if (inputs.drilling && inputs.machineInternalFeatures) {
            reportProgress(0.2, "Generating drilling toolpaths...", result);
            
            std::vector<double> diameters = {6.0, 8.0, 10.0, 12.0}; // Example diameters
            for (double d : diameters) {
                if (d > inputs.largestDrillSize) {
                    // Boring operation - placeholder
                    continue;
                }
                
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
        }

        // -----------------------------------------------------------------------
        // 3.3  Internal Roughing
        // -----------------------------------------------------------------------
        if (inputs.internalRoughing && inputs.machineInternalFeatures) {
            reportProgress(0.3, "Generating internal roughing toolpaths...", result);
            
            IntuiCAM::Geometry::Point3D coordinates(inputs.z0, 0.0, 0.0);
            auto internalRoughingPaths = internalRoughingToolpath(
                coordinates, inputs.internalRoughingTool, inputs.profile2D);
            for (auto& tp : internalRoughingPaths) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // -----------------------------------------------------------------------
        // 3.4  Internal Finishing
        // -----------------------------------------------------------------------
        if (inputs.internalFinishing && inputs.machineInternalFeatures) {
            reportProgress(0.4, "Generating internal finishing toolpaths...", result);
            
            for (int pass = 0; pass < inputs.internalFinishingPasses; ++pass) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                IntuiCAM::Geometry::Point3D coordinates(inputs.z0, 0.0, 0.0);
                auto internalFinishingPaths = internalFinishingToolpath(
                    coordinates, inputs.internalFinishingTool, inputs.profile2D);
                for (auto& tp : internalFinishingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.5  Internal Grooving
        // -----------------------------------------------------------------------
        if (inputs.internalGrooving && inputs.machineInternalFeatures) {
            reportProgress(0.5, "Generating internal grooving toolpaths...", result);
            
            for (const auto& groove : inputs.internalFeaturesToBeGrooved) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto groovingPaths = internalGroovingToolpath(
                    groove.coordinates, groove.geometry, groove.tool, groove.chamferEdges);
                for (auto& tp : groovingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.6  External Roughing
        // -----------------------------------------------------------------------
        if (inputs.externalRoughing) {
            reportProgress(0.6, "Generating external roughing toolpaths...", result);
            
            // IMPROVED: Use actual profile bounds for roughing positioning
            IntuiCAM::Geometry::Point3D coordinates;
            
            if (!inputs.profile2D.isEmpty()) {
                // Extract actual geometry from profile for accurate positioning
                double profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius;
                inputs.profile2D.getBounds(profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius);
                
                // Position roughing based on actual part profile bounds
                // Start from the maximum Z and radius of the part
                coordinates = IntuiCAM::Geometry::Point3D(profileMaxZ, 0.0, profileMaxRadius);
                
                std::cout << "  - Using profile-based roughing coordinates: Z=" << profileMaxZ 
                          << ", radius=" << profileMaxRadius << std::endl;
            } else {
                // Fallback to input-based coordinates if no profile available
                coordinates = IntuiCAM::Geometry::Point3D(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0);
                
                std::cout << "  - Using fallback roughing coordinates (no profile): Z=" << inputs.z0 
                          << ", radius=" << inputs.rawMaterialDiameter / 2.0 << std::endl;
            }
            
            auto roughingPaths = externalRoughingToolpath(
                coordinates, inputs.externalRoughingTool, inputs.profile2D
            );
            for (auto& tp : roughingPaths) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // -----------------------------------------------------------------------
        // 3.7  External Finishing
        // -----------------------------------------------------------------------
        if (inputs.externalFinishing) {
            reportProgress(0.7, "Generating external finishing toolpaths...", result);
            
            for (int pass = 0; pass < inputs.externalFinishingPasses; ++pass) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                IntuiCAM::Geometry::Point3D coordinates(inputs.z0, 0.0, inputs.rawMaterialDiameter / 2.0);
                auto finishingPaths = externalFinishingToolpath(
                    coordinates, inputs.externalFinishingTool, inputs.profile2D
                );
                for (auto& tp : finishingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.8  External Grooving
        // -----------------------------------------------------------------------
        if (inputs.externalGrooving) {
            reportProgress(0.75, "Generating external grooving toolpaths...", result);
            
            for (const auto& groove : inputs.externalFeaturesToBeGrooved) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto groovingPaths = externalGroovingToolpath(
                    groove.coordinates, groove.geometry, groove.tool, groove.chamferEdges);
                for (auto& tp : groovingPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.9  Chamfering
        // -----------------------------------------------------------------------
        if (inputs.chamfering) {
            reportProgress(0.8, "Generating chamfering toolpaths...", result);
            
            for (const auto& chamfer : inputs.featuresToBeChamfered) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto chamferPaths = chamferingToolpath(
                    chamfer.coordinates, chamfer.geometry, chamfer.tool);
                for (auto& tp : chamferPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.10 Threading
        // -----------------------------------------------------------------------
        if (inputs.threading) {
            reportProgress(0.8, "Generating threading toolpaths...", result);
            for (const auto& thread : inputs.featuresToBeThreaded) {
                if (m_cancelRequested) {
                    result.errorMessage = "Generation cancelled by user";
                    m_isGenerating = false;
                    return result;
                }
                
                auto threadPaths = threadingToolpath(
                    thread.coordinates, thread.geometry, thread.tool
                );
                for (auto& tp : threadPaths) {
                    result.timeline.push_back(std::move(tp));
                }
            }
        }

        // -----------------------------------------------------------------------
        // 3.11 Parting – always LAST
        // -----------------------------------------------------------------------
        if (inputs.parting) {
            reportProgress(0.9, "Generating parting toolpaths...", result);
            
            IntuiCAM::Geometry::Point3D partingCoordinates(
                inputs.z0 - inputs.partLength - inputs.partingAllowance, 0.0, 0.0);
            auto partingPaths = partingToolpath(partingCoordinates, inputs.partingTool, false);
            for (auto& tp : partingPaths) {
                result.timeline.push_back(std::move(tp));
            }
        }

        // Finalize result
        auto endTime = std::chrono::high_resolution_clock::now();
        result.processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        result.success = true;
        
        reportProgress(1.0, "Toolpath generation complete!", result);
        
    } catch (const std::exception& e) {
        result.errorMessage = "Unexpected error during pipeline execution: " + std::string(e.what());
        result.success = false;
    }
    
    m_isGenerating = false;
    return result;
}

ToolpathGenerationPipeline::PipelineInputs 
ToolpathGenerationPipeline::extractInputsFromPart(const TopoDS_Shape& partGeometry, const gp_Ax1& turningAxis) {
    PipelineInputs inputs;
    
    // IMPROVED: Store the actual part geometry for use by operations
    m_currentPartGeometry = partGeometry;
    
    // IMPROVED: Extract 2D profile from part geometry first - this is critical for profile-based toolpaths
    if (!partGeometry.IsNull()) {
        // Extract the 2D profile using ProfileExtractor
        ProfileExtractor::ExtractionParameters extractParams;
        extractParams.tolerance = 0.01;                  // 0.01mm tolerance
        extractParams.minSegmentLength = 0.001;          // Filter very small segments
        extractParams.turningAxis = turningAxis;         // Use provided turning axis
        extractParams.sortSegments = true;               // Ensure proper ordering
        
        std::cout << "ToolpathGenerationPipeline: Extracting 2D profile from part geometry..." << std::endl;
        inputs.profile2D = ProfileExtractor::extractProfile(partGeometry, extractParams);
        
        if (!inputs.profile2D.isEmpty()) {
            std::cout << "  - Profile extracted successfully with " << inputs.profile2D.getSegmentCount() 
                      << " segments" << std::endl;
        } else {
            std::cout << "  - Warning: Profile extraction returned empty profile" << std::endl;
        }
    }
    
    // DYNAMIC EXTRACTION: Extract dimensions from actual part geometry
    if (!partGeometry.IsNull()) {
        // Calculate bounding box of the part to get actual dimensions
        Bnd_Box partBox;
        BRepBndLib::Add(partGeometry, partBox);
        
        if (!partBox.IsVoid()) {
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            partBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            
            // Extract dimensions from part geometry (convert to lathe coordinates)
            double partLength = std::abs(zMax - zMin);
            double maxRadius = std::max(std::abs(xMax), std::abs(xMin));
            maxRadius = std::max(maxRadius, std::max(std::abs(yMax), std::abs(yMin)));
            
            // IMPROVED: Use profile bounds if available for more accurate dimensions
            if (!inputs.profile2D.isEmpty()) {
                double profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius;
                inputs.profile2D.getBounds(profileMinZ, profileMaxZ, profileMinRadius, profileMaxRadius);
                
                // Use profile dimensions which are more accurate for lathe operations
                partLength = std::abs(profileMaxZ - profileMinZ);
                maxRadius = profileMaxRadius;
                
                std::cout << "  - Using profile bounds: length=" << partLength 
                          << "mm, max radius=" << maxRadius << "mm" << std::endl;
            }
            
            // Set dynamic values based on actual part
            inputs.rawMaterialDiameter = maxRadius * 2.1; // 105% of part size for stock allowance
            inputs.rawMaterialLength = partLength * 1.2;  // 120% of part length
            inputs.z0 = inputs.rawMaterialLength;
            inputs.partLength = partLength;
            
            std::cout << "ToolpathGenerationPipeline: Dynamic extraction from part geometry:" << std::endl;
            std::cout << "  - Part length: " << partLength << "mm" << std::endl;
            std::cout << "  - Max radius: " << maxRadius << "mm" << std::endl;
            std::cout << "  - Raw material diameter: " << inputs.rawMaterialDiameter << "mm" << std::endl;
            std::cout << "  - Raw material length: " << inputs.rawMaterialLength << "mm" << std::endl;
        } else {
            std::cerr << "ToolpathGenerationPipeline: Warning - Part geometry bounding box is void, using fallback values" << std::endl;
            // Fallback values if geometry analysis fails
            inputs.rawMaterialDiameter = 25.0;
            inputs.rawMaterialLength = 60.0;
            inputs.z0 = 60.0;
            inputs.partLength = 50.0;
        }
    } else {
        std::cerr << "ToolpathGenerationPipeline: Warning - No part geometry provided, using default values" << std::endl;
        // Default values if no geometry provided
        inputs.rawMaterialDiameter = 25.0;
        inputs.rawMaterialLength = 60.0;
        inputs.z0 = 60.0;
        inputs.partLength = 50.0;
    }
    
    // DYNAMIC FEATURE DETECTION: Extract features from actual part geometry  
    auto features = detectFeatures(inputs.profile2D, partGeometry);
    for (const auto& feature : features) {
        if (feature.type == "hole") {
            inputs.featuresToBeDrilled.push_back(feature);
        } else if (feature.type == "groove" && feature.diameter < inputs.rawMaterialDiameter) {
            inputs.internalFeaturesToBeGrooved.push_back(feature);
        } else if (feature.type == "groove") {
            inputs.externalFeaturesToBeGrooved.push_back(feature);
        } else if (feature.type == "chamfer") {
            inputs.featuresToBeChamfered.push_back(feature);
        } else if (feature.type == "thread") {
            inputs.featuresToBeThreaded.push_back(feature);
        }
    }
    
    return inputs;
}

std::vector<ToolpathGenerationPipeline::DetectedFeature> 
ToolpathGenerationPipeline::detectFeatures(const LatheProfile::Profile2D& profile, const TopoDS_Shape& partGeometry) {
    std::vector<DetectedFeature> features;
    
    // DYNAMIC FEATURE DETECTION: Analyze actual part geometry instead of hardcoded features
    if (!partGeometry.IsNull()) {
        std::cout << "ToolpathGenerationPipeline: Analyzing part geometry for features..." << std::endl;
        
        // TODO: Implement actual feature detection algorithms:
        // - Cylindrical hole detection using TopExp_Explorer for circular faces
        // - Groove detection by analyzing profile segments  
        // - Thread detection using surface analysis
        // - Chamfer detection using edge analysis
        
        // For now, only detect features if profile contains specific patterns
        if (!profile.segments.empty()) {
            // Look for potential hole patterns in profile (concave features)
            for (size_t i = 0; i < profile.segments.size(); ++i) {
                const auto& segment = profile.segments[i];
                
                // Simple heuristic: if segment is short and potentially represents a hole feature
                // In lathe coordinates, start.x is typically the radius, start.z is the axial position
                double segmentRadius = std::abs(segment.start.x);
                
                if (segmentRadius > 0.0 && segmentRadius < 5.0 && segment.length > 0.5) {
                    DetectedFeature holeFeature;
                    holeFeature.type = "hole";
                    holeFeature.depth = segment.length;
                    holeFeature.diameter = segmentRadius * 2.0;
                    holeFeature.coordinates = IntuiCAM::Geometry::Point3D(segment.start.z, 0.0, segment.start.x);
                    std::ostringstream toolNameStream;
                    toolNameStream << "drill_" << holeFeature.diameter << "mm";
                    holeFeature.tool = toolNameStream.str();
                    features.push_back(holeFeature);
                    
                    std::cout << "  - Detected hole feature: diameter = " << holeFeature.diameter 
                             << "mm, depth = " << holeFeature.depth << "mm" << std::endl;
                }
            }
        }
        
        if (features.empty()) {
            std::cout << "  - No features detected in part geometry" << std::endl;
        }
    } else {
        std::cout << "ToolpathGenerationPipeline: No part geometry provided for feature detection" << std::endl;
    }
    
    return features;
}

void ToolpathGenerationPipeline::cancelGeneration() {
    m_cancelRequested = true;
}

// Real implementations for toolpath generation functions using actual operation classes
std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::facingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const IntuiCAM::Geometry::Point3D& startPos,
    const IntuiCAM::Geometry::Point3D& endPos,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    // Create tool from tool data
    auto tool = std::make_shared<Tool>(Tool::Type::Facing, toolData);
    
    // Create FacingOperation instance with name and tool
    FacingOperation facingOp("Facing Pass", tool);
    
    // IMPROVED: Use actual coordinates from part geometry instead of hardcoded values
    // Extract geometry bounds from the current profile if available
    double startZ = coordinates.x;  // Lathe coordinate system: X=axial (Z in machine)
    double endZ = endPos.x;
    double maxRadius = coordinates.z;  // Z=radius in lathe coordinates (X in machine)
    double minRadius = 0.0;  // Usually face to center
    
    // Set up operation parameters based on actual coordinates and realistic GUI parameters
    FacingOperation::Parameters params;
    params.startZ = startZ;
    params.endZ = endZ;
    params.maxRadius = maxRadius;
    params.minRadius = minRadius;
    params.stockAllowance = 0.2;  // Default GUI parameter
    params.depthOfCut = 0.5;
    params.radialStepover = 0.8;
    params.feedRate = 0.15;  // mm/rev
    params.surfaceSpeed = 200.0;  // m/min
    params.strategy = FacingOperation::FacingStrategy::InsideOut;
    params.surfaceQuality = FacingOperation::SurfaceQuality::Medium;
    
    facingOp.setParameters(params);
    
    // IMPROVED: Pass actual part geometry instead of empty part
    auto part = createPartFromGeometry();
    
    // Generate toolpath using the operation
    auto toolpath = facingOp.generateToolpath(*part);
    if (toolpath) {
        // IMPROVED: Ensure operation type is properly set for coloring
        toolpath->setOperationType(OperationType::Facing);
        
        // Override movements to ensure proper operation type setting
        auto& movements = const_cast<std::vector<Movement>&>(toolpath->getMovements());
        for (auto& movement : movements) {
            movement.operationType = OperationType::Facing;
            movement.operationName = "Facing Pass";
        }
        
        result.push_back(std::move(toolpath));
    }
    
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::drillingToolpath(
    double depth,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    auto toolpath = std::make_unique<Toolpath>("Center Drilling", tool, OperationType::Drilling);
    
    // Realistic drilling cycle with peck drilling
    double clearanceZ = 5.0;  // mm above workpiece
    double peckDepth = std::min(2.0, depth / 3.0);  // Peck depth
    double feedRate = 80.0;  // mm/min
    double rapidFeedRate = 500.0;  // mm/min
    
    IntuiCAM::Geometry::Point3D startPos(0.0, 0.0, clearanceZ);
    IntuiCAM::Geometry::Point3D centerPos(0.0, 0.0, 0.0);  // Workpiece surface
    
    // Rapid to clearance height
    toolpath->addRapidMove(startPos);
    
    // Peck drilling cycle
    double currentDepth = 0.0;
    while (currentDepth < depth) {
        double nextDepth = std::min(currentDepth + peckDepth, depth);
        
        // Drill to next depth
        IntuiCAM::Geometry::Point3D drillPos(nextDepth, 0.0, 0.0);
        toolpath->addLinearMove(drillPos, feedRate);
        
        // Retract partially for chip breaking (except on final depth)
        if (nextDepth < depth) {
            IntuiCAM::Geometry::Point3D retractPos(currentDepth + 0.5, 0.0, 0.0);
            toolpath->addRapidMove(retractPos);
        }
        
        currentDepth = nextDepth;
    }
    
    // Final retract to clearance
    toolpath->addRapidMove(startPos);
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalRoughingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    auto toolpath = std::make_unique<Toolpath>("Internal Roughing", tool, OperationType::InternalRoughing);
    
    // Realistic internal roughing with multiple passes
    double startDiameter = std::max(2.0, coordinates.z * 0.5);  // Start with small internal diameter
    double finalDiameter = coordinates.z * 1.8;  // Target diameter (90% of coordinates radius)
    double depthOfCut = 1.0;  // mm per pass
    double feedRate = 120.0;  // mm/min
    double clearanceZ = 2.0;  // mm clearance
    double lengthZ = 15.0;  // mm internal length
    
    // Calculate number of passes
    int numPasses = static_cast<int>(std::ceil((finalDiameter - startDiameter) / depthOfCut));
    
    for (int pass = 0; pass < numPasses; ++pass) {
        double currentDiameter = startDiameter + (pass * depthOfCut);
        double currentRadius = currentDiameter / 2.0;
        
        // Entry point (rapid to start)
        IntuiCAM::Geometry::Point3D entryPos(coordinates.x + clearanceZ, 0.0, currentRadius);
        toolpath->addRapidMove(entryPos);
        
        // Feed to cutting position
        IntuiCAM::Geometry::Point3D startCut(coordinates.x, 0.0, currentRadius);
        toolpath->addLinearMove(startCut, feedRate);
        
        // Internal roughing pass (cutting along Z-axis)
        IntuiCAM::Geometry::Point3D endCut(coordinates.x - lengthZ, 0.0, currentRadius);
        toolpath->addLinearMove(endCut, feedRate);
        
        // Retract axially
        IntuiCAM::Geometry::Point3D retractPos(coordinates.x + clearanceZ, 0.0, currentRadius);
        toolpath->addRapidMove(retractPos);
    }
    
    // Final rapid to safe position
    IntuiCAM::Geometry::Point3D safePos(coordinates.x + clearanceZ, 0.0, coordinates.z + 5.0);
    toolpath->addRapidMove(safePos);
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalRoughingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    // Create tool from tool data
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    
    // Create ExternalRoughingOperation instance with name and tool
    ExternalRoughingOperation roughingOp("External Roughing", tool);
    
    // IMPROVED: Use actual profile geometry instead of hardcoded values
    double startDiameter, endDiameter, startZ, endZ;
    
    if (!profile.isEmpty()) {
        // Extract actual geometry from profile
        double minZ, maxZ, minRadius, maxRadius;
        profile.getBounds(minZ, maxZ, minRadius, maxRadius);
        
        // Use actual profile bounds for roughing parameters
        startDiameter = maxRadius * 2.0;  // Start from maximum radius
        endDiameter = minRadius * 2.0 + 1.0;  // Leave some material for finishing
        startZ = maxZ;  // Start from maximum Z (furthest from chuck)
        endZ = minZ;    // End at minimum Z (closest to chuck)
    } else {
        // Fallback to coordinates if no profile available
        startDiameter = coordinates.z * 2.0;  // Convert radius to diameter
        endDiameter = coordinates.z * 2.0 - 4.0;  // Remove some material
        startZ = coordinates.x;  // X=axial in lathe coordinates
        endZ = coordinates.x - 20.0;  // Roughing length
    }
    
    // Set up operation parameters based on actual profile geometry
    ExternalRoughingOperation::Parameters params;
    params.startDiameter = startDiameter;
    params.endDiameter = endDiameter;
    params.startZ = startZ;
    params.endZ = endZ;
    params.depthOfCut = 2.0;  // mm - Default GUI parameter
    params.stepover = 1.5;  // mm
    params.stockAllowance = 0.5;  // mm - Default GUI parameter
    params.feedRate = 150.0;  // mm/min - Default GUI parameter
    params.spindleSpeed = 1000.0;  // RPM - Default GUI parameter
    params.useProfileFollowing = !profile.isEmpty();  // Use profile if available
    params.enableChipBreaking = true;
    
    roughingOp.setParameters(params);
    
    // IMPROVED: Pass actual part geometry instead of empty part
    auto part = createPartFromGeometry();
    
    // Generate toolpath using the operation
    auto toolpath = roughingOp.generateToolpath(*part);
    if (toolpath) {
        // IMPROVED: Ensure operation type is properly set for coloring
        toolpath->setOperationType(OperationType::ExternalRoughing);
        
        // Override movements to ensure proper operation type setting
        auto& movements = const_cast<std::vector<Movement>&>(toolpath->getMovements());
        for (auto& movement : movements) {
            movement.operationType = OperationType::ExternalRoughing;
            movement.operationName = "External Roughing";
        }
        
        result.push_back(std::move(toolpath));
    }
    
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalFinishingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    // Create tool from tool data
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    
    // Create FinishingOperation instance with name and tool for internal finishing
    FinishingOperation finishingOp("Internal Finishing", tool);
    
    // Set up operation parameters for internal finishing based on coordinates and default GUI parameters
    FinishingOperation::Parameters params;
    params.startZ = coordinates.x;  // X=axial in lathe coordinates
    params.endZ = coordinates.x - 15.0;  // Internal finishing length
    params.stockAllowance = 0.05;  // mm - Default GUI parameter
    params.finalStockAllowance = 0.0;  // mm
    params.strategy = FinishingOperation::FinishingStrategy::MultiPass;
    params.targetQuality = FinishingOperation::SurfaceQuality::Medium;
    params.enableSpringPass = true;
    params.numberOfPasses = 2;
    params.surfaceSpeed = 180.0;  // m/min - slightly slower for internal
    params.feedRate = 0.08;  // mm/rev - Default GUI parameter
    params.springPassFeedRate = 0.05;  // mm/rev
    params.depthOfCut = 0.025;  // mm
    params.profileTolerance = 0.002;  // mm
    params.enableConstantSurfaceSpeed = true;
    params.maxSpindleSpeed = 1500.0;  // RPM - Default GUI parameter
    
    finishingOp.setParameters(params);
    
    // Create an empty Part object for generateToolpath
    auto part = createPartFromGeometry();
    
    // Generate toolpath using the operation
    auto toolpath = finishingOp.generateToolpath(*part);
    if (toolpath) {
        result.push_back(std::move(toolpath));
    }
    
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalFinishingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    const LatheProfile::Profile2D& profile) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    // Create tool from tool data
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    
    // Create FinishingOperation instance with name and tool
    FinishingOperation finishingOp("External Finishing", tool);
    
    // Set up operation parameters based on coordinates and default GUI parameters
    FinishingOperation::Parameters params;
    params.startZ = coordinates.x;  // X=axial in lathe coordinates
    params.endZ = coordinates.x - 20.0;  // Finishing length
    params.stockAllowance = 0.05;  // mm - Default GUI parameter
    params.finalStockAllowance = 0.0;  // mm
    params.strategy = FinishingOperation::FinishingStrategy::MultiPass;
    params.targetQuality = FinishingOperation::SurfaceQuality::Medium;
    params.enableSpringPass = true;
    params.numberOfPasses = 2;
    params.surfaceSpeed = 200.0;  // m/min
    params.feedRate = 0.08;  // mm/rev - Default GUI parameter
    params.springPassFeedRate = 0.05;  // mm/rev
    params.depthOfCut = 0.025;  // mm
    params.profileTolerance = 0.002;  // mm
    params.enableConstantSurfaceSpeed = true;
    params.maxSpindleSpeed = 1500.0;  // RPM - Default GUI parameter
    
    finishingOp.setParameters(params);
    
    // Create an empty Part object for generateToolpath
    auto part = createPartFromGeometry();
    
    // Generate toolpath using the operation
    auto toolpath = finishingOp.generateToolpath(*part);
    if (toolpath) {
        result.push_back(std::move(toolpath));
    }
    
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::externalGroovingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& grooveGeometry,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Grooving, toolData);
    auto toolpath = std::make_unique<Toolpath>("External Grooving", tool, OperationType::ExternalGrooving);
    
    // Extract groove parameters from geometry map or use defaults
    double grooveWidth = grooveGeometry.count("width") ? grooveGeometry.at("width") : 3.0;  // mm
    double grooveDepth = grooveGeometry.count("depth") ? grooveGeometry.at("depth") : 2.0;  // mm
    double toolWidth = grooveGeometry.count("tool_width") ? grooveGeometry.at("tool_width") : 2.5;  // mm
    
    double feedRate = 40.0;  // mm/min (slow for grooving)
    double clearanceDistance = 2.0;  // mm
    double safeRadius = coordinates.z + 5.0;  // Safe approach radius
    
    // Calculate groove positions
    double grooveStartZ = coordinates.x - grooveWidth / 2.0;
    double grooveEndZ = coordinates.x + grooveWidth / 2.0;
    double finalRadius = coordinates.z - grooveDepth;
    
    // Rapid to approach position
    IntuiCAM::Geometry::Point3D approachPos(grooveStartZ, 0.0, safeRadius);
    toolpath->addRapidMove(approachPos);
    
    // Rapid to cutting start position
    IntuiCAM::Geometry::Point3D startPos(grooveStartZ, 0.0, coordinates.z + clearanceDistance);
    toolpath->addRapidMove(startPos);
    
    // Multiple passes if groove is wider than tool
    int numPasses = static_cast<int>(std::ceil(grooveWidth / toolWidth));
    double passStep = grooveWidth / numPasses;
    
    for (int pass = 0; pass < numPasses; ++pass) {
        double currentZ = grooveStartZ + (pass * passStep);
        
        // Position at current Z location
        IntuiCAM::Geometry::Point3D currentStart(currentZ, 0.0, coordinates.z + clearanceDistance);
        toolpath->addLinearMove(currentStart, feedRate);
        
        // Plunge to full depth
        IntuiCAM::Geometry::Point3D bottomPos(currentZ, 0.0, finalRadius);
        toolpath->addLinearMove(bottomPos, feedRate * 0.5);  // Slower plunge feed
        
        // Retract to clearance
        IntuiCAM::Geometry::Point3D retractPos(currentZ, 0.0, coordinates.z + clearanceDistance);
        toolpath->addLinearMove(retractPos, feedRate);
    }
    
    // Add chamfers if requested
    if (chamferEdges) {
        double chamferSize = 0.5;  // mm
        
        // Left chamfer
        IntuiCAM::Geometry::Point3D leftChamferStart(grooveStartZ, 0.0, coordinates.z);
        IntuiCAM::Geometry::Point3D leftChamferEnd(grooveStartZ - chamferSize, 0.0, coordinates.z - chamferSize);
        toolpath->addLinearMove(leftChamferStart, feedRate);
        toolpath->addLinearMove(leftChamferEnd, feedRate);
        
        // Right chamfer
        IntuiCAM::Geometry::Point3D rightChamferStart(grooveEndZ, 0.0, coordinates.z);
        IntuiCAM::Geometry::Point3D rightChamferEnd(grooveEndZ + chamferSize, 0.0, coordinates.z - chamferSize);
        toolpath->addLinearMove(rightChamferStart, feedRate);
        toolpath->addLinearMove(rightChamferEnd, feedRate);
    }
    
    // Final retract to safe position
    IntuiCAM::Geometry::Point3D safePos(coordinates.x, 0.0, safeRadius);
    toolpath->addRapidMove(safePos);
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::internalGroovingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& grooveGeometry,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Grooving, toolData);
    auto toolpath = std::make_unique<Toolpath>("Internal Grooving", tool, OperationType::InternalGrooving);
    
    // Extract groove parameters from geometry map or use defaults
    double grooveWidth = grooveGeometry.count("width") ? grooveGeometry.at("width") : 3.0;  // mm
    double grooveDepth = grooveGeometry.count("depth") ? grooveGeometry.at("depth") : 2.0;  // mm
    double toolWidth = grooveGeometry.count("tool_width") ? grooveGeometry.at("tool_width") : 2.5;  // mm
    double boreDiameter = grooveGeometry.count("bore_diameter") ? grooveGeometry.at("bore_diameter") : coordinates.z * 1.6;  // mm
    
    double feedRate = 35.0;  // mm/min (slower for internal grooving)
    double clearanceDistance = 1.0;  // mm
    double startRadius = boreDiameter / 2.0 - clearanceDistance;  // Start inside bore
    
    // Calculate groove positions
    double grooveStartZ = coordinates.x - grooveWidth / 2.0;
    double grooveEndZ = coordinates.x + grooveWidth / 2.0;
    double finalRadius = startRadius + grooveDepth;  // Internal grooving expands outward
    
    // Rapid to approach position (inside the bore)
    IntuiCAM::Geometry::Point3D approachPos(grooveStartZ, 0.0, 0.0);  // Approach from center
    toolpath->addRapidMove(approachPos);
    
    // Move to cutting start position inside bore
    IntuiCAM::Geometry::Point3D startPos(grooveStartZ, 0.0, startRadius);
    toolpath->addRapidMove(startPos);
    
    // Multiple passes if groove is wider than tool
    int numPasses = static_cast<int>(std::ceil(grooveWidth / toolWidth));
    double passStep = grooveWidth / numPasses;
    
    for (int pass = 0; pass < numPasses; ++pass) {
        double currentZ = grooveStartZ + (pass * passStep);
        
        // Position at current Z location
        IntuiCAM::Geometry::Point3D currentStart(currentZ, 0.0, startRadius);
        toolpath->addLinearMove(currentStart, feedRate);
        
        // Plunge outward to full depth (internal grooving expands radius)
        IntuiCAM::Geometry::Point3D bottomPos(currentZ, 0.0, finalRadius);
        toolpath->addLinearMove(bottomPos, feedRate * 0.5);  // Slower plunge feed
        
        // Retract to start radius
        IntuiCAM::Geometry::Point3D retractPos(currentZ, 0.0, startRadius);
        toolpath->addLinearMove(retractPos, feedRate);
    }
    
    // Add chamfers if requested (internal chamfering)
    if (chamferEdges) {
        double chamferSize = 0.3;  // mm (smaller for internal)
        
        // Left chamfer
        IntuiCAM::Geometry::Point3D leftChamferStart(grooveStartZ, 0.0, finalRadius);
        IntuiCAM::Geometry::Point3D leftChamferEnd(grooveStartZ - chamferSize, 0.0, finalRadius - chamferSize);
        toolpath->addLinearMove(leftChamferStart, feedRate);
        toolpath->addLinearMove(leftChamferEnd, feedRate);
        
        // Right chamfer
        IntuiCAM::Geometry::Point3D rightChamferStart(grooveEndZ, 0.0, finalRadius);
        IntuiCAM::Geometry::Point3D rightChamferEnd(grooveEndZ + chamferSize, 0.0, finalRadius - chamferSize);
        toolpath->addLinearMove(rightChamferStart, feedRate);
        toolpath->addLinearMove(rightChamferEnd, feedRate);
    }
    
    // Final retract to center for safe exit
    IntuiCAM::Geometry::Point3D safePos(coordinates.x, 0.0, 0.0);
    toolpath->addRapidMove(safePos);
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::chamferingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& chamferGeometry,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, toolData);
    auto toolpath = std::make_unique<Toolpath>("Chamfering", tool, OperationType::Chamfering);
    
    // Extract chamfer parameters from geometry map or use defaults
    double chamferSize = chamferGeometry.count("size") ? chamferGeometry.at("size") : 1.0;  // mm
    double chamferAngle = chamferGeometry.count("angle") ? chamferGeometry.at("angle") : 45.0;  // degrees
    bool isInternal = chamferGeometry.count("internal") ? (chamferGeometry.at("internal") > 0.5) : false;
    bool isFrontFace = chamferGeometry.count("front_face") ? (chamferGeometry.at("front_face") > 0.5) : true;
    
    double feedRate = 80.0;  // mm/min
    double clearanceDistance = 2.0;  // mm
    double safeDistance = 5.0;  // mm
    
    // Calculate chamfer geometry based on angle
    double angleRad = chamferAngle * M_PI / 180.0;
    double radialComponent = chamferSize * cos(angleRad);
    double axialComponent = chamferSize * sin(angleRad);
    
    if (isInternal) {
        // Internal chamfer (inside a bore)
        double boreRadius = coordinates.z * 0.8;  // Assume 80% of coordinate radius is bore
        
        // Approach from center
        IntuiCAM::Geometry::Point3D approachPos(coordinates.x + clearanceDistance, 0.0, 0.0);
        toolpath->addRapidMove(approachPos);
        
        // Move to chamfer start position
        IntuiCAM::Geometry::Point3D startPos(coordinates.x, 0.0, boreRadius - radialComponent);
        toolpath->addLinearMove(startPos, feedRate);
        
        // Execute chamfer move
        IntuiCAM::Geometry::Point3D endPos;
        if (isFrontFace) {
            endPos = IntuiCAM::Geometry::Point3D(coordinates.x + axialComponent, 0.0, boreRadius);
        } else {
            endPos = IntuiCAM::Geometry::Point3D(coordinates.x - axialComponent, 0.0, boreRadius);
        }
        toolpath->addLinearMove(endPos, feedRate);
        
        // Retract to center
        IntuiCAM::Geometry::Point3D retractPos(coordinates.x + clearanceDistance, 0.0, 0.0);
        toolpath->addRapidMove(retractPos);
        
    } else {
        // External chamfer
        double safeRadius = coordinates.z + safeDistance;
        
        // Rapid to approach position
        IntuiCAM::Geometry::Point3D approachPos(coordinates.x + clearanceDistance, 0.0, safeRadius);
        toolpath->addRapidMove(approachPos);
        
        // Move to chamfer start position
        IntuiCAM::Geometry::Point3D startPos;
        IntuiCAM::Geometry::Point3D endPos;
        
        if (isFrontFace) {
            // Front face chamfer (face to OD)
            startPos = IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, coordinates.z - radialComponent);
            endPos = IntuiCAM::Geometry::Point3D(coordinates.x + axialComponent, 0.0, coordinates.z);
        } else {
            // Back face chamfer (OD to face)
            startPos = IntuiCAM::Geometry::Point3D(coordinates.x - axialComponent, 0.0, coordinates.z);
            endPos = IntuiCAM::Geometry::Point3D(coordinates.x, 0.0, coordinates.z - radialComponent);
        }
        
        // Rapid to start position
        toolpath->addLinearMove(startPos, feedRate);
        
        // Execute chamfer move
        toolpath->addLinearMove(endPos, feedRate);
        
        // Retract to safe position
        IntuiCAM::Geometry::Point3D retractPos(coordinates.x + clearanceDistance, 0.0, safeRadius);
        toolpath->addRapidMove(retractPos);
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::threadingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::map<std::string, double>& threadGeometry,
    const std::string& toolData) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    auto tool = std::make_shared<Tool>(Tool::Type::Threading, toolData);
    auto toolpath = std::make_unique<Toolpath>("Threading", tool, OperationType::Threading);
    
    // Extract thread parameters from geometry map or use defaults
    double pitch = threadGeometry.count("pitch") ? threadGeometry.at("pitch") : 1.5;  // mm (M10 thread)
    double threadLength = threadGeometry.count("length") ? threadGeometry.at("length") : 15.0;  // mm
    double threadDepth = threadGeometry.count("depth") ? threadGeometry.at("depth") : 0.9;  // mm (60% of pitch)
    double majorDiameter = threadGeometry.count("major_diameter") ? threadGeometry.at("major_diameter") : coordinates.z * 2.0;  // mm
    bool isInternal = threadGeometry.count("internal") ? (threadGeometry.at("internal") > 0.5) : false;
    int numPasses = threadGeometry.count("passes") ? static_cast<int>(threadGeometry.at("passes")) : 3;
    
    double feedRate = 60.0;  // mm/min (slower for threading)
    double clearanceDistance = 3.0;  // mm
    double safeDistance = 5.0;  // mm
    
    // Calculate thread parameters
    double threadStartZ = coordinates.x;
    double threadEndZ = coordinates.x - threadLength;
    double minorDiameter = majorDiameter - (2.0 * threadDepth);
    
    if (isInternal) {
        // Internal threading
        double boreRadius = majorDiameter / 2.0;  // Start at major diameter
        double threadRadius = minorDiameter / 2.0;  // End at minor diameter
        
        // Approach from center
        IntuiCAM::Geometry::Point3D approachPos(threadStartZ + clearanceDistance, 0.0, 0.0);
        toolpath->addRapidMove(approachPos);
        
        // Multiple threading passes with increasing depth
        for (int pass = 0; pass < numPasses; ++pass) {
            double passDepth = threadDepth * (pass + 1) / numPasses;
            double currentRadius = boreRadius - passDepth;
            
            // Position at thread start
            IntuiCAM::Geometry::Point3D startPos(threadStartZ, 0.0, currentRadius);
            toolpath->addLinearMove(startPos, feedRate);
            
            // Threading pass with synchronized motion (simplified as linear move)
            IntuiCAM::Geometry::Point3D endPos(threadEndZ, 0.0, currentRadius);
            toolpath->addLinearMove(endPos, feedRate);
            
            // Retract axially
            IntuiCAM::Geometry::Point3D retractPos(threadEndZ - clearanceDistance, 0.0, currentRadius);
            toolpath->addRapidMove(retractPos);
            
            // Return to start position for next pass
            if (pass < numPasses - 1) {
                toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(threadStartZ + clearanceDistance, 0.0, 0.0));
            }
        }
        
        // Final retract to center
        IntuiCAM::Geometry::Point3D finalRetract(threadStartZ + clearanceDistance, 0.0, 0.0);
        toolpath->addRapidMove(finalRetract);
        
    } else {
        // External threading
        double majorRadius = majorDiameter / 2.0;
        double minorRadius = minorDiameter / 2.0;
        double safeRadius = majorRadius + safeDistance;
        
        // Rapid to approach position
        IntuiCAM::Geometry::Point3D approachPos(threadStartZ + clearanceDistance, 0.0, safeRadius);
        toolpath->addRapidMove(approachPos);
        
        // Multiple threading passes with increasing depth
        for (int pass = 0; pass < numPasses; ++pass) {
            double passDepth = threadDepth * (pass + 1) / numPasses;
            double currentRadius = majorRadius - passDepth;
            
            // Position at thread start
            IntuiCAM::Geometry::Point3D startPos(threadStartZ, 0.0, currentRadius);
            toolpath->addLinearMove(startPos, feedRate);
            
            // Threading pass with synchronized motion (simplified as linear move)
            IntuiCAM::Geometry::Point3D endPos(threadEndZ, 0.0, currentRadius);
            toolpath->addLinearMove(endPos, feedRate);
            
            // Retract radially and axially
            IntuiCAM::Geometry::Point3D retractPos(threadEndZ - clearanceDistance, 0.0, safeRadius);
            toolpath->addRapidMove(retractPos);
            
            // Return to start position for next pass
            if (pass < numPasses - 1) {
                toolpath->addRapidMove(IntuiCAM::Geometry::Point3D(threadStartZ + clearanceDistance, 0.0, safeRadius));
            }
        }
        
        // Final retract to safe position
        IntuiCAM::Geometry::Point3D finalRetract(threadStartZ + clearanceDistance, 0.0, safeRadius);
        toolpath->addRapidMove(finalRetract);
    }
    
    result.push_back(std::move(toolpath));
    return result;
}

std::vector<std::unique_ptr<Toolpath>> ToolpathGenerationPipeline::partingToolpath(
    const IntuiCAM::Geometry::Point3D& coordinates,
    const std::string& toolData,
    bool chamferEdges) {
    
    std::vector<std::unique_ptr<Toolpath>> result;
    
    // Create tool from tool data
    auto tool = std::make_shared<Tool>(Tool::Type::Parting, toolData);
    
    // Create PartingOperation instance
    PartingOperation partingOp;
    
    // Set up operation parameters based on coordinates and default GUI parameters
    PartingOperation::Parameters params;
    params.partingDiameter = coordinates.z * 2.0;  // Convert radius to diameter
    params.partingZ = coordinates.x;  // X position for parting in lathe coordinates
    params.centerHoleDiameter = 0.0;  // Part through center
    params.partingWidth = 3.0;  // mm - Default parting tool width
    params.strategy = PartingOperation::PartingStrategy::Straight;
    params.approach = PartingOperation::ApproachDirection::Radial;
    params.feedRate = 30.0;  // mm/min - Default GUI parameter
    params.spindleSpeed = 800.0;  // RPM - Default GUI parameter
    params.depthOfCut = 0.5;  // mm
    params.numberOfPasses = 1;
    params.safetyHeight = 5.0;  // mm
    params.clearanceDistance = 1.0;  // mm
    params.retractDistance = 5.0;  // mm
    params.finishingAllowance = 0.1;  // mm
    params.enableFinishingPass = true;
    params.finishingFeedRate = 25.0;  // mm/min
    params.enableCoolant = true;
    params.enableChipBreaking = true;
    
    // Create an empty Part object for generateToolpaths
    auto part = createPartFromGeometry();
    
    // Generate toolpaths using the operation (note: different interface)
    auto partingResult = partingOp.generateToolpaths(*part, tool, params);
    
    if (partingResult.success) {
        if (partingResult.grooveToolpath) {
            result.push_back(std::move(partingResult.grooveToolpath));
        }
        if (partingResult.partingToolpath) {
            result.push_back(std::move(partingResult.partingToolpath));
        }
        if (partingResult.finishingToolpath) {
            result.push_back(std::move(partingResult.finishingToolpath));
        }
    }
    
    return result;
}

std::vector<Handle(AIS_InteractiveObject)> ToolpathGenerationPipeline::createToolpathDisplayObjects(
    const std::vector<std::unique_ptr<Toolpath>>& toolpaths,
    const gp_Trsf& workpieceTransform) {
    
    std::vector<Handle(AIS_InteractiveObject)> displayObjects;
    
    // Create proper ToolpathDisplayObject instances for each toolpath
    for (const auto& toolpath : toolpaths) {
        if (!toolpath || toolpath->getMovements().empty()) {
            continue;
        }
        
        try {
            // Create ToolpathDisplayObject with appropriate visualization settings
            ToolpathDisplayObject::VisualizationSettings settings;
            
            // Set color and visualization based on operation type
            switch (toolpath->getOperationType()) {
                case OperationType::Facing:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 2.5;
                    break;
                case OperationType::ExternalRoughing:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 2.0;
                    break;
                case OperationType::InternalRoughing:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 2.0;
                    break;
                case OperationType::ExternalFinishing:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::DepthBased;
                    settings.lineWidth = 1.5;
                    break;
                case OperationType::InternalFinishing:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::DepthBased;
                    settings.lineWidth = 1.5;
                    break;
                case OperationType::Parting:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 3.0;
                    break;
                case OperationType::ExternalGrooving:
                case OperationType::InternalGrooving:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 2.5;
                    break;
                case OperationType::Threading:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::Rainbow;
                    settings.lineWidth = 2.0;
                    break;
                case OperationType::Chamfering:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 1.5;
                    break;
                case OperationType::Drilling:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::OperationType;
                    settings.lineWidth = 2.0;
                    break;
                default:
                    settings.colorScheme = ToolpathDisplayObject::ColorScheme::Default;
                    settings.lineWidth = 2.0;
                    break;
            }
            
            // Create shared pointer to toolpath for the display object
            std::shared_ptr<Toolpath> sharedToolpath;
            
            // Create a default tool for the display object (since ToolpathDisplayObject needs it)
            auto defaultTool = std::make_shared<Tool>(Tool::Type::Turning, "Display Tool");
            
            // Create a new toolpath with the same operation type and movements
            auto newToolpath = std::make_shared<Toolpath>("Display Toolpath", defaultTool, toolpath->getOperationType());
            
            // Copy movements from the original toolpath
            const auto& movements = toolpath->getMovements();
            for (const auto& movement : movements) {
                newToolpath->addMovement(movement);
            }
            
            sharedToolpath = newToolpath;
            
            // Create the ToolpathDisplayObject
            Handle(ToolpathDisplayObject) displayObj = ToolpathDisplayObject::create(sharedToolpath, settings);
            
            if (!displayObj.IsNull()) {
                // Apply coordinate transformation if needed
                // The ToolpathDisplayObject should handle coordinate transformation internally
                
                // IMPROVED: Use color scheme instead of manual color override
                displayObj->SetDisplayMode(AIS_WireFrame);
                displayObj->SetTransparency(0.0);
                
                displayObjects.push_back(displayObj);
            }
            
        } catch (const std::exception& e) {
            // Log error but continue with other toolpaths
            continue;
        }
    }
    
    return displayObjects;
}

void ToolpathGenerationPipeline::reportProgress(double progress, const std::string& status, const PipelineResult& result) {
    if (result.progressCallback) {
        result.progressCallback(progress, status);
    }
    
    std::cout << "ToolpathGenerationPipeline: " << static_cast<int>(progress * 100) << "% - " << status << std::endl;
}

// IMPROVED: Create Part object from stored geometry
std::unique_ptr<IntuiCAM::Geometry::OCCTPart> ToolpathGenerationPipeline::createPartFromGeometry() const {
    if (!m_currentPartGeometry.IsNull()) {
        // Create Part object from actual stored geometry
        return std::make_unique<IntuiCAM::Geometry::OCCTPart>(&m_currentPartGeometry);
    } else {
        // Fallback to empty part if no geometry stored
        return createEmptyPart();
    }
}

} // namespace Toolpath
} // namespace IntuiCAM
