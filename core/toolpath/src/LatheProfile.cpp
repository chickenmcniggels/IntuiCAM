#include <IntuiCAM/Toolpath/LatheProfile.h>

// OpenCASCADE headers – included only in the .cpp to avoid polluting the public API
#include <TopoDS_Shape.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax1.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

using Geometry::Point2D;
using Geometry::Point3D;
using Geometry::BoundingBox;
using Geometry::OCCTPart;

namespace {
//------------------------------------------------------------------------------
// Helper: find maximum radial distance from Z-axis in an OCCT intersection shape
//------------------------------------------------------------------------------
double findMaxRadius(const TopoDS_Shape& sectionShape)
{
    double maxRadius = 0.0;

    for (TopExp_Explorer exp(sectionShape, TopAbs_EDGE); exp.More(); exp.Next()) {
        const TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        BRepAdaptor_Curve curve(edge);

        // Determine sampling density based on curve type
        int samples = 10;
        GeomAbs_CurveType ct = curve.GetType();
        if (ct == GeomAbs_Circle || ct == GeomAbs_Ellipse) {
            samples = 36; // one every 10°
        } else if (ct == GeomAbs_BSplineCurve || ct == GeomAbs_BezierCurve) {
            samples = 20;
        }

        for (int i = 0; i <= samples; ++i) {
            const double u = curve.FirstParameter() + (curve.LastParameter() - curve.FirstParameter()) * (static_cast<double>(i) / samples);
            const gp_Pnt p = curve.Value(u);
            const double r = std::sqrt(p.X() * p.X() + p.Y() * p.Y());
            maxRadius = std::max(maxRadius, r);
        }
    }

    return maxRadius;
}

//------------------------------------------------------------------------------
// Fallback: simple cylindrical profile derived from bounding box
//------------------------------------------------------------------------------
std::vector<Point2D> makeCylindricalProfile(const BoundingBox& bbox, int numPoints = 30)
{
    std::vector<Point2D> profile;
    if (numPoints < 2) numPoints = 2;

    const double zStart = bbox.min.z;
    const double zEnd   = bbox.max.z;
    const double zStep  = (zEnd - zStart) / (numPoints - 1);

    const double maxRadius = std::max({ std::abs(bbox.min.x), std::abs(bbox.max.x),
                                        std::abs(bbox.min.y), std::abs(bbox.max.y) });

    for (int i = 0; i < numPoints; ++i) {
        const double z = zStart + i * zStep;
        profile.emplace_back(maxRadius, z);
    }
    return profile;
}
} // anonymous namespace

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
LatheProfile::Profile2D LatheProfile::extract(const Geometry::Part& part, int numSections, double extraMargin)
{
    Profile2D profile;

    if (numSections < 2) numSections = 2;

    // Bounding box is used both for fallback and to determine sampling region
    const BoundingBox bbox = part.getBoundingBox();

    // Attempt OpenCASCADE-based extraction if the part is an OCCTPart
    const auto* occPart = dynamic_cast<const OCCTPart*>(&part);
    if (occPart != nullptr) {
        try {
            const TopoDS_Shape& shape = occPart->getOCCTShape();
            if (!shape.IsNull()) {
                // Axis of revolution is assumed to be global Z (0,0,1)
                gp_Ax1 axis(gp_Pnt(0,0,0), gp_Dir(0,0,1));

                const double zStart = bbox.min.z;
                const double zEnd   = bbox.max.z;
                const double zStep  = (zEnd - zStart) / (numSections - 1);

                // Pre-compute search radius with a safety margin
                const double maxRadiusBBox = std::max({ std::abs(bbox.min.x), std::abs(bbox.max.x),
                                                         std::abs(bbox.min.y), std::abs(bbox.max.y) });
                const double searchRadius  = maxRadiusBBox * (1.0 + extraMargin);
                (void)searchRadius; // Currently unused but left for future intersection optimisations

                for (int i = 0; i < numSections; ++i) {
                    const double z = zStart + i * zStep;

                    gp_Pln sectionPlane(gp_Pnt(0,0,z), axis.Direction());
                    BRepAlgoAPI_Section section(shape, sectionPlane, Standard_False);
                    section.Build();
                    if (!section.IsDone()) continue;

                    const TopoDS_Shape sectionShape = section.Shape();
                    const double radius = findMaxRadius(sectionShape);
                    if (radius > 0.0) {
                        profile.emplace_back(radius, z);
                    }
                }
            }
        } catch (...) {
            // Suppress all OCC exceptions; we'll fall back to simple profile below
        }
    }

    // Fallback or insufficient points → cylindrical proxy
    if (profile.size() < 5) {
        profile = makeCylindricalProfile(bbox);
    }

    // Ensure monotonic Z ordering and remove noisy duplicates
    std::sort(profile.begin(), profile.end(), [](const Point2D& a, const Point2D& b){ return a.z < b.z; });

    // Simple smoothing: collapse consecutive points whose radius differs less than tol
    constexpr double tol = 0.05; // mm
    Profile2D cleaned;
    for (const auto& pt : profile) {
        if (cleaned.empty() || std::abs(pt.x - cleaned.back().x) > tol) {
            cleaned.push_back(pt);
        }
    }

    return cleaned;
}

} // namespace Toolpath
} // namespace IntuiCAM 