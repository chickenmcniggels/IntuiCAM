#pragma once

#include <vector>
#include <string>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>

// IntuiCAM includes
#include <IntuiCAM/Toolpath/LatheProfile.h>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Segment-based 2D profile extraction from 3D part geometry
 * 
 * This class provides segment-based extraction of 2D lathe profiles from arbitrary
 * 3D part geometries using OCCT sectioning. It preserves the original geometric
 * segments rather than approximating with sample points.
 * 
 * The extraction process uses OCCT's sectioning algorithms to create a cross-section
 * through the XZ-plane centered on the turning axis, then extracts all edges that
 * are in the positive X direction from the axis.
 */
class ProfileExtractor {
public:
    /**
     * @brief Parameters controlling profile extraction
     */
    struct ExtractionParameters {
        gp_Ax1 turningAxis;                     // Main turning axis (typically Z-axis)
        double tolerance = 0.01;                // mm - geometric tolerance for sectioning
        double minSegmentLength = 0.001;       // mm - minimum segment length to include
        bool sortSegments = true;               // Sort segments by Z coordinate
        
        // Constructor with default Z-axis
        ExtractionParameters() : turningAxis(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1))) {}
        ExtractionParameters(const gp_Ax1& axis, double tol = 0.01) 
            : turningAxis(axis), tolerance(tol) {}
    };

    /**
     * @brief Extract segment-based 2D profile from 3D part geometry
     * @param partGeometry 3D part shape to analyze
     * @param params Extraction parameters
     * @return Extracted segment-based profile
     */
    static LatheProfile::Profile2D extractProfile(
        const TopoDS_Shape& partGeometry,
        const ExtractionParameters& params);

    /**
     * @brief Validate extraction parameters
     * @param params Parameters to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateParameters(const ExtractionParameters& params);

    /**
     * @brief Get recommended parameters for common lathe setups
     * @param highPrecision True for high precision/small parts, false for standard
     * @return Recommended extraction parameters
     */
    static ExtractionParameters getRecommendedParameters(bool highPrecision = false);

    // =====================================================================================
    // Legacy Support - DEPRECATED - Use LatheProfile::extractSegmentProfile directly
    // =====================================================================================
    
    /**
     * @brief Legacy feature classification for backward compatibility
     * @deprecated This is no longer used in segment-based extraction
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
     * @brief Legacy profile point structure for backward compatibility
     * @deprecated Use ProfileSegment instead
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
     * @brief Legacy analysis result structure
     * @deprecated No longer used in segment-based extraction
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
};

} // namespace Toolpath
} // namespace IntuiCAM 