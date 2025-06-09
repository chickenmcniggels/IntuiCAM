#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Roughing operation for material removal
class RoughingOperation : public Operation {
public:
    struct Parameters {
        double startDiameter = 50.0;    // mm
        double endDiameter = 20.0;      // mm
        double startZ = 0.0;            // mm
        double endZ = -50.0;            // mm
        double depthOfCut = 2.0;        // mm per pass
        double stockAllowance = 0.5;    // mm for finishing
    };
    
private:
    Parameters params_;
    
public:
    RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 