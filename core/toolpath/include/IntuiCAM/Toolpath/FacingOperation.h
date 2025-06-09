#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Facing operation for lathe
class FacingOperation : public Operation {
public:
    struct Parameters {
        double startDiameter = 50.0;    // mm
        double endDiameter = 0.0;       // mm (center)
        double stepover = 0.5;          // mm
        double stockAllowance = 0.2;    // mm
        bool roughingOnly = false;
    };
    
private:
    Parameters params_;
    
public:
    FacingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 