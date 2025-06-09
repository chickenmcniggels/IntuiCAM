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
        Parting
    };

    // Parameter structures for each operation type
    struct FacingParameters {
        double stepover = 0.5;           // mm
        double feedRate = 100.0;         // mm/min
        double spindleSpeed = 1200.0;    // RPM
        double stockAllowance = 0.2;     // mm
        bool useClimbing = true;         // Climbing vs conventional milling
        bool roughingOnly = false;       // Skip finishing pass
    };

    struct RoughingParameters {
        double depthOfCut = 2.0;         // mm per pass
        double stockAllowance = 0.5;     // mm for finishing
        double feedRate = 150.0;         // mm/min
        double spindleSpeed = 1000.0;    // RPM
        double stepover = 75.0;          // % of tool diameter
        bool adaptiveClearing = true;    // Adaptive toolpath strategy
        bool useHelicalEntry = true;     // Helical vs ramping entry
    };

    struct FinishingParameters {
        double targetSurfaceFinish = 3.2;  // μm Ra
        double feedRate = 80.0;            // mm/min
        double spindleSpeed = 1500.0;      // RPM
        double axialDepthOfCut = 0.2;      // mm
        double radialStepover = 0.1;       // mm
        bool useSpindleSpeedControl = true; // Constant surface speed
        bool multipleSpringPasses = false;  // Multiple spring passes
        int springPassCount = 2;           // Number of spring passes
    };

    struct PartingParameters {
        double feedRate = 30.0;            // mm/min
        double spindleSpeed = 800.0;       // RPM
        double peckinDepth = 0.5;          // mm (for pecking cycle)
        double retractDistance = 2.0;      // mm
        double dwellTime = 0.5;            // seconds at full depth
        bool usePeckingCycle = true;       // Use pecking vs straight plunge
        bool useFloodCoolant = true;       // Coolant control
        double safetyMargin = 1.0;         // mm from part edge
    };

    explicit OperationParameterDialog(OperationType operationType, 
                                     QWidget *parent = nullptr);
    ~OperationParameterDialog();

    // Parameter getters
    FacingParameters getFacingParameters() const;
    RoughingParameters getRoughingParameters() const;
    FinishingParameters getFinishingParameters() const;
    PartingParameters getPartingParameters() const;

    // Parameter setters
    void setFacingParameters(const FacingParameters& params);
    void setRoughingParameters(const RoughingParameters& params);
    void setFinishingParameters(const FinishingParameters& params);
    void setPartingParameters(const PartingParameters& params);

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