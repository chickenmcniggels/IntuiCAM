#include "toolmanagementdialog.h"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QSlider>
#include <QDebug>

using namespace IntuiCAM::Toolpath;

// Constructor for editing an existing tool
ToolManagementDialog::ToolManagementDialog(const QString& toolId, QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_contentLayout(nullptr)
    , m_toolEditPanel(nullptr)
    , m_toolEditLayout(nullptr)
    , m_toolEditTabs(nullptr)
    , m_insertTab(nullptr)
    , m_holderTab(nullptr)
    , m_cuttingDataTab(nullptr)
    , m_toolInfoTab(nullptr)
    , m_visualization3DPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_currentToolAssembly()
    , m_currentToolId(toolId)
    , m_currentToolType(ToolType::GENERAL_TURNING)
    , m_isNewTool(false)
    , m_dataModified(false)
    , m_autoSaveTimer(new QTimer(this))
    , m_autoSaveEnabled(true)
{
    setupUI();
    setupAutoSave();
    loadToolData(toolId);
    connectParameterSignals();
    
    setWindowTitle(QString("Edit Tool: %1").arg(toolId));
}

// Constructor for creating a new tool
ToolManagementDialog::ToolManagementDialog(ToolType toolType, QWidget *parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_contentLayout(nullptr)
    , m_toolEditPanel(nullptr)
    , m_toolEditLayout(nullptr)
    , m_toolEditTabs(nullptr)
    , m_insertTab(nullptr)
    , m_holderTab(nullptr)
    , m_cuttingDataTab(nullptr)
    , m_toolInfoTab(nullptr)
    , m_visualization3DPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_currentToolAssembly()
    , m_currentToolId("")
    , m_currentToolType(toolType)
    , m_isNewTool(true)
    , m_dataModified(false)
    , m_autoSaveTimer(new QTimer(this))
    , m_autoSaveEnabled(true)
{
    setupUI();
    setupAutoSave();
    initializeNewTool(toolType);
    connectParameterSignals();
    
    setWindowTitle("Create New Tool");
}

ToolManagementDialog::~ToolManagementDialog() {
    // Save any pending changes before closing
    if (m_dataModified && m_autoSaveEnabled) {
        saveCurrentTool();
    }
}

void ToolManagementDialog::setupUI() {
    setMinimumSize(1000, 700);
    resize(1200, 800);
    setModal(true);
    
    createMainLayout();
    createToolEditPanel();
    create3DVisualizationPanel();
}

void ToolManagementDialog::createMainLayout() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // Create horizontal layout for tool edit panel and 3D viewer
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(10);
    m_mainLayout->addLayout(m_contentLayout);
}

void ToolManagementDialog::setupAutoSave() {
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(AUTO_SAVE_DELAY_MS);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &ToolManagementDialog::onAutoSaveTimeout);
}

void ToolManagementDialog::createToolEditPanel() {
    m_toolEditPanel = new QWidget();
    m_toolEditPanel->setMinimumWidth(600);
    
    m_toolEditLayout = new QVBoxLayout(m_toolEditPanel);
    
    // Create the tab widget for different tool properties
    m_toolEditTabs = new QTabWidget();
    
    // Create the individual tabs
    m_insertTab = createInsertPropertiesTab();
    m_holderTab = createHolderPropertiesTab();
    m_cuttingDataTab = createCuttingDataTab();
    m_toolInfoTab = createToolInfoTab();
    
    // Add tabs to the tab widget
    m_toolEditTabs->addTab(m_insertTab, "Insert Properties");
    m_toolEditTabs->addTab(m_holderTab, "Tool Holder");
    m_toolEditTabs->addTab(m_cuttingDataTab, "Cutting Data");
    m_toolEditTabs->addTab(m_toolInfoTab, "Tool Information");
    
    m_toolEditLayout->addWidget(m_toolEditTabs);
    
    // Add to main layout
    m_contentLayout->addWidget(m_toolEditPanel, 2); // Give it more space than 3D viewer
}

void ToolManagementDialog::create3DVisualizationPanel() {
    m_visualization3DPanel = new QWidget();
    m_visualization3DPanel->setMaximumWidth(400);
    m_visualization3DPanel->setMinimumWidth(350);
    
    m_visualizationLayout = new QVBoxLayout(m_visualization3DPanel);
    
    // 3D Viewer
    setup3DViewer();
    
    // View controls
    m_viewControlsGroup = new QGroupBox("View Controls");
    auto controlsLayout = new QVBoxLayout(m_viewControlsGroup);
    
    // Visualization mode
    auto modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel("Mode:"));
    m_visualizationModeCombo = new QComboBox();
    m_visualizationModeCombo->addItem("Wireframe");
    m_visualizationModeCombo->addItem("Shaded");
    m_visualizationModeCombo->addItem("Shaded + Edges");
    modeLayout->addWidget(m_visualizationModeCombo);
    controlsLayout->addLayout(modeLayout);
    
    // View buttons
    auto buttonLayout = new QHBoxLayout();
    m_fitViewButton = new QPushButton("Fit View");
    m_resetViewButton = new QPushButton("Reset View");
    buttonLayout->addWidget(m_fitViewButton);
    buttonLayout->addWidget(m_resetViewButton);
    controlsLayout->addLayout(buttonLayout);
    
    // Display options
    m_showDimensionsCheck = new QCheckBox("Show Dimensions");
    m_showAnnotationsCheck = new QCheckBox("Show Annotations");
    controlsLayout->addWidget(m_showDimensionsCheck);
    controlsLayout->addWidget(m_showAnnotationsCheck);
    
    // Zoom slider
    auto zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(new QLabel("Zoom:"));
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 200);
    m_zoomSlider->setValue(100);
    zoomLayout->addWidget(m_zoomSlider);
    controlsLayout->addLayout(zoomLayout);
    
    m_visualizationLayout->addWidget(m_viewControlsGroup);
    
    // Connect signals
    connect(m_visualizationModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementDialog::onVisualizationModeChanged);
    connect(m_fitViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onFitViewClicked);
    connect(m_resetViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onResetViewClicked);
    connect(m_showDimensionsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onShowDimensionsChanged);
    connect(m_showAnnotationsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onShowAnnotationsChanged);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ToolManagementDialog::onZoomChanged);
    
    // Add to main layout
    m_contentLayout->addWidget(m_visualization3DPanel, 1);
}

void ToolManagementDialog::setup3DViewer() {
    m_3dViewer = new OpenGL3DWidget();
    m_3dViewer->setMinimumSize(350, 300);
    m_visualizationLayout->addWidget(m_3dViewer);
}

IntuiCAM::Toolpath::ToolAssembly ToolManagementDialog::getToolAssembly() const {
    return m_currentToolAssembly;
}

void ToolManagementDialog::markAsModified() {
    if (!m_dataModified) {
        m_dataModified = true;
        // Update window title to show unsaved changes
        QString currentTitle = windowTitle();
        if (!currentTitle.endsWith(" *")) {
            setWindowTitle(currentTitle + " *");
        }
    }
    
    // Restart the auto-save timer
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start();
    }
}

void ToolManagementDialog::onAutoSaveTimeout() {
    if (m_dataModified && m_autoSaveEnabled) {
        saveCurrentTool();
    }
}

void ToolManagementDialog::saveCurrentTool() {
    if (!validateCurrentTool()) {
        qDebug() << "Tool validation failed, skipping save";
        return;
    }
    
    updateToolAssemblyFromFields();
    
    // TODO: Save to database/file system
    qDebug() << "Auto-saving tool:" << m_currentToolId;
    
    m_dataModified = false;
    
    // Update window title to remove unsaved indicator
    QString currentTitle = windowTitle();
    if (currentTitle.endsWith(" *")) {
        setWindowTitle(currentTitle.left(currentTitle.length() - 2));
    }
    
    emit toolSaved(m_currentToolId);
}

void ToolManagementDialog::connectParameterSignals() {
    // Connect all parameter change signals to trigger auto-save
    
    // General Turning Insert Parameters
    if (m_isoCodeEdit) {
        connect(m_isoCodeEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onISOCodeChanged);
    }
    if (m_insertShapeCombo) {
        connect(m_insertShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_reliefAngleCombo) {
        connect(m_reliefAngleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_toleranceCombo) {
        connect(m_toleranceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_materialCombo) {
        connect(m_materialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_substrateCombo) {
        connect(m_substrateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_coatingCombo) {
        connect(m_coatingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_inscribedCircleSpin) {
        connect(m_inscribedCircleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_thicknessSpin) {
        connect(m_thicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_cornerRadiusSpin) {
        connect(m_cornerRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_cuttingEdgeLengthSpin) {
        connect(m_cuttingEdgeLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_widthSpin) {
        connect(m_widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_rakeAngleSpin) {
        connect(m_rakeAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_inclinationAngleSpin) {
        connect(m_inclinationAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    
    // Threading Insert Parameters
    if (m_threadingISOCodeEdit) {
        connect(m_threadingISOCodeEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadingShapeCombo) {
        connect(m_threadingShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadingToleranceCombo) {
        connect(m_threadingToleranceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_crossSectionEdit) {
        connect(m_crossSectionEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadingMaterialCombo) {
        connect(m_threadingMaterialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadingThicknessSpin) {
        connect(m_threadingThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadingWidthSpin) {
        connect(m_threadingWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_minThreadPitchSpin) {
        connect(m_minThreadPitchSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_maxThreadPitchSpin) {
        connect(m_maxThreadPitchSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_internalThreadsCheck) {
        connect(m_internalThreadsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_externalThreadsCheck) {
        connect(m_externalThreadsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadProfileCombo) {
        connect(m_threadProfileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadProfileAngleSpin) {
        connect(m_threadProfileAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadTipTypeCombo) {
        connect(m_threadTipTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_threadTipRadiusSpin) {
        connect(m_threadTipRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    
    // Grooving Insert Parameters
    if (m_groovingISOCodeEdit) {
        connect(m_groovingISOCodeEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingShapeCombo) {
        connect(m_groovingShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingToleranceCombo) {
        connect(m_groovingToleranceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingCrossSectionEdit) {
        connect(m_groovingCrossSectionEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingMaterialCombo) {
        connect(m_groovingMaterialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingThicknessSpin) {
        connect(m_groovingThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingOverallLengthSpin) {
        connect(m_groovingOverallLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingWidthSpin) {
        connect(m_groovingWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingCornerRadiusSpin) {
        connect(m_groovingCornerRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_groovingHeadLengthSpin) {
        connect(m_groovingHeadLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    if (m_grooveWidthSpin) {
        connect(m_grooveWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onInsertParameterChanged);
    }
    
    // Tool Holder Parameters
    if (m_holderISOCodeEdit) {
        connect(m_holderISOCodeEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_handOrientationCombo) {
        connect(m_handOrientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_clampingStyleCombo) {
        connect(m_clampingStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_cuttingWidthSpin) {
        connect(m_cuttingWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_headLengthSpin) {
        connect(m_headLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_overallLengthSpin) {
        connect(m_overallLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_shankWidthSpin) {
        connect(m_shankWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_shankHeightSpin) {
        connect(m_shankHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_roundShankCheck) {
        connect(m_roundShankCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_shankDiameterSpin) {
        connect(m_shankDiameterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_insertSeatAngleSpin) {
        connect(m_insertSeatAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_insertSetbackSpin) {
        connect(m_insertSetbackSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_sideAngleSpin) {
        connect(m_sideAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_backAngleSpin) {
        connect(m_backAngleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_isInternalCheck) {
        connect(m_isInternalCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_isGroovingCheck) {
        connect(m_isGroovingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onHolderParameterChanged);
    }
    if (m_isThreadingCheck) {
        connect(m_isThreadingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onHolderParameterChanged);
    }
    
    // Cutting Data Parameters
    if (m_constantSurfaceSpeedCheck) {
        connect(m_constantSurfaceSpeedCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_surfaceSpeedSpin) {
        connect(m_surfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_spindleRPMSpin) {
        connect(m_spindleRPMSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_feedPerRevolutionCheck) {
        connect(m_feedPerRevolutionCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_cuttingFeedrateSpin) {
        connect(m_cuttingFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_leadInFeedrateSpin) {
        connect(m_leadInFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_leadOutFeedrateSpin) {
        connect(m_leadOutFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_maxDepthOfCutSpin) {
        connect(m_maxDepthOfCutSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_maxFeedrateSpin) {
        connect(m_maxFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_minSurfaceSpeedSpin) {
        connect(m_minSurfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_maxSurfaceSpeedSpin) {
        connect(m_maxSurfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_floodCoolantCheck) {
        connect(m_floodCoolantCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_mistCoolantCheck) {
        connect(m_mistCoolantCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_preferredCoolantCombo) {
        connect(m_preferredCoolantCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_coolantPressureSpin) {
        connect(m_coolantPressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_coolantFlowSpin) {
        connect(m_coolantFlowSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    
    // Tool Info Parameters
    if (m_toolNameEdit) {
        connect(m_toolNameEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_vendorEdit) {
        connect(m_vendorEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_manufacturerEdit) {
        connect(m_manufacturerEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_partNumberEdit) {
        connect(m_partNumberEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_productIdEdit) {
        connect(m_productIdEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_productLinkEdit) {
        connect(m_productLinkEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_isActiveCheck) {
        connect(m_isActiveCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_toolNumberEdit) {
        connect(m_toolNumberEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_turretPositionSpin) {
        connect(m_turretPositionSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_toolOffsetXSpin) {
        connect(m_toolOffsetXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_toolOffsetZSpin) {
        connect(m_toolOffsetZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_toolLengthOffsetSpin) {
        connect(m_toolLengthOffsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_toolRadiusOffsetSpin) {
        connect(m_toolRadiusOffsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_notesEdit) {
        connect(m_notesEdit, &QTextEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
}

// Stub implementations for required methods
void ToolManagementDialog::loadToolData(const QString& toolId) {
    // TODO: Load tool data from database/file system
    qDebug() << "Loading tool data for:" << toolId;
}

void ToolManagementDialog::initializeNewTool(ToolType toolType) {
    m_currentToolType = toolType;
    // TODO: Initialize with default values based on tool type
    qDebug() << "Initializing new tool of type:" << static_cast<int>(toolType);
}

bool ToolManagementDialog::validateCurrentTool() {
    // TODO: Implement validation logic
    return true;
}

void ToolManagementDialog::updateToolAssemblyFromFields() {
    // TODO: Update m_currentToolAssembly from form fields
}

void ToolManagementDialog::loadToolParametersIntoFields(const ToolAssembly& assembly) {
    // TODO: Load assembly data into form fields
}

// Tab creation methods - implement complete parameter sets
QWidget* ToolManagementDialog::createInsertPropertiesTab() {
    auto widget = new QWidget();
    auto mainLayout = new QVBoxLayout(widget);
    
    // Create tab widget for different insert types
    auto insertTypeTabWidget = new QTabWidget();
    
    // General Turning Insert Tab
    createGeneralTurningPanel();
    insertTypeTabWidget->addTab(m_turningInsertTab, "General Turning");
    
    // Threading Insert Tab  
    createThreadingPanel();
    insertTypeTabWidget->addTab(m_threadingInsertTab, "Threading");
    
    // Grooving Insert Tab
    createGroovingPanel();
    insertTypeTabWidget->addTab(m_groovingInsertTab, "Grooving");
    
    mainLayout->addWidget(insertTypeTabWidget);
    
    return widget;
}

QWidget* ToolManagementDialog::createHolderPropertiesTab() {
    auto widget = new QWidget();
    m_holderLayout = new QFormLayout(widget);
    
    // ISO Holder Identification Group
    auto isoGroup = new QGroupBox("ISO Holder Identification");
    auto isoLayout = new QFormLayout(isoGroup);
    
    m_holderISOCodeEdit = new QLineEdit();
    m_holderISOCodeEdit->setPlaceholderText("e.g., MCLNR2020K12");
    isoLayout->addRow("Holder ISO Code:", m_holderISOCodeEdit);
    
    m_handOrientationCombo = new QComboBox();
    m_handOrientationCombo->addItem("Right Hand", static_cast<int>(HandOrientation::RIGHT_HAND));
    m_handOrientationCombo->addItem("Left Hand", static_cast<int>(HandOrientation::LEFT_HAND));
    m_handOrientationCombo->addItem("Neutral", static_cast<int>(HandOrientation::NEUTRAL));
    isoLayout->addRow("Hand Orientation:", m_handOrientationCombo);
    
    m_clampingStyleCombo = new QComboBox();
    m_clampingStyleCombo->addItem("Top Clamp (M)", static_cast<int>(ClampingStyle::TOP_CLAMP));
    m_clampingStyleCombo->addItem("Top Clamp Hole (G)", static_cast<int>(ClampingStyle::TOP_CLAMP_HOLE));
    m_clampingStyleCombo->addItem("Lever Clamp (C)", static_cast<int>(ClampingStyle::LEVER_CLAMP));
    m_clampingStyleCombo->addItem("Screw Clamp (S)", static_cast<int>(ClampingStyle::SCREW_CLAMP));
    m_clampingStyleCombo->addItem("Wedge Clamp (W)", static_cast<int>(ClampingStyle::WEDGE_CLAMP));
    m_clampingStyleCombo->addItem("Pin Lock (P)", static_cast<int>(ClampingStyle::PIN_LOCK));
    m_clampingStyleCombo->addItem("Cartridge (K)", static_cast<int>(ClampingStyle::CARTRIDGE));
    isoLayout->addRow("Clamping Style:", m_clampingStyleCombo);
    
    m_holderLayout->addRow(isoGroup);
    
    // Physical Dimensions Group
    auto dimensionsGroup = new QGroupBox("Physical Dimensions");
    auto dimLayout = new QFormLayout(dimensionsGroup);
    
    m_cuttingWidthSpin = new QDoubleSpinBox();
    m_cuttingWidthSpin->setRange(0.0, 100.0);
    m_cuttingWidthSpin->setDecimals(3);
    m_cuttingWidthSpin->setSuffix(" mm");
    dimLayout->addRow("Cutting Width:", m_cuttingWidthSpin);
    
    m_headLengthSpin = new QDoubleSpinBox();
    m_headLengthSpin->setRange(0.0, 200.0);
    m_headLengthSpin->setDecimals(3);
    m_headLengthSpin->setSuffix(" mm");
    dimLayout->addRow("Head Length:", m_headLengthSpin);
    
    m_overallLengthSpin = new QDoubleSpinBox();
    m_overallLengthSpin->setRange(0.0, 500.0);
    m_overallLengthSpin->setDecimals(3);
    m_overallLengthSpin->setSuffix(" mm");
    dimLayout->addRow("Overall Length:", m_overallLengthSpin);
    
    // Shank dimensions
    auto shankGroup = new QGroupBox("Shank Dimensions");
    auto shankLayout = new QFormLayout(shankGroup);
    
    m_roundShankCheck = new QCheckBox();
    shankLayout->addRow("Round Shank:", m_roundShankCheck);
    
    m_shankWidthSpin = new QDoubleSpinBox();
    m_shankWidthSpin->setRange(0.0, 100.0);
    m_shankWidthSpin->setDecimals(3);
    m_shankWidthSpin->setSuffix(" mm");
    shankLayout->addRow("Shank Width:", m_shankWidthSpin);
    
    m_shankHeightSpin = new QDoubleSpinBox();
    m_shankHeightSpin->setRange(0.0, 100.0);
    m_shankHeightSpin->setDecimals(3);
    m_shankHeightSpin->setSuffix(" mm");
    shankLayout->addRow("Shank Height:", m_shankHeightSpin);
    
    m_shankDiameterSpin = new QDoubleSpinBox();
    m_shankDiameterSpin->setRange(0.0, 100.0);
    m_shankDiameterSpin->setDecimals(3);
    m_shankDiameterSpin->setSuffix(" mm");
    shankLayout->addRow("Shank Diameter:", m_shankDiameterSpin);
    
    dimLayout->addRow(shankGroup);
    m_holderLayout->addRow(dimensionsGroup);
    
    // Insert Seat Geometry Group
    auto seatGroup = new QGroupBox("Insert Seat Geometry");
    auto seatLayout = new QFormLayout(seatGroup);
    
    m_insertSeatAngleSpin = new QDoubleSpinBox();
    m_insertSeatAngleSpin->setRange(0.0, 90.0);
    m_insertSeatAngleSpin->setDecimals(1);
    m_insertSeatAngleSpin->setSuffix("°");
    seatLayout->addRow("Insert Seat Angle:", m_insertSeatAngleSpin);
    
    m_insertSetbackSpin = new QDoubleSpinBox();
    m_insertSetbackSpin->setRange(0.0, 50.0);
    m_insertSetbackSpin->setDecimals(3);
    m_insertSetbackSpin->setSuffix(" mm");
    seatLayout->addRow("Insert Setback:", m_insertSetbackSpin);
    
    m_sideAngleSpin = new QDoubleSpinBox();
    m_sideAngleSpin->setRange(-90.0, 90.0);
    m_sideAngleSpin->setDecimals(1);
    m_sideAngleSpin->setSuffix("°");
    seatLayout->addRow("Side Angle:", m_sideAngleSpin);
    
    m_backAngleSpin = new QDoubleSpinBox();
    m_backAngleSpin->setRange(-90.0, 90.0);
    m_backAngleSpin->setDecimals(1);
    m_backAngleSpin->setSuffix("°");
    seatLayout->addRow("Back Angle:", m_backAngleSpin);
    
    m_holderLayout->addRow(seatGroup);
    
    // Holder Capabilities Group
    auto capabilitiesGroup = new QGroupBox("Holder Capabilities");
    auto capLayout = new QFormLayout(capabilitiesGroup);
    
    m_isInternalCheck = new QCheckBox();
    capLayout->addRow("Internal Operations:", m_isInternalCheck);
    
    m_isGroovingCheck = new QCheckBox();
    capLayout->addRow("Grooving Operations:", m_isGroovingCheck);
    
    m_isThreadingCheck = new QCheckBox();
    capLayout->addRow("Threading Operations:", m_isThreadingCheck);
    
    m_holderLayout->addRow(capabilitiesGroup);
    
    return widget;
}

QWidget* ToolManagementDialog::createCuttingDataTab() {
    auto widget = new QWidget();
    m_cuttingDataLayout = new QFormLayout(widget);
    
    // Speed Control Group
    auto speedGroup = new QGroupBox("Speed Control");
    auto speedLayout = new QFormLayout(speedGroup);
    
    m_constantSurfaceSpeedCheck = new QCheckBox();
    speedLayout->addRow("Constant Surface Speed (CSS):", m_constantSurfaceSpeedCheck);
    
    m_surfaceSpeedSpin = new QDoubleSpinBox();
    m_surfaceSpeedSpin->setRange(0.0, 10000.0);
    m_surfaceSpeedSpin->setDecimals(1);
    m_surfaceSpeedSpin->setSuffix(" m/min");
    speedLayout->addRow("Surface Speed:", m_surfaceSpeedSpin);
    
    m_spindleRPMSpin = new QDoubleSpinBox();
    m_spindleRPMSpin->setRange(0.0, 10000.0);
    m_spindleRPMSpin->setDecimals(0);
    m_spindleRPMSpin->setSuffix(" RPM");
    speedLayout->addRow("Spindle RPM:", m_spindleRPMSpin);
    
    m_cuttingDataLayout->addRow(speedGroup);
    
    // Feed Control Group
    auto feedGroup = new QGroupBox("Feed Control");
    auto feedLayout = new QFormLayout(feedGroup);
    
    m_feedPerRevolutionCheck = new QCheckBox();
    feedLayout->addRow("Feed per Revolution:", m_feedPerRevolutionCheck);
    
    m_cuttingFeedrateSpin = new QDoubleSpinBox();
    m_cuttingFeedrateSpin->setRange(0.0, 10.0);
    m_cuttingFeedrateSpin->setDecimals(3);
    m_cuttingFeedrateSpin->setSuffix(" mm/rev");
    feedLayout->addRow("Cutting Feedrate:", m_cuttingFeedrateSpin);
    
    m_leadInFeedrateSpin = new QDoubleSpinBox();
    m_leadInFeedrateSpin->setRange(0.0, 10.0);
    m_leadInFeedrateSpin->setDecimals(3);
    m_leadInFeedrateSpin->setSuffix(" mm/rev");
    feedLayout->addRow("Lead-in Feedrate:", m_leadInFeedrateSpin);
    
    m_leadOutFeedrateSpin = new QDoubleSpinBox();
    m_leadOutFeedrateSpin->setRange(0.0, 10.0);
    m_leadOutFeedrateSpin->setDecimals(3);
    m_leadOutFeedrateSpin->setSuffix(" mm/rev");
    feedLayout->addRow("Lead-out Feedrate:", m_leadOutFeedrateSpin);
    
    m_cuttingDataLayout->addRow(feedGroup);
    
    // Cutting Limits Group
    auto limitsGroup = new QGroupBox("Cutting Limits");
    auto limitsLayout = new QFormLayout(limitsGroup);
    
    m_maxDepthOfCutSpin = new QDoubleSpinBox();
    m_maxDepthOfCutSpin->setRange(0.0, 50.0);
    m_maxDepthOfCutSpin->setDecimals(3);
    m_maxDepthOfCutSpin->setSuffix(" mm");
    limitsLayout->addRow("Max Depth of Cut:", m_maxDepthOfCutSpin);
    
    m_maxFeedrateSpin = new QDoubleSpinBox();
    m_maxFeedrateSpin->setRange(0.0, 1000.0);
    m_maxFeedrateSpin->setDecimals(1);
    m_maxFeedrateSpin->setSuffix(" mm/min");
    limitsLayout->addRow("Max Feedrate:", m_maxFeedrateSpin);
    
    m_minSurfaceSpeedSpin = new QDoubleSpinBox();
    m_minSurfaceSpeedSpin->setRange(0.0, 10000.0);
    m_minSurfaceSpeedSpin->setDecimals(1);
    m_minSurfaceSpeedSpin->setSuffix(" m/min");
    limitsLayout->addRow("Min Surface Speed:", m_minSurfaceSpeedSpin);
    
    m_maxSurfaceSpeedSpin = new QDoubleSpinBox();
    m_maxSurfaceSpeedSpin->setRange(0.0, 10000.0);
    m_maxSurfaceSpeedSpin->setDecimals(1);
    m_maxSurfaceSpeedSpin->setSuffix(" m/min");
    limitsLayout->addRow("Max Surface Speed:", m_maxSurfaceSpeedSpin);
    
    m_cuttingDataLayout->addRow(limitsGroup);
    
    // Coolant Control Group
    auto coolantGroup = new QGroupBox("Coolant Control");
    auto coolantLayout = new QFormLayout(coolantGroup);
    
    m_floodCoolantCheck = new QCheckBox();
    coolantLayout->addRow("Flood Coolant:", m_floodCoolantCheck);
    
    m_mistCoolantCheck = new QCheckBox();
    coolantLayout->addRow("Mist Coolant:", m_mistCoolantCheck);
    
    m_preferredCoolantCombo = new QComboBox();
    m_preferredCoolantCombo->addItem("None", static_cast<int>(CoolantType::NONE));
    m_preferredCoolantCombo->addItem("Mist", static_cast<int>(CoolantType::MIST));
    m_preferredCoolantCombo->addItem("Flood", static_cast<int>(CoolantType::FLOOD));
    m_preferredCoolantCombo->addItem("High Pressure", static_cast<int>(CoolantType::HIGH_PRESSURE));
    m_preferredCoolantCombo->addItem("Internal (Through Tool)", static_cast<int>(CoolantType::INTERNAL));
    m_preferredCoolantCombo->addItem("Air Blast", static_cast<int>(CoolantType::AIR_BLAST));
    coolantLayout->addRow("Preferred Coolant:", m_preferredCoolantCombo);
    
    m_coolantPressureSpin = new QDoubleSpinBox();
    m_coolantPressureSpin->setRange(0.0, 200.0);
    m_coolantPressureSpin->setDecimals(1);
    m_coolantPressureSpin->setSuffix(" bar");
    coolantLayout->addRow("Coolant Pressure:", m_coolantPressureSpin);
    
    m_coolantFlowSpin = new QDoubleSpinBox();
    m_coolantFlowSpin->setRange(0.0, 100.0);
    m_coolantFlowSpin->setDecimals(1);
    m_coolantFlowSpin->setSuffix(" L/min");
    coolantLayout->addRow("Coolant Flow:", m_coolantFlowSpin);
    
    m_cuttingDataLayout->addRow(coolantGroup);
    
    return widget;
}

QWidget* ToolManagementDialog::createToolInfoTab() {
    auto widget = new QWidget();
    m_toolInfoLayout = new QFormLayout(widget);
    
    // Basic Tool Information Group
    auto basicInfoGroup = new QGroupBox("Basic Tool Information");
    auto basicLayout = new QFormLayout(basicInfoGroup);
    
    m_toolNameEdit = new QLineEdit();
    m_toolNameEdit->setPlaceholderText("e.g., CNMG120408 General Turning Tool");
    basicLayout->addRow("Tool Name:", m_toolNameEdit);
    
    m_vendorEdit = new QLineEdit();
    m_vendorEdit->setPlaceholderText("e.g., Sandvik, Kennametal, Mitsubishi");
    basicLayout->addRow("Vendor:", m_vendorEdit);
    
    m_manufacturerEdit = new QLineEdit();
    m_manufacturerEdit->setPlaceholderText("Manufacturer name");
    basicLayout->addRow("Manufacturer:", m_manufacturerEdit);
    
    m_partNumberEdit = new QLineEdit();
    m_partNumberEdit->setPlaceholderText("Manufacturer part number");
    basicLayout->addRow("Part Number:", m_partNumberEdit);
    
    m_productIdEdit = new QLineEdit();
    m_productIdEdit->setPlaceholderText("Product/Catalog ID");
    basicLayout->addRow("Product ID:", m_productIdEdit);
    
    m_productLinkEdit = new QLineEdit();
    m_productLinkEdit->setPlaceholderText("https://example.com/product-page");
    basicLayout->addRow("Product Link:", m_productLinkEdit);
    
    m_isActiveCheck = new QCheckBox();
    m_isActiveCheck->setChecked(true);
    basicLayout->addRow("Active Tool:", m_isActiveCheck);
    
    m_toolInfoLayout->addRow(basicInfoGroup);
    
    // Machine Tool Configuration Group
    auto machineGroup = new QGroupBox("Machine Tool Configuration");
    auto machineLayout = new QFormLayout(machineGroup);
    
    m_toolNumberEdit = new QLineEdit();
    m_toolNumberEdit->setPlaceholderText("e.g., T01, T02, T03");
    machineLayout->addRow("Tool Number:", m_toolNumberEdit);
    
    m_turretPositionSpin = new QSpinBox();
    m_turretPositionSpin->setRange(1, 24);
    m_turretPositionSpin->setValue(1);
    machineLayout->addRow("Turret Position:", m_turretPositionSpin);
    
    m_toolInfoLayout->addRow(machineGroup);
    
    // Tool Offsets Group
    auto offsetsGroup = new QGroupBox("Tool Offsets");
    auto offsetsLayout = new QFormLayout(offsetsGroup);
    
    m_toolOffsetXSpin = new QDoubleSpinBox();
    m_toolOffsetXSpin->setRange(-500.0, 500.0);
    m_toolOffsetXSpin->setDecimals(4);
    m_toolOffsetXSpin->setSuffix(" mm");
    offsetsLayout->addRow("X Offset:", m_toolOffsetXSpin);
    
    m_toolOffsetZSpin = new QDoubleSpinBox();
    m_toolOffsetZSpin->setRange(-500.0, 500.0);
    m_toolOffsetZSpin->setDecimals(4);
    m_toolOffsetZSpin->setSuffix(" mm");
    offsetsLayout->addRow("Z Offset:", m_toolOffsetZSpin);
    
    m_toolLengthOffsetSpin = new QDoubleSpinBox();
    m_toolLengthOffsetSpin->setRange(-500.0, 500.0);
    m_toolLengthOffsetSpin->setDecimals(4);
    m_toolLengthOffsetSpin->setSuffix(" mm");
    offsetsLayout->addRow("Length Offset:", m_toolLengthOffsetSpin);
    
    m_toolRadiusOffsetSpin = new QDoubleSpinBox();
    m_toolRadiusOffsetSpin->setRange(-50.0, 50.0);
    m_toolRadiusOffsetSpin->setDecimals(4);
    m_toolRadiusOffsetSpin->setSuffix(" mm");
    offsetsLayout->addRow("Radius Offset:", m_toolRadiusOffsetSpin);
    
    m_toolInfoLayout->addRow(offsetsGroup);
    
    // Notes and Additional Information Group
    auto notesGroup = new QGroupBox("Notes and Additional Information");
    auto notesLayout = new QVBoxLayout(notesGroup);
    
    auto notesLabel = new QLabel("Notes:");
    notesLayout->addWidget(notesLabel);
    
    m_notesEdit = new QTextEdit();
    m_notesEdit->setMaximumHeight(100);
    m_notesEdit->setPlaceholderText("Enter any additional notes about this tool...");
    notesLayout->addWidget(m_notesEdit);
    
    m_toolInfoLayout->addRow(notesGroup);
    
    return widget;
}

// 3D visualization methods
void ToolManagementDialog::generate3DToolGeometry() {
    // TODO: Generate 3D representation of the tool
}

void ToolManagementDialog::updateRealTime3DVisualization() {
    // TODO: Update 3D visualization based on current parameters
}

void ToolManagementDialog::updateToolVisualization() {
    generate3DToolGeometry();
    updateRealTime3DVisualization();
}

// Slot implementations
void ToolManagementDialog::onInsertParameterChanged() {
    markAsModified();
                updateToolVisualization();
            }
            
void ToolManagementDialog::onHolderParameterChanged() {
    markAsModified();
        updateToolVisualization();
}

void ToolManagementDialog::onCuttingDataChanged() {
    markAsModified();
}

void ToolManagementDialog::onToolInfoChanged() {
    markAsModified();
}

void ToolManagementDialog::onISOCodeChanged() {
    markAsModified();
    // TODO: Validate ISO code format
}

void ToolManagementDialog::onVisualizationModeChanged(int mode) {
    if (m_3dViewer) {
        // TODO: Update 3D viewer visualization mode
        qDebug() << "Visualization mode changed to:" << mode;
    }
}

void ToolManagementDialog::onFitViewClicked() {
    if (m_3dViewer) {
        // TODO: Fit view to tool geometry
        qDebug() << "Fit view clicked";
    }
}

void ToolManagementDialog::onResetViewClicked() {
    if (m_3dViewer) {
        // TODO: Reset view to default
        qDebug() << "Reset view clicked";
    }
}

void ToolManagementDialog::onShowDimensionsChanged(bool show) {
    if (m_3dViewer) {
        // TODO: Toggle dimension display
        qDebug() << "Show dimensions:" << show;
    }
}

void ToolManagementDialog::onShowAnnotationsChanged(bool show) {
    if (m_3dViewer) {
        // TODO: Toggle annotation display
        qDebug() << "Show annotations:" << show;
    }
}

void ToolManagementDialog::onZoomChanged(int value) {
    if (m_3dViewer) {
        // TODO: Adjust zoom level
        qDebug() << "Zoom changed to:" << value;
    }
}

// Helper methods
void ToolManagementDialog::setComboBoxByValue(QComboBox* comboBox, int value) {
    if (!comboBox) return;
    
    for (int i = 0; i < comboBox->count(); ++i) {
        if (comboBox->itemData(i).toInt() == value) {
            comboBox->setCurrentIndex(i);
            break;
        }
    }
}

void ToolManagementDialog::clearAllParameterFields() {
    // TODO: Clear all form fields
}

void ToolManagementDialog::updateToolTypeSpecificUI() {
    // TODO: Update UI based on current tool type
}

bool ToolManagementDialog::validateISOCode(const QString& isoCode) {
    // TODO: Implement ISO code validation
    return !isoCode.isEmpty();
}

QString ToolManagementDialog::formatToolType(ToolType toolType) {
    switch (toolType) {
        case ToolType::GENERAL_TURNING: return "General Turning";
        case ToolType::BORING: return "Boring";
        case ToolType::THREADING: return "Threading";
        case ToolType::GROOVING: return "Grooving";
        case ToolType::PARTING: return "Parting";
        case ToolType::FORM_TOOL: return "Form Tool";
        case ToolType::LIVE_TOOLING: return "Live Tooling";
        default: return "Unknown";
    }
}

// Stub implementations for parameter loading methods
void ToolManagementDialog::loadGeneralTurningInsertParameters(const GeneralTurningInsert& insert) {
    // TODO: Implement
}

void ToolManagementDialog::loadThreadingInsertParameters(const ThreadingInsert& insert) {
    // TODO: Implement
}

void ToolManagementDialog::loadGroovingInsertParameters(const GroovingInsert& insert) {
    // TODO: Implement
}

void ToolManagementDialog::loadHolderParameters(const ToolHolder& holder) {
    // TODO: Implement
}

void ToolManagementDialog::loadCuttingDataParameters(const CuttingData& cuttingData) {
    // TODO: Implement
}

// Stub implementations for parameter updating methods
void ToolManagementDialog::updateGeneralTurningInsertFromFields() {
    // TODO: Implement
}

void ToolManagementDialog::updateThreadingInsertFromFields() {
    // TODO: Implement
}

void ToolManagementDialog::updateGroovingInsertFromFields() {
    // TODO: Implement
}

void ToolManagementDialog::updateHolderDataFromFields() {
    // TODO: Implement
}

void ToolManagementDialog::updateCuttingDataFromFields() {
    // TODO: Implement
}

void ToolManagementDialog::createGeneralTurningPanel() {
    m_turningInsertTab = new QWidget();
    m_turningInsertLayout = new QFormLayout(m_turningInsertTab);
    
    // ISO Identification Group
    auto isoGroup = new QGroupBox("ISO Identification");
    auto isoLayout = new QFormLayout(isoGroup);
    
    m_isoCodeEdit = new QLineEdit();
    m_isoCodeEdit->setPlaceholderText("e.g., CNMG120408");
    isoLayout->addRow("ISO Code:", m_isoCodeEdit);
    
    m_insertShapeCombo = new QComboBox();
    m_insertShapeCombo->addItem("Triangle (T)", static_cast<int>(InsertShape::TRIANGLE));
    m_insertShapeCombo->addItem("Square (S)", static_cast<int>(InsertShape::SQUARE));
    m_insertShapeCombo->addItem("Pentagon (P)", static_cast<int>(InsertShape::PENTAGON));
    m_insertShapeCombo->addItem("Diamond 80° (D)", static_cast<int>(InsertShape::DIAMOND_80));
    m_insertShapeCombo->addItem("Diamond 55° (C)", static_cast<int>(InsertShape::DIAMOND_55));
    m_insertShapeCombo->addItem("Hexagon (H)", static_cast<int>(InsertShape::HEXAGON));
    m_insertShapeCombo->addItem("Octagon (O)", static_cast<int>(InsertShape::OCTAGON));
    m_insertShapeCombo->addItem("Rhombic 86° (V)", static_cast<int>(InsertShape::RHOMBIC_86));
    m_insertShapeCombo->addItem("Rhombic 75° (E)", static_cast<int>(InsertShape::RHOMBIC_75));
    m_insertShapeCombo->addItem("Round (R)", static_cast<int>(InsertShape::ROUND));
    m_insertShapeCombo->addItem("Trigon (W)", static_cast<int>(InsertShape::TRIGON));
    m_insertShapeCombo->addItem("Custom (X)", static_cast<int>(InsertShape::CUSTOM));
    isoLayout->addRow("Insert Shape:", m_insertShapeCombo);
    
    m_reliefAngleCombo = new QComboBox();
    m_reliefAngleCombo->addItem("0° (N)", static_cast<int>(InsertReliefAngle::ANGLE_0));
    m_reliefAngleCombo->addItem("3° (A)", static_cast<int>(InsertReliefAngle::ANGLE_3));
    m_reliefAngleCombo->addItem("5° (B)", static_cast<int>(InsertReliefAngle::ANGLE_5));
    m_reliefAngleCombo->addItem("7° (C)", static_cast<int>(InsertReliefAngle::ANGLE_7));
    m_reliefAngleCombo->addItem("11° (D)", static_cast<int>(InsertReliefAngle::ANGLE_11));
    m_reliefAngleCombo->addItem("15° (E)", static_cast<int>(InsertReliefAngle::ANGLE_15));
    m_reliefAngleCombo->addItem("20° (F)", static_cast<int>(InsertReliefAngle::ANGLE_20));
    m_reliefAngleCombo->addItem("25° (G)", static_cast<int>(InsertReliefAngle::ANGLE_25));
    m_reliefAngleCombo->addItem("30° (H)", static_cast<int>(InsertReliefAngle::ANGLE_30));
    isoLayout->addRow("Relief Angle:", m_reliefAngleCombo);
    
    m_toleranceCombo = new QComboBox();
    m_toleranceCombo->addItem("±0.005mm (A)", static_cast<int>(InsertTolerance::A_PRECISION));
    m_toleranceCombo->addItem("±0.008mm (B)", static_cast<int>(InsertTolerance::B_PRECISION));
    m_toleranceCombo->addItem("±0.013mm (C)", static_cast<int>(InsertTolerance::C_PRECISION));
    m_toleranceCombo->addItem("±0.020mm (D)", static_cast<int>(InsertTolerance::D_PRECISION));
    m_toleranceCombo->addItem("±0.025mm (E)", static_cast<int>(InsertTolerance::E_PRECISION));
    m_toleranceCombo->addItem("±0.050mm (F)", static_cast<int>(InsertTolerance::F_PRECISION));
    m_toleranceCombo->addItem("±0.080mm (G)", static_cast<int>(InsertTolerance::G_PRECISION));
    m_toleranceCombo->addItem("±0.130mm (H)", static_cast<int>(InsertTolerance::H_PRECISION));
    m_toleranceCombo->addItem("±0.200mm (K)", static_cast<int>(InsertTolerance::K_PRECISION));
    m_toleranceCombo->addItem("±0.250mm (L)", static_cast<int>(InsertTolerance::L_PRECISION));
    m_toleranceCombo->addItem("±0.380mm (M)", static_cast<int>(InsertTolerance::M_PRECISION));
    m_toleranceCombo->addItem("±0.500mm (N)", static_cast<int>(InsertTolerance::N_PRECISION));
    isoLayout->addRow("Tolerance:", m_toleranceCombo);
    
    m_turningInsertLayout->addRow(isoGroup);
    
    // Physical Dimensions Group
    auto dimensionsGroup = new QGroupBox("Physical Dimensions");
    auto dimLayout = new QFormLayout(dimensionsGroup);
    
    m_inscribedCircleSpin = new QDoubleSpinBox();
    m_inscribedCircleSpin->setRange(0.0, 50.0);
    m_inscribedCircleSpin->setDecimals(3);
    m_inscribedCircleSpin->setSuffix(" mm");
    dimLayout->addRow("Inscribed Circle (IC):", m_inscribedCircleSpin);
    
    m_thicknessSpin = new QDoubleSpinBox();
    m_thicknessSpin->setRange(0.0, 20.0);
    m_thicknessSpin->setDecimals(3);
    m_thicknessSpin->setSuffix(" mm");
    dimLayout->addRow("Thickness (S):", m_thicknessSpin);
    
    m_cornerRadiusSpin = new QDoubleSpinBox();
    m_cornerRadiusSpin->setRange(0.0, 5.0);
    m_cornerRadiusSpin->setDecimals(3);
    m_cornerRadiusSpin->setSuffix(" mm");
    dimLayout->addRow("Corner Radius (r):", m_cornerRadiusSpin);
    
    m_cuttingEdgeLengthSpin = new QDoubleSpinBox();
    m_cuttingEdgeLengthSpin->setRange(0.0, 50.0);
    m_cuttingEdgeLengthSpin->setDecimals(3);
    m_cuttingEdgeLengthSpin->setSuffix(" mm");
    dimLayout->addRow("Cutting Edge Length (l):", m_cuttingEdgeLengthSpin);
    
    m_widthSpin = new QDoubleSpinBox();
    m_widthSpin->setRange(0.0, 50.0);
    m_widthSpin->setDecimals(3);
    m_widthSpin->setSuffix(" mm");
    dimLayout->addRow("Width (d1):", m_widthSpin);
    
    m_turningInsertLayout->addRow(dimensionsGroup);
    
    // Material Properties Group
    auto materialGroup = new QGroupBox("Material Properties");
    auto matLayout = new QFormLayout(materialGroup);
    
    m_materialCombo = new QComboBox();
    m_materialCombo->addItem("Uncoated Carbide", static_cast<int>(InsertMaterial::UNCOATED_CARBIDE));
    m_materialCombo->addItem("Coated Carbide", static_cast<int>(InsertMaterial::COATED_CARBIDE));
    m_materialCombo->addItem("Cermet", static_cast<int>(InsertMaterial::CERMET));
    m_materialCombo->addItem("Ceramic", static_cast<int>(InsertMaterial::CERAMIC));
    m_materialCombo->addItem("CBN", static_cast<int>(InsertMaterial::CBN));
    m_materialCombo->addItem("PCD", static_cast<int>(InsertMaterial::PCD));
    m_materialCombo->addItem("HSS", static_cast<int>(InsertMaterial::HSS));
    m_materialCombo->addItem("Cast Alloy", static_cast<int>(InsertMaterial::CAST_ALLOY));
    m_materialCombo->addItem("Diamond", static_cast<int>(InsertMaterial::DIAMOND));
    matLayout->addRow("Material:", m_materialCombo);
    
    m_substrateCombo = new QComboBox();
    m_substrateCombo->setEditable(true);
    m_substrateCombo->addItem("P10");
    m_substrateCombo->addItem("P20");
    m_substrateCombo->addItem("P30");
    m_substrateCombo->addItem("M10");
    m_substrateCombo->addItem("M20");
    m_substrateCombo->addItem("K10");
    m_substrateCombo->addItem("K20");
    matLayout->addRow("Substrate Grade:", m_substrateCombo);
    
    m_coatingCombo = new QComboBox();
    m_coatingCombo->setEditable(true);
    m_coatingCombo->addItem("None");
    m_coatingCombo->addItem("TiN");
    m_coatingCombo->addItem("TiCN");
    m_coatingCombo->addItem("TiAlN");
    m_coatingCombo->addItem("CVD Multi-layer");
    m_coatingCombo->addItem("PVD Multi-layer");
    matLayout->addRow("Coating:", m_coatingCombo);
    
    m_turningInsertLayout->addRow(materialGroup);
    
    // Cutting Geometry Group
    auto geometryGroup = new QGroupBox("Cutting Geometry");
    auto geoLayout = new QFormLayout(geometryGroup);
    
    m_rakeAngleSpin = new QDoubleSpinBox();
    m_rakeAngleSpin->setRange(-30.0, 30.0);
    m_rakeAngleSpin->setDecimals(1);
    m_rakeAngleSpin->setSuffix("°");
    geoLayout->addRow("Rake Angle (γ):", m_rakeAngleSpin);
    
    m_inclinationAngleSpin = new QDoubleSpinBox();
    m_inclinationAngleSpin->setRange(-30.0, 30.0);
    m_inclinationAngleSpin->setDecimals(1);
    m_inclinationAngleSpin->setSuffix("°");
    geoLayout->addRow("Inclination Angle (λ):", m_inclinationAngleSpin);
    
    m_turningInsertLayout->addRow(geometryGroup);
}

void ToolManagementDialog::createThreadingPanel() {
    m_threadingInsertTab = new QWidget();
    m_threadingInsertLayout = new QFormLayout(m_threadingInsertTab);
    
    // ISO Threading Insert Identification Group
    auto isoGroup = new QGroupBox("ISO Threading Insert Identification");
    auto isoLayout = new QFormLayout(isoGroup);
    
    m_threadingISOCodeEdit = new QLineEdit();
    m_threadingISOCodeEdit->setPlaceholderText("e.g., 16ER1.0ISO");
    isoLayout->addRow("Threading ISO Code:", m_threadingISOCodeEdit);
    
    m_threadingShapeCombo = new QComboBox();
    m_threadingShapeCombo->addItem("Triangle (T)", static_cast<int>(InsertShape::TRIANGLE));
    m_threadingShapeCombo->addItem("Square (S)", static_cast<int>(InsertShape::SQUARE));
    m_threadingShapeCombo->addItem("Diamond 55° (C)", static_cast<int>(InsertShape::DIAMOND_55));
    m_threadingShapeCombo->addItem("Diamond 80° (D)", static_cast<int>(InsertShape::DIAMOND_80));
    m_threadingShapeCombo->addItem("Custom (X)", static_cast<int>(InsertShape::CUSTOM));
    isoLayout->addRow("Threading Shape:", m_threadingShapeCombo);
    
    m_threadingToleranceCombo = new QComboBox();
    m_threadingToleranceCombo->addItem("±0.005mm (A)", static_cast<int>(InsertTolerance::A_PRECISION));
    m_threadingToleranceCombo->addItem("±0.008mm (B)", static_cast<int>(InsertTolerance::B_PRECISION));
    m_threadingToleranceCombo->addItem("±0.013mm (C)", static_cast<int>(InsertTolerance::C_PRECISION));
    m_threadingToleranceCombo->addItem("±0.020mm (D)", static_cast<int>(InsertTolerance::D_PRECISION));
    m_threadingToleranceCombo->addItem("±0.025mm (E)", static_cast<int>(InsertTolerance::E_PRECISION));
    m_threadingToleranceCombo->addItem("±0.050mm (F)", static_cast<int>(InsertTolerance::F_PRECISION));
    isoLayout->addRow("Threading Tolerance:", m_threadingToleranceCombo);
    
    m_crossSectionEdit = new QLineEdit();
    m_crossSectionEdit->setPlaceholderText("Threading cross-section code");
    isoLayout->addRow("Cross Section:", m_crossSectionEdit);
    
    m_threadingMaterialCombo = new QComboBox();
    m_threadingMaterialCombo->addItem("Uncoated Carbide", static_cast<int>(InsertMaterial::UNCOATED_CARBIDE));
    m_threadingMaterialCombo->addItem("Coated Carbide", static_cast<int>(InsertMaterial::COATED_CARBIDE));
    m_threadingMaterialCombo->addItem("Cermet", static_cast<int>(InsertMaterial::CERMET));
    m_threadingMaterialCombo->addItem("Ceramic", static_cast<int>(InsertMaterial::CERAMIC));
    m_threadingMaterialCombo->addItem("CBN", static_cast<int>(InsertMaterial::CBN));
    m_threadingMaterialCombo->addItem("PCD", static_cast<int>(InsertMaterial::PCD));
    m_threadingMaterialCombo->addItem("HSS", static_cast<int>(InsertMaterial::HSS));
    isoLayout->addRow("Threading Material:", m_threadingMaterialCombo);
    
    m_threadingInsertLayout->addRow(isoGroup);
    
    // Threading Dimensions Group
    auto dimGroup = new QGroupBox("Threading Dimensions");
    auto dimLayout = new QFormLayout(dimGroup);
    
    m_threadingThicknessSpin = new QDoubleSpinBox();
    m_threadingThicknessSpin->setRange(0.0, 20.0);
    m_threadingThicknessSpin->setDecimals(3);
    m_threadingThicknessSpin->setSuffix(" mm");
    dimLayout->addRow("Thickness:", m_threadingThicknessSpin);
    
    m_threadingWidthSpin = new QDoubleSpinBox();
    m_threadingWidthSpin->setRange(0.0, 50.0);
    m_threadingWidthSpin->setDecimals(3);
    m_threadingWidthSpin->setSuffix(" mm");
    dimLayout->addRow("Width:", m_threadingWidthSpin);
    
    m_minThreadPitchSpin = new QDoubleSpinBox();
    m_minThreadPitchSpin->setRange(0.0, 10.0);
    m_minThreadPitchSpin->setDecimals(3);
    m_minThreadPitchSpin->setSuffix(" mm");
    dimLayout->addRow("Min Thread Pitch:", m_minThreadPitchSpin);
    
    m_maxThreadPitchSpin = new QDoubleSpinBox();
    m_maxThreadPitchSpin->setRange(0.0, 10.0);
    m_maxThreadPitchSpin->setDecimals(3);
    m_maxThreadPitchSpin->setSuffix(" mm");
    dimLayout->addRow("Max Thread Pitch:", m_maxThreadPitchSpin);
    
    m_threadingInsertLayout->addRow(dimGroup);
    
    // Threading Capabilities Group
    auto capabilitiesGroup = new QGroupBox("Threading Capabilities");
    auto capLayout = new QFormLayout(capabilitiesGroup);
    
    m_internalThreadsCheck = new QCheckBox();
    capLayout->addRow("Internal Threads:", m_internalThreadsCheck);
    
    m_externalThreadsCheck = new QCheckBox();
    capLayout->addRow("External Threads:", m_externalThreadsCheck);
    
    m_threadProfileCombo = new QComboBox();
    m_threadProfileCombo->addItem("Metric (60°)", static_cast<int>(ThreadProfile::METRIC));
    m_threadProfileCombo->addItem("Unified (60°)", static_cast<int>(ThreadProfile::UNIFIED));
    m_threadProfileCombo->addItem("Whitworth (55°)", static_cast<int>(ThreadProfile::WHITWORTH));
    m_threadProfileCombo->addItem("ACME (29°)", static_cast<int>(ThreadProfile::ACME));
    m_threadProfileCombo->addItem("Trapezoidal (30°)", static_cast<int>(ThreadProfile::TRAPEZOIDAL));
    m_threadProfileCombo->addItem("Square", static_cast<int>(ThreadProfile::SQUARE));
    m_threadProfileCombo->addItem("Buttress", static_cast<int>(ThreadProfile::BUTTRESS));
    m_threadProfileCombo->addItem("Custom", static_cast<int>(ThreadProfile::CUSTOM));
    capLayout->addRow("Thread Profile:", m_threadProfileCombo);
    
    m_threadProfileAngleSpin = new QDoubleSpinBox();
    m_threadProfileAngleSpin->setRange(0.0, 90.0);
    m_threadProfileAngleSpin->setDecimals(1);
    m_threadProfileAngleSpin->setSuffix("°");
    m_threadProfileAngleSpin->setValue(60.0);
    capLayout->addRow("Profile Angle:", m_threadProfileAngleSpin);
    
    m_threadTipTypeCombo = new QComboBox();
    m_threadTipTypeCombo->addItem("Sharp Point", static_cast<int>(ThreadTipType::SHARP_POINT));
    m_threadTipTypeCombo->addItem("Flat Tip", static_cast<int>(ThreadTipType::FLAT_TIP));
    m_threadTipTypeCombo->addItem("Round Tip", static_cast<int>(ThreadTipType::ROUND_TIP));
    capLayout->addRow("Thread Tip Type:", m_threadTipTypeCombo);
    
    m_threadTipRadiusSpin = new QDoubleSpinBox();
    m_threadTipRadiusSpin->setRange(0.0, 5.0);
    m_threadTipRadiusSpin->setDecimals(3);
    m_threadTipRadiusSpin->setSuffix(" mm");
    capLayout->addRow("Thread Tip Radius:", m_threadTipRadiusSpin);
    
    m_threadingInsertLayout->addRow(capabilitiesGroup);
}

void ToolManagementDialog::createGroovingPanel() {
    m_groovingInsertTab = new QWidget();
    m_groovingInsertLayout = new QFormLayout(m_groovingInsertTab);
    
    // ISO Grooving Insert Identification Group
    auto isoGroup = new QGroupBox("ISO Grooving Insert Identification");
    auto isoLayout = new QFormLayout(isoGroup);
    
    m_groovingISOCodeEdit = new QLineEdit();
    m_groovingISOCodeEdit->setPlaceholderText("e.g., MGN300-M");
    isoLayout->addRow("Grooving ISO Code:", m_groovingISOCodeEdit);
    
    m_groovingShapeCombo = new QComboBox();
    m_groovingShapeCombo->addItem("Square (S)", static_cast<int>(InsertShape::SQUARE));
    m_groovingShapeCombo->addItem("Triangle (T)", static_cast<int>(InsertShape::TRIANGLE));
    m_groovingShapeCombo->addItem("Diamond 80° (D)", static_cast<int>(InsertShape::DIAMOND_80));
    m_groovingShapeCombo->addItem("Diamond 55° (C)", static_cast<int>(InsertShape::DIAMOND_55));
    m_groovingShapeCombo->addItem("Custom (X)", static_cast<int>(InsertShape::CUSTOM));
    isoLayout->addRow("Grooving Shape:", m_groovingShapeCombo);
    
    m_groovingToleranceCombo = new QComboBox();
    m_groovingToleranceCombo->addItem("±0.005mm (A)", static_cast<int>(InsertTolerance::A_PRECISION));
    m_groovingToleranceCombo->addItem("±0.008mm (B)", static_cast<int>(InsertTolerance::B_PRECISION));
    m_groovingToleranceCombo->addItem("±0.013mm (C)", static_cast<int>(InsertTolerance::C_PRECISION));
    m_groovingToleranceCombo->addItem("±0.020mm (D)", static_cast<int>(InsertTolerance::D_PRECISION));
    m_groovingToleranceCombo->addItem("±0.025mm (E)", static_cast<int>(InsertTolerance::E_PRECISION));
    m_groovingToleranceCombo->addItem("±0.050mm (F)", static_cast<int>(InsertTolerance::F_PRECISION));
    isoLayout->addRow("Grooving Tolerance:", m_groovingToleranceCombo);
    
    m_groovingCrossSectionEdit = new QLineEdit();
    m_groovingCrossSectionEdit->setPlaceholderText("Grooving cross-section code");
    isoLayout->addRow("Cross Section:", m_groovingCrossSectionEdit);
    
    m_groovingMaterialCombo = new QComboBox();
    m_groovingMaterialCombo->addItem("Uncoated Carbide", static_cast<int>(InsertMaterial::UNCOATED_CARBIDE));
    m_groovingMaterialCombo->addItem("Coated Carbide", static_cast<int>(InsertMaterial::COATED_CARBIDE));
    m_groovingMaterialCombo->addItem("Cermet", static_cast<int>(InsertMaterial::CERMET));
    m_groovingMaterialCombo->addItem("Ceramic", static_cast<int>(InsertMaterial::CERAMIC));
    m_groovingMaterialCombo->addItem("CBN", static_cast<int>(InsertMaterial::CBN));
    m_groovingMaterialCombo->addItem("PCD", static_cast<int>(InsertMaterial::PCD));
    m_groovingMaterialCombo->addItem("HSS", static_cast<int>(InsertMaterial::HSS));
    isoLayout->addRow("Grooving Material:", m_groovingMaterialCombo);
    
    m_groovingInsertLayout->addRow(isoGroup);
    
    // Grooving Dimensions Group
    auto dimGroup = new QGroupBox("Grooving Dimensions");
    auto dimLayout = new QFormLayout(dimGroup);
    
    m_groovingThicknessSpin = new QDoubleSpinBox();
    m_groovingThicknessSpin->setRange(0.0, 20.0);
    m_groovingThicknessSpin->setDecimals(3);
    m_groovingThicknessSpin->setSuffix(" mm");
    dimLayout->addRow("Thickness:", m_groovingThicknessSpin);
    
    m_groovingOverallLengthSpin = new QDoubleSpinBox();
    m_groovingOverallLengthSpin->setRange(0.0, 100.0);
    m_groovingOverallLengthSpin->setDecimals(3);
    m_groovingOverallLengthSpin->setSuffix(" mm");
    dimLayout->addRow("Overall Length:", m_groovingOverallLengthSpin);
    
    m_groovingWidthSpin = new QDoubleSpinBox();
    m_groovingWidthSpin->setRange(0.0, 50.0);
    m_groovingWidthSpin->setDecimals(3);
    m_groovingWidthSpin->setSuffix(" mm");
    dimLayout->addRow("Width:", m_groovingWidthSpin);
    
    m_groovingCornerRadiusSpin = new QDoubleSpinBox();
    m_groovingCornerRadiusSpin->setRange(0.0, 5.0);
    m_groovingCornerRadiusSpin->setDecimals(3);
    m_groovingCornerRadiusSpin->setSuffix(" mm");
    dimLayout->addRow("Corner Radius:", m_groovingCornerRadiusSpin);
    
    m_groovingHeadLengthSpin = new QDoubleSpinBox();
    m_groovingHeadLengthSpin->setRange(0.0, 50.0);
    m_groovingHeadLengthSpin->setDecimals(3);
    m_groovingHeadLengthSpin->setSuffix(" mm");
    dimLayout->addRow("Head Length:", m_groovingHeadLengthSpin);
    
    m_grooveWidthSpin = new QDoubleSpinBox();
    m_grooveWidthSpin->setRange(0.0, 20.0);
    m_grooveWidthSpin->setDecimals(3);
    m_grooveWidthSpin->setSuffix(" mm");
    dimLayout->addRow("Groove Width (Cutting):", m_grooveWidthSpin);
    
    m_groovingInsertLayout->addRow(dimGroup);
} 