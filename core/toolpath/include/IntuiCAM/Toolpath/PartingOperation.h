#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Parting operation for cutting off parts
class PartingOperation : public Operation {
public:
    struct Parameters {
        double partingDiameter = 20.0;  // mm
        double partingZ = -50.0;        // mm
        double centerHoleDiameter = 3.0; // mm (0 for solid)
        double partingWidth = 3.0;      // mm width of cut
        double feedRate = 0.02;         // mm/rev
        double retractDistance = 2.0;   // mm
        double partingWidth = 3.0;      // mm
    };
    
private:
    Parameters params_;
    
public:
    PartingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 