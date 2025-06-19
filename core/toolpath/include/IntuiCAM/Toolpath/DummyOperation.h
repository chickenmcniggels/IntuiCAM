#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Example operation illustrating how to implement custom toolpaths.
 *
 * DummyOperation generates a short two‑move path: a rapid approach and
 * a single linear cut. It is intended purely as a learning aid and can
 * be used as a template when creating your own operations.
 */
class DummyOperation : public Operation {
public:
    struct Parameters {
        Geometry::Point3D startPosition {0, 0, 0};
        Geometry::Point3D endPosition   {10, 0, 0};
        double feedRate {100.0};
    };

private:
    Parameters params_;

public:
    DummyOperation(const std::string& name, std::shared_ptr<Tool> tool);

    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }

    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM
