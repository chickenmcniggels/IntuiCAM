#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>

#include <iostream>
#include <sstream>

// OpenCASCADE includes
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>

namespace IntuiCAM {
namespace Toolpath {

// =====================================================================================
// Main Profile Extraction Implementation
// =====================================================================================

LatheProfile::Profile2D ProfileExtractor::extractProfile(
    const TopoDS_Shape& partGeometry,
    const ExtractionParameters& params) {
    
    // Validate parameters first
    std::string validationError = validateParameters(params);
    if (!validationError.empty()) {
        std::cout << "ProfileExtractor: Parameter validation failed: " << validationError << std::endl;
        return LatheProfile::Profile2D();
    }
    
    std::cout << "ProfileExtractor: Starting segment-based profile extraction..." << std::endl;
    
    // Use the new LatheProfile segment-based extraction
    LatheProfile::Profile2D profile = LatheProfile::extractSegmentProfile(
        partGeometry, 
        params.turningAxis, 
        params.tolerance
    );
    
    if (profile.isEmpty()) {
        std::cout << "ProfileExtractor: No profile segments extracted" << std::endl;
        return profile;
    }
    
    // Filter segments by minimum length if specified
    if (params.minSegmentLength > 0.0) {
        std::vector<LatheProfile::ProfileSegment> filteredSegments;
        for (const auto& segment : profile.segments) {
            if (segment.length >= params.minSegmentLength) {
                filteredSegments.push_back(segment);
            }
        }
        profile.segments = std::move(filteredSegments);
        
        std::cout << "ProfileExtractor: Filtered to " << profile.getSegmentCount() 
                  << " segments after minimum length filtering" << std::endl;
    }
    
    // Sort segments if requested
    if (params.sortSegments) {
        LatheProfile::sortSegmentsByZ(profile.segments);
        std::cout << "ProfileExtractor: Segments sorted by Z coordinate" << std::endl;
    }
    
    std::cout << "ProfileExtractor: Profile extraction completed successfully with " 
              << profile.getSegmentCount() << " segments and total length " 
              << profile.getTotalLength() << std::endl;
    
    return profile;
}

// =====================================================================================
// Parameter Validation and Utilities
// =====================================================================================

std::string ProfileExtractor::validateParameters(const ExtractionParameters& params) {
    std::ostringstream errors;
    
    // Check tolerance values
    if (params.tolerance <= 0.0) {
        errors << "Tolerance must be positive; ";
    }
    
    if (params.tolerance > 10.0) {
        errors << "Tolerance seems too large (>10mm); ";
    }
    
    if (params.minSegmentLength < 0.0) {
        errors << "Minimum segment length cannot be negative; ";
    }
    
    if (params.minSegmentLength > params.tolerance * 100) {
        errors << "Minimum segment length seems too large compared to tolerance; ";
    }
    
    // Check turning axis validity
    gp_Dir axisDirection = params.turningAxis.Direction();
    // gp_Dir is always a unit vector, so no need to check magnitude
    // Just verify the direction is reasonable (not degenerate)
    try {
        // This will throw if the direction is invalid
        gp_Vec testVec(axisDirection);
        if (testVec.SquareMagnitude() < Precision::SquareConfusion()) {
            errors << "Invalid turning axis direction; ";
        }
    } catch (...) {
        errors << "Invalid turning axis direction; ";
    }
    
    std::string errorString = errors.str();
    if (!errorString.empty()) {
        // Remove trailing "; "
        errorString = errorString.substr(0, errorString.length() - 2);
    }
    
    return errorString;
}

ProfileExtractor::ExtractionParameters ProfileExtractor::getRecommendedParameters(bool highPrecision) {
    ExtractionParameters params;
    
    // Default Z-axis setup
    params.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    
    if (highPrecision) {
        // High precision settings for small or detailed parts
        params.tolerance = 0.001;           // 0.001mm tolerance
        params.minSegmentLength = 0.0001;   // 0.0001mm minimum segment length
        params.sortSegments = true;
        
        std::cout << "ProfileExtractor: Using high precision parameters" << std::endl;
    } else {
        // Standard settings for typical parts
        params.tolerance = 0.01;            // 0.01mm tolerance
        params.minSegmentLength = 0.001;    // 0.001mm minimum segment length
        params.sortSegments = true;
        
        std::cout << "ProfileExtractor: Using standard precision parameters" << std::endl;
    }
    
    return params;
}

} // namespace Toolpath
} // namespace IntuiCAM 