#pragma once

#include <memory>
#include <vector>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>

namespace IntuiCAM {
namespace Geometry {
    class Part;
}
namespace Toolpath {

class Tool;
class Toolpath;

/**
 * @brief Unified contouring operation that manages facing, roughing, and finishing
 * 
 * This operation coordinates multiple sub-operations to achieve complete contouring:
 * 1. Facing - removes material from the face of the part
 * 2. Roughing - removes bulk material following the contour 
 * 3. Finishing - achieves final surface finish and dimensional accuracy
 * 
 * The operation uses the extracted 2D profile to plan the optimal sequence
 * and parameters for each sub-operation.
 */
class ContouringOperation {
public:
    /**
     * @brief Parameters for the complete contouring operation
     */
    struct Parameters {
        // Overall contouring parameters
        double safetyHeight;        ///< Safe height for rapid moves (mm)
        double clearanceDistance;   ///< Clearance from part surface (mm)
        
        // Facing sub-operation
        bool enableFacing;          ///< Enable facing operation
        FacingOperation::Parameters facingParams;
        
        // Roughing sub-operation  
        bool enableRoughing;        ///< Enable roughing operation
        RoughingOperation::Parameters roughingParams;
        
        // Finishing sub-operation
        bool enableFinishing;       ///< Enable finishing operation
        FinishingOperation::Parameters finishingParams;
        
        // Quality settings
        double profileTolerance;    ///< Tolerance for profile extraction (mm)
        int profileSections;        ///< Number of sections for profile extraction
        
        // Default constructor with sensible defaults
        Parameters() :
            safetyHeight(5.0),
            clearanceDistance(1.0),
            enableFacing(true),
            enableRoughing(true), 
            enableFinishing(true),
            profileTolerance(0.01),
            profileSections(100) {}
    };

    /**
     * @brief Result of contouring operation generation
     */
    struct Result {
        bool success;
        std::string errorMessage;
        
        // Generated toolpaths for each sub-operation
        std::unique_ptr<Toolpath> facingToolpath;
        std::unique_ptr<Toolpath> roughingToolpath;
        std::unique_ptr<Toolpath> finishingToolpath;
        
        // Extracted profile used for generation
        LatheProfile::Profile2D extractedProfile;
        
        // Statistics
        double estimatedTime;       ///< Total estimated machining time (minutes)
        int totalMoves;            ///< Total number of toolpath moves
        double materialRemoved;    ///< Estimated material volume removed (mm³)
        
        Result() : success(false), estimatedTime(0.0), totalMoves(0), materialRemoved(0.0) {}
    };

    /**
     * @brief Constructor
     */
    ContouringOperation() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ContouringOperation() = default;

    /**
     * @brief Generate complete contouring toolpaths
     * @param part The part geometry to machine
     * @param tool The cutting tool to use
     * @param params Operation parameters
     * @return Generation result with toolpaths and statistics
     */
    Result generateToolpaths(const IntuiCAM::Geometry::Part& part,
                           std::shared_ptr<Tool> tool,
                           const Parameters& params);

    /**
     * @brief Extract 2D profile from part geometry
     * @param part The part to analyze
     * @param params Profile extraction parameters
     * @return Extracted 2D profile points (radius, z-coordinate)
     */
    static LatheProfile::Profile2D extractProfile(const IntuiCAM::Geometry::Part& part,
                                                 const Parameters& params);

    /**
     * @brief Validate parameters for operation
     * @param params Parameters to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateParameters(const Parameters& params);

    /**
     * @brief Get default parameters for common materials and operations
     * @param materialType Type of material being machined
     * @param partComplexity Complexity level of the part geometry
     * @return Recommended default parameters
     */
    static Parameters getDefaultParameters(const std::string& materialType = "steel",
                                         const std::string& partComplexity = "medium");

private:
    /**
     * @brief Generate facing toolpath if enabled
     */
    std::unique_ptr<Toolpath> generateFacingPass(const LatheProfile::Profile2D& profile,
                                                std::shared_ptr<Tool> tool,
                                                const Parameters& params);

    /**
     * @brief Generate roughing toolpath if enabled
     */
    std::unique_ptr<Toolpath> generateRoughingPass(const LatheProfile::Profile2D& profile,
                                                  std::shared_ptr<Tool> tool,
                                                  const Parameters& params);

    /**
     * @brief Generate finishing toolpath if enabled
     */
    std::unique_ptr<Toolpath> generateFinishingPass(const LatheProfile::Profile2D& profile,
                                                   std::shared_ptr<Tool> tool,
                                                   const Parameters& params);

    /**
     * @brief Calculate optimal operation sequence based on part geometry
     */
    std::vector<std::string> planOperationSequence(const LatheProfile::Profile2D& profile,
                                                  const Parameters& params);

    /**
     * @brief Estimate total machining time for all operations
     */
    double estimateTotalTime(const Result& result, std::shared_ptr<Tool> tool);

    /**
     * @brief Calculate material removal volume
     */
    double calculateMaterialRemoval(const LatheProfile::Profile2D& profile,
                                  const Parameters& params);
};

} // namespace Toolpath
} // namespace IntuiCAM 