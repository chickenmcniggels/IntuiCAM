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
    // Structured profile containing both external and internal profiles
    struct Profile2D {
        struct ProfileSection {
            std::vector<IntuiCAM::Geometry::Point2D> points;  // (r,z) pairs sorted by z
        };
        
        ProfileSection externalProfile;  // External surface profile
        ProfileSection internalProfile;  // Internal features profile (if any)
        
        // Helper methods
        bool isEmpty() const {
            return externalProfile.points.empty() && internalProfile.points.empty();
        }
        
        size_t getTotalPointCount() const {
            return externalProfile.points.size() + internalProfile.points.size();
        }
        
        // Convenience methods for backward compatibility (primarily uses external profile)
        bool empty() const { 
            return externalProfile.points.empty(); 
        }
        
        size_t size() const { 
            return externalProfile.points.size(); 
        }
        
        const IntuiCAM::Geometry::Point2D& front() const { 
            return externalProfile.points.front(); 
        }
        
        const IntuiCAM::Geometry::Point2D& back() const { 
            return externalProfile.points.back(); 
        }
        
        const IntuiCAM::Geometry::Point2D& operator[](size_t index) const { 
            return externalProfile.points[index]; 
        }
        
        // Iterator support for range-based loops
        auto begin() const { return externalProfile.points.begin(); }
        auto end() const { return externalProfile.points.end(); }
    };

    // Legacy typedef for backward compatibility
    using SimpleProfile2D = std::vector<IntuiCAM::Geometry::Point2D>; // (r,z) pairs sorted by z

    // Extracts the profile by performing a series of planar section cuts perpendicular
    // to the turning axis (Z).  If extraction fails the function returns an empty vector.
    //
    //  part          – geometric representation of the finished part
    //  numSections   – number of sampling planes along Z (higher = more fidelity)
    //  extraMargin   – additional radius to search for intersections (safety)
    static SimpleProfile2D extract(const IntuiCAM::Geometry::Part& part,
                                  int numSections = 100,
                                  double extraMargin = 1.0);
};

} // namespace Toolpath
} // namespace IntuiCAM 