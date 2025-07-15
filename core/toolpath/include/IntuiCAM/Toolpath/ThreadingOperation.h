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
class ThreadingOperation : public Operation {
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
        double threadDepth;             ///< Thread depth (mm)
        double threadLength;            ///< Length of threaded section (mm)
        double startZ;                  ///< Z position to start threading (mm)
        double endZ;                    ///< Z position to end threading (mm)
        
        // Threading strategy
        int numberOfPasses;             ///< Number of threading passes
        bool constantDepthPasses;       ///< Use constant depth per pass
        bool variableDepthPasses;       ///< Use variable depth (spring cuts)
        double degression;              ///< Degression factor for variable depth
        
        // Cutting parameters
        double feedRate;                ///< Threading feed rate (mm/min)
        double spindleSpeed;            ///< Spindle speed (RPM)
        double leadInDistance;          ///< Lead-in distance (mm)
        double leadOutDistance;         ///< Lead-out distance (mm)
        double safetyHeight;            ///< Safe height above part (mm)
        double clearanceDistance;       ///< Clearance from part (mm)
        double retractDistance;         ///< Retract distance for passes (mm)
        
        // Quality and finishing
        double threadTolerance;         ///< Thread tolerance (mm)
        bool chamferThreadStart;        ///< Chamfer thread start
        bool chamferThreadEnd;          ///< Chamfer thread end
        double chamferLength;           ///< Chamfer length (mm)
        
        // Advanced options
        bool useConstantSurfaceSpeed;   ///< Use constant surface speed
        double maxSpindleSpeed;         ///< Maximum spindle speed limit (RPM)
        bool enableCoolant;             ///< Enable coolant during threading
        bool enableChipBreaking;        ///< Enable chip breaking
        double chipBreakDistance;       ///< Chip break retract distance (mm)
        
        Parameters() : 
            threadForm(ThreadForm::Metric),
            threadType(ThreadType::External),
            cuttingMethod(CuttingMethod::SinglePoint),
            majorDiameter(10.0),
            pitch(1.5),
            threadDepth(0.9),
            threadLength(15.0),
            startZ(0.0),
            endZ(-15.0),
            numberOfPasses(3),
            constantDepthPasses(false),
            variableDepthPasses(true),
            degression(0.8),
            feedRate(60.0),
            spindleSpeed(300.0),
            leadInDistance(5.0),
            leadOutDistance(5.0),
            safetyHeight(5.0),
            clearanceDistance(3.0),
            retractDistance(2.0),
            threadTolerance(0.02),
            chamferThreadStart(true),
            chamferThreadEnd(true),
            chamferLength(0.5),
            useConstantSurfaceSpeed(false),
            maxSpindleSpeed(1500.0),
            enableCoolant(true),
            enableChipBreaking(false),
            chipBreakDistance(1.0) {}
    };
    
    /**
     * @brief Thread feature detected from profile
     */
    struct ThreadFeature {
        gp_Pnt position;                ///< Thread position
        ThreadType type;                ///< Thread type
        double diameter;                ///< Thread diameter
        double pitch;                   ///< Detected pitch
        double length;                  ///< Thread length
        bool isMetric;                  ///< Is metric thread
        std::string designation;        ///< Standard designation (e.g., "M20x1.5")
        double confidence;              ///< Detection confidence (0-1)
    };

    /**
     * @brief Threading operation result
     */
    struct Result {
        bool success;                   ///< Operation successful
        std::string errorMessage;       ///< Error message if failed
        std::vector<std::string> warnings; ///< Warning messages
        
        // Generated toolpaths
        std::unique_ptr<Toolpath> threadingToolpath;    ///< Main threading toolpath
        std::unique_ptr<Toolpath> chamferToolpath;      ///< Thread chamfer toolpath
        
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

private:
    Parameters params_;

public:
    /**
     * @brief Constructor - inherits from Operation base class
     * @param name Operation name
     * @param tool Threading tool to use
     */
    ThreadingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ThreadingOperation() = default;

    /**
     * @brief Set threading parameters
     * @param params Threading parameters
     */
    void setParameters(const Parameters& params) { params_ = params; }
    
    /**
     * @brief Get threading parameters
     * @return Current parameters
     */
    const Parameters& getParameters() const { return params_; }

    /**
     * @brief Standard Operation interface - generates main threading toolpath
     * @param part The part geometry to thread
     * @return Generated threading toolpath
     */
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    
    /**
     * @brief Validate operation parameters and tool
     * @return True if valid
     */
    bool validate() const override;

    /**
     * @brief Advanced interface - Generate threading toolpaths with detailed control
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
    static Parameters getDefaultParameters(ThreadForm threadForm,
                                         double diameter,
                                         const std::string& materialType = "steel");

private:
    /**
     * @brief Generate threading passes for the specified parameters
     */
    std::vector<std::unique_ptr<Toolpath>> generateThreadingPasses(
        const IntuiCAM::Geometry::Part& part,
        std::shared_ptr<Tool> tool,
        const Parameters& params);

    /**
     * @brief Generate chamfer toolpaths for thread start/end
     */
    std::unique_ptr<Toolpath> generateThreadChamfer(
        const Parameters& params,
        std::shared_ptr<Tool> tool,
        bool isStart);

    /**
     * @brief Calculate threading depth progression for multiple passes
     */
    std::vector<double> calculateDepthProgression(const Parameters& params);

    /**
     * @brief Generate single threading pass
     */
    std::unique_ptr<Toolpath> generateSinglePass(
        const Parameters& params,
        std::shared_ptr<Tool> tool,
        double depth,
        int passNumber);

    /**
     * @brief Calculate thread geometry from parameters
     */
    void calculateThreadGeometry(const Parameters& params,
                               double& minorDiameter,
                               double& pitchDiameter,
                               double& threadAngle);

    /**
     * @brief Validate thread parameters for manufacturing constraints
     */
    std::string validateManufacturingConstraints(const Parameters& params,
                                                std::shared_ptr<Tool> tool);
};

} // namespace Toolpath
} // namespace IntuiCAM 