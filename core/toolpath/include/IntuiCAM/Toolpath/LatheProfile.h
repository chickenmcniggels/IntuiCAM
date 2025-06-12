#pragma once

#include <vector>
#include <IntuiCAM/Geometry/Types.h>

// OCCT forward declarations
class TopoDS_Shape;

namespace IntuiCAM {
namespace Toolpath {

// Utility class that extracts the (radius, Z) profile of a revolved part. The returned
// set of points forms a continuous curve which, when revolved around the Z-axis, recreates
// the part's outer surface.
//
// NOTE: The implementation lives in LatheProfile.cpp to avoid OCCT headers leaking into
// public interfaces.
class LatheProfile {
public:
    using Profile2D = std::vector<IntuiCAM::Geometry::Point2D>; // (r,z) pairs sorted by z

    // Extracts the profile by performing a series of planar section cuts perpendicular
    // to the turning axis (Z).  If extraction fails the function returns an empty vector.
    //
    //  part          – geometric representation of the finished part
    //  numSections   – number of sampling planes along Z (higher = more fidelity)
    //  extraMargin   – additional radius to search for intersections (safety)
    static Profile2D extract(const IntuiCAM::Geometry::Part& part,
                             int numSections = 100,
                             double extraMargin = 1.0);
};

} // namespace Toolpath
} // namespace IntuiCAM 