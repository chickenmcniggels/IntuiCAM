#pragma once

#include <memory>
#include <vector>
#include <string>
#include <future>
#include <functional>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>
#include <AIS_InteractiveObject.hxx>

// IntuiCAM includes
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Forward declarations
class Tool;
class Toolpath;

/**
 * @brief Central coordinator for toolpath generation pipeline
 * 
 * This class orchestrates the complete toolpath generation workflow:
 * 1. Validates operation parameters and fills missing ones
 * 2. Extracts 2D profile from 3D part geometry
 * 3. Generates toolpaths for each enabled operation
 * 4. Creates renderable objects for display
 * 5. Provides progress reporting and error handling
 * 
 * The pipeline is designed to be thread-safe and can run in background
 * while providing progress updates to the UI.
 */
class ToolpathGenerationPipeline {
public:
    /**
     * @brief Operation configuration with parameters
     */
    struct EnabledOperation {
        std::string operationType;      // "Contouring", "Threading", "Chamfering", "Parting"
        bool enabled = false;
        std::map<std::string, double> numericParams;
        std::map<std::string, std::string> stringParams;
        std::map<std::string, bool> booleanParams;
        std::vector<TopoDS_Shape> targetGeometry; // Faces for threading, edges for chamfering
    };

    /**
     * @brief Global parameters affecting all operations
     */
    struct ToolpathGenerationParameters {
        gp_Ax1 turningAxis;             // Main turning axis from workspace
        double safetyHeight = 5.0;      // mm - safe height for rapid moves
        double clearanceDistance = 1.0; // mm - clearance from part surface
        double profileTolerance = 0.01; // mm - tolerance for profile extraction
        int profileSections = 100;      // number of sections for profile
        std::string materialType = "steel"; // Material type for parameter defaults
        double partDiameter = 50.0;     // mm - estimated part diameter
        double partLength = 100.0;      // mm - estimated part length
    };

    /**
     * @brief Complete request for toolpath generation
     */
    struct GenerationRequest {
        TopoDS_Shape partGeometry;                    // 3D part to machine
        std::vector<EnabledOperation> enabledOps;    // Operations to perform
        ToolpathGenerationParameters globalParams;   // Global settings
        std::shared_ptr<Tool> primaryTool;          // Primary cutting tool
        
        // Optional callback for progress reporting (0.0 to 1.0)
        std::function<void(double, const std::string&)> progressCallback;
    };

    /**
     * @brief Statistics about generated toolpaths
     */
    struct ToolpathStatistics {
        double totalMachiningTime = 0.0;    // minutes
        double totalRapidTime = 0.0;        // minutes
        double materialRemovalVolume = 0.0; // mm³
        int totalMovements = 0;             // number of G-code moves
        double totalDistance = 0.0;         // mm - total tool travel
        
        // Per-operation breakdown
        std::map<std::string, double> operationTimes;
        std::map<std::string, int> operationMoves;
    };

    /**
     * @brief Result of toolpath generation pipeline
     */
    struct GenerationResult {
        bool success = false;
        std::string errorMessage;
        std::vector<std::string> warnings;
        
        // Generated data
        LatheProfile::Profile2D extractedProfile;
        std::vector<std::unique_ptr<Toolpath>> generatedToolpaths;
        ToolpathStatistics statistics;
        
        // Display objects
        std::vector<Handle(AIS_InteractiveObject)> toolpathDisplayObjects;
        Handle(AIS_InteractiveObject) profileDisplayObject;
        
        // Metadata
        std::chrono::milliseconds processingTime{0};
        std::string generationTimestamp;
    };

    /**
     * @brief Constructor
     */
    ToolpathGenerationPipeline();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ToolpathGenerationPipeline() = default;

    /**
     * @brief Generate complete toolpath pipeline
     * @param request Complete generation request
     * @return Generation result with toolpaths and display objects
     */
    GenerationResult generateToolpaths(const GenerationRequest& request);

    /**
     * @brief Generate toolpaths asynchronously
     * @param request Complete generation request
     * @return Future that will contain the generation result
     */
    std::future<GenerationResult> generateToolpathsAsync(const GenerationRequest& request);

    /**
     * @brief Validate request before processing
     * @param request Request to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateRequest(const GenerationRequest& request);

    /**
     * @brief Get estimated processing time for request
     * @param request Request to estimate
     * @return Estimated time in seconds
     */
    static double estimateProcessingTime(const GenerationRequest& request);

    /**
     * @brief Cancel ongoing generation (if running asynchronously)
     */
    void cancelGeneration();

    /**
     * @brief Check if generation is currently running
     */
    bool isGenerating() const { return m_isGenerating; }

private:
    /**
     * @brief Validate and fill missing operation parameters
     */
    std::vector<EnabledOperation> validateAndFillParameters(
        const std::vector<EnabledOperation>& operations,
        const ToolpathGenerationParameters& globalParams);

    /**
     * @brief Extract 2D profile from part geometry
     */
    LatheProfile::Profile2D extractProfile(
        const TopoDS_Shape& partGeometry,
        const ToolpathGenerationParameters& params);

    /**
     * @brief Generate toolpath for a specific operation
     */
    std::unique_ptr<Toolpath> generateOperationToolpath(
        const EnabledOperation& operation,
        const LatheProfile::Profile2D& profile,
        std::shared_ptr<Tool> tool,
        const ToolpathGenerationParameters& globalParams);

    /**
     * @brief Create display objects for toolpaths
     */
    std::vector<Handle(AIS_InteractiveObject)> createToolpathDisplayObjects(
        const std::vector<std::unique_ptr<Toolpath>>& toolpaths);

    /**
     * @brief Create display object for 2D profile
     */
    Handle(AIS_InteractiveObject) createProfileDisplayObject(
        const LatheProfile::Profile2D& profile,
        const gp_Ax1& turningAxis);

    /**
     * @brief Calculate comprehensive statistics
     */
    ToolpathStatistics calculateStatistics(
        const std::vector<std::unique_ptr<Toolpath>>& toolpaths,
        const LatheProfile::Profile2D& profile);

    /**
     * @brief Report progress to callback if available
     */
    void reportProgress(double progress, const std::string& status,
                       const GenerationRequest& request);

    // State management
    std::atomic<bool> m_isGenerating{false};
    std::atomic<bool> m_cancelRequested{false};
};

} // namespace Toolpath
} // namespace IntuiCAM 