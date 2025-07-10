#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>
#include <vector>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief External roughing operation for material removal on outside of parts
 * 
 * Implements the ExternalRoughingToolpath stub from the pipeline.
 * Used for roughing external features like turned profiles.
 */
class ExternalRoughingOperation : public Operation {
public:
    struct Parameters {
        // Geometry parameters
        double startDiameter = 50.0;        // mm - starting external diameter (raw material)
        double endDiameter = 20.0;          // mm - final external diameter (part)
        double startZ = 0.0;                // mm - Z position to start
        double endZ = -40.0;                // mm - Z position to end
        
        // Cutting strategy
        double depthOfCut = 2.0;            // mm - axial depth per pass
        double stepover = 1.5;              // mm - radial stepover
        double stockAllowance = 0.5;        // mm - material left for finishing
        
        // Process parameters
        double feedRate = 120.0;            // mm/min - roughing feed rate
        double spindleSpeed = 800.0;        // RPM - spindle speed
        double safetyHeight = 5.0;          // mm - safe height above part
        
        // Strategy options
        bool useProfileFollowing = true;    // follow part profile instead of simple cylinder
        bool enableChipBreaking = true;     // enable chip breaking retracts
        double chipBreakDistance = 0.5;     // mm - retract distance for chip breaking
        bool reversePass = false;           // reverse direction for alternate passes
    };
    
private:
    Parameters params_;
    
public:
    ExternalRoughingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
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
    std::unique_ptr<Toolpath> generateProfileFollowingRoughing(const LatheProfile::Profile2D& profile);
    void generateProfileFollowingPass(Toolpath* toolpath, const LatheProfile::Profile2D& profile, double targetRadius, bool reverse);
    void addRoughingPass(Toolpath* toolpath, double currentZ, double currentDiameter, bool reverse = false);
};

} // namespace Toolpath
} // namespace IntuiCAM 