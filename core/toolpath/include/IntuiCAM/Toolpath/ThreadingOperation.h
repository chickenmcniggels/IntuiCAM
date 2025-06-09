#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Threading operation for creating threads
class ThreadingOperation : public Operation {
public:
    struct Parameters {
        double majorDiameter = 20.0;    // mm
        double pitch = 1.5;             // mm
        double threadLength = 30.0;     // mm
        double startZ = 0.0;            // mm
        int numberOfPasses = 5;
        bool isMetric = true;
    };
    
private:
    Parameters params_;
    
public:
    ThreadingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 