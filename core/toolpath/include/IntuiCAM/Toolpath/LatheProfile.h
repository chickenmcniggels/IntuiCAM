#pragma once

#include <vector>
#include <IntuiCAM/Geometry/Types.h>

// OCCT includes
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Ax1.hxx>

// Forward declarations
class BRepAdaptor_Curve;

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Segment-based profile extraction for lathe operations
 * 
 * This class extracts 2D profiles from 3D geometry using OCCT sectioning,
 * preserving the original geometric segments rather than approximating with points.
 */
class LatheProfile {
public:
    /**
     * @brief Represents a geometric segment in the profile
     */
    struct ProfileSegment {
        TopoDS_Edge edge;                    // Original OCCT edge
        IntuiCAM::Geometry::Point2D start;   // Start point (radius, z) 
        IntuiCAM::Geometry::Point2D end;     // End point (radius, z)
        double length;                       // Segment length
        bool isLinear;                       // True if linear, false if curved
        
        ProfileSegment() : length(0.0), isLinear(true) {}
        ProfileSegment(const TopoDS_Edge& e, const IntuiCAM::Geometry::Point2D& s, 
                      const IntuiCAM::Geometry::Point2D& e_pt, double len, bool linear)
            : edge(e), start(s), end(e_pt), length(len), isLinear(linear) {}
    };

    /**
     * @brief Segment-based 2D profile containing all contour segments
     */
    struct Profile2D {
        std::vector<ProfileSegment> segments;  // All profile segments
        
        // Helper methods
        bool isEmpty() const {
            return segments.empty();
        }
        
        size_t getSegmentCount() const {
            return segments.size();
        }
        
        double getTotalLength() const {
            double total = 0.0;
            for (const auto& seg : segments) {
                total += seg.length;
            }
            return total;
        }
        
        // Get bounding box of profile
        void getBounds(double& minZ, double& maxZ, double& minRadius, double& maxRadius) const;
        
        // Convert to point array for backward compatibility
        std::vector<IntuiCAM::Geometry::Point2D> toPointArray(double tolerance = 0.1) const;
        
        // Backward compatibility methods (uses first/last segments)
        bool empty() const { 
            return segments.empty(); 
        }
        
        size_t size() const { 
            return segments.size(); 
        }
        
        // Legacy support - approximate point count for compatibility
        size_t getTotalPointCount() const {
            return segments.size() * 2; // Start and end of each segment
        }
        
        // Iterator support for segments
        auto begin() const { return segments.begin(); }
        auto end() const { return segments.end(); }
        
        const ProfileSegment& front() const { return segments.front(); }
        const ProfileSegment& back() const { return segments.back(); }
        const ProfileSegment& operator[](size_t index) const { return segments[index]; }
    };

    // Legacy typedef for backward compatibility - will be removed in future
    using SimpleProfile2D = std::vector<IntuiCAM::Geometry::Point2D>;

    /**
     * @brief Extract segment-based profile from 3D part geometry using OCCT sectioning
     * @param partGeometry 3D part shape (TopoDS_Shape)
     * @param turningAxis Axis of rotation for the lathe operation
     * @param tolerance Geometric tolerance for sectioning operation
     * @return Extracted profile containing geometric segments
     */
    static Profile2D extractSegmentProfile(const TopoDS_Shape& partGeometry,
                                          const gp_Ax1& turningAxis,
                                          double tolerance = 0.01);

    /**
     * @brief Legacy point-based extraction for backward compatibility
     * @deprecated Use extractSegmentProfile instead
     */
    static SimpleProfile2D extract(const IntuiCAM::Geometry::Part& part,
                                  int numSections = 100,
                                  double extraMargin = 1.0);

    /**
     * @brief Sort segments by Z coordinate for proper ordering
     */
    static void sortSegmentsByZ(std::vector<ProfileSegment>& segments);

private:
    /**
     * @brief Create XZ-plane section through the part
     */
    static TopoDS_Shape createSectionPlane(const TopoDS_Shape& partGeometry, 
                                         const gp_Ax1& turningAxis,
                                         double tolerance);
    
    /**
     * @brief Extract all edges from section that are in positive X direction
     */
    static std::vector<TopoDS_Edge> extractProfileEdges(const TopoDS_Shape& section,
                                                       const gp_Ax1& turningAxis);
    
    /**
     * @brief Split an edge at intersections with the Z-axis, keeping only positive X portions
     * @param edge Edge to process
     * @param turningAxis The turning axis (Z-axis)
     * @return Vector of edge portions in positive X space
     */
    static std::vector<TopoDS_Edge> splitEdgeAtZAxis(const TopoDS_Edge& edge, const gp_Ax1& turningAxis);

    /**
     * @brief Split an edge at its intersection with the Z-axis
     * @param edge Edge to split
     * @param turningAxis The turning axis
     * @param startX X coordinate of start point
     * @param endX X coordinate of end point
     * @return The portion of the edge in positive X space
     */
    static TopoDS_Edge splitEdgeAtZAxisIntersection(const TopoDS_Edge& edge, const gp_Ax1& turningAxis, 
                                                   double startX, double endX);

    /**
     * @brief Find the parameter where an edge intersects the Z-axis
     * @param curve The curve adapter for the edge
     * @param turningAxis The turning axis
     * @param firstParam First parameter of the curve
     * @param lastParam Last parameter of the curve
     * @return Parameter value at Z-axis intersection
     */
    static double findZAxisIntersectionParameter(const BRepAdaptor_Curve& curve, const gp_Ax1& turningAxis,
                                               double firstParam, double lastParam);

    /**
     * @brief Convert a 3D edge to a 2D profile segment
     */
    static ProfileSegment convertEdgeToSegment(const TopoDS_Edge& edge,
                                             const gp_Ax1& turningAxis);
};

} // namespace Toolpath
} // namespace IntuiCAM 