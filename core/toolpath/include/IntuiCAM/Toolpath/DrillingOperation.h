#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <vector>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Drilling operation for creating holes
 * 
 * Supports various drilling strategies including pilot holes, peck drilling,
 * and deep hole drilling with chip breaking.
 */
class DrillingOperation : public Operation {
public:
    struct Parameters {
        double holeDiameter = 6.0;      // mm - diameter of hole to drill
        double holeDepth = 20.0;        // mm - depth of hole
        double peckDepth = 5.0;         // mm - depth per peck for deep holes
        double retractHeight = 2.0;     // mm - retract height for chip clearing
        double dwellTime = 0.5;         // seconds - dwell at bottom of hole
        bool usePeckDrilling = true;    // enable peck drilling for deep holes
        bool useChipBreaking = true;    // enable chip breaking retracts
        double feedRate = 100.0;        // mm/min - drilling feed rate
        double spindleSpeed = 1200.0;   // RPM - spindle speed
        double safetyHeight = 5.0;      // mm - safe height above part
        double startZ = 0.0;            // mm - Z position of hole start
    };
    
private:
    Parameters params_;
    
public:
    DrillingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Helper methods for different drilling strategies
    std::unique_ptr<Toolpath> generateSimpleDrilling();
    std::unique_ptr<Toolpath> generatePeckDrilling();
    std::unique_ptr<Toolpath> generateDeepHoleDrilling();
};

} // namespace Toolpath
} // namespace IntuiCAM 