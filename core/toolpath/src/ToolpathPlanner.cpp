#include <IntuiCAM/Toolpath/ToolpathPlanner.h>

#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

using Geometry::BoundingBox;
using Geometry::Point2D;

std::vector<std::unique_ptr<IntuiCAM::Toolpath::Toolpath>>
ToolpathPlanner::generateSequence(const Geometry::Part& rawMaterial,
                                  const Geometry::Part& finishedPart,
                                  const Parameters& params,
                                  const Geometry::Matrix4x4& worldTransform)
{
    std::vector<std::unique_ptr<IntuiCAM::Toolpath::Toolpath>> sequence;

    // ---------------------------------------------------------------------
    // Compute basic stock / part measures
    // ---------------------------------------------------------------------
    const BoundingBox rawBBox  = rawMaterial.getBoundingBox();
    const BoundingBox partBBox = finishedPart.getBoundingBox();

    auto maxAbs = [](double a, double b){ return std::max(std::abs(a), std::abs(b)); };
    const double rawRadius  = maxAbs(maxAbs(rawBBox.min.x, rawBBox.max.x), maxAbs(rawBBox.min.y, rawBBox.max.y));
    const double partRadius = maxAbs(maxAbs(partBBox.min.x, partBBox.max.x), maxAbs(partBBox.min.y, partBBox.max.y));

    const double rawDiameter  = rawRadius * 2.0;
    const double partDiameter = partRadius * 2.0;

    const double rawFrontZ    = rawBBox.max.z; // Assume spindle face at raw max Z
    const double rawBackZ     = rawBBox.min.z;
    const double partFrontZ   = partBBox.max.z;
    const double partBackZ    = partBBox.min.z;

    // ---------------------------------------------------------------------
    // Create a generic turning tool (TODO: choose per-operation tools)
    // ---------------------------------------------------------------------
    auto turningTool = std::make_shared<Tool>(Tool::Type::Turning, "GenericTurningTool");

    // ---------------------------------------------------------------------
    // 1. Facing Operation
    // ---------------------------------------------------------------------
    FacingOperation faceOp("Facing", turningTool);
    FacingOperation::Parameters faceParams;
    faceParams.startDiameter   = rawDiameter;
    faceParams.endDiameter     = 0.0;
    faceParams.stepover        = params.facing_stepover;
    faceParams.stockAllowance  = params.facing_allowance;
    faceOp.setParameters(faceParams);

    sequence.emplace_back(faceOp.generateToolpath(finishedPart));

    // ---------------------------------------------------------------------
    // 2. Roughing Operation (linear clearing)
    // ---------------------------------------------------------------------
    // Determine minimum diameter along part profile to avoid over-cutting
    double minPartRadius = partRadius;
    const auto profile = LatheProfile::extract(finishedPart, 150);
    for (const auto& p : profile) {
        minPartRadius = std::min(minPartRadius, p.x);
    }

    RoughingOperation roughOp("Roughing", turningTool);
    RoughingOperation::Parameters roughParams;
    roughParams.startDiameter  = rawDiameter;
    roughParams.endDiameter    = std::max(1.0, minPartRadius * 2.0); // avoid zero
    roughParams.startZ         = rawFrontZ;
    roughParams.endZ           = partBackZ - params.finishing_allowance; // leave allowance
    roughParams.depthOfCut     = params.roughing_depth_of_cut;
    roughParams.stockAllowance = params.finishing_allowance;
    roughOp.setParameters(roughParams);

    sequence.emplace_back(roughOp.generateToolpath(finishedPart));

    // ---------------------------------------------------------------------
    // 3. Finishing Operation (profile following)
    // ---------------------------------------------------------------------
    FinishingOperation finishOp("Finishing", turningTool);
    FinishingOperation::Parameters finishParams;
    finishParams.targetDiameter = partDiameter; // Not used in new algo but kept for compatibility
    finishParams.startZ         = partFrontZ;
    finishParams.endZ           = partBackZ;
    finishParams.feedRate       = turningTool->getCuttingParameters().feedRate * 0.5; // slower feed
    finishParams.surfaceSpeed   = 150.0;
    finishOp.setParameters(finishParams);

    sequence.emplace_back(finishOp.generateToolpath(finishedPart));

    // ---------------------------------------------------------------------
    // 4. Parting Operation
    // ---------------------------------------------------------------------
    PartingOperation partOp("Parting", turningTool);
    PartingOperation::Parameters partParams;
    partParams.partingDiameter   = rawDiameter;
    partParams.partingZ          = partBackZ - params.parting_allowance;
    partParams.centerHoleDiameter = 0.0; // solid bar
    partParams.feedRate          = turningTool->getCuttingParameters().feedRate * 0.8;
    partParams.retractDistance   = 2.0;
    partOp.setParameters(partParams);

    sequence.emplace_back(partOp.generateToolpath(finishedPart));

    // ---------------------------------------------------------------------
    // Apply world transform so that toolpaths align with viewer coordinates
    // ---------------------------------------------------------------------
    for (auto& tp : sequence) {
        tp->applyTransform(worldTransform);
    }

    return sequence;
}

} // namespace Toolpath
} // namespace IntuiCAM 