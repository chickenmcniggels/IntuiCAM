#pragma once

#include <memory>
#include <vector>
#include <string>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <gp_Pnt.hxx>

namespace IntuiCAM {
namespace Geometry {
    class Part;
}
namespace Toolpath {

class Tool;
class Toolpath;

/**
 * @brief Threading operation for creating internal and external threads
 * 
 * Supports multiple thread forms, metric and imperial threading,
 * multi-pass threading with variable depth, and thread feature detection
 * from 2D profiles.
 */
class ThreadingOperation {
public:
    /**
     * @brief Thread form types
     */
    enum class ThreadForm {
        Metric,         ///< ISO metric thread (60 degree)
        UNC,           ///< Unified National Coarse
        UNF,           ///< Unified National Fine
        BSW,           ///< British Standard Whitworth
        ACME,          ///< ACME thread (29 degree)
        Trapezoidal,   ///< Trapezoidal thread (30 degree)
        Custom         ///< User-defined thread form
    };
    
    /**
     * @brief Thread type
     */
    enum class ThreadType {
        External,      ///< External thread (on shaft)
        Internal       ///< Internal thread (in hole)
    };
    
    /**
     * @brief Thread cutting method
     */
    enum class CuttingMethod {
        SinglePoint,   ///< Single point threading tool
        MultiPoint,    ///< Multiple point threading tool
        ChaseThreading ///< Chasing with existing thread
    };
    
    /**
     * @brief Parameters for threading operation
     */
    struct Parameters {
        // Thread specifications
        ThreadForm threadForm;          ///< Thread form type
        ThreadType threadType;          ///< External or internal
        CuttingMethod cuttingMethod;    ///< Cutting method
        
        double majorDiameter;           ///< Major diameter (mm)
        double pitch;                   ///< Thread pitch (mm)
        double threadLength;            ///< Length of threaded section (mm)
        double startZ;                  ///< Start position along Z-axis (mm)
        double endZ;                    ///< End position along Z-axis (mm)
        
        // Thread profile parameters
        double threadAngle;             ///< Thread angle (degrees, typically 60)
        double threadDepth;             ///< Full thread depth (mm)
        double minorDiameter;           ///< Minor diameter (mm)
        double pitchDiameter;           ///< Pitch diameter (mm)
        
        // Cutting parameters
        int numberOfPasses;             ///< Number of threading passes
        double firstPassDepth;          ///< Depth of first pass (mm)
        double finalPassDepth;          ///< Depth of final pass (mm)
        double springPassCount;         ///< Number of spring passes at full depth
        
        // Threading strategy
        bool constantDepthPasses;       ///< Use constant depth per pass
        bool variableDepthPasses;       ///< Use decreasing depth per pass
        double degression;              ///< Degression factor for variable depth
        
        // Feed and speed
        double feedRate;                ///< Feed rate (mm/min)
        double spindleSpeed;            ///< Spindle speed (RPM)
        double leadInDistance;          ///< Lead-in distance (mm)
        double leadOutDistance;         ///< Lead-out distance (mm)
        
        // Safety and clearance
        double safetyHeight;            ///< Safe height for rapid moves (mm)
        double clearanceDistance;       ///< Clearance from thread surface (mm)
        double retractDistance;         ///< Retract distance between passes (mm)
        
        // Quality settings
        double threadTolerance;         ///< Threading tolerance (mm)
        bool chamferThreadStart;        ///< Add chamfer at thread start
        bool chamferThreadEnd;          ///< Add chamfer at thread end
        double chamferLength;           ///< Chamfer length (mm)
        
        // Default constructor with metric M20x1.5 thread
        Parameters() :
            threadForm(ThreadForm::Metric),
            threadType(ThreadType::External),
            cuttingMethod(CuttingMethod::SinglePoint),
            majorDiameter(20.0),
            pitch(1.5),
            threadLength(30.0),
            startZ(0.0),
            endZ(-30.0),
            threadAngle(60.0),
            threadDepth(1.299), // For M20x1.5
            minorDiameter(18.376),
            pitchDiameter(19.188),
            numberOfPasses(6),
            firstPassDepth(0.4),
            finalPassDepth(0.1),
            springPassCount(2),
            constantDepthPasses(false),
            variableDepthPasses(true),
            degression(0.8),
            feedRate(150.0),
            spindleSpeed(300.0),
            leadInDistance(5.0),
            leadOutDistance(5.0),
            safetyHeight(5.0),
            clearanceDistance(1.0),
            retractDistance(2.0),
            threadTolerance(0.02),
            chamferThreadStart(true),
            chamferThreadEnd(true),
            chamferLength(0.5) {}
    };
    
    /**
     * @brief Thread feature detected in profile
     */
    struct ThreadFeature {
        double startZ;              ///< Start position of thread
        double endZ;                ///< End position of thread
        double nominalDiameter;     ///< Detected diameter
        double estimatedPitch;      ///< Estimated pitch from profile
        ThreadType type;            ///< External or internal
        bool isComplete;            ///< Whether thread feature is complete
        double confidence;          ///< Detection confidence (0-1)
    };
    
    /**
     * @brief Result of threading operation generation
     */
    struct Result {
        bool success;
        std::string errorMessage;
        
        // Generated toolpaths
        std::unique_ptr<Toolpath> threadingToolpath;
        std::unique_ptr<Toolpath> chamferToolpath;  ///< Optional chamfer toolpath
        
        // Threading information
        Parameters usedParameters;      ///< Final parameters used
        std::vector<ThreadFeature> detectedThreads; ///< Detected thread features
        
        // Threading statistics
        double estimatedTime;           ///< Total threading time (minutes)
        int totalPasses;               ///< Total number of threading passes
        double actualThreadDepth;      ///< Actual achieved thread depth
        double materialRemoved;        ///< Material volume removed (mm³)
        
        Result() : success(false), estimatedTime(0.0), totalPasses(0), 
                  actualThreadDepth(0.0), materialRemoved(0.0) {}
    };

    /**
     * @brief Constructor
     */
    ThreadingOperation() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ThreadingOperation() = default;

    /**
     * @brief Generate threading toolpaths
     * @param part The part geometry to thread
     * @param tool The threading tool to use
     * @param params Threading parameters
     * @return Generation result with toolpaths and statistics
     */
    Result generateToolpaths(const IntuiCAM::Geometry::Part& part,
                           std::shared_ptr<Tool> tool,
                           const Parameters& params);

    /**
     * @brief Detect thread features from 2D profile
     * @param profile Extracted 2D profile
     * @param params Threading parameters for detection criteria
     * @return Detected thread features
     */
    static std::vector<ThreadFeature> detectThreadFeatures(
        const LatheProfile::Profile2D& profile,
        const Parameters& params);

    /**
     * @brief Calculate thread parameters from standard designation
     * @param threadDesignation Standard thread designation (e.g., "M20x1.5", "1/4-20")
     * @return Calculated thread parameters
     */
    static Parameters calculateThreadParameters(const std::string& threadDesignation);

    /**
     * @brief Validate threading parameters
     * @param params Parameters to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateParameters(const Parameters& params);

    /**
     * @brief Get default parameters for specific thread forms
     * @param threadForm Type of thread form
     * @param diameter Major diameter for the thread
     * @param materialType Material being threaded
     * @return Recommended default parameters
     */
    static Parameters getDefaultParameters(ThreadForm threadForm = ThreadForm::Metric,
                                         double diameter = 20.0,
                                         const std::string& materialType = "steel");

private:
    /**
     * @brief Generate single-point threading toolpath
     */
    std::unique_ptr<Toolpath> generateSinglePointThreading(const Parameters& params,
                                                          std::shared_ptr<Tool> tool);

    /**
     * @brief Generate multi-point threading toolpath
     */
    std::unique_ptr<Toolpath> generateMultiPointThreading(const Parameters& params,
                                                         std::shared_ptr<Tool> tool);

    /**
     * @brief Generate chamfer toolpath for thread ends
     */
    std::unique_ptr<Toolpath> generateChamferToolpath(const Parameters& params,
                                                     std::shared_ptr<Tool> tool);

    /**
     * @brief Calculate threading pass depths
     */
    std::vector<double> calculatePassDepths(const Parameters& params);

    /**
     * @brief Calculate thread profile coordinates
     */
    std::vector<gp_Pnt> calculateThreadProfile(const Parameters& params);

    /**
     * @brief Estimate threading time based on parameters
     */
    double estimateThreadingTime(const Parameters& params, std::shared_ptr<Tool> tool);

    /**
     * @brief Calculate material removal for threading
     */
    double calculateMaterialRemoval(const Parameters& params);

    /**
     * @brief Validate tool compatibility with threading operation
     */
    bool validateToolCompatibility(std::shared_ptr<Tool> tool, const Parameters& params);
};

} // namespace Toolpath
} // namespace IntuiCAM 