#include <IntuiCAM/Toolpath/ProfileExtractor.h>

#include <algorithm>
#include <cmath>

#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenCASCADE includes for geometry processing
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Cone.hxx>
#include <gp_Vec2d.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GeomAbs_CurveType.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <Bnd_Box.hxx>

namespace IntuiCAM {
namespace Toolpath {

LatheProfile::Profile2D ProfileExtractor::extractProfile(
    const TopoDS_Shape& partGeometry,
    const ExtractionParameters& params) {
    
    // Validate parameters first
    std::string validationError = validateParameters(params);
    if (!validationError.empty()) {
        // Return empty profile on validation error
        return LatheProfile::Profile2D();
    }
    
    // Analyze geometry characteristics
    GeometryAnalysis analysis = analyzeGeometry(partGeometry, params.turningAxis);
    
    // Extract enhanced profile with feature information
    std::vector<ProfilePoint> enhancedProfile = extractEnhancedProfile(partGeometry, params);
    
    // Convert to simple profile format
    LatheProfile::Profile2D profile = convertToSimpleProfile(enhancedProfile);
    
    // If no profile was extracted, create a simple rectangular profile as fallback
    if (profile.externalProfile.points.empty() && profile.internalProfile.points.empty()) {
        std::cout << "ProfileExtractor: No profile extracted, creating fallback rectangular profile" << std::endl;
        
        // Create a simple external profile based on bounding box
        Bnd_Box bbox;
        BRepBndLib::Add(partGeometry, bbox);
        
        if (!bbox.IsVoid()) {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            // Create a simple rectangular profile
            double maxRadius = std::max(std::abs(ymax), std::abs(ymin));
            double length = xmax - xmin;
            
            if (maxRadius > 0.1 && length > 0.1) { // Reasonable minimum size
                profile.externalProfile.points = {
                    IntuiCAM::Geometry::Point2D{xmin, maxRadius},
                    IntuiCAM::Geometry::Point2D{xmax, maxRadius},
                    IntuiCAM::Geometry::Point2D{xmax, 0.0},
                    IntuiCAM::Geometry::Point2D{xmin, 0.0}
                };
                
                std::cout << "ProfileExtractor: Created fallback profile with radius " << maxRadius 
                          << " and length " << length << std::endl;
            }
        }
    }
    
    return profile;
}

std::vector<ProfileExtractor::ProfilePoint> ProfileExtractor::extractEnhancedProfile(
    const TopoDS_Shape& partGeometry,
    const ExtractionParameters& params) {
    
    std::vector<ProfilePoint> profilePoints;
    
    // Find surfaces of revolution
    std::vector<TopoDS_Face> revolutionFaces = findRevolutionFaces(
        partGeometry, params.turningAxis, params.profileTolerance);
    
    // Process revolution faces
    for (const auto& face : revolutionFaces) {
        auto facePoints = projectFaceToProfile(face, params.turningAxis, params);
        profilePoints.insert(profilePoints.end(), facePoints.begin(), facePoints.end());
    }
    
    // If projectAllFaces is enabled, process all faces
    if (params.projectAllFaces) {
        TopExp_Explorer explorer(partGeometry, TopAbs_FACE);
        for (; explorer.More(); explorer.Next()) {
            TopoDS_Face face = TopoDS::Face(explorer.Current());
            
            // Skip if already processed as revolution face
            bool alreadyProcessed = false;
            for (const auto& revFace : revolutionFaces) {
                if (face.IsSame(revFace)) {
                    alreadyProcessed = true;
                    break;
                }
            }
            
            if (!alreadyProcessed) {
                auto facePoints = projectFaceToProfile(face, params.turningAxis, params);
                profilePoints.insert(profilePoints.end(), facePoints.begin(), facePoints.end());
            }
        }
    }
    
    // Sort points by Z coordinate
    std::sort(profilePoints.begin(), profilePoints.end(),
        [](const ProfilePoint& a, const ProfilePoint& b) {
            return a.position.X() < b.position.X(); // X is Z-axis in profile coordinates
        });
    
    // Remove duplicates and merge nearby points
    if (params.mergeTangentSegments) {
        profilePoints = mergeNearbyPoints(profilePoints, params.mergeTolerance);
    }
    
    // Detect sharp corners if requested
    if (params.autoDetectFeatures) {
        detectAndMarkSharpCorners(profilePoints, params.angleTolerance);
    }
    
    return profilePoints;
}

ProfileExtractor::GeometryAnalysis ProfileExtractor::analyzeGeometry(
    const TopoDS_Shape& partGeometry,
    const gp_Ax1& turningAxis) {
    
    GeometryAnalysis analysis;
    
    // Calculate bounding box to estimate dimensions
    Bnd_Box boundingBox;
    BRepBndLib::Add(partGeometry, boundingBox);
    
    if (!boundingBox.IsVoid()) {
        double xmin, ymin, zmin, xmax, ymax, zmax;
        boundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Estimate max radius and length along turning axis
        gp_Pnt center = turningAxis.Location();
        gp_Dir direction = turningAxis.Direction();
        
        // Project bounds to axis system
        analysis.estimatedLength = std::abs(xmax - xmin); // Simplified
        analysis.estimatedMaxRadius = std::max(
            std::abs(ymax - ymin), std::abs(zmax - zmin)) / 2.0;
    }
    
    // Analyze faces for revolution characteristics
    TopExp_Explorer faceExplorer(partGeometry, TopAbs_FACE);
    int totalFaces = 0;
    int revolutionFaces = 0;
    int flatFaces = 0;
    
    for (; faceExplorer.More(); faceExplorer.Next()) {
        totalFaces++;
        TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
        BRepAdaptor_Surface surface(face);
        
        GeomAbs_SurfaceType surfaceType = surface.GetType();
        
        switch (surfaceType) {
            case GeomAbs_Cylinder:
            case GeomAbs_Cone:
            case GeomAbs_Sphere:
                revolutionFaces++;
                analysis.revolutionFaces.push_back(face);
                break;
                
            case GeomAbs_Plane:
                flatFaces++;
                analysis.endFaces.push_back(face);
                break;
                
            default:
                analysis.featureFaces.push_back(face);
                break;
        }
    }
    
    // Determine geometry classification
    analysis.isRevolutionSolid = (revolutionFaces > 0);
    analysis.hasInternalFeatures = (analysis.featureFaces.size() > 0);
    analysis.hasComplexProfile = (totalFaces > 6); // Heuristic
    
    if (totalFaces <= 6 && revolutionFaces >= totalFaces / 2) {
        analysis.geometryType = "simple";
    } else if (analysis.isRevolutionSolid) {
        analysis.geometryType = "complex";
    } else {
        analysis.geometryType = "invalid";
    }
    
    return analysis;
}

std::string ProfileExtractor::validateParameters(const ExtractionParameters& params) {
    if (params.profileTolerance <= 0.0) {
        return "Profile tolerance must be positive";
    }
    
    if (params.projectionTolerance <= 0.0) {
        return "Projection tolerance must be positive";
    }
    
    if (params.profileSections < 10) {
        return "Profile sections must be at least 10";
    }
    
    if (params.minFeatureSize < 0.0) {
        return "Minimum feature size cannot be negative";
    }
    
    if (params.angleTolerance <= 0.0 || params.angleTolerance > M_PI) {
        return "Angle tolerance must be between 0 and π";
    }
    
    // Validate turning axis - gp_Dir is always unit length, just check if it's valid
    try {
        gp_Dir testDir = params.turningAxis.Direction();
        (void)testDir; // Suppress unused variable warning
    } catch (const Standard_Failure&) {
        return "Invalid turning axis direction";
    }
    
    return ""; // Valid
}

ProfileExtractor::ExtractionParameters ProfileExtractor::getRecommendedParameters(
    const GeometryAnalysis& analysis) {
    
    ExtractionParameters params;
    
    // Adjust based on geometry complexity
    if (analysis.geometryType == "simple") {
        params.profileSections = 50;
        params.profileTolerance = 0.05;
        params.projectAllFaces = false;
    } else if (analysis.geometryType == "complex") {
        params.profileSections = 150;
        params.profileTolerance = 0.01;
        params.projectAllFaces = true;
        params.includeInternalFeatures = true;
    } else {
        // Invalid or unknown geometry
        params.profileSections = 100;
        params.profileTolerance = 0.02;
        params.projectAllFaces = true;
    }
    
    // Adjust for estimated size
    if (analysis.estimatedMaxRadius > 100.0) { // Large part
        params.profileSections *= 2;
        params.profileTolerance /= 2.0;
    } else if (analysis.estimatedMaxRadius < 10.0) { // Small part
        params.profileSections /= 2;
        params.profileTolerance *= 2.0;
    }
    
    return params;
}

std::vector<TopoDS_Face> ProfileExtractor::findRevolutionFaces(
    const TopoDS_Shape& shape,
    const gp_Ax1& axis,
    double tolerance) {
    
    std::vector<TopoDS_Face> revolutionFaces;
    
    TopExp_Explorer explorer(shape, TopAbs_FACE);
    for (; explorer.More(); explorer.Next()) {
        TopoDS_Face face = TopoDS::Face(explorer.Current());
        BRepAdaptor_Surface surface(face);
        
        GeomAbs_SurfaceType surfaceType = surface.GetType();
        
        // Check if surface is a revolution surface aligned with our axis
        bool isRevolutionSurface = false;
        
        switch (surfaceType) {
            case GeomAbs_Cylinder: {
                gp_Cylinder cylinder = surface.Cylinder();
                gp_Ax1 cylinderAxis = cylinder.Axis();
                
                // Check if axes are parallel within tolerance
                double angleDiff = cylinderAxis.Direction().Angle(axis.Direction());
                if (angleDiff < tolerance || std::abs(angleDiff - M_PI) < tolerance) {
                    isRevolutionSurface = true;
                }
                break;
            }
            
            case GeomAbs_Cone: {
                gp_Cone cone = surface.Cone();
                gp_Ax1 coneAxis = cone.Axis();
                
                double angleDiff = coneAxis.Direction().Angle(axis.Direction());
                if (angleDiff < tolerance || std::abs(angleDiff - M_PI) < tolerance) {
                    isRevolutionSurface = true;
                }
                break;
            }
            
            case GeomAbs_Sphere:
                // Spheres can be considered revolution surfaces
                isRevolutionSurface = true;
                break;
                
            default:
                break;
        }
        
        if (isRevolutionSurface) {
            revolutionFaces.push_back(face);
        }
    }
    
    return revolutionFaces;
}

std::vector<ProfileExtractor::ProfilePoint> ProfileExtractor::projectFaceToProfile(
    const TopoDS_Face& face,
    const gp_Ax1& axis,
    const ExtractionParameters& params) {
    
    std::vector<ProfilePoint> points;
    BRepAdaptor_Surface surface(face);
    
    // Classify the surface type for feature detection
    FeatureType featureType = classifySurface(surface, axis);
    
    // Sample points across the surface parameter space
    double uMin, uMax, vMin, vMax;
    BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
    
    int uSections = params.profileSections / 10; // Reduce sections for surface sampling
    int vSections = params.profileSections / 10;
    
    for (int i = 0; i <= uSections; ++i) {
        for (int j = 0; j <= vSections; ++j) {
            double u = uMin + (uMax - uMin) * i / uSections;
            double v = vMin + (vMax - vMin) * j / vSections;
            
            try {
                gp_Pnt surfacePoint = surface.Value(u, v);
                
                // Project point to turning axis to get Z and radius coordinates
                gp_Vec axisToPoint(axis.Location(), surfacePoint);
                gp_Dir axisDirection = axis.Direction();
                
                // Z coordinate is projection along axis
                double z = axisToPoint.Dot(axisDirection);
                
                // Radius is distance from axis
                gp_Vec axisVec(axisDirection);
                gp_Pnt projectedOnAxis = axis.Location().Translated(axisVec.Multiplied(z));
                double radius = surfacePoint.Distance(projectedOnAxis);
                
                // Create profile point
                gp_Pnt2d profilePoint(z, radius);
                ProfilePoint point(profilePoint, featureType);
                point.sourceGeometry = face;
                
                points.push_back(point);
                
            } catch (const Standard_Failure&) {
                // Skip invalid parameter combinations
                continue;
            }
        }
    }
    
    return points;
}

std::vector<ProfileExtractor::ProfilePoint> ProfileExtractor::projectEdgeToProfile(
    const TopoDS_Edge& edge,
    const gp_Ax1& axis,
    const ExtractionParameters& params) {
    
    std::vector<ProfilePoint> points;
    BRepAdaptor_Curve curve(edge);
    
    double firstParam = curve.FirstParameter();
    double lastParam = curve.LastParameter();
    
    int sections = params.profileSections / 20; // Fewer sections for edges
    
    for (int i = 0; i <= sections; ++i) {
        double param = firstParam + (lastParam - firstParam) * i / sections;
        
        try {
            gp_Pnt curvePoint = curve.Value(param);
            
            // Project to turning axis
            gp_Vec axisToPoint(axis.Location(), curvePoint);
            gp_Dir axisDirection = axis.Direction();
            
            double z = axisToPoint.Dot(axisDirection);
            gp_Vec axisVec(axisDirection);
            gp_Pnt projectedOnAxis = axis.Location().Translated(axisVec.Multiplied(z));
            double radius = curvePoint.Distance(projectedOnAxis);
            
            gp_Pnt2d profilePoint(z, radius);
            ProfilePoint point(profilePoint, FeatureType::External);
            point.sourceGeometry = edge;
            
            points.push_back(point);
            
        } catch (const Standard_Failure&) {
            continue;
        }
    }
    
    return points;
}

ProfileExtractor::FeatureType ProfileExtractor::classifySurface(
    const BRepAdaptor_Surface& surface,
    const gp_Ax1& axis) {
    
    GeomAbs_SurfaceType surfaceType = surface.GetType();
    
    switch (surfaceType) {
        case GeomAbs_Cylinder:
        case GeomAbs_Cone:
            return FeatureType::External;
            
        case GeomAbs_Plane:
            return FeatureType::Flat;
            
        case GeomAbs_Sphere:
            return FeatureType::Radius;
            
        default:
            return FeatureType::Unknown;
    }
}

LatheProfile::Profile2D ProfileExtractor::optimizeProfile(
    const std::vector<gp_Pnt2d>& rawPoints,
    const ExtractionParameters& params) {
    
    if (rawPoints.empty()) {
        return LatheProfile::Profile2D();
    }
    
    std::vector<gp_Pnt2d> optimizedPoints = rawPoints;
    
    // Sort by Z coordinate
    optimizedPoints = sortPointsByZ(optimizedPoints);
    
    // Remove duplicates
    optimizedPoints = removeDuplicates(optimizedPoints, params.mergeTolerance);
    
    // Merge nearby points if requested
    if (params.mergeTangentSegments) {
        optimizedPoints = mergeNearbyPoints(optimizedPoints, params.mergeTolerance);
    }
    
    LatheProfile::Profile2D profile;
    
    // Convert gp_Pnt2d to IntuiCAM::Geometry::Point2D and add to external profile
    for (const auto& point : optimizedPoints) {
        IntuiCAM::Geometry::Point2D profilePoint;
        profilePoint.x = point.Y(); // Radius coordinate (Y in gp_Pnt2d)
        profilePoint.z = point.X(); // Axial coordinate (X in gp_Pnt2d)
        profile.externalProfile.points.push_back(profilePoint);
    }
    
    return profile;
}

std::vector<gp_Pnt2d> ProfileExtractor::mergeNearbyPoints(
    const std::vector<gp_Pnt2d>& points,
    double tolerance) {
    
    if (points.empty()) return points;
    
    std::vector<gp_Pnt2d> mergedPoints;
    mergedPoints.push_back(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        const gp_Pnt2d& lastPoint = mergedPoints.back();
        const gp_Pnt2d& currentPoint = points[i];
        
        if (lastPoint.Distance(currentPoint) > tolerance) {
            mergedPoints.push_back(currentPoint);
        }
    }
    
    return mergedPoints;
}

std::vector<gp_Pnt2d> ProfileExtractor::sortPointsByZ(const std::vector<gp_Pnt2d>& points) {
    std::vector<gp_Pnt2d> sortedPoints = points;
    
    std::sort(sortedPoints.begin(), sortedPoints.end(),
        [](const gp_Pnt2d& a, const gp_Pnt2d& b) {
            return a.X() < b.X(); // X coordinate represents Z-axis position
        });
    
    return sortedPoints;
}

std::vector<gp_Pnt2d> ProfileExtractor::removeDuplicates(
    const std::vector<gp_Pnt2d>& points,
    double tolerance) {
    
    if (points.empty()) return points;
    
    std::vector<gp_Pnt2d> uniquePoints;
    uniquePoints.push_back(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        bool isDuplicate = false;
        
        for (const auto& existingPoint : uniquePoints) {
            if (points[i].Distance(existingPoint) < tolerance) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate) {
            uniquePoints.push_back(points[i]);
        }
    }
    
    return uniquePoints;
}

LatheProfile::Profile2D ProfileExtractor::convertToSimpleProfile(
    const std::vector<ProfilePoint>& enhancedProfile) {
    
    LatheProfile::Profile2D profile;
    
    std::cout << "ProfileExtractor: Converting " << enhancedProfile.size() << " enhanced profile points" << std::endl;
    
    for (const auto& point : enhancedProfile) {
        IntuiCAM::Geometry::Point2D profilePoint;
        profilePoint.x = point.position.Y(); // Radius coordinate (Y in gp_Pnt2d)
        profilePoint.z = point.position.X(); // Axial coordinate (X in gp_Pnt2d)
        
        if (point.featureType == FeatureType::External) {
            profile.externalProfile.points.push_back(profilePoint);
        } else if (point.featureType == FeatureType::Internal) {
            profile.internalProfile.points.push_back(profilePoint);
        } else {
            // Default to external profile for unknown types
            profile.externalProfile.points.push_back(profilePoint);
        }
    }
    
    return profile;
}

// Helper method for merging nearby ProfilePoints (with tolerance)
std::vector<ProfileExtractor::ProfilePoint> ProfileExtractor::mergeNearbyPoints(
    const std::vector<ProfilePoint>& points,
    double tolerance) {
    
    if (points.empty()) return points;
    
    std::vector<ProfilePoint> mergedPoints;
    mergedPoints.push_back(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        const gp_Pnt2d& lastPos = mergedPoints.back().position;
        const gp_Pnt2d& currentPos = points[i].position;
        
        if (lastPos.Distance(currentPos) > tolerance) {
            mergedPoints.push_back(points[i]);
        }
    }
    
    return mergedPoints;
}

// Helper method for detecting sharp corners
void ProfileExtractor::detectAndMarkSharpCorners(
    std::vector<ProfilePoint>& points,
    double angleTolerance) {
    
    if (points.size() < 3) return;
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        const gp_Pnt2d& p1 = points[i-1].position;
        const gp_Pnt2d& p2 = points[i].position;
        const gp_Pnt2d& p3 = points[i+1].position;
        
        gp_Vec2d v1(p1, p2);
        gp_Vec2d v2(p2, p3);
        
        if (v1.Magnitude() > Precision::Confusion() && 
            v2.Magnitude() > Precision::Confusion()) {
            
            double angle = v1.Angle(v2);
            if (std::abs(angle) > angleTolerance) {
                points[i].isSharpCorner = true;
            }
        }
    }
}

} // namespace Toolpath
} // namespace IntuiCAM 