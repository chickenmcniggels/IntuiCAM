#include "IntuiCAM/Toolpath/LatheProfile.h"

#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Precision.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TopTools_ListOfShape.hxx>

#include <algorithm>
#include <iostream>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

// =====================================================================================
// Segment-based Profile Extraction Implementation
// =====================================================================================

LatheProfile::Profile2D LatheProfile::extractSegmentProfile(const TopoDS_Shape& partGeometry,
                                                           const gp_Ax1& turningAxis,
                                                           double tolerance) {
    Profile2D profile;
    
    try {
        std::cout << "LatheProfile: Starting segment-based profile extraction..." << std::endl;
        
        // Step 1: Create section plane through XZ-plane centered on turning axis
        TopoDS_Shape section = createSectionPlane(partGeometry, turningAxis, tolerance);
        
        if (section.IsNull()) {
            std::cout << "LatheProfile: Failed to create section plane" << std::endl;
            return profile;
        }
        
        // Step 2: Extract all edges from the section that are in positive X direction
        std::vector<TopoDS_Edge> profileEdges = extractProfileEdges(section, turningAxis);
        
        if (profileEdges.empty()) {
            std::cout << "LatheProfile: No profile edges found in section" << std::endl;
            return profile;
        }
        
        std::cout << "LatheProfile: Found " << profileEdges.size() << " profile edges" << std::endl;
        
        // Step 3: Convert edges to profile segments
        std::vector<ProfileSegment> segments;
        for (const auto& edge : profileEdges) {
            ProfileSegment segment = convertEdgeToSegment(edge, turningAxis);
            if (segment.length > tolerance) { // Filter out tiny segments
                segments.push_back(segment);
            }
        }
        
        // Step 4: Sort segments by Z coordinate for proper ordering
        sortSegmentsByZ(segments);
        
        profile.segments = std::move(segments);
        
        std::cout << "LatheProfile: Successfully extracted " << profile.getSegmentCount() 
                  << " segments with total length " << profile.getTotalLength() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception during extraction: " << e.what() << std::endl;
    }
    
    return profile;
}

TopoDS_Shape LatheProfile::createSectionPlane(const TopoDS_Shape& partGeometry,
                                             const gp_Ax1& turningAxis,
                                             double tolerance) {
    try {
        // Create XZ-plane passing through the turning axis origin
        // The plane normal should be perpendicular to the XZ-plane (i.e., in Y direction)
        
        gp_Pnt origin = turningAxis.Location();
        gp_Dir axisDir = turningAxis.Direction();
        
        // For a standard lathe setup, the turning axis should be Z and we want the XZ plane
        // The normal to XZ plane is Y direction
        gp_Dir planeNormal(0, 1, 0);
        
        // If turning axis is not aligned with Z, we need to compute the proper normal
        if (!axisDir.IsEqual(gp_Dir(0, 0, 1), Precision::Angular())) {
            // Create a coordinate system with the turning axis as Z
            gp_Dir xDir(1, 0, 0);
            if (axisDir.IsParallel(xDir, Precision::Angular())) {
                xDir = gp_Dir(0, 1, 0);
            }
            
            // Ensure xDir is perpendicular to axisDir
            gp_Vec crossProduct = gp_Vec(xDir).Crossed(gp_Vec(axisDir));
            if (crossProduct.Magnitude() > Precision::Confusion()) {
                planeNormal = gp_Dir(crossProduct);
            }
        }
        
        // Create the cutting plane
        gp_Pln cuttingPlane(origin, planeNormal);
        
        // Create a large face from the plane to ensure it cuts through the entire part
        const double planeSize = 1000.0; // Large enough to cut through any reasonable part
        TopoDS_Face planeFace = BRepBuilderAPI_MakeFace(cuttingPlane, -planeSize, planeSize, -planeSize, planeSize).Face();
        
        // Perform the section operation
        BRepAlgoAPI_Section sectionOp;
        
        TopTools_ListOfShape arguments, tools;
        arguments.Append(partGeometry);
        tools.Append(planeFace);
        
        sectionOp.SetArguments(arguments);
        sectionOp.SetTools(tools);
        sectionOp.SetFuzzyValue(tolerance);
        
        sectionOp.Build();
        
        if (!sectionOp.IsDone() || sectionOp.HasErrors()) {
            std::cout << "LatheProfile: Section operation failed" << std::endl;
            return TopoDS_Shape();
        }
        
        TopoDS_Shape result = sectionOp.Shape();
        std::cout << "LatheProfile: Section operation completed successfully" << std::endl;
        
        return result;
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception in createSectionPlane: " << e.what() << std::endl;
        return TopoDS_Shape();
    }
}

std::vector<TopoDS_Edge> LatheProfile::extractProfileEdges(const TopoDS_Shape& section,
                                                          const gp_Ax1& turningAxis) {
    std::vector<TopoDS_Edge> profileEdges;
    
    try {
        gp_Pnt axisOrigin = turningAxis.Location();
        gp_Dir axisDirection = turningAxis.Direction();
        
        // Explore all edges in the section
        TopExp_Explorer edgeExplorer(section, TopAbs_EDGE);
        
        for (; edgeExplorer.More(); edgeExplorer.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(edgeExplorer.Current());
            
            if (edge.IsNull()) continue;
            
            // Process this edge to handle Z-axis intersection
            std::vector<TopoDS_Edge> processedEdges = splitEdgeAtZAxis(edge, turningAxis);
            
            // Add all valid processed edges
            for (const auto& processedEdge : processedEdges) {
                profileEdges.push_back(processedEdge);
            }
        }
        
        std::cout << "LatheProfile: Extracted " << profileEdges.size() << " profile edges after Z-axis processing" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception in extractProfileEdges: " << e.what() << std::endl;
    }
    
    return profileEdges;
}

std::vector<TopoDS_Edge> LatheProfile::splitEdgeAtZAxis(const TopoDS_Edge& edge, const gp_Ax1& turningAxis) {
    std::vector<TopoDS_Edge> result;
    
    try {
        gp_Pnt axisOrigin = turningAxis.Location();
        gp_Dir axisDirection = turningAxis.Direction();
        
        // Get edge endpoints
        TopoDS_Vertex startVertex, endVertex;
        TopExp::Vertices(edge, startVertex, endVertex);
        
        if (startVertex.IsNull() || endVertex.IsNull()) {
            return result;
        }
        
        gp_Pnt startPnt3D = BRep_Tool::Pnt(startVertex);
        gp_Pnt endPnt3D = BRep_Tool::Pnt(endVertex);
        
        // Calculate radial distances from the turning axis (X coordinates in profile space)
        gp_Vec toStart(axisOrigin, startPnt3D);
        gp_Vec toEnd(axisOrigin, endPnt3D);
        
        double startZ = toStart.Dot(gp_Vec(axisDirection));
        double endZ = toEnd.Dot(gp_Vec(axisDirection));
        
        gp_Pnt projectedStart = axisOrigin.Translated(gp_Vec(axisDirection) * startZ);
        gp_Pnt projectedEnd = axisOrigin.Translated(gp_Vec(axisDirection) * endZ);
        
        double startRadius = startPnt3D.Distance(projectedStart);
        double endRadius = endPnt3D.Distance(projectedEnd);
        
        // Determine which side of Z-axis each endpoint is on
        // Need to check the actual X coordinate in the profile coordinate system
        gp_Vec radialStart = gp_Vec(projectedStart, startPnt3D);
        gp_Vec radialEnd = gp_Vec(projectedEnd, endPnt3D);
        
        // For simplicity, assume X direction is perpendicular to both Y and Z
        gp_Dir xDir(1, 0, 0);
        if (axisDirection.IsParallel(xDir, Precision::Angular())) {
            xDir = gp_Dir(0, 1, 0);
        }
        
        double startX = radialStart.Dot(gp_Vec(xDir));
        double endX = radialEnd.Dot(gp_Vec(xDir));
        
        const double tolerance = Precision::Confusion();
        
        // Case 1: Both endpoints in positive X - keep entire edge
        if (startX > tolerance && endX > tolerance) {
            result.push_back(edge);
            std::cout << "LatheProfile: Edge entirely in positive X, keeping whole edge" << std::endl;
        }
        // Case 2: Both endpoints in negative X - discard entire edge
        else if (startX < -tolerance && endX < -tolerance) {
            std::cout << "LatheProfile: Edge entirely in negative X, discarding" << std::endl;
            // Don't add anything to result
        }
        // Case 3: Edge crosses Z-axis - need to split
        else {
            std::cout << "LatheProfile: Edge crosses Z-axis, attempting to split" << std::endl;
            
            // Try to find intersection with Z-axis and split the edge
            TopoDS_Edge splitEdge = splitEdgeAtZAxisIntersection(edge, turningAxis, startX, endX);
            if (!splitEdge.IsNull()) {
                result.push_back(splitEdge);
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception in splitEdgeAtZAxis: " << e.what() << std::endl;
    }
    
    return result;
}

TopoDS_Edge LatheProfile::splitEdgeAtZAxisIntersection(const TopoDS_Edge& edge, const gp_Ax1& turningAxis, 
                                                      double startX, double endX) {
    try {
        // For edges that cross the Z-axis, we need to find the intersection parameter
        // and create a new edge from that point to the endpoint in positive X
        
        BRepAdaptor_Curve curve(edge);
        double firstParam = curve.FirstParameter();
        double lastParam = curve.LastParameter();
        
        // Use binary search to find the parameter where X coordinate is approximately 0
        double paramAtZAxis = findZAxisIntersectionParameter(curve, turningAxis, firstParam, lastParam);
        
        if (paramAtZAxis >= firstParam && paramAtZAxis <= lastParam) {
            gp_Pnt intersectionPoint = curve.Value(paramAtZAxis);
            
            // Determine which portion to keep (the one in positive X)
            double keepStart, keepEnd;
            if (startX > 0) {
                // Keep from start to intersection
                keepStart = firstParam;
                keepEnd = paramAtZAxis;
            } else {
                // Keep from intersection to end  
                keepStart = paramAtZAxis;
                keepEnd = lastParam;
            }
            
            // Create new edge with the trimmed portion
            Handle(Geom_Curve) geomCurve = curve.Curve().Curve();
            Handle(Geom_TrimmedCurve) trimmedCurve = new Geom_TrimmedCurve(geomCurve, keepStart, keepEnd);
            
            BRepBuilderAPI_MakeEdge edgeBuilder(trimmedCurve);
            if (edgeBuilder.IsDone()) {
                std::cout << "LatheProfile: Successfully split edge at Z-axis" << std::endl;
                return edgeBuilder.Edge();
            }
        }
        
        std::cout << "LatheProfile: Could not split edge at Z-axis, keeping as-is" << std::endl;
        return edge; // Fallback: return original edge
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception in splitEdgeAtZAxisIntersection: " << e.what() << std::endl;
        return edge; // Fallback: return original edge
    }
}

double LatheProfile::findZAxisIntersectionParameter(const BRepAdaptor_Curve& curve, const gp_Ax1& turningAxis,
                                                   double firstParam, double lastParam) {
    gp_Pnt axisOrigin = turningAxis.Location();
    gp_Dir axisDirection = turningAxis.Direction();
    gp_Dir xDir(1, 0, 0);
    
    const double tolerance = 1e-6;
    const int maxIterations = 50;
    
    // Binary search for parameter where X coordinate is approximately 0
    double paramLow = firstParam;
    double paramHigh = lastParam;
    
    for (int i = 0; i < maxIterations; i++) {
        double paramMid = (paramLow + paramHigh) / 2.0;
        gp_Pnt pointMid = curve.Value(paramMid);
        
        // Calculate X coordinate in profile space
        gp_Vec toPoint(axisOrigin, pointMid);
        double z = toPoint.Dot(gp_Vec(axisDirection));
        gp_Pnt projectedPoint = axisOrigin.Translated(gp_Vec(axisDirection) * z);
        gp_Vec radial = gp_Vec(projectedPoint, pointMid);
        double x = radial.Dot(gp_Vec(xDir));
        
        if (std::abs(x) < tolerance) {
            return paramMid; // Found intersection
        }
        
        // Determine which half contains the zero crossing
        gp_Pnt pointLow = curve.Value(paramLow);
        gp_Vec toLow(axisOrigin, pointLow);
        double zLow = toLow.Dot(gp_Vec(axisDirection));
        gp_Pnt projectedLow = axisOrigin.Translated(gp_Vec(axisDirection) * zLow);
        gp_Vec radialLow = gp_Vec(projectedLow, pointLow);
        double xLow = radialLow.Dot(gp_Vec(xDir));
        
        if ((xLow > 0) != (x > 0)) {
            paramHigh = paramMid;
        } else {
            paramLow = paramMid;
        }
    }
    
    return (paramLow + paramHigh) / 2.0; // Return best approximation
}

LatheProfile::ProfileSegment LatheProfile::convertEdgeToSegment(const TopoDS_Edge& edge,
                                                               const gp_Ax1& turningAxis) {
    ProfileSegment segment;
    
    try {
        // Get edge endpoints
        TopoDS_Vertex startVertex, endVertex;
        TopExp::Vertices(edge, startVertex, endVertex);
        
        if (startVertex.IsNull() || endVertex.IsNull()) {
            return segment;
        }
        
        gp_Pnt startPnt3D = BRep_Tool::Pnt(startVertex);
        gp_Pnt endPnt3D = BRep_Tool::Pnt(endVertex);
        
        gp_Pnt axisOrigin = turningAxis.Location();
        gp_Dir axisDirection = turningAxis.Direction();
        
        // Convert 3D points to 2D profile coordinates (radius, Z)
        gp_Vec toStart(axisOrigin, startPnt3D);
        gp_Vec toEnd(axisOrigin, endPnt3D);
        
        double startZ = toStart.Dot(gp_Vec(axisDirection));
        double endZ = toEnd.Dot(gp_Vec(axisDirection));
        
        gp_Pnt projectedStart = axisOrigin.Translated(gp_Vec(axisDirection) * startZ);
        gp_Pnt projectedEnd = axisOrigin.Translated(gp_Vec(axisDirection) * endZ);
        
        double startRadius = startPnt3D.Distance(projectedStart);
        double endRadius = endPnt3D.Distance(projectedEnd);
        
        // Create 2D profile points
        segment.start = IntuiCAM::Geometry::Point2D{startRadius, startZ};
        segment.end = IntuiCAM::Geometry::Point2D{endRadius, endZ};
        segment.edge = edge;
        
        // Calculate segment length
        double dx = segment.end.x - segment.start.x;
        double dz = segment.end.z - segment.start.z;
        segment.length = std::sqrt(dx * dx + dz * dz);
        
        // Determine if the segment is linear or curved
        BRepAdaptor_Curve curve(edge);
        GeomAbs_CurveType curveType = curve.GetType();
        segment.isLinear = (curveType == GeomAbs_Line);
        
        std::cout << "LatheProfile: Created segment - Start(" << segment.start.x << ", " << segment.start.z 
                  << ") End(" << segment.end.x << ", " << segment.end.z << ") Length=" << segment.length 
                  << " Linear=" << segment.isLinear << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "LatheProfile: Exception in convertEdgeToSegment: " << e.what() << std::endl;
    }
    
    return segment;
}

void LatheProfile::sortSegmentsByZ(std::vector<ProfileSegment>& segments) {
    // Sort segments by their average Z coordinate
    std::sort(segments.begin(), segments.end(), 
              [](const ProfileSegment& a, const ProfileSegment& b) {
                  double avgZ_a = (a.start.z + a.end.z) / 2.0;
                  double avgZ_b = (b.start.z + b.end.z) / 2.0;
                  return avgZ_a < avgZ_b;
              });
}

// =====================================================================================
// Profile2D Helper Methods Implementation
// =====================================================================================

void LatheProfile::Profile2D::getBounds(double& minZ, double& maxZ, double& minRadius, double& maxRadius) const {
    if (segments.empty()) {
        minZ = maxZ = minRadius = maxRadius = 0.0;
        return;
    }
    
    minZ = maxZ = segments[0].start.z;
    minRadius = maxRadius = segments[0].start.x;
    
    for (const auto& segment : segments) {
        minZ = std::min({minZ, segment.start.z, segment.end.z});
        maxZ = std::max({maxZ, segment.start.z, segment.end.z});
        minRadius = std::min({minRadius, segment.start.x, segment.end.x});
        maxRadius = std::max({maxRadius, segment.start.x, segment.end.x});
    }
}

std::vector<IntuiCAM::Geometry::Point2D> LatheProfile::Profile2D::toPointArray(double tolerance) const {
    std::vector<IntuiCAM::Geometry::Point2D> points;
    
    for (const auto& segment : segments) {
        points.push_back(segment.start);
        
        // For curved segments, we might want to add intermediate points
        if (!segment.isLinear && segment.length > tolerance * 2) {
            // Add midpoint for curved segments
            IntuiCAM::Geometry::Point2D mid{
                (segment.start.x + segment.end.x) / 2.0,
                (segment.start.z + segment.end.z) / 2.0
            };
            points.push_back(mid);
        }
        
        points.push_back(segment.end);
    }
    
    return points;
}

// =====================================================================================
// Legacy Implementation for Backward Compatibility
// =====================================================================================

LatheProfile::SimpleProfile2D LatheProfile::extract(const IntuiCAM::Geometry::Part& part,
                                                     int numSections,
                                                     double extraMargin) {
    SimpleProfile2D profile;
    
    // Get part bounding box for analysis
    auto bbox = part.getBoundingBox();
    
    // Calculate section positions along Z-axis
    double zStart = bbox.min.z;
    double zEnd = bbox.max.z;
    double zStep = (zEnd - zStart) / (numSections - 1);
    
    // For now, create a simple cylindrical profile as placeholder
    // In a full implementation, this would use OpenCASCADE sectioning
    double radius = std::max(bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y) / 2.0;
    
    for (int i = 0; i < numSections; ++i) {
        double z = zStart + i * zStep;
        IntuiCAM::Geometry::Point2D point(radius, z);
        profile.push_back(point);
    }
    
    std::cout << "LatheProfile: Legacy extraction created " << profile.size() << " points" << std::endl;
    
    return profile;
}

} // namespace Toolpath
} // namespace IntuiCAM
