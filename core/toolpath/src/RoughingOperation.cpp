#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <cmath>
#include <vector>
#include <algorithm>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace Toolpath {

RoughingOperation::RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Roughing, name, tool) {
}

std::unique_ptr<Toolpath> RoughingOperation::generateToolpath(const Geometry::Part& part) {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    // Extract the 2D profile from the part
    // This extracts the profile that when revolved creates the part
    std::vector<Geometry::Point2D> profile = extractProfile(part);
    
    if (profile.empty()) {
        // Fallback to basic roughing if profile extraction fails
        return generateBasicRoughing();
    }
    
    // Sort profile points by Z coordinate (ascending)
    std::sort(profile.begin(), profile.end(), 
              [](const Geometry::Point2D& a, const Geometry::Point2D& b) {
                  return a.z < b.z;
              });
    
    // Calculate the bounding box for the profile
    double minRadius = std::numeric_limits<double>::max();
    double maxRadius = 0.0;
    double minZ = profile.front().z;
    double maxZ = profile.back().z;
    
    for (const auto& point : profile) {
        minRadius = std::min(minRadius, point.x);
        maxRadius = std::max(maxRadius, point.x);
    }
    
    // Adjust parameters based on part profile if needed
    if (params_.startDiameter < maxRadius * 2.0) {
        params_.startDiameter = maxRadius * 2.0 + 5.0; // Add more clearance for safety
    }
    
    if (params_.endDiameter > minRadius * 2.0) {
        params_.endDiameter = std::max(minRadius * 2.0 - params_.stockAllowance * 2.0, 1.0);
    }
    
    // Adjust Z ranges to cover the entire part with safe clearance
    params_.startZ = std::max(params_.startZ, minZ - 5.0);
    params_.endZ = std::min(params_.endZ, maxZ + 5.0);
    
    // Get tool parameters
    double feedRate = tool_->getCuttingParameters().feedRate;
    double rapidFeedRate = tool_->getCuttingParameters().rapidFeedRate;
    
    // Calculate the number of radial roughing passes
    double radialDepth = params_.depthOfCut;
    double radialRange = (params_.startDiameter - params_.endDiameter) / 2.0 - params_.stockAllowance;
    int numPasses = std::ceil(radialRange / radialDepth);
    
    // Adjust radial depth to get even passes
    if (numPasses > 0) {
        radialDepth = radialRange / numPasses;
    }
    
    // Initial rapid move to safe position
    double safeRadialDistance = params_.startDiameter/2.0 + 10.0;
    toolpath->addRapidMove(Geometry::Point3D(safeRadialDistance, 0, params_.startZ + 10.0));
    
    // Current working diameter (outside in)
    double currentDiameter = params_.startDiameter;
    
    // Execute roughing passes
    for (int pass = 0; pass <= numPasses; pass++) {
        // Calculate target diameter for this pass
        double targetDiameter = std::max(
            params_.startDiameter - (pass + 1) * radialDepth * 2.0,
            params_.endDiameter + params_.stockAllowance * 2.0
        );
        
        double currentRadius = currentDiameter / 2.0;
        double targetRadius = targetDiameter / 2.0;
        
        // Move to safe Z position at current X
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, params_.startZ + 5.0));
        
        // Initial plunge to start Z
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.startZ), feedRate);
        
        // Create points for this pass following the profile shape
        std::vector<Geometry::Point3D> passPoints;
        
        // Z step calculation - use smaller steps for more detail
        const int numZSteps = 100; // Increased for better detail
        double zRange = params_.endZ - params_.startZ;
        double zStep = zRange / numZSteps;
        
        // Calculate points along the profile for this pass
        for (int zIdx = 0; zIdx <= numZSteps; zIdx++) {
            double z = params_.startZ + zIdx * zStep;
            
            // Get radius at this Z from profile
            double profileRadiusAtZ = getProfileRadiusAtZ(profile, z) + params_.stockAllowance;
            
            // Calculate cutting radius for this pass (constrained by current pass depth)
            double cuttingRadius = std::min(currentRadius, std::max(targetRadius, profileRadiusAtZ));
            
            // Add point to pass
            passPoints.push_back(Geometry::Point3D(cuttingRadius, 0, z));
        }
        
        // Simplify the path by removing unnecessary points (if points are colinear)
        std::vector<Geometry::Point3D> simplifiedPoints = simplifyPath(passPoints, 0.01);
        
        // Execute the path for this pass
        for (const auto& point : simplifiedPoints) {
            toolpath->addLinearMove(point, feedRate);
        }
        
        // Retract to safe position
        toolpath->addRapidMove(Geometry::Point3D(simplifiedPoints.back().x, 0, simplifiedPoints.back().z + 5.0));
        toolpath->addRapidMove(Geometry::Point3D(safeRadialDistance, 0, simplifiedPoints.back().z + 5.0));
        
        // Update current diameter for next pass
        currentDiameter = targetDiameter;
    }
    
    // Final safe position
    toolpath->addRapidMove(Geometry::Point3D(safeRadialDistance, 0, params_.startZ + 10.0));
    
    return toolpath;
}

bool RoughingOperation::validate() const {
    return params_.startDiameter > params_.endDiameter && 
           params_.startZ > params_.endZ && 
           params_.depthOfCut > 0.0;
}

std::vector<Geometry::Point2D> RoughingOperation::extractProfile(const Geometry::Part& part) {
    std::vector<Geometry::Point2D> profile;
    
    // Get the part's bounding box
    Geometry::BoundingBox bbox = part.getBoundingBox();
    
    // Extract OpenCASCADE shape from part via dynamic_cast if possible
    auto* occPart = dynamic_cast<const Geometry::OCCTPart*>(&part);
    if (occPart) {
        try {
            // Get the underlying TopoDS_Shape
            const TopoDS_Shape& partShape = occPart->getOCCTShape();
            
            if (partShape.IsNull()) {
                // Fallback to simple profile if shape is null
                return generateSimpleProfile(bbox);
            }
            
            // Get turning axis (should be Z-axis in standard setup)
            gp_Ax1 turningAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
            
            // Create section planes perpendicular to turning axis
            // Increase number of sections for more detailed profile
            const int numSections = 100;
            double zStart = bbox.min.z;
            double zEnd = bbox.max.z;
            double zStep = (zEnd - zStart) / (numSections - 1);
            
            // Find maximum X or Y coordinate to determine initial search radius
            double maxRadius = std::max(std::max(std::abs(bbox.min.x), std::abs(bbox.max.x)),
                                       std::max(std::abs(bbox.min.y), std::abs(bbox.max.y)));
            maxRadius *= 1.1; // Add some margin
            
            // For each Z position, find intersection with part and calculate radius
            for (int i = 0; i < numSections; i++) {
                double z = zStart + i * zStep;
                
                // Create a section plane at position z
                gp_Pnt planeOrigin(0, 0, z);
                gp_Dir planeNormal = turningAxis.Direction();
                gp_Pln sectionPlane(planeOrigin, planeNormal);
                
                // Find intersection with part shape (using BRepAlgoAPI_Section)
                BRepAlgoAPI_Section section(partShape, sectionPlane, Standard_False);
                section.Build();
                
                if (section.IsDone()) {
                    TopoDS_Shape sectionShape = section.Shape();
                    
                    // Find the maximum radius from intersection curves
                    double sectionRadius = findMaxRadiusFromSection(sectionShape);
                    
                    if (sectionRadius > 0) {
                        // Add point to profile
                        profile.push_back(Geometry::Point2D(sectionRadius, z));
                    }
                }
            }
            
            // Post-process profile to ensure it's clean and well-formed
            if (profile.size() >= 5) {
                // Ensure the profile is sorted by Z coordinate
                std::sort(profile.begin(), profile.end(), 
                        [](const Geometry::Point2D& a, const Geometry::Point2D& b) {
                            return a.z < b.z;
                        });
                
                // Smooth the profile by removing noise
                std::vector<Geometry::Point2D> smoothedProfile;
                const double smoothingTolerance = 0.1; // mm
                
                smoothedProfile.push_back(profile.front());
                for (size_t i = 1; i < profile.size() - 1; i++) {
                    // Check if this point is significantly different from the previous one
                    double radiusDiff = std::abs(profile[i].x - smoothedProfile.back().x);
                    if (radiusDiff > smoothingTolerance) {
                        smoothedProfile.push_back(profile[i]);
                    }
                }
                smoothedProfile.push_back(profile.back());
                
                return smoothedProfile;
            }
        } catch (const std::exception&) {
            // Handle OpenCASCADE exceptions
            return generateSimpleProfile(bbox);
        }
    } else {
        // If not an OCC part, generate simple profile
        return generateSimpleProfile(bbox);
    }
    
    // If we couldn't extract enough points, generate simple profile
    if (profile.size() < 5) {
        return generateSimpleProfile(bbox);
    }
    
    return profile;
}

// Helper function to generate a simple profile when extraction fails
std::vector<Geometry::Point2D> RoughingOperation::generateSimpleProfile(const Geometry::BoundingBox& bbox) {
    std::vector<Geometry::Point2D> profile;
    
    // Calculate points along the z-axis
    const int numPoints = 30;
    double zStart = bbox.min.z;
    double zEnd = bbox.max.z;
    double zStep = (zEnd - zStart) / (numPoints - 1);
    
    // Use bounding box to create a simple profile
    double maxRadius = std::max(std::max(std::abs(bbox.min.x), std::abs(bbox.max.x)),
                               std::max(std::abs(bbox.min.y), std::abs(bbox.max.y)));
    
    // Simple cylinder profile
    for (int i = 0; i < numPoints; i++) {
        double z = zStart + i * zStep;
        profile.push_back(Geometry::Point2D(maxRadius, z));
    }
    
    return profile;
}

// Helper function to find maximum radius in a section
double RoughingOperation::findMaxRadiusFromSection(const TopoDS_Shape& sectionShape) {
    double maxRadius = 0.0;
    
    // Iterate through all edges in the section
    for (TopExp_Explorer explorer(sectionShape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        BRepAdaptor_Curve curve(edge);
        
        // Get curve type
        GeomAbs_CurveType curveType = curve.GetType();
        
        // Get number of points to sample based on curve type
        int numSamples = 10;
        if (curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse) {
            numSamples = 36; // Every 10 degrees
        } else if (curveType == GeomAbs_BSplineCurve || curveType == GeomAbs_BezierCurve) {
            numSamples = 20;
        }
        
        // Sample points along the curve
        for (int i = 0; i <= numSamples; i++) {
            double param = curve.FirstParameter() + 
                          (curve.LastParameter() - curve.FirstParameter()) * i / numSamples;
            
            gp_Pnt point = curve.Value(param);
            
            // Calculate radius (distance from Z axis)
            double radius = std::sqrt(point.X() * point.X() + point.Y() * point.Y());
            
            // Update maximum radius
            maxRadius = std::max(maxRadius, radius);
        }
    }
    
    return maxRadius;
}

// Helper function to get profile radius at a specific Z coordinate
double RoughingOperation::getProfileRadiusAtZ(const std::vector<Geometry::Point2D>& profile, double z) {
    // Handle empty profile case
    if (profile.empty()) {
        return 0.0;
    }
    
    // Handle case where z is outside the profile range
    if (z <= profile.front().z) {
        return profile.front().x;
    }
    
    if (z >= profile.back().z) {
        return profile.back().x;
    }
    
    // Find the points that bracket the requested Z coordinate
    for (size_t i = 0; i < profile.size() - 1; i++) {
        if (z >= profile[i].z && z <= profile[i+1].z) {
            // Linear interpolation between points
            double t = (z - profile[i].z) / (profile[i+1].z - profile[i].z);
            return profile[i].x * (1.0 - t) + profile[i+1].x * t;
        }
    }
    
    // Fallback (should not reach here if profile is properly sorted)
    return 0.0;
}

// Helper function to generate basic roughing toolpath
std::unique_ptr<Toolpath> RoughingOperation::generateBasicRoughing() {
    auto toolpath = std::make_unique<Toolpath>(name_, tool_);
    
    // Get tool parameters
    double feedRate = tool_->getCuttingParameters().feedRate;
    
    // Calculate the number of radial roughing passes
    double radialDepth = params_.depthOfCut;
    double radialRange = (params_.startDiameter - params_.endDiameter) / 2.0 - params_.stockAllowance;
    int numPasses = std::ceil(radialRange / radialDepth);
    
    // Adjust radial depth to get even passes
    if (numPasses > 0) {
        radialDepth = radialRange / numPasses;
    }
    
    // Initial rapid move to safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.startDiameter/2.0 + 5.0, 0, params_.startZ + 5.0));
    
    // Current working diameter
    double currentDiameter = params_.startDiameter;
    
    // Roughing passes
    for (int pass = 0; pass <= numPasses; pass++) {
        // Calculate target diameter for this pass
        double targetDiameter = std::max(
            params_.startDiameter - (pass + 1) * radialDepth * 2.0,
            params_.endDiameter + params_.stockAllowance * 2.0
        );
        
        double currentRadius = currentDiameter / 2.0;
        
        // Move to start position
        toolpath->addRapidMove(Geometry::Point3D(currentRadius, 0, params_.startZ + 2.0));
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.startZ), feedRate);
        
        // Cut along Z
        toolpath->addLinearMove(Geometry::Point3D(currentRadius, 0, params_.endZ), feedRate);
        
        // Retract
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 5.0, 0, params_.endZ));
        toolpath->addRapidMove(Geometry::Point3D(currentRadius + 5.0, 0, params_.startZ + 2.0));
        
        // Update current diameter for next pass
        currentDiameter = targetDiameter;
    }
    
    // Final safe position
    toolpath->addRapidMove(Geometry::Point3D(params_.startDiameter/2.0 + 5.0, 0, params_.startZ + 5.0));
    
    return toolpath;
}

// Helper function to simplify a path by removing points that are nearly colinear
std::vector<Geometry::Point3D> RoughingOperation::simplifyPath(
    const std::vector<Geometry::Point3D>& points, double tolerance) {
    
    if (points.size() <= 2) {
        return points;
    }
    
    std::vector<Geometry::Point3D> result;
    result.push_back(points[0]);
    
    for (size_t i = 1; i < points.size() - 1; i++) {
        Geometry::Point3D prev = result.back();
        Geometry::Point3D curr = points[i];
        Geometry::Point3D next = points[i+1];
        
        // Check if current point is nearly colinear with previous and next
        double dx1 = curr.x - prev.x;
        double dz1 = curr.z - prev.z;
        double dx2 = next.x - curr.x;
        double dz2 = next.z - curr.z;
        
        // Normalize vectors
        double len1 = std::sqrt(dx1*dx1 + dz1*dz1);
        double len2 = std::sqrt(dx2*dx2 + dz2*dz2);
        
        if (len1 > 0 && len2 > 0) {
            dx1 /= len1;
            dz1 /= len1;
            dx2 /= len2;
            dz2 /= len2;
            
            // Calculate dot product
            double dotProduct = dx1*dx2 + dz1*dz2;
            
            // If vectors are nearly parallel, skip the point
            if (std::abs(dotProduct) > (1.0 - tolerance)) {
                continue;
            }
        }
        
        result.push_back(curr);
    }
    
    result.push_back(points.back());
    return result;
}

} // namespace Toolpath
} // namespace IntuiCAM 