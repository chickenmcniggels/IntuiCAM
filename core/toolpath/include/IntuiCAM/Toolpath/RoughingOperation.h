#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>
#include <vector>

// OpenCASCADE headers
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

namespace IntuiCAM {
namespace Toolpath {

// Roughing operation for material removal
class RoughingOperation : public Operation {
public:
    struct Parameters {
        double startDiameter = 50.0;    // mm
        double endDiameter = 20.0;      // mm
        double startZ = 0.0;            // mm
        double endZ = -50.0;            // mm
        double depthOfCut = 2.0;        // mm per pass
        double stockAllowance = 0.5;    // mm for finishing
    };
    
private:
    Parameters params_;
    
    // Helper methods for profile-based roughing
    std::vector<Geometry::Point2D> extractProfile(const Geometry::Part& part);
    std::vector<Geometry::Point2D> generateSimpleProfile(const Geometry::BoundingBox& bbox);
    double findMaxRadiusFromSection(const TopoDS_Shape& sectionShape);
    double getProfileRadiusAtZ(const std::vector<Geometry::Point2D>& profile, double z);
    std::unique_ptr<Toolpath> generateBasicRoughing();
    std::vector<Geometry::Point3D> simplifyPath(const std::vector<Geometry::Point3D>& points, double tolerance);
    
public:
    RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method for use by ContouringOperation
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM 