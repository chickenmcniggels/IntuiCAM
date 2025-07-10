#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Geometry/Types.h>
#include <vector>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief External finishing operation for achieving final surface quality and dimensional accuracy
 * 
 * Implements profile-based external finishing using extracted 2D profile.
 * Focuses on surface finish, dimensional accuracy, and professional cutting strategies.
 */
class FinishingOperation : public Operation {
public:
    /**
     * @brief Finishing strategy types
     */
    enum class FinishingStrategy {
        SinglePass,         // Single finishing pass following profile
        MultiPass,          // Multiple passes with decreasing depth
        SpringPass,         // Final spring pass at exact dimension
        ClimbFinishing,     // Climb milling for better surface finish
        ConventionalFinishing // Conventional cutting direction
    };
    
    /**
     * @brief Surface finish quality levels
     */
    enum class SurfaceQuality {
        Rough,      // Ra 3.2-6.3 μm - Basic finishing
        Medium,     // Ra 1.6-3.2 μm - Standard finishing  
        Fine,       // Ra 0.8-1.6 μm - High quality finishing
        Mirror      // Ra 0.4-0.8 μm - Mirror finish
    };
    
    struct Parameters {
        // Profile and geometry parameters
        double startZ = 0.0;                    // mm - Z position to start finishing
        double endZ = -50.0;                    // mm - Z position to end finishing
        double stockAllowance = 0.05;           // mm - material left by roughing operation
        double finalStockAllowance = 0.0;       // mm - final material allowance (usually 0)
        
        // Finishing strategy
        FinishingStrategy strategy = FinishingStrategy::MultiPass;
        SurfaceQuality targetQuality = SurfaceQuality::Medium;
        bool enableSpringPass = true;           // Enable final spring pass
        int numberOfPasses = 2;                 // Number of finishing passes
        
        // Cutting parameters
        double surfaceSpeed = 200.0;            // m/min - optimized for finishing
        double feedRate = 0.08;                 // mm/rev - fine feed for finishing
        double springPassFeedRate = 0.05;       // mm/rev - slower feed for spring pass
        double depthOfCut = 0.025;              // mm - shallow cuts for finishing
        
        // Quality and precision settings
        double profileTolerance = 0.002;        // mm - tighter tolerance for finishing
        double dimensionalTolerance = 0.01;     // mm - final dimensional tolerance
        bool enableToolRadiusCompensation = true; // Enable tool radius compensation
        double toolRadiusCompensation = 0.0;    // mm - tool nose radius compensation
        
        // Speed and feed optimization
        bool enableConstantSurfaceSpeed = true; // Constant surface speed mode
        double maxSpindleSpeed = 3000.0;        // RPM - maximum spindle speed limit
        double minSpindleSpeed = 500.0;         // RPM - minimum spindle speed limit
        bool adaptiveFeedRate = true;           // Adapt feed rate based on profile complexity
        
        // Surface finish optimization
        bool enableDwells = false;              // Enable dwells for surface finish
        double dwellTime = 0.1;                 // seconds - dwell time at sharp corners
        bool minimizeToolMarks = true;          // Optimize to minimize tool marks
        double approachAngle = 3.0;             // degrees - tool approach angle
        
        // Safety parameters
        double safetyHeight = 5.0;              // mm - safe height for rapid moves
        double clearanceDistance = 1.0;         // mm - clearance from part surface
        double retractDistance = 0.5;           // mm - retract distance after cuts
        
        // Advanced finishing options
        bool enableBackCutting = false;         // Enable back cutting for undercuts
        bool followProfileContour = true;       // Follow exact profile contour
        double cornerRounding = 0.01;           // mm - corner rounding radius
        bool enableVibrationDamping = false;    // Enable vibration damping moves
        
        // Default constructor with optimized finishing parameters
        Parameters() = default;
    };
    
private:
    Parameters params_;
    
public:
    FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method for use by other operations
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Helper methods for finishing strategies
    std::unique_ptr<Toolpath> generateProfileBasedFinishing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateSinglePassFinishing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateMultiPassFinishing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateSpringPassFinishing(const LatheProfile::Profile2D& profile);
    
    // Profile processing methods
    std::vector<IntuiCAM::Geometry::Point2D> optimizeProfileForFinishing(const LatheProfile::Profile2D& profile);
    double calculateSpindleSpeed(double diameter) const;
    double calculateAdaptiveFeedRate(const IntuiCAM::Geometry::Point2D& point, 
                                   const IntuiCAM::Geometry::Point2D& nextPoint) const;
    
    // Tool path optimization
    void addFinishingMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& point, double feedRate);
    void addApproachMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& startPoint);
    void addRetractMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& endPoint);
};

} // namespace Toolpath
} // namespace IntuiCAM 