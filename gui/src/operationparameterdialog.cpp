#define _USE_MATH_DEFINES
#include "operationparameterdialog.h"
#include "setupconfigurationpanel.h"

#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Static data initialization
const QStringList IntuiCAM::GUI::OperationParameterDialog::TOOL_MATERIALS = {
    "High Speed Steel (HSS)", "Carbide", "Ceramic", "Diamond", "CBN"
};

const QStringList IntuiCAM::GUI::OperationParameterDialog::COOLANT_MODES = {
    "Flood", "Mist", "Air Blast", "Through Spindle", "None"
};

IntuiCAM::GUI::OperationParameterDialog::OperationParameterDialog(OperationType operationType, QWidget *parent)
    : QDialog(parent)
    , m_operationType(operationType)
    , m_partDiameter(25.0)
    , m_partLength(50.0)
{
    setModal(true);
    setMinimumSize(600, 500);
    
    // Set dialog title based on operation type
    QString title;
    switch (operationType) {
        case OperationType::Facing: title = "Facing Operation Parameters"; break;
        case OperationType::Roughing: title = "Roughing Operation Parameters"; break;
        case OperationType::Finishing: title = "Finishing Operation Parameters"; break;
        case OperationType::Parting: title = "Parting Operation Parameters"; break;
        case OperationType::Threading: title = "Threading Operation Parameters"; break;
        case OperationType::Chamfering: title = "Chamfering Operation Parameters"; break;
        case OperationType::Grooving: title = "Grooving Operation Parameters"; break;
        case OperationType::Drilling: title = "Drilling Operation Parameters"; break;
    }
    setWindowTitle(title);
    
    setupUI();
    updateCalculatedValues();
}

IntuiCAM::GUI::OperationParameterDialog::~OperationParameterDialog()
{
    // Qt handles cleanup automatically
}

void IntuiCAM::GUI::OperationParameterDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);

    // Create tab widget for organized parameters
    m_tabWidget = new QTabWidget();
    
    // Create tabs
    m_parametersTab = new QWidget();
    m_advancedTab = new QWidget();
    m_presetsTab = new QWidget();
    
    m_tabWidget->addTab(m_parametersTab, "Basic Parameters");
    m_tabWidget->addTab(m_advancedTab, "Advanced");
    m_tabWidget->addTab(m_presetsTab, "Presets");
    
    // Setup operation-specific UI
    switch (m_operationType) {
        case OperationType::Facing: setupFacingUI(); break;
        case OperationType::Roughing: setupRoughingUI(); break;
        case OperationType::Finishing: setupFinishingUI(); break;
        case OperationType::Parting: setupPartingUI(); break;
        case OperationType::Threading: setupThreadingUI(); break;
        case OperationType::Chamfering: setupChamferingUI(); break;
        case OperationType::Grooving: setupGroovingUI(); break;
        case OperationType::Drilling: setupDrillingUI(); break;
    }
    
    setupCommonUI();
    
    // Control buttons
    m_buttonLayout = new QHBoxLayout();
    m_resetButton = new QPushButton("Reset to Defaults");
    m_calculateButton = new QPushButton("Calculate Optimal");
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    
    m_okButton->setDefault(true);
    
    m_buttonLayout->addWidget(m_resetButton);
    m_buttonLayout->addWidget(m_calculateButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_okButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    // Main layout assembly
    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect buttons
    connect(m_resetButton, &QPushButton::clicked, this, &IntuiCAM::GUI::OperationParameterDialog::onResetToDefaults);
    connect(m_calculateButton, &QPushButton::clicked, this, &IntuiCAM::GUI::OperationParameterDialog::onCalculateOptimalSpeeds);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void IntuiCAM::GUI::OperationParameterDialog::setupFacingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    // Basic Parameters Group
    m_facingBasicGroup = new QGroupBox("Facing Parameters");
    QFormLayout* formLayout = new QFormLayout(m_facingBasicGroup);
    
    m_facingStepoverSpin = new QDoubleSpinBox();
    m_facingStepoverSpin->setRange(0.1, 5.0);
    m_facingStepoverSpin->setValue(m_facingParams.stepover);
    m_facingStepoverSpin->setSuffix(" mm");
    m_facingStepoverSpin->setDecimals(2);
    formLayout->addRow("Stepover:", m_facingStepoverSpin);
    
    m_facingFeedRateSpin = new QDoubleSpinBox();
    m_facingFeedRateSpin->setRange(10.0, 1000.0);
    m_facingFeedRateSpin->setValue(m_facingParams.feedRate);
    m_facingFeedRateSpin->setSuffix(" mm/min");
    formLayout->addRow("Feed Rate:", m_facingFeedRateSpin);
    
    m_facingSpindleSpeedSpin = new QDoubleSpinBox();
    m_facingSpindleSpeedSpin->setRange(100.0, 5000.0);
    m_facingSpindleSpeedSpin->setValue(m_facingParams.spindleSpeed);
    m_facingSpindleSpeedSpin->setSuffix(" RPM");
    formLayout->addRow("Spindle Speed:", m_facingSpindleSpeedSpin);
    
    m_facingStockAllowanceSpin = new QDoubleSpinBox();
    m_facingStockAllowanceSpin->setRange(0.0, 2.0);
    m_facingStockAllowanceSpin->setValue(m_facingParams.stockAllowance);
    m_facingStockAllowanceSpin->setSuffix(" mm");
    m_facingStockAllowanceSpin->setDecimals(2);
    formLayout->addRow("Stock Allowance:", m_facingStockAllowanceSpin);
    
    m_facingClimbingCheck = new QCheckBox("Use Climbing Milling");
    m_facingClimbingCheck->setChecked(m_facingParams.useClimbing);
    formLayout->addRow(m_facingClimbingCheck);
    
    m_facingRoughingOnlyCheck = new QCheckBox("Roughing Only (Skip Finishing)");
    m_facingRoughingOnlyCheck->setChecked(m_facingParams.roughingOnly);
    formLayout->addRow(m_facingRoughingOnlyCheck);
    
    tabLayout->addWidget(m_facingBasicGroup);
    tabLayout->addStretch();
    
    // Connect signals
    connect(m_facingStepoverSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
    connect(m_facingFeedRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
    connect(m_facingSpindleSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
}

void IntuiCAM::GUI::OperationParameterDialog::setupRoughingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    m_roughingBasicGroup = new QGroupBox("Roughing Parameters");
    QFormLayout* formLayout = new QFormLayout(m_roughingBasicGroup);
    
    m_roughingDepthOfCutSpin = new QDoubleSpinBox();
    m_roughingDepthOfCutSpin->setRange(0.1, 10.0);
    m_roughingDepthOfCutSpin->setValue(m_roughingParams.depthOfCut);
    m_roughingDepthOfCutSpin->setSuffix(" mm");
    formLayout->addRow("Depth of Cut:", m_roughingDepthOfCutSpin);
    
    m_roughingStockAllowanceSpin = new QDoubleSpinBox();
    m_roughingStockAllowanceSpin->setRange(0.1, 3.0);
    m_roughingStockAllowanceSpin->setValue(m_roughingParams.stockAllowance);
    m_roughingStockAllowanceSpin->setSuffix(" mm");
    formLayout->addRow("Stock Allowance:", m_roughingStockAllowanceSpin);
    
    m_roughingFeedRateSpin = new QDoubleSpinBox();
    m_roughingFeedRateSpin->setRange(20.0, 1500.0);
    m_roughingFeedRateSpin->setValue(m_roughingParams.feedRate);
    m_roughingFeedRateSpin->setSuffix(" mm/min");
    formLayout->addRow("Feed Rate:", m_roughingFeedRateSpin);
    
    m_roughingSpindleSpeedSpin = new QDoubleSpinBox();
    m_roughingSpindleSpeedSpin->setRange(100.0, 4000.0);
    m_roughingSpindleSpeedSpin->setValue(m_roughingParams.spindleSpeed);
    m_roughingSpindleSpeedSpin->setSuffix(" RPM");
    formLayout->addRow("Spindle Speed:", m_roughingSpindleSpeedSpin);
    
    m_roughingStepoverSpin = new QDoubleSpinBox();
    m_roughingStepoverSpin->setRange(10.0, 100.0);
    m_roughingStepoverSpin->setValue(m_roughingParams.stepover);
    m_roughingStepoverSpin->setSuffix(" %");
    formLayout->addRow("Stepover (% tool):", m_roughingStepoverSpin);
    
    m_roughingAdaptiveCheck = new QCheckBox("Adaptive Clearing");
    m_roughingAdaptiveCheck->setChecked(m_roughingParams.adaptiveClearing);
    formLayout->addRow(m_roughingAdaptiveCheck);
    
    m_roughingHelicalEntryCheck = new QCheckBox("Helical Entry");
    m_roughingHelicalEntryCheck->setChecked(m_roughingParams.useHelicalEntry);
    formLayout->addRow(m_roughingHelicalEntryCheck);
    
    tabLayout->addWidget(m_roughingBasicGroup);
    tabLayout->addStretch();
    
    // Connect signals
    connect(m_roughingDepthOfCutSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
    connect(m_roughingFeedRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
}

void IntuiCAM::GUI::OperationParameterDialog::setupFinishingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    m_finishingBasicGroup = new QGroupBox("Finishing Parameters");
    QFormLayout* formLayout = new QFormLayout(m_finishingBasicGroup);
    
    m_finishingSurfaceFinishSpin = new QDoubleSpinBox();
    m_finishingSurfaceFinishSpin->setRange(0.1, 50.0);
    m_finishingSurfaceFinishSpin->setValue(m_finishingParams.targetSurfaceFinish);
    m_finishingSurfaceFinishSpin->setSuffix(" μm Ra");
    formLayout->addRow("Target Surface Finish:", m_finishingSurfaceFinishSpin);
    
    m_finishingFeedRateSpin = new QDoubleSpinBox();
    m_finishingFeedRateSpin->setRange(5.0, 500.0);
    m_finishingFeedRateSpin->setValue(m_finishingParams.feedRate);
    m_finishingFeedRateSpin->setSuffix(" mm/min");
    formLayout->addRow("Feed Rate:", m_finishingFeedRateSpin);
    
    m_finishingSpindleSpeedSpin = new QDoubleSpinBox();
    m_finishingSpindleSpeedSpin->setRange(200.0, 6000.0);
    m_finishingSpindleSpeedSpin->setValue(m_finishingParams.spindleSpeed);
    m_finishingSpindleSpeedSpin->setSuffix(" RPM");
    formLayout->addRow("Spindle Speed:", m_finishingSpindleSpeedSpin);
    
    m_finishingAxialDepthSpin = new QDoubleSpinBox();
    m_finishingAxialDepthSpin->setRange(0.01, 1.0);
    m_finishingAxialDepthSpin->setValue(m_finishingParams.axialDepthOfCut);
    m_finishingAxialDepthSpin->setSuffix(" mm");
    m_finishingAxialDepthSpin->setDecimals(3);
    formLayout->addRow("Axial Depth:", m_finishingAxialDepthSpin);
    
    m_finishingSpindleControlCheck = new QCheckBox("Constant Surface Speed");
    m_finishingSpindleControlCheck->setChecked(m_finishingParams.useSpindleSpeedControl);
    formLayout->addRow(m_finishingSpindleControlCheck);
    
    m_finishingSpringPassesCheck = new QCheckBox("Multiple Spring Passes");
    m_finishingSpringPassesCheck->setChecked(m_finishingParams.multipleSpringPasses);
    formLayout->addRow(m_finishingSpringPassesCheck);
    
    m_finishingSpringPassCountSpin = new QSpinBox();
    m_finishingSpringPassCountSpin->setRange(1, 5);
    m_finishingSpringPassCountSpin->setValue(m_finishingParams.springPassCount);
    m_finishingSpringPassCountSpin->setEnabled(m_finishingParams.multipleSpringPasses);
    formLayout->addRow("Spring Pass Count:", m_finishingSpringPassCountSpin);
    
    tabLayout->addWidget(m_finishingBasicGroup);
    tabLayout->addStretch();
    
    // Connect spring pass toggle
    connect(m_finishingSpringPassesCheck, &QCheckBox::toggled,
            m_finishingSpringPassCountSpin, &QSpinBox::setEnabled);
    connect(m_finishingFeedRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
}

void IntuiCAM::GUI::OperationParameterDialog::setupPartingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    m_partingBasicGroup = new QGroupBox("Parting Parameters");
    QFormLayout* formLayout = new QFormLayout(m_partingBasicGroup);
    
    m_partingFeedRateSpin = new QDoubleSpinBox();
    m_partingFeedRateSpin->setRange(5.0, 200.0);
    m_partingFeedRateSpin->setValue(m_partingParams.feedRate);
    m_partingFeedRateSpin->setSuffix(" mm/min");
    formLayout->addRow("Feed Rate:", m_partingFeedRateSpin);
    
    m_partingSpindleSpeedSpin = new QDoubleSpinBox();
    m_partingSpindleSpeedSpin->setRange(100.0, 2000.0);
    m_partingSpindleSpeedSpin->setValue(m_partingParams.spindleSpeed);
    m_partingSpindleSpeedSpin->setSuffix(" RPM");
    formLayout->addRow("Spindle Speed:", m_partingSpindleSpeedSpin);
    
    m_partingRetractDistanceSpin = new QDoubleSpinBox();
    m_partingRetractDistanceSpin->setRange(0.5, 10.0);
    m_partingRetractDistanceSpin->setValue(m_partingParams.retractDistance);
    m_partingRetractDistanceSpin->setSuffix(" mm");
    formLayout->addRow("Retract Distance:", m_partingRetractDistanceSpin);
    
    m_partingPeckingCycleCheck = new QCheckBox("Use Pecking Cycle");
    m_partingPeckingCycleCheck->setChecked(m_partingParams.usePeckingCycle);
    formLayout->addRow(m_partingPeckingCycleCheck);
    
    m_partingFloodCoolantCheck = new QCheckBox("Flood Coolant");
    m_partingFloodCoolantCheck->setChecked(m_partingParams.useFloodCoolant);
    formLayout->addRow(m_partingFloodCoolantCheck);
    
    tabLayout->addWidget(m_partingBasicGroup);
    tabLayout->addStretch();
    
    connect(m_partingFeedRateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &IntuiCAM::GUI::OperationParameterDialog::onParameterChanged);
}

void IntuiCAM::GUI::OperationParameterDialog::setupCommonUI()
{
    // Advanced tab
    QVBoxLayout* advancedLayout = new QVBoxLayout(m_advancedTab);
    
    m_calculatedValuesGroup = new QGroupBox("Calculated Values");
    QFormLayout* calcLayout = new QFormLayout(m_calculatedValuesGroup);
    
    m_calculatedSpeedLabel = new QLabel("120 m/min");
    m_calculatedTimeLabel = new QLabel("2.5 minutes");
    m_materialRemovalRateLabel = new QLabel("15.6 cm³/min");
    
    calcLayout->addRow("Cutting Speed:", m_calculatedSpeedLabel);
    calcLayout->addRow("Estimated Time:", m_calculatedTimeLabel);
    calcLayout->addRow("Material Removal:", m_materialRemovalRateLabel);
    
    advancedLayout->addWidget(m_calculatedValuesGroup);
    advancedLayout->addStretch();
    
    // Presets tab
    QVBoxLayout* presetsLayout = new QVBoxLayout(m_presetsTab);
    
    m_presetsGroup = new QGroupBox("Parameter Presets");
    QFormLayout* presetLayout = new QFormLayout(m_presetsGroup);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItems({"Default", "Rough Cut", "Fine Finish", "High Speed"});
    presetLayout->addRow("Preset:", m_presetCombo);
    
    QHBoxLayout* presetButtons = new QHBoxLayout();
    m_loadPresetButton = new QPushButton("Load");
    m_savePresetButton = new QPushButton("Save");
    m_deletePresetButton = new QPushButton("Delete");
    
    presetButtons->addWidget(m_loadPresetButton);
    presetButtons->addWidget(m_savePresetButton);
    presetButtons->addWidget(m_deletePresetButton);
    presetLayout->addRow(presetButtons);
    
    presetsLayout->addWidget(m_presetsGroup);
    presetsLayout->addStretch();
    
    // Connect preset buttons
    connect(m_loadPresetButton, &QPushButton::clicked, this, &IntuiCAM::GUI::OperationParameterDialog::onLoadPreset);
    connect(m_savePresetButton, &QPushButton::clicked, this, &IntuiCAM::GUI::OperationParameterDialog::onSavePreset);
}

void IntuiCAM::GUI::OperationParameterDialog::onParameterChanged()
{
    updateCalculatedValues();
    emit parametersChanged();
}

void IntuiCAM::GUI::OperationParameterDialog::onResetToDefaults()
{
    switch (m_operationType) {
        case OperationType::Facing:
            m_facingParams = FacingParameters();
            setFacingParameters(m_facingParams);
            break;
        case OperationType::Roughing:
            m_roughingParams = RoughingParameters();
            setRoughingParameters(m_roughingParams);
            break;
        case OperationType::Finishing:
            m_finishingParams = FinishingParameters();
            setFinishingParameters(m_finishingParams);
            break;
        case OperationType::Parting:
            m_partingParams = PartingParameters();
            setPartingParameters(m_partingParams);
            break;
        case OperationType::Threading:
            m_threadingParams = ThreadingParameters();
            setThreadingParameters(m_threadingParams);
            break;
        case OperationType::Chamfering:
            m_chamferingParams = ChamferingParameters();
            setChamferingParameters(m_chamferingParams);
            break;
        case OperationType::Grooving:
            m_groovingParams = GroovingParameters();
            setGroovingParameters(m_groovingParams);
            break;
        case OperationType::Drilling:
            m_drillingParams = DrillingParameters();
            setDrillingParameters(m_drillingParams);
            break;
    }
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::onLoadPreset()
{
    QString presetName = m_presetCombo->currentText();
    QMessageBox::information(this, "Load Preset", 
                           QString("Loading preset: %1\n\n(Preset functionality coming soon)").arg(presetName));
}

void IntuiCAM::GUI::OperationParameterDialog::onSavePreset()
{
    QMessageBox::information(this, "Save Preset", "Save preset functionality coming soon");
}

void IntuiCAM::GUI::OperationParameterDialog::onCalculateOptimalSpeeds()
{
    // TODO: Implement speed calculation based on material and tool
    QMessageBox::information(this, "Calculate Optimal Speeds", 
                           "Optimal speed calculation based on material properties and tool data coming soon");
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::updateCalculatedValues()
{
    // Simple calculations for demonstration
    double spindleSpeed = 1000.0;
    double feedRate = 100.0;
    
    switch (m_operationType) {
        case OperationType::Facing:
            if (m_facingSpindleSpeedSpin) {
                spindleSpeed = m_facingSpindleSpeedSpin->value();
                feedRate = m_facingFeedRateSpin ? m_facingFeedRateSpin->value() : 100.0;
            }
            break;
        case OperationType::Roughing:
            if (m_roughingSpindleSpeedSpin) {
                spindleSpeed = m_roughingSpindleSpeedSpin->value();
                feedRate = m_roughingFeedRateSpin ? m_roughingFeedRateSpin->value() : 150.0;
            }
            break;
        case OperationType::Finishing:
            if (m_finishingSpindleSpeedSpin) {
                spindleSpeed = m_finishingSpindleSpeedSpin->value();
                feedRate = m_finishingFeedRateSpin ? m_finishingFeedRateSpin->value() : 80.0;
            }
            break;
        case OperationType::Parting:
            if (m_partingSpindleSpeedSpin) {
                spindleSpeed = m_partingSpindleSpeedSpin->value();
                feedRate = m_partingFeedRateSpin ? m_partingFeedRateSpin->value() : 30.0;
            }
            break;
    }
    
    // Calculate cutting speed (surface speed)
    double cuttingSpeed = (M_PI * m_partDiameter * spindleSpeed) / 1000.0; // m/min
    
    // Estimate machining time (simplified)
    double estimatedTime = (m_partLength / feedRate) * 2.0; // minutes (rough estimate)
    
    // Material removal rate (simplified)
    double materialRemovalRate = (feedRate * 2.0 * m_partDiameter) / 1000.0; // cm³/min
    
    // Update labels
    if (m_calculatedSpeedLabel) {
        m_calculatedSpeedLabel->setText(QString("%1 m/min").arg(cuttingSpeed, 0, 'f', 1));
    }
    if (m_calculatedTimeLabel) {
        m_calculatedTimeLabel->setText(QString("%1 minutes").arg(estimatedTime, 0, 'f', 1));
    }
    if (m_materialRemovalRateLabel) {
        m_materialRemovalRateLabel->setText(QString("%1 cm³/min").arg(materialRemovalRate, 0, 'f', 1));
    }
}

// Parameter getters
IntuiCAM::GUI::OperationParameterDialog::FacingParameters IntuiCAM::GUI::OperationParameterDialog::getFacingParameters() const
{
    return m_facingParams;
}

IntuiCAM::GUI::OperationParameterDialog::RoughingParameters IntuiCAM::GUI::OperationParameterDialog::getRoughingParameters() const
{
    return m_roughingParams;
}

IntuiCAM::GUI::OperationParameterDialog::FinishingParameters IntuiCAM::GUI::OperationParameterDialog::getFinishingParameters() const
{
    return m_finishingParams;
}

IntuiCAM::GUI::OperationParameterDialog::PartingParameters IntuiCAM::GUI::OperationParameterDialog::getPartingParameters() const
{
    return m_partingParams;
}

// New parameter getters
IntuiCAM::GUI::OperationParameterDialog::ThreadingParameters IntuiCAM::GUI::OperationParameterDialog::getThreadingParameters() const
{
    return m_threadingParams;
}

IntuiCAM::GUI::OperationParameterDialog::ChamferingParameters IntuiCAM::GUI::OperationParameterDialog::getChamferingParameters() const
{
    return m_chamferingParams;
}

IntuiCAM::GUI::OperationParameterDialog::GroovingParameters IntuiCAM::GUI::OperationParameterDialog::getGroovingParameters() const
{
    return m_groovingParams;
}

IntuiCAM::GUI::OperationParameterDialog::DrillingParameters IntuiCAM::GUI::OperationParameterDialog::getDrillingParameters() const
{
    return m_drillingParams;
}

// Parameter setters
void IntuiCAM::GUI::OperationParameterDialog::setFacingParameters(const FacingParameters& params)
{
    m_facingParams = params;
    
    // Update UI if initialized
    if (m_facingStepoverSpin) {
        m_facingStepoverSpin->setValue(params.stepover);
        m_facingFeedRateSpin->setValue(params.feedRate);
        m_facingSpindleSpeedSpin->setValue(params.spindleSpeed);
        m_facingStockAllowanceSpin->setValue(params.stockAllowance);
        m_facingClimbingCheck->setChecked(params.useClimbing);
        m_facingRoughingOnlyCheck->setChecked(params.roughingOnly);
    }
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setRoughingParameters(const RoughingParameters& params)
{
    m_roughingParams = params;
    
    // Update UI if initialized
    if (m_roughingDepthOfCutSpin) {
        m_roughingDepthOfCutSpin->setValue(params.depthOfCut);
        m_roughingStockAllowanceSpin->setValue(params.stockAllowance);
        m_roughingFeedRateSpin->setValue(params.feedRate);
        m_roughingSpindleSpeedSpin->setValue(params.spindleSpeed);
        m_roughingStepoverSpin->setValue(params.stepover);
        m_roughingAdaptiveCheck->setChecked(params.adaptiveClearing);
        m_roughingHelicalEntryCheck->setChecked(params.useHelicalEntry);
    }
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setFinishingParameters(const FinishingParameters& params)
{
    m_finishingParams = params;
    
    // Update UI if initialized
    if (m_finishingSurfaceFinishSpin) {
        m_finishingSurfaceFinishSpin->setValue(params.targetSurfaceFinish);
        m_finishingFeedRateSpin->setValue(params.feedRate);
        m_finishingSpindleSpeedSpin->setValue(params.spindleSpeed);
        m_finishingAxialDepthSpin->setValue(params.axialDepthOfCut);
        m_finishingRadialStepoverSpin->setValue(params.radialStepover);
        m_finishingSpindleControlCheck->setChecked(params.useSpindleSpeedControl);
        m_finishingSpringPassesCheck->setChecked(params.multipleSpringPasses);
        m_finishingSpringPassCountSpin->setValue(params.springPassCount);
    }
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setPartingParameters(const PartingParameters& params)
{
    m_partingParams = params;
    
    // Update UI if initialized
    if (m_partingFeedRateSpin) {
        m_partingFeedRateSpin->setValue(params.feedRate);
        m_partingSpindleSpeedSpin->setValue(params.spindleSpeed);
        m_partingPeckingDepthSpin->setValue(params.peckinDepth);
        m_partingRetractDistanceSpin->setValue(params.retractDistance);
        m_partingDwellTimeSpin->setValue(params.dwellTime);
        m_partingPeckingCycleCheck->setChecked(params.usePeckingCycle);
        m_partingFloodCoolantCheck->setChecked(params.useFloodCoolant);
        m_partingSafetyMarginSpin->setValue(params.safetyMargin);
    }
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setThreadingParameters(const ThreadingParameters& params)
{
    m_threadingParams = params;
    
    // Update UI if initialized (UI widgets will be added when setupThreadingUI is implemented)
    // TODO: Update UI controls when threading UI is implemented
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setChamferingParameters(const ChamferingParameters& params)
{
    m_chamferingParams = params;
    
    // Update UI if initialized (UI widgets will be added when setupChamferingUI is implemented)
    // TODO: Update UI controls when chamfering UI is implemented
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setGroovingParameters(const GroovingParameters& params)
{
    m_groovingParams = params;
    
    // Update UI if initialized (UI widgets will be added when setupGroovingUI is implemented)
    // TODO: Update UI controls when grooving UI is implemented
    
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setDrillingParameters(const DrillingParameters& params)
{
    m_drillingParams = params;
    
    // Update UI if initialized (UI widgets will be added when setupDrillingUI is implemented)
    // TODO: Update UI controls when drilling UI is implemented
    
    updateCalculatedValues();
}

// Placeholder implementations for new setup UI methods (to be fully implemented)
void IntuiCAM::GUI::OperationParameterDialog::setupThreadingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    // Basic threading parameters placeholder
    QGroupBox* threadingGroup = new QGroupBox("Threading Parameters");
    QFormLayout* formLayout = new QFormLayout(threadingGroup);
    
    QLabel* placeholderLabel = new QLabel("Threading parameter UI implementation in progress...");
    formLayout->addRow(placeholderLabel);
    
    tabLayout->addWidget(threadingGroup);
    tabLayout->addStretch();
}

void IntuiCAM::GUI::OperationParameterDialog::setupChamferingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    // Basic chamfering parameters placeholder
    QGroupBox* chamferingGroup = new QGroupBox("Chamfering Parameters");
    QFormLayout* formLayout = new QFormLayout(chamferingGroup);
    
    QLabel* placeholderLabel = new QLabel("Chamfering parameter UI implementation in progress...");
    formLayout->addRow(placeholderLabel);
    
    tabLayout->addWidget(chamferingGroup);
    tabLayout->addStretch();
}

void IntuiCAM::GUI::OperationParameterDialog::setupGroovingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    // Basic grooving parameters placeholder
    QGroupBox* groovingGroup = new QGroupBox("Grooving Parameters");
    QFormLayout* formLayout = new QFormLayout(groovingGroup);
    
    QLabel* placeholderLabel = new QLabel("Grooving parameter UI implementation in progress...");
    formLayout->addRow(placeholderLabel);
    
    tabLayout->addWidget(groovingGroup);
    tabLayout->addStretch();
}

void IntuiCAM::GUI::OperationParameterDialog::setupDrillingUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(m_parametersTab);
    
    // Basic drilling parameters placeholder
    QGroupBox* drillingGroup = new QGroupBox("Drilling Parameters");
    QFormLayout* formLayout = new QFormLayout(drillingGroup);
    
    QLabel* placeholderLabel = new QLabel("Drilling parameter UI implementation in progress...");
    formLayout->addRow(placeholderLabel);
    
    tabLayout->addWidget(drillingGroup);
    tabLayout->addStretch();
}

void IntuiCAM::GUI::OperationParameterDialog::setMaterialType(IntuiCAM::GUI::MaterialType material)
{
    m_materialType = material;
    applyMaterialDefaults();
}

void IntuiCAM::GUI::OperationParameterDialog::setPartDiameter(double diameter)
{
    m_partDiameter = diameter;
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::setPartLength(double length)
{
    m_partLength = length;
    updateCalculatedValues();
}

void IntuiCAM::GUI::OperationParameterDialog::applyMaterialDefaults()
{
    // TODO: Apply material-specific defaults
    updateCalculatedValues();
} 