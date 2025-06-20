#pragma once

#include <vector>
#include <string>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Circ.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_SurfaceType.hxx>

// IntuiCAM includes
#include <IntuiCAM/Toolpath/LatheProfile.h>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief 2D profile extraction from 3D part geometry
 * 
 * This class provides robust, profile-agnostic extraction of 2D lathe profiles
 * from arbitrary 3D part geometries. It handles:
 * - Revolution surfaces (cylinders, cones, spheres)
 * - Swept profiles with complex features
 * - Internal features (grooves, bores)
 * - Non-manifold geometries
 * - Automatic feature detection and classification
 * 
 * The extraction process projects 3D geometry onto a 2D plane perpendicular
 * to the turning axis, creating a radius vs Z-coordinate profile suitable
 * for lathe operations.
 */
class ProfileExtractor {
public:
    /**
     * @brief Parameters controlling profile extraction
     */
    struct ExtractionParameters {
        gp_Ax1 turningAxis;                     // Main turning axis
        double profileTolerance = 0.01;        // mm - geometric tolerance
        double projectionTolerance = 0.001;    // mm - projection tolerance
        int profileSections = 100;             // number of analysis sections
        bool includeInternalFeatures = true;   // extract grooves, bores
        bool autoDetectFeatures = true;        // automatic feature classification
        bool optimizeProfile = true;           // post-process for smoothness
        double minFeatureSize = 0.1;           // mm - minimum feature size
        double angleTolerance = 0.1;           // radians - angle tolerance
        
        // Advanced options
        bool projectAllFaces = false;          // project all faces vs revolution only
        bool mergeTangentSegments = true;      // merge tangent segments
        double mergeTolerance = 0.01;          // mm - merging tolerance
    };

    /**
     * @brief Analysis result for geometry validation
     */
    struct GeometryAnalysis {
        bool isRevolutionSolid = false;        // Part is a revolution solid
        bool hasInternalFeatures = false;     // Has grooves, bores, etc.
        bool hasComplexProfile = false;       // Non-simple profile
        std::vector<TopoDS_Face> revolutionFaces;  // Cylindrical/conical faces
        std::vector<TopoDS_Face> endFaces;         // Flat end faces
        std::vector<TopoDS_Face> featureFaces;     // Internal feature faces
        double estimatedMaxRadius = 0.0;      // mm - maximum radius
        double estimatedLength = 0.0;         // mm - length along axis
        std::string geometryType;             // "simple", "complex", "invalid"
    };

    /**
     * @brief Feature classification for profile points
     */
    enum class FeatureType {
        External,       // External turning surface
        Internal,       // Internal bore or cavity
        Groove,         // Groove or undercut
        Chamfer,        // Chamfered edge
        Radius,         // Rounded corner
        Thread,         // Threaded surface
        Flat,           // Flat face (perpendicular to axis)
        Unknown         // Unclassified feature
    };

    /**
     * @brief Enhanced profile point with feature information
     */
    struct ProfilePoint {
        gp_Pnt2d position;              // (Z, Radius) coordinates
        FeatureType featureType;        // Feature classification
        double curvature = 0.0;         // Local curvature
        bool isSharpCorner = false;     // Sharp corner indicator
        TopoDS_Shape sourceGeometry;    // Source 3D geometry
        
        ProfilePoint(const gp_Pnt2d& pos, FeatureType type = FeatureType::External)
            : position(pos), featureType(type) {}
    };

    /**
     * @brief Extract 2D profile from 3D part geometry
     * @param partGeometry 3D part shape to analyze
     * @param params Extraction parameters
     * @return Extracted 2D profile points
     */
    static LatheProfile::Profile2D extractProfile(
        const TopoDS_Shape& partGeometry,
        const ExtractionParameters& params);

    /**
     * @brief Extract enhanced profile with feature classification
     * @param partGeometry 3D part shape to analyze
     * @param params Extraction parameters
     * @return Enhanced profile with feature information
     */
    static std::vector<ProfilePoint> extractEnhancedProfile(
        const TopoDS_Shape& partGeometry,
        const ExtractionParameters& params);

    /**
     * @brief Analyze geometry for revolution characteristics
     * @param partGeometry 3D part shape to analyze
     * @param turningAxis Turning axis for analysis
     * @return Geometry analysis result
     */
    static GeometryAnalysis analyzeGeometry(
        const TopoDS_Shape& partGeometry,
        const gp_Ax1& turningAxis);

    /**
     * @brief Validate extraction parameters
     * @param params Parameters to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateParameters(const ExtractionParameters& params);

    /**
     * @brief Get recommended parameters for geometry type
     * @param analysis Geometry analysis result
     * @return Recommended extraction parameters
     */
    static ExtractionParameters getRecommendedParameters(const GeometryAnalysis& analysis);

private:
    /**
     * @brief Find faces that are surfaces of revolution
     */
    static std::vector<TopoDS_Face> findRevolutionFaces(
        const TopoDS_Shape& shape, 
        const gp_Ax1& axis,
        double tolerance);

    /**
     * @brief Project 3D geometry to 2D profile plane
     */
    static std::vector<gp_Pnt2d> projectToProfile(
        const TopoDS_Shape& shape,
        const gp_Ax1& axis,
        const ExtractionParameters& params);

    /**
     * @brief Project specific face to profile
     */
    static std::vector<ProfilePoint> projectFaceToProfile(
        const TopoDS_Face& face,
        const gp_Ax1& axis,
        const ExtractionParameters& params);

    /**
     * @brief Project edge to profile points
     */
    static std::vector<ProfilePoint> projectEdgeToProfile(
        const TopoDS_Edge& edge,
        const gp_Ax1& axis,
        const ExtractionParameters& params);

    /**
     * @brief Classify surface type for feature detection
     */
    static FeatureType classifySurface(
        const BRepAdaptor_Surface& surface,
        const gp_Ax1& axis);

    /**
     * @brief Optimize profile for smoothness and accuracy
     */
    static LatheProfile::Profile2D optimizeProfile(
        const std::vector<gp_Pnt2d>& rawPoints,
        const ExtractionParameters& params);

    /**
     * @brief Merge nearby points within tolerance
     */
    static std::vector<ProfilePoint> mergeNearbyPoints(
        const std::vector<ProfilePoint>& points,
        double tolerance);

    /**
     * @brief Merge nearby points within tolerance (gp_Pnt2d version)
     */
    static std::vector<gp_Pnt2d> mergeNearbyPoints(
        const std::vector<gp_Pnt2d>& points,
        double tolerance);

    /**
     * @brief Detect and mark sharp corners in profile
     */
    static void detectAndMarkSharpCorners(
        std::vector<ProfilePoint>& points,
        double angleTolerance);

    /**
     * @brief Sort points by Z coordinate (along axis)
     */
    static std::vector<gp_Pnt2d> sortPointsByZ(const std::vector<gp_Pnt2d>& points);

    /**
     * @brief Remove duplicate points
     */
    static std::vector<gp_Pnt2d> removeDuplicates(
        const std::vector<gp_Pnt2d>& points,
        double tolerance);

    /**
     * @brief Calculate local curvature at profile points
     */
    static std::vector<double> calculateCurvature(const std::vector<gp_Pnt2d>& points);

    /**
     * @brief Convert enhanced profile to simple profile
     */
    static LatheProfile::Profile2D convertToSimpleProfile(
        const std::vector<ProfilePoint>& enhancedProfile);
};

} // namespace Toolpath
} // namespace IntuiCAM 