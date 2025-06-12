#pragma once

#include <memory>
#include <vector>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// High-level planner that generates facing → roughing → finishing → parting
// toolpaths in the mandatory sequence specified for turning operations.
class ToolpathPlanner {
public:
    struct Parameters {
        double facing_allowance     = 0.2;  // mm to leave after facing
        double finishing_allowance  = 0.2;  // mm to leave for finish cut after roughing
        double parting_allowance    = 0.5;  // mm extra Z for part-off
        double roughing_depth_of_cut = 2.0; // mm radial DOC per pass
        double facing_stepover       = 0.5; // mm radial stepover for facing
    };

    // Generates a complete toolpath sequence and returns it in the predefined order.
    static std::vector<std::unique_ptr<IntuiCAM::Toolpath::Toolpath>>
    generateSequence(const Geometry::Part& rawMaterial,
                     const Geometry::Part& finishedPart,
                     const Parameters& params = Parameters(),
                     const Geometry::Matrix4x4& worldTransform = Geometry::Matrix4x4::identity());
};

} // namespace Toolpath
} // namespace IntuiCAM 