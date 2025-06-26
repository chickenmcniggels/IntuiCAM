#pragma once

#include <memory>
#include <vector>
#include <string>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>

namespace IntuiCAM {
namespace Geometry {
    class Part;
}
namespace Toolpath {

class Tool;
class Toolpath;

/**
 * @brief Parting operation for cutting off parts and creating grooves
 * 
 * Supports various parting strategies including straight parting, groove parting,
 * stepped parting for large diameters, and automatic parting position detection
 * from 2D profiles.
 */
class PartingOperation {
public:
    /**
     * @brief Parting strategy types
     */
    enum class PartingStrategy {
        Straight,       ///< Single straight cut from outside to center
        Stepped,        ///< Multiple stepped cuts for large diameters
        Groove,         ///< Create groove before final parting
        Undercut,       ///< Undercut parting to avoid tool interference
        Trepanning      ///< Trepanning for large parts or hollow sections
    };
    
    /**
     * @brief Tool approach direction
     */
    enum class ApproachDirection {
        Radial,         ///< Feed radially inward (most common)
        Axial,          ///< Feed axially along the part
        Angular         ///< Angled approach for undercuts
    };
    
    /**
     * @brief Parameters for parting operation
     */
    struct Parameters {
        // Basic parting geometry
        double partingDiameter;         ///< Diameter to part at (mm)
        double partingZ;                ///< Z position for parting (mm)
        double centerHoleDiameter;      ///< Center hole diameter (0 for solid) (mm)
        double partingWidth;            ///< Width of parting cut (mm)
        
        // Parting strategy
        PartingStrategy strategy;       ///< Parting strategy to use
        ApproachDirection approach;     ///< Tool approach direction
        
        // Cutting parameters
        double feedRate;                ///< Feed rate (mm/min)
        double spindleSpeed;            ///< Spindle speed (RPM)
        double depthOfCut;              ///< Depth per pass (mm)
        int numberOfPasses;             ///< Number of parting passes
        
        // Advanced parameters
        double stepSize;                ///< Step size for stepped parting (mm)
        double grooveWidth;             ///< Width of relief groove (mm)
        double grooveDepth;             ///< Depth of relief groove (mm)
        double undercutAngle;           ///< Undercut angle (degrees)
        double undercutDepth;           ///< Undercut depth (mm)
        
        // Safety and clearance
        double safetyHeight;            ///< Safe height for rapid moves (mm)
        double clearanceDistance;       ///< Clearance from part surface (mm)
        double retractDistance;         ///< Retract distance after parting (mm)
        
        // Quality settings
        double finishingAllowance;      ///< Material left for finishing pass (mm)
        bool enableFinishingPass;       ///< Enable final finishing pass
        double finishingFeedRate;       ///< Feed rate for finishing pass (mm/min)
        
        // Part handling
        bool supportPart;               ///< Use part catcher/support
        double supportPosition;         ///< Position of part support (mm)
        bool ejectPart;                 ///< Eject part after cutting
        
        // Coolant and chip management
        bool enableCoolant;             ///< Enable coolant during parting
        bool enableChipBreaking;        ///< Enable chip breaking moves
        double chipBreakDistance;       ///< Retract distance for chip breaking (mm)
        
        // Default constructor with reasonable defaults
        Parameters() :
            partingDiameter(20.0),
            partingZ(-50.0),
            centerHoleDiameter(0.0),
            partingWidth(3.0),
            strategy(PartingStrategy::Straight),
            approach(ApproachDirection::Radial),
            feedRate(50.0),
            spindleSpeed(800.0),
            depthOfCut(0.5),
            numberOfPasses(1),
            stepSize(2.0),
            grooveWidth(4.0),
            grooveDepth(2.0),
            undercutAngle(5.0),
            undercutDepth(1.0),
            safetyHeight(5.0),
            clearanceDistance(1.0),
            retractDistance(5.0),
            finishingAllowance(0.1),
            enableFinishingPass(true),
            finishingFeedRate(25.0),
            supportPart(false),
            supportPosition(0.0),
            ejectPart(false),
            enableCoolant(true),
            enableChipBreaking(true),
            chipBreakDistance(0.5) {}
    };
    
    /**
     * @brief Parting position detected from profile
     */
    struct PartingPosition {
        double zPosition;               ///< Z coordinate for parting
        double diameter;                ///< Diameter at parting position
        double neckedDiameter;          ///< Minimum diameter (if necked)
        bool hasNeck;                   ///< Whether part has necking feature
        bool isOptimal;                 ///< Whether this is optimal parting position
        double confidence;              ///< Detection confidence (0-1)
        std::string reason;             ///< Reason for this position choice
    };
    
    /**
     * @brief Result of parting operation generation
     */
    struct Result {
        bool success;
        std::string errorMessage;
        
        // Generated toolpaths
        std::unique_ptr<Toolpath> grooveToolpath;    ///< Relief groove toolpath
        std::unique_ptr<Toolpath> partingToolpath;   ///< Main parting toolpath
        std::unique_ptr<Toolpath> finishingToolpath; ///< Finishing pass toolpath
        
        // Parting information
        Parameters usedParameters;      ///< Final parameters used
        std::vector<PartingPosition> detectedPositions; ///< Detected parting positions
        PartingPosition selectedPosition; ///< Selected parting position
        
        // Statistics
        double estimatedTime;           ///< Total parting time (minutes)
        int totalPasses;               ///< Total number of passes
        double materialRemoved;        ///< Material volume removed (mm³)
        double partLength;             ///< Length of cut part (mm)
        
        Result() : success(false), estimatedTime(0.0), totalPasses(0), 
                  materialRemoved(0.0), partLength(0.0) {}
    };

    /**
     * @brief Constructor
     */
    PartingOperation() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PartingOperation() = default;

    /**
     * @brief Generate parting toolpaths
     * @param part The part geometry to part
     * @param tool The parting tool to use
     * @param params Parting parameters
     * @return Generation result with toolpaths and statistics
     */
    Result generateToolpaths(const IntuiCAM::Geometry::Part& part,
                           std::shared_ptr<Tool> tool,
                           const Parameters& params);

    /**
     * @brief Detect optimal parting positions from 2D profile
     * @param profile Extracted 2D profile
     * @param params Parting parameters for detection criteria
     * @return Detected parting positions sorted by preference
     */
    static std::vector<PartingPosition> detectPartingPositions(
        const LatheProfile::Profile2D& profile,
        const Parameters& params);

    /**
     * @brief Select optimal parting position from candidates
     * @param positions Candidate parting positions
     * @param params Parting parameters
     * @return Selected optimal position
     */
    static PartingPosition selectOptimalPosition(
        const std::vector<PartingPosition>& positions,
        const Parameters& params);

    /**
     * @brief Validate parting parameters
     * @param params Parameters to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateParameters(const Parameters& params);

    /**
     * @brief Get default parameters for specific parting scenarios
     * @param diameter Part diameter to be parted
     * @param materialType Material being parted
     * @param partType Type of part (solid, hollow, thin-wall)
     * @return Recommended default parameters
     */
    static Parameters getDefaultParameters(double diameter = 20.0,
                                         const std::string& materialType = "steel",
                                         const std::string& partType = "solid");

private:
    /**
     * @brief Generate straight parting toolpath
     */
    std::unique_ptr<Toolpath> generateStraightParting(const Parameters& params,
                                                     std::shared_ptr<Tool> tool);

    /**
     * @brief Generate stepped parting toolpath for large diameters
     */
    std::unique_ptr<Toolpath> generateSteppedParting(const Parameters& params,
                                                    std::shared_ptr<Tool> tool);

    /**
     * @brief Generate groove relief before parting
     */
    std::unique_ptr<Toolpath> generateGrooveRelief(const Parameters& params,
                                                  std::shared_ptr<Tool> tool);

    /**
     * @brief Generate undercut parting toolpath
     */
    std::unique_ptr<Toolpath> generateUndercutParting(const Parameters& params,
                                                     std::shared_ptr<Tool> tool);

    /**
     * @brief Generate trepanning toolpath for large parts
     */
    std::unique_ptr<Toolpath> generateTrepanningParting(const Parameters& params,
                                                       std::shared_ptr<Tool> tool);

    /**
     * @brief Generate finishing pass toolpath
     */
    std::unique_ptr<Toolpath> generateFinishingPass(const Parameters& params,
                                                   std::shared_ptr<Tool> tool);

    /**
     * @brief Calculate optimal step sizes for stepped parting
     */
    std::vector<double> calculateStepSizes(const Parameters& params);

    /**
     * @brief Estimate parting time based on strategy and parameters
     */
    double estimatePartingTime(const Parameters& params, std::shared_ptr<Tool> tool);

    /**
     * @brief Calculate material removal volume for parting
     */
    double calculateMaterialRemoval(const Parameters& params);

    /**
     * @brief Validate tool compatibility with parting operation
     */
    bool validateToolCompatibility(std::shared_ptr<Tool> tool, const Parameters& params);

    /**
     * @brief Calculate chip breaking positions during parting
     */
    std::vector<double> calculateChipBreakingPositions(const Parameters& params);
};

} // namespace Toolpath
} // namespace IntuiCAM 