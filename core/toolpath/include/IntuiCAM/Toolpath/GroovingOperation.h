#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Grooving operation for creating grooves
class GroovingOperation : public Operation {
public:
    struct Parameters {
        double grooveDiameter = 20.0;   // mm
        double grooveWidth = 3.0;       // mm
        double grooveDepth = 2.0;       // mm
        double grooveZ = -25.0;         // mm
        double feedRate = 0.02;         // mm/rev
    };
    
private:
    Parameters params_;
    
public:
    GroovingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 