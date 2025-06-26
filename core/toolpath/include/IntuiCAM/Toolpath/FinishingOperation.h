#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Finishing operation for final surface quality
class FinishingOperation : public Operation {
public:
    struct Parameters {
        double targetDiameter = 20.0;   // mm
        double startZ = 0.0;            // mm
        double endZ = -50.0;            // mm
        double surfaceSpeed = 150.0;    // m/min
        double feedRate = 0.05;         // mm/rev
    };
    
private:
    Parameters params_;
    
public:
    FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method for use by ContouringOperation
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 