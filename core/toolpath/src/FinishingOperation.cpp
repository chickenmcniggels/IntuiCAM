#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <cmath>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <algorithm>

namespace IntuiCAM {
namespace Toolpath {

FinishingOperation::FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Finishing, name, tool) {
}

std::unique_ptr<Toolpath> FinishingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);

    // Extract lathe profile (r,z pairs)
    auto profile = LatheProfile::extract(part, 150 /*numSections*/);

    // Fallback to legacy behaviour if extraction failed
    if (profile.size() < 2) {
        double radius = params_.targetDiameter / 2.0;
        toolpath->addRapidMove(Geometry::Point3D(radius + 5.0, 0, params_.startZ + 5.0));
        toolpath->addRapidMove(Geometry::Point3D(radius, 0, params_.startZ + 2.0));
        toolpath->addLinearMove(Geometry::Point3D(radius, 0, params_.startZ), params_.feedRate);
        toolpath->addLinearMove(Geometry::Point3D(radius, 0, params_.endZ), params_.feedRate);
        toolpath->addRapidMove(Geometry::Point3D(radius, 0, params_.endZ - 2.0));
        toolpath->addRapidMove(Geometry::Point3D(radius + 10.0, 0, params_.endZ - 2.0));
        return toolpath;
    }

    // Ensure profile sorted by Z ascending
    std::sort(profile.begin(), profile.end(), [](const Geometry::Point2D& a, const Geometry::Point2D& b){return a.z < b.z;});

    // Determine safe clearance radius
    double maxRadius = 0.0;
    for (const auto& p : profile) maxRadius = std::max(maxRadius, p.x);
    const double clearance = 5.0; // mm radial clearance

    const double safeRadius = maxRadius + clearance;

    // Rapid above first point
    const double firstZ = profile.front().z;
    toolpath->addRapidMove(Geometry::Point3D(safeRadius, 0, firstZ + 5.0));
    toolpath->addRapidMove(Geometry::Point3D(profile.front().x + 1.0 /*approach*/, 0, firstZ + 2.0));

    // Plunge to cutting position
    toolpath->addLinearMove(Geometry::Point3D(profile.front().x, 0, firstZ), params_.feedRate);

    // Follow the profile
    for (size_t i = 1; i < profile.size(); ++i) {
        const auto& pt = profile[i];
        toolpath->addLinearMove(Geometry::Point3D(pt.x, 0, pt.z), params_.feedRate);
    }

    // Retract at the end of profile
    const double lastZ = profile.back().z;
    toolpath->addRapidMove(Geometry::Point3D(profile.back().x + 2.0, 0, lastZ + 2.0));
    toolpath->addRapidMove(Geometry::Point3D(safeRadius, 0, lastZ + 5.0));

    return toolpath;
}

bool FinishingOperation::validate() const {
    return params_.targetDiameter > 0.0 && 
           params_.startZ > params_.endZ && 
           params_.feedRate > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM 