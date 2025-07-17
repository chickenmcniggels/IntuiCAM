#ifndef OPERATIONPARAMETERDIALOG_H
#define OPERATIONPARAMETERDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QTextEdit>
#include <QTabWidget>

namespace IntuiCAM {
namespace GUI {
    enum class MaterialType;
    enum class SurfaceFinish;
}
}

namespace IntuiCAM {
namespace GUI {

/**
 * @brief Modal dialog for configuring operation-specific parameters
 * 
 * This dialog provides detailed parameter configuration for each machining operation:
 * - Facing: stepover, feed rate, spindle speed, strategy
 * - Roughing: depth of cut, stock allowance, feed rate, speed optimization
 * - Finishing: surface finish, final dimensions, cutting speed, tool path strategy
 * - Parting: parting position, feed rate, safety settings, retract distance
 */
class OperationParameterDialog : public QDialog
{
    Q_OBJECT

public:
    enum class OperationType {
        Facing,
        Roughing,
        Finishing,
        Parting,
        Threading,
        Chamfering,
        Grooving,
        Drilling
    };

    // Parameter structures for each operation type
    struct FacingParameters {
        // Basic geometry and positioning (from raw material and part analysis)
        double startZ = 0.0;                        // Z position to start facing (mm)
        double endZ = -2.0;                         // Z position to end facing (mm) 
        double maxRadius = 25.0;                    // Maximum radius to face (mm)
        double minRadius = 0.0;                     // Minimum radius (center) (mm)
        double stockAllowance = 0.1;                // Stock allowance for roughing (mm)
        double finalStockAllowance = 0.02;          // Final stock allowance for finishing (mm)
        
        // Cutting strategy and parameters
        int facingStrategy = 0;                     // 0=InsideOut, 1=OutsideIn, 2=Conventional, 3=Climb
        int surfaceQuality = 1;                     // 0=Rough, 1=Medium, 2=Fine, 3=Mirror
        int chipControl = 0;                        // 0=None, 1=ChipBreaking, 2=HighPressureCoolant
        
        // Cutting parameters
        double depthOfCut = 0.5;                    // Depth of cut per pass (mm)
        double radialStepover = 0.8;                // Radial stepover (mm)
        double axialStepover = 0.3;                 // Axial stepover for multi-pass (mm)
        double feedRate = 0.15;                     // Primary feed rate (mm/rev)
        double finishingFeedRate = 0.08;            // Finishing pass feed rate (mm/rev)
        double roughingFeedRate = 0.25;             // Roughing pass feed rate (mm/rev)
        
        // Speed and feed optimization
        double surfaceSpeed = 200.0;                // Surface speed (m/min)
        double minSpindleSpeed = 200.0;             // Minimum spindle speed (RPM)
        double maxSpindleSpeed = 3000.0;            // Maximum spindle speed (RPM)
        bool enableConstantSurfaceSpeed = true;     // Enable constant surface speed
        bool adaptiveFeedRate = true;               // Enable adaptive feed rate
        
        // Pass management
        int numberOfRoughingPasses = 3;             // Number of roughing passes
        bool enableFinishingPass = true;            // Enable finishing pass
        bool enableSpringPass = false;              // Enable spring pass for precision
        double springPassFeedRate = 0.05;           // Spring pass feed rate (mm/rev)
        
        // Safety and clearances
        double safetyHeight = 5.0;                  // Safety height above part (mm)
        double clearanceDistance = 2.0;             // Clearance for approach/retract (mm)
        double retractDistance = 1.0;               // Retract distance between passes (mm)
        
        // Quality and precision control
        double profileTolerance = 0.01;             // Profile extraction tolerance (mm)
        double dimensionalTolerance = 0.02;         // Dimensional tolerance (mm)
        double surfaceRoughnessTolerance = 0.8;     // Surface roughness tolerance (μm)
        
        // Chip control parameters
        double chipBreakFrequency = 5.0;            // Chip break frequency (mm)
        double chipBreakRetract = 0.2;              // Chip break retract distance (mm)
        double dwellTime = 0.1;                     // Dwell time at corners (s)
        bool enableDwells = false;                  // Enable dwells for surface finish
        
        // Advanced facing options
        bool enableBackFacing = false;              // Enable back facing operation
        bool enableCounterBoring = false;           // Enable counter boring
        double counterBoreDepth = 1.0;              // Counter bore depth (mm)
        double counterBoreDiameter = 10.0;          // Counter bore diameter (mm)
        
        // Tool compensation and wear
        bool enableToolWearCompensation = false;    // Enable tool wear compensation
        double toolWearRate = 0.001;                // Tool wear rate (mm/min)
        bool enableDynamicToolCompensation = false; // Enable dynamic tool radius compensation
        
        // Legacy compatibility parameters (to be phased out)
        double stepover = 0.5;                      // Legacy: use radialStepover instead
        double spindleSpeed = 1200.0;               // Legacy: calculated from surface speed
        bool useClimbing = true;                    // Legacy: use facingStrategy instead
        bool roughingOnly = false;                  // Legacy: use enableFinishingPass instead
    };

    struct RoughingParameters {
        // Geometry parameters (calculated from part and raw material)
        double startDiameter = 50.0;               // Starting diameter (raw material) (mm)
        double endDiameter = 20.0;                 // Final diameter (part) (mm)
        double startZ = 0.0;                       // Z position to start (mm)
        double endZ = -40.0;                       // Z position to end (mm)
        bool isInternal = false;                   // true for internal roughing, false for external
        
        // Cutting strategy
        double depthOfCut = 2.0;                   // Axial depth per pass (mm)
        double stepover = 1.5;                     // Radial stepover (mm)
        double stockAllowance = 0.5;               // Material left for finishing (mm)
        
        // Process parameters
        double feedRate = 120.0;                   // Roughing feed rate (mm/min)
        double spindleSpeed = 800.0;               // Spindle speed (RPM)
        double safetyHeight = 5.0;                 // Safe height above part (mm)
        
        // Strategy options
        bool useProfileFollowing = true;           // Follow part profile instead of simple cylinder
        bool enableChipBreaking = true;            // Enable chip breaking retracts
        double chipBreakDistance = 0.5;            // Retract distance for chip breaking (mm)
        bool reversePass = false;                  // Reverse direction for alternate passes
        bool useClimbMilling = false;              // Use climb milling if applicable (internal)
        
        // Speed and feed optimization
        bool enableConstantSurfaceSpeed = false;   // Enable constant surface speed
        double maxSpindleSpeed = 2000.0;           // Maximum spindle speed limit (RPM)
        double minSpindleSpeed = 200.0;            // Minimum spindle speed limit (RPM)
        bool adaptiveFeedRate = true;              // Adapt feed based on material removal
        
        // Quality and precision
        double profileTolerance = 0.05;            // Profile following tolerance (mm)
        double dimensionalTolerance = 0.1;         // Dimensional tolerance (mm)
        
        // Advanced options
        bool enableRoughingGrooves = false;        // Create relief grooves during roughing
        double grooveSpacing = 10.0;               // Spacing between relief grooves (mm)
        double grooveDepth = 1.0;                  // Depth of relief grooves (mm)
        
        // Legacy compatibility parameters (to be phased out)
        double stepoverPercent = 75.0;             // Legacy: % of tool diameter
        bool adaptiveClearing = true;              // Legacy: use useProfileFollowing instead
        bool useHelicalEntry = true;               // Legacy: not applicable to lathe operations
    };

    struct FinishingParameters {
        // Profile and geometry parameters
        double startZ = 0.0;                       // Z position to start finishing (mm)
        double endZ = -50.0;                       // Z position to end finishing (mm)
        double stockAllowance = 0.05;              // Material left by roughing operation (mm)
        double finalStockAllowance = 0.0;          // Final material allowance (mm)
        
        // Finishing strategy
        int finishingStrategy = 1;                 // 0=SinglePass, 1=MultiPass, 2=ProfileFollowing, 3=AdaptiveFinishing
        int targetQuality = 1;                     // 0=Rough, 1=Medium, 2=Fine, 3=Mirror
        bool enableSpringPass = true;              // Enable final spring pass
        int numberOfPasses = 2;                    // Number of finishing passes
        
        // Cutting parameters
        double surfaceSpeed = 200.0;               // Surface speed (m/min)
        double feedRate = 0.08;                    // Primary feed rate (mm/rev)
        double springPassFeedRate = 0.05;          // Spring pass feed rate (mm/rev)
        double depthOfCut = 0.025;                 // Depth of cut per pass (mm)
        
        // Quality and precision settings
        double profileTolerance = 0.002;           // Profile extraction tolerance (mm)
        double dimensionalTolerance = 0.01;        // Final dimensional tolerance (mm)
        bool enableToolRadiusCompensation = true;  // Enable tool radius compensation
        double toolRadiusCompensation = 0.0;       // Tool nose radius compensation (mm)
        
        // Speed and feed optimization
        bool enableConstantSurfaceSpeed = true;    // Constant surface speed mode
        double maxSpindleSpeed = 3000.0;           // Maximum spindle speed (RPM)
        double minSpindleSpeed = 500.0;            // Minimum spindle speed limit (RPM)
        bool adaptiveFeedRate = true;              // Adapt feed rate based on profile complexity
        
        // Surface finish optimization
        bool enableDwells = false;                 // Enable dwells for surface finish
        double dwellTime = 0.1;                    // Dwell time at sharp corners (seconds)
        bool minimizeToolMarks = true;             // Optimize to minimize tool marks
        double approachAngle = 3.0;                // Tool approach angle (degrees)
        
        // Safety parameters
        double safetyHeight = 5.0;                 // Safe height for rapid moves (mm)
        double clearanceDistance = 1.0;            // Clearance from part surface (mm)
        double retractDistance = 0.5;              // Retract distance after cuts (mm)
        
        // Advanced finishing options
        bool enableBackCutting = false;            // Enable back cutting for undercuts
        bool followProfileContour = true;          // Follow exact profile contour
        double cornerRounding = 0.01;              // Corner rounding radius (mm)
        bool enableVibrationDamping = false;       // Enable vibration damping moves
        
        // Legacy compatibility parameters (to be phased out)
        double targetSurfaceFinish = 3.2;          // Legacy: μm Ra - use targetQuality instead
        double spindleSpeed = 1500.0;              // Legacy: calculated from surface speed
        double axialDepthOfCut = 0.2;              // Legacy: use depthOfCut instead
        double radialStepover = 0.1;               // Legacy: calculated from strategy
        bool useSpindleSpeedControl = true;        // Legacy: use enableConstantSurfaceSpeed instead
        bool multipleSpringPasses = false;         // Legacy: use enableSpringPass instead
        int springPassCount = 2;                   // Legacy: use numberOfPasses instead
    };

    struct PartingParameters {
        // Basic parting geometry (calculated from part analysis and user input)
        double partingDiameter = 20.0;             // Diameter to part at (mm)
        double partingZ = -40.0;                   // Z position for parting (mm)
        double centerHoleDiameter = 0.0;           // Center hole diameter (0 for solid) (mm)
        double partingWidth = 3.0;                 // Width of parting cut (mm)
        
        // Parting strategy
        int partingStrategy = 0;                   // 0=Straight, 1=Stepped, 2=Groove, 3=Undercut, 4=Trepanning
        int approachDirection = 0;                 // 0=Radial, 1=Axial, 2=Angular
        
        // Cutting parameters
        double feedRate = 30.0;                    // Parting feed rate (mm/min)
        double spindleSpeed = 800.0;               // Spindle speed (RPM)
        double depthOfCut = 0.5;                   // Radial depth of cut per pass (mm)
        int numberOfPasses = 1;                    // Number of parting passes
        
        // Safety and clearance
        double safetyHeight = 5.0;                 // Safe height for rapid moves (mm)
        double clearanceDistance = 1.0;            // Clearance from part surface (mm)
        double retractDistance = 5.0;              // Retract distance after cut (mm)
        
        // Finishing parameters
        double finishingAllowance = 0.1;           // Material left for finishing pass (mm)
        bool enableFinishingPass = true;           // Enable final finishing pass
        double finishingFeedRate = 25.0;           // Finishing pass feed rate (mm/min)
        
        // Quality settings
        bool enableCoolant = true;                 // Enable coolant during parting
        bool enableChipBreaking = true;            // Enable chip breaking
        double chipBreakDistance = 2.0;            // Chip break retract distance (mm)
        
        // Advanced options
        bool useConstantSurfaceSpeed = false;      // Use constant surface speed
        double maxSpindleSpeed = 1500.0;           // Maximum spindle speed limit (RPM)
        bool enableRoughingGroove = false;         // Create relief groove before parting
        double grooveWidth = 2.0;                  // Relief groove width (mm)
        double grooveDepth = 1.0;                  // Relief groove depth (mm)
        
        // Quality control
        double partingTolerance = 0.05;            // Parting dimensional tolerance (mm)
        bool chamferPartingEdges = false;          // Chamfer parting cut edges
        double chamferSize = 0.2;                  // Chamfer size (mm)
        
        // Advanced parting options
        bool enableProgressiveCutting = false;     // Progressive depth cutting for large diameters
        double maxCutDepth = 2.0;                  // Maximum cut depth per pass (mm)
        bool enableBackChamfer = false;            // Chamfer back side of parting cut
        
        // Legacy compatibility parameters (to be phased out)
        double peckinDepth = 0.5;                  // Legacy: typo - should be "peckDepth"
        double dwellTime = 0.5;                    // Legacy: not typically used in parting
        bool usePeckingCycle = true;               // Legacy: use enableChipBreaking instead
        bool useFloodCoolant = true;               // Legacy: use enableCoolant instead
        double safetyMargin = 1.0;                 // Legacy: use clearanceDistance instead
    };

    struct ThreadingParameters {
        // Thread specifications
        int threadForm = 0;                        // 0=Metric, 1=UNC, 2=UNF, 3=BSW, 4=ACME, 5=Trapezoidal, 6=Custom
        int threadType = 0;                        // 0=External, 1=Internal
        int cuttingMethod = 0;                     // 0=SinglePoint, 1=MultiPoint, 2=ChaseThreading
        
        double majorDiameter = 20.0;               // Major diameter (mm)
        double pitch = 1.5;                        // Thread pitch (mm)
        double threadDepth = 0.9;                  // Thread depth (mm)
        double threadLength = 15.0;                // Length of threaded section (mm)
        double startZ = 0.0;                       // Z position to start threading (mm)
        double endZ = -15.0;                       // Z position to end threading (mm)
        
        // Threading strategy
        int numberOfPasses = 3;                    // Number of threading passes
        bool constantDepthPasses = false;          // Use constant depth per pass
        bool variableDepthPasses = true;           // Use variable depth (spring cuts)
        double degression = 0.7;                   // Degression factor for variable depth
        
        // Cutting parameters
        double feedRate = 60.0;                    // Threading feed rate (mm/min)
        double spindleSpeed = 400.0;               // Spindle speed (RPM)
        double leadInDistance = 3.0;               // Lead-in distance (mm)
        double leadOutDistance = 3.0;              // Lead-out distance (mm)
        double safetyHeight = 5.0;                 // Safe height above part (mm)
        double clearanceDistance = 2.0;            // Clearance from part (mm)
        double retractDistance = 1.0;              // Retract distance for passes (mm)
        
        // Quality and finishing
        double threadTolerance = 0.05;             // Thread tolerance (mm)
        bool chamferThreadStart = true;            // Chamfer thread start
        bool chamferThreadEnd = true;              // Chamfer thread end
        double chamferLength = 0.5;                // Chamfer length (mm)
        
        // Advanced options
        bool useConstantSurfaceSpeed = false;      // Use constant surface speed
        double maxSpindleSpeed = 800.0;            // Maximum spindle speed limit (RPM)
        bool enableCoolant = true;                 // Enable coolant during threading
        bool enableChipBreaking = false;           // Enable chip breaking
        double chipBreakDistance = 0.3;            // Chip break retract distance (mm)
    };

    struct ChamferingParameters {
        // Chamfer type and geometry
        int chamferType = 0;                       // 0=Linear, 1=Radius, 2=CustomAngle
        double chamferSize = 0.5;                  // Size of chamfer (mm)
        double chamferAngle = 45.0;                // Angle of chamfer (degrees)
        double radiusSize = 0.5;                   // Radius size for radius chamfers (mm)
        
        // Position and geometry
        double startZ = 0.0;                       // Z position of chamfer start (mm)
        double startDiameter = 20.0;               // Diameter at chamfer start (mm)
        double endDiameter = 18.0;                 // Diameter at chamfer end (mm)
        bool isExternal = true;                    // true for external, false for internal
        bool isFrontFace = true;                   // true for front face, false for back face
        
        // Cutting parameters
        double feedRate = 100.0;                   // Chamfering feed rate (mm/min)
        double spindleSpeed = 1000.0;              // Spindle speed (RPM)
        double depthOfCut = 0.1;                   // Depth of cut per pass (mm)
        int numberOfPasses = 1;                    // Number of chamfering passes
        
        // Safety and clearance
        double safetyHeight = 5.0;                 // Safe height above part (mm)
        double clearanceDistance = 1.0;            // Clearance from part surface (mm)
        double retractDistance = 2.0;              // Retract distance after chamfer (mm)
        
        // Quality and finishing
        double chamferTolerance = 0.02;            // Chamfer tolerance (mm)
        bool enableSpringPass = false;             // Enable spring pass for precision
        double springPassFeedRate = 80.0;          // Spring pass feed rate (mm/min)
        
        // Advanced options
        bool useConstantSurfaceSpeed = false;      // Use constant surface speed
        double maxSpindleSpeed = 2000.0;           // Maximum spindle speed limit (RPM)
        bool enableCoolant = false;                // Enable coolant during chamfering
    };

    struct GroovingParameters {
        // Groove geometry
        double grooveDiameter = 20.0;              // Diameter for groove (mm)
        double grooveWidth = 3.0;                  // Width of groove (mm)
        double grooveDepth = 2.0;                  // Depth of groove (mm)
        double grooveZ = -25.0;                    // Z position of groove (mm)
        bool isInternal = false;                   // true for internal groove, false for external
        
        // Groove profile
        int grooveProfile = 0;                     // 0=Rectangular, 1=VShape, 2=UShape, 3=Custom
        double grooveAngle = 90.0;                 // Groove side angle (degrees)
        double cornerRadius = 0.1;                 // Corner radius for groove (mm)
        
        // Cutting parameters
        double feedRate = 0.02;                    // Grooving feed rate (mm/rev)
        double spindleSpeed = 600.0;               // Spindle speed (RPM)
        double depthOfCut = 0.3;                   // Radial depth per pass (mm)
        int numberOfPasses = 1;                    // Number of grooving passes
        
        // Process strategy
        int groovingStrategy = 0;                  // 0=Plunge, 1=Pecking, 2=Progressive
        double peckDepth = 0.5;                    // Peck depth for pecking strategy (mm)
        double retractDistance = 1.0;              // Retract distance between pecks (mm)
        double dwellTime = 0.2;                    // Dwell time at groove bottom (seconds)
        
        // Safety and clearance
        double safetyHeight = 5.0;                 // Safe height above part (mm)
        double clearanceDistance = 1.0;            // Clearance from part surface (mm)
        
        // Quality and finishing
        double grooveTolerance = 0.02;             // Groove tolerance (mm)
        bool enableFinishingPass = false;          // Enable finishing pass
        double finishingFeedRate = 0.01;           // Finishing pass feed rate (mm/rev)
        
        // Advanced options
        bool enableCoolant = true;                 // Enable coolant during grooving
        bool enableChipBreaking = true;            // Enable chip breaking
        double chipBreakFrequency = 0.8;           // Chip break frequency (mm)
    };

    struct DrillingParameters {
        // Hole geometry
        double holeDiameter = 6.0;                 // Diameter of hole to drill (mm)
        double holeDepth = 20.0;                   // Depth of hole (mm)
        double startZ = 0.0;                       // Z position of hole start (mm)
        bool throughHole = false;                  // true for through hole, false for blind hole
        
        // Drilling strategy
        int drillingStrategy = 0;                  // 0=Simple, 1=Peck, 2=DeepHole, 3=HighSpeed
        double peckDepth = 5.0;                    // Depth per peck for deep holes (mm)
        double retractHeight = 2.0;                // Retract height for chip clearing (mm)
        double dwellTime = 0.5;                    // Dwell time at bottom of hole (seconds)
        bool usePeckDrilling = true;               // Enable peck drilling for deep holes
        bool useChipBreaking = true;               // Enable chip breaking retracts
        
        // Cutting parameters
        double feedRate = 100.0;                   // Drilling feed rate (mm/min)
        double spindleSpeed = 1200.0;              // Spindle speed (RPM)
        double safetyHeight = 5.0;                 // Safe height above part (mm)
        
        // Quality and finishing
        double holeTolerance = 0.05;               // Hole diameter tolerance (mm)
        bool enableChamfer = false;                // Chamfer hole entrance
        double chamferSize = 0.3;                  // Entrance chamfer size (mm)
        bool enableCountersink = false;            // Enable countersink
        double countersinkDiameter = 12.0;         // Countersink diameter (mm)
        double countersinkDepth = 2.0;             // Countersink depth (mm)
        
        // Advanced options
        bool enableCoolant = true;                 // Enable coolant during drilling
        bool useRigidTapping = false;              // Use rigid tapping (for tapped holes)
        double tapPitch = 1.0;                     // Tap pitch for threaded holes (mm)
    };

    explicit OperationParameterDialog(OperationType operationType, 
                                     QWidget *parent = nullptr);
    ~OperationParameterDialog();

    // Parameter getters
    FacingParameters getFacingParameters() const;
    RoughingParameters getRoughingParameters() const;
    FinishingParameters getFinishingParameters() const;
    PartingParameters getPartingParameters() const;
    ThreadingParameters getThreadingParameters() const;
    ChamferingParameters getChamferingParameters() const;
    GroovingParameters getGroovingParameters() const;
    DrillingParameters getDrillingParameters() const;

    // Parameter setters
    void setFacingParameters(const FacingParameters& params);
    void setRoughingParameters(const RoughingParameters& params);
    void setFinishingParameters(const FinishingParameters& params);
    void setPartingParameters(const PartingParameters& params);
    void setThreadingParameters(const ThreadingParameters& params);
    void setChamferingParameters(const ChamferingParameters& params);
    void setGroovingParameters(const GroovingParameters& params);
    void setDrillingParameters(const DrillingParameters& params);

    // Context information
    void setMaterialType(IntuiCAM::GUI::MaterialType material);
    void setPartDiameter(double diameter);
    void setPartLength(double length);

public slots:
    void onParameterChanged();
    void onResetToDefaults();
    void onLoadPreset();
    void onSavePreset();
    void onCalculateOptimalSpeeds();

signals:
    void parametersChanged();

private:
    void setupUI();
    void setupFacingUI();
    void setupRoughingUI();
    void setupFinishingUI();
    void setupPartingUI();
    void setupThreadingUI();
    void setupChamferingUI();
    void setupGroovingUI();
    void setupDrillingUI();
    void setupCommonUI();
    void updateCalculatedValues();
    void applyMaterialDefaults();
    void validateParameters();

    // Operation type and parameters
    OperationType m_operationType;
    FacingParameters m_facingParams;
    RoughingParameters m_roughingParams;
    FinishingParameters m_finishingParams;
    PartingParameters m_partingParams;
    ThreadingParameters m_threadingParams;
    ChamferingParameters m_chamferingParams;
    GroovingParameters m_groovingParams;
    DrillingParameters m_drillingParams;

    // Context information
    IntuiCAM::GUI::MaterialType m_materialType;
    double m_partDiameter;
    double m_partLength;

    // Main layout
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;
    
    // Parameter tabs
    QWidget* m_parametersTab;
    QWidget* m_advancedTab;
    QWidget* m_presetsTab;

    // Facing operation UI components
    QGroupBox* m_facingBasicGroup;
    QDoubleSpinBox* m_facingStepoverSpin;
    QDoubleSpinBox* m_facingFeedRateSpin;
    QDoubleSpinBox* m_facingSpindleSpeedSpin;
    QDoubleSpinBox* m_facingStockAllowanceSpin;
    QCheckBox* m_facingClimbingCheck;
    QCheckBox* m_facingRoughingOnlyCheck;

    // Roughing operation UI components
    QGroupBox* m_roughingBasicGroup;
    QDoubleSpinBox* m_roughingDepthOfCutSpin;
    QDoubleSpinBox* m_roughingStockAllowanceSpin;
    QDoubleSpinBox* m_roughingFeedRateSpin;
    QDoubleSpinBox* m_roughingSpindleSpeedSpin;
    QDoubleSpinBox* m_roughingStepoverSpin;
    QCheckBox* m_roughingAdaptiveCheck;
    QCheckBox* m_roughingHelicalEntryCheck;

    // Finishing operation UI components
    QGroupBox* m_finishingBasicGroup;
    QDoubleSpinBox* m_finishingSurfaceFinishSpin;
    QDoubleSpinBox* m_finishingFeedRateSpin;
    QDoubleSpinBox* m_finishingSpindleSpeedSpin;
    QDoubleSpinBox* m_finishingAxialDepthSpin;
    QDoubleSpinBox* m_finishingRadialStepoverSpin;
    QCheckBox* m_finishingSpindleControlCheck;
    QCheckBox* m_finishingSpringPassesCheck;
    QSpinBox* m_finishingSpringPassCountSpin;

    // Parting operation UI components
    QGroupBox* m_partingBasicGroup;
    QDoubleSpinBox* m_partingFeedRateSpin;
    QDoubleSpinBox* m_partingSpindleSpeedSpin;
    QDoubleSpinBox* m_partingPeckingDepthSpin;
    QDoubleSpinBox* m_partingRetractDistanceSpin;
    QDoubleSpinBox* m_partingDwellTimeSpin;
    QCheckBox* m_partingPeckingCycleCheck;
    QCheckBox* m_partingFloodCoolantCheck;
    QDoubleSpinBox* m_partingSafetyMarginSpin;

    // Common UI components
    QGroupBox* m_calculatedValuesGroup;
    QLabel* m_calculatedSpeedLabel;
    QLabel* m_calculatedTimeLabel;
    QLabel* m_materialRemovalRateLabel;

    // Advanced parameters
    QGroupBox* m_advancedGroup;
    QComboBox* m_toolMaterialCombo;
    QComboBox* m_coolantModeCombo;
    QDoubleSpinBox* m_toolWearFactorSpin;
    QCheckBox* m_adaptiveFeedCheck;

    // Preset management
    QGroupBox* m_presetsGroup;
    QComboBox* m_presetCombo;
    QPushButton* m_loadPresetButton;
    QPushButton* m_savePresetButton;
    QPushButton* m_deletePresetButton;

    // Control buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_resetButton;
    QPushButton* m_calculateButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Calculated values display
    QTextEdit* m_calculationsDisplay;

    // Static data
    static const QStringList TOOL_MATERIALS;
    static const QStringList COOLANT_MODES;
    static const QMap<QString, FacingParameters> FACING_PRESETS;
    static const QMap<QString, RoughingParameters> ROUGHING_PRESETS;
    static const QMap<QString, FinishingParameters> FINISHING_PRESETS;
    static const QMap<QString, PartingParameters> PARTING_PRESETS;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // OPERATIONPARAMETERDIALOG_H 