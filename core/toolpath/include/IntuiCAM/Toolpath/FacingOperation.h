#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Facing operation for lathe - establishes reference surface at Z-max
 * 
 * Implements the FacingToolpath stub from the pipeline. Always performed first
 * to establish a clean reference surface for subsequent operations.
 */
class FacingOperation : public Operation {
public:
    struct Parameters {
        // Position and geometry
        double startPosition = 0.0;         // mm - Z position to start facing
        double endPosition = -2.0;          // mm - Z position to end facing 
        double startRadius = 25.0;          // mm - radius to start facing from
        double endRadius = 0.0;             // mm - radius to face to (center)
        
        // Cutting strategy
        double depthOfCut = 1.0;            // mm - depth per facing pass
        double stepover = 0.5;              // mm - radial stepover
        double stockAllowance = 0.2;        // mm - material left for finishing
        
        // Process parameters
        double feedRate = 100.0;            // mm/min - facing feed rate
        double spindleSpeed = 800.0;        // RPM - spindle speed
        double safetyHeight = 5.0;          // mm - safe height above part
        
        // Strategy options
        bool roughingOnly = false;          // true for roughing pass only
        bool enableFinishingPass = true;    // enable final finishing pass
        double finishingFeedRate = 50.0;    // mm/min - finishing pass feed rate
    };
    
private:
    Parameters params_;
    
public:
    FacingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method for use by ContouringOperation
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Helper methods for facing strategy
    std::unique_ptr<Toolpath> generateMultiPassFacing();
    std::unique_ptr<Toolpath> generateSinglePassFacing();
    void addFacingPass(Toolpath* toolpath, double zPosition, double feedRate);
};

} // namespace Toolpath
} // namespace IntuiCAM 