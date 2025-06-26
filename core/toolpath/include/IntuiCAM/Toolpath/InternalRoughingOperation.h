#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>
#include <vector>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Internal roughing operation for material removal inside parts
 * 
 * Implements the InternalRoughingToolpath stub from the pipeline.
 * Used for roughing internal features like bores and holes.
 */
class InternalRoughingOperation : public Operation {
public:
    struct Parameters {
        // Geometry parameters
        double startDiameter = 10.0;        // mm - starting internal diameter
        double endDiameter = 20.0;          // mm - final internal diameter
        double startZ = 0.0;                // mm - Z position to start
        double endZ = -30.0;                // mm - Z position to end
        
        // Cutting strategy
        double depthOfCut = 1.0;            // mm - axial depth per pass
        double stepover = 1.0;              // mm - radial stepover
        double stockAllowance = 0.5;        // mm - material left for finishing
        
        // Process parameters
        double feedRate = 80.0;             // mm/min - roughing feed rate
        double spindleSpeed = 600.0;        // RPM - spindle speed
        double safetyHeight = 5.0;          // mm - safe height above part
        
        // Strategy options
        bool useClimbMilling = false;       // use climb milling if applicable
        bool enableChipBreaking = true;     // enable chip breaking retracts
        double chipBreakDistance = 0.5;     // mm - retract distance for chip breaking
    };
    
private:
    Parameters params_;
    
public:
    InternalRoughingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Helper methods for roughing strategy
    std::unique_ptr<Toolpath> generateAxialRoughing();
    std::unique_ptr<Toolpath> generateRadialRoughing();
    void addRoughingPass(Toolpath* toolpath, double currentZ, double currentDiameter);
};

} // namespace Toolpath
} // namespace IntuiCAM 