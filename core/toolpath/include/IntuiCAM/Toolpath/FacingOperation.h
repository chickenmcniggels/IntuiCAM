#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Professional facing operation for CNC lathe operations
 * 
 * Tool-agnostic facing operation that establishes a precise reference surface
 * by removing material from the end face of the workpiece. Uses profile extraction
 * to determine optimal facing boundaries and implements multiple cutting strategies
 * for different material types and surface quality requirements.
 * 
 * The operation works entirely in the positive X radius domain and uses
 * extracted 2D profiles for tool-agnostic implementation.
 */
class FacingOperation : public Operation {
public:
    /**
     * @brief Facing cutting strategies
     */
    enum class FacingStrategy {
        InsideOut,          ///< Face from center to outside (standard)
        OutsideIn,          ///< Face from outside to center
        Spiral,             ///< Continuous spiral facing
        Conventional,       ///< Conventional cutting direction
        Climb,              ///< Climb cutting direction
        AdaptiveRoughing,   ///< Adaptive roughing with variable stepover
        HighSpeedFacing     ///< High-speed facing with optimized parameters
    };
    
    /**
     * @brief Surface quality requirements
     */
    enum class SurfaceQuality {
        Rough,              ///< Ra 3.2-6.3 μm (125-250 μin)
        Medium,             ///< Ra 1.6-3.2 μm (63-125 μin)
        Fine,               ///< Ra 0.8-1.6 μm (32-63 μin)
        VeryFine,           ///< Ra 0.4-0.8 μm (16-32 μin)
        Mirror              ///< Ra < 0.4 μm (< 16 μin)
    };
    
    /**
     * @brief Chip control strategy
     */
    enum class ChipControl {
        None,               ///< No special chip control
        ChipBreaking,       ///< Periodic chip breaking
        HighPressureCoolant,///< High pressure coolant for chip evacuation
        CyclicFacing,       ///< Cyclic facing with pauses
        PeckFacing          ///< Peck facing with retract cycles
    };
    
    /**
     * @brief Comprehensive facing operation parameters
     */
    struct Parameters {
        // Basic geometry and positioning
        double startZ = 0.0;                        ///< Z position to start facing (mm)
        double endZ = -2.0;                         ///< Z position to end facing (mm) 
        double maxRadius = 25.0;                    ///< Maximum radius to face (mm)
        double minRadius = 0.0;                     ///< Minimum radius (center) (mm)
        double stockAllowance = 0.1;                ///< Stock allowance for roughing (mm)
        double finalStockAllowance = 0.02;          ///< Final stock allowance for finishing (mm)
        
        // Cutting strategy and parameters
        FacingStrategy strategy = FacingStrategy::InsideOut;
        SurfaceQuality surfaceQuality = SurfaceQuality::Medium;
        ChipControl chipControl = ChipControl::None;
        
        // Cutting parameters
        double depthOfCut = 0.5;                    ///< Depth of cut per pass (mm)
        double radialStepover = 0.8;                ///< Radial stepover (mm)
        double axialStepover = 0.3;                 ///< Axial stepover for multi-pass (mm)
        double feedRate = 0.15;                     ///< Feed rate (mm/rev)
        double finishingFeedRate = 0.08;            ///< Finishing pass feed rate (mm/rev)
        double roughingFeedRate = 0.25;             ///< Roughing pass feed rate (mm/rev)
        
        // Speed and feed optimization
        double surfaceSpeed = 200.0;                ///< Surface speed (m/min)
        double minSpindleSpeed = 200.0;             ///< Minimum spindle speed (RPM)
        double maxSpindleSpeed = 3000.0;            ///< Maximum spindle speed (RPM)
        bool enableConstantSurfaceSpeed = true;     ///< Enable constant surface speed
        bool adaptiveFeedRate = true;               ///< Enable adaptive feed rate
        
        // Pass management
        int numberOfRoughingPasses = 3;             ///< Number of roughing passes
        bool enableFinishingPass = true;            ///< Enable finishing pass
        bool enableSpringPass = false;              ///< Enable spring pass for precision
        double springPassFeedRate = 0.05;           ///< Spring pass feed rate (mm/rev)
        
        // Safety and clearances
        double safetyHeight = 5.0;                  ///< Safety height above part (mm)
        double clearanceDistance = 2.0;             ///< Clearance for approach/retract (mm)
        double retractDistance = 1.0;               ///< Retract distance between passes (mm)
        
        // Quality and precision control
        double profileTolerance = 0.01;             ///< Profile extraction tolerance (mm)
        double dimensionalTolerance = 0.02;         ///< Dimensional tolerance (mm)
        double surfaceRoughnessTolerance = 0.8;     ///< Surface roughness tolerance (μm)
        
        // Chip control parameters
        double chipBreakFrequency = 5.0;            ///< Chip break frequency (mm)
        double chipBreakRetract = 0.2;              ///< Chip break retract distance (mm)
        double dwellTime = 0.1;                     ///< Dwell time at corners (s)
        bool enableDwells = false;                  ///< Enable dwells for surface finish
        
        // Advanced facing options
        bool enableBackFacing = false;              ///< Enable back facing operation
        bool enableCounterBoring = false;           ///< Enable counter boring
        double counterBoreDepth = 1.0;              ///< Counter bore depth (mm)
        double counterBoreDiameter = 10.0;          ///< Counter bore diameter (mm)
        
        // Tool compensation and wear
        bool enableToolWearCompensation = false;    ///< Enable tool wear compensation
        double toolWearRate = 0.001;                ///< Tool wear rate (mm/min)
        bool enableDynamicToolCompensation = false; ///< Enable dynamic tool radius compensation
        
        // Optimization settings
        bool optimizeForCycleTime = false;          ///< Optimize for minimum cycle time
        bool optimizeForSurfaceFinish = true;       ///< Optimize for surface finish
        bool enableAdaptiveStepover = false;        ///< Enable adaptive stepover based on geometry
        double maxStepoverVariation = 0.3;          ///< Maximum stepover variation (mm)
    };
    
private:
    Parameters params_;
    
public:
    FacingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Profile-based facing generation
    std::unique_ptr<Toolpath> generateProfileBasedFacing(const LatheProfile::Profile2D& profile);
    
    // Strategy-specific implementations
    std::unique_ptr<Toolpath> generateInsideOutFacing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateOutsideInFacing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateSpiralFacing(const LatheProfile::Profile2D& profile);
    std::unique_ptr<Toolpath> generateAdaptiveFacing(const LatheProfile::Profile2D& profile);
    
    // Profile processing and optimization
    std::vector<IntuiCAM::Geometry::Point2D> extractFacingBoundary(const LatheProfile::Profile2D& profile);
    std::vector<double> calculateOptimalRadialSteps(double minRadius, double maxRadius);
    std::vector<double> calculateOptimalAxialSteps(double startZ, double endZ);
    
    // Tool path generation helpers
    void addFacingPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius, 
                       double feedRate, const std::string& description = "");
    void addSpiralPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius, 
                       double feedRate, int spiralTurns = 1);
    void addChipBreak(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& position);
    
    // Speed and feed calculations
    double calculateSpindleSpeed(double radius) const;
    double calculateAdaptiveFeedRate(double radius, double curvature = 0.0) const;
    double calculateOptimalDepthOfCut(double radius, double materialHardness = 1.0) const;
    
    // Safety and approach/retract moves
    void addApproachMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& startPoint);
    void addRetractMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& endPoint);
    void addSafetyMove(Toolpath* toolpath, const IntuiCAM::Geometry::Point3D& position);
    
    // Quality control and optimization
    void optimizeForSurfaceFinish(Toolpath* toolpath);
    void optimizeForCycleTime(Toolpath* toolpath);
    void addFinishingPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius);
    void addSpringPass(Toolpath* toolpath, double zPosition, double startRadius, double endRadius);
};

} // namespace Toolpath
} // namespace IntuiCAM 