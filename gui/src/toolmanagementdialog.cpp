#include "toolmanagementdialog.h"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSet>

#include "toolmanager.h"
#include "opengl3dwidget.h"

// OpenCASCADE includes for geometry generation
#include <Standard_Failure.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_ViewCube.hxx>
#include <math.h>

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
    , m_currentToolType(ToolType::GENERAL_TURNING)  // This will be updated when loading tool data
    , m_isNewTool(false)
    , m_dataModified(false)
    , m_autoSaveTimer(new QTimer(this))
    , m_autoSaveEnabled(true)
    , m_toolManager(nullptr)
{
    // Load tool data first to get the correct tool type
    if (loadToolAssemblyFromDatabase(toolId)) {
        m_currentToolType = m_currentToolAssembly.toolType;
        qDebug() << "Loaded existing tool type:" << static_cast<int>(m_currentToolType);
    } else {
        qDebug() << "Tool not found in database, defaulting to GENERAL_TURNING:" << toolId;
        // If tool not found, default to GENERAL_TURNING
        m_currentToolType = ToolType::GENERAL_TURNING;
    }
    
    setupUI();
    setupAutoSave();
    
    // Load the tool data into fields after UI is setup
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
    , m_toolManager(nullptr)
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
    
    // Create tool type selector
    createToolTypeSelector();
    
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
    
    // View controls are set up in setupViewControls() method
    // which is called from setup3DViewer()
    
    // Add to main layout
    m_contentLayout->addWidget(m_visualization3DPanel, 1);
}

void ToolManagementDialog::createToolTypeSelector() {
    // Create tool type selection group
    auto toolTypeGroup = new QGroupBox("Tool Type");
    auto toolTypeLayout = new QHBoxLayout(toolTypeGroup);
    
    // Tool type combo box
    m_toolTypeCombo = new QComboBox();
    m_toolTypeCombo->addItem("General Turning", static_cast<int>(ToolType::GENERAL_TURNING));
    m_toolTypeCombo->addItem("Boring", static_cast<int>(ToolType::BORING));
    m_toolTypeCombo->addItem("Threading", static_cast<int>(ToolType::THREADING));
    m_toolTypeCombo->addItem("Grooving", static_cast<int>(ToolType::GROOVING));
    m_toolTypeCombo->addItem("Parting", static_cast<int>(ToolType::PARTING));
    m_toolTypeCombo->addItem("Form Tool", static_cast<int>(ToolType::FORM_TOOL));
    
    // Set current tool type
    setComboBoxByValue(m_toolTypeCombo, static_cast<int>(m_currentToolType));
    
    toolTypeLayout->addWidget(new QLabel("Select Tool Type:"));
    toolTypeLayout->addWidget(m_toolTypeCombo);
    toolTypeLayout->addStretch();
    
    // Connect signal
    connect(m_toolTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementDialog::onToolTypeChanged);
    
    m_toolEditLayout->addWidget(toolTypeGroup);
}

void ToolManagementDialog::setup3DViewer() {
    m_3dViewer = new OpenGL3DWidget();
    m_3dViewer->setMinimumSize(350, 300);
    m_visualizationLayout->addWidget(m_3dViewer);
    
    // Setup view controls
    setupViewControls();
    
    // Initialize visualization state
    m_currentVisualizationMode = 1; // Shaded
    m_showDimensions = false;
    m_showAnnotations = false;
    m_currentZoomLevel = 1.0;
    
    // Wait for viewer initialization, then set up tool visualization
    connect(m_3dViewer, &OpenGL3DWidget::viewerInitialized, this, [this]() {
        qDebug() << "3D Viewer initialized, setting up tool visualization";
        // Initial tool geometry generation
        generate3DToolGeometry();
    });
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
    
    // Save to database/file system
    bool wasNewTool = m_isNewTool;
    if (saveToolAssemblyToDatabase()) {
        qDebug() << "Successfully saved tool:" << m_currentToolId;
        
        m_dataModified = false;
        
        // Update window title to remove unsaved indicator and reflect new state
        QString currentTitle = windowTitle();
        if (currentTitle.endsWith(" *")) {
            currentTitle = currentTitle.left(currentTitle.length() - 2);
        }
        
        // If this was a new tool that just got saved, update the title to show it's now editing
        if (wasNewTool && !m_isNewTool) {
            setWindowTitle(QString("Edit Tool: %1").arg(m_currentToolId));
        } else {
            setWindowTitle(currentTitle);
        }
        
        emit toolSaved(m_currentToolId);
    } else {
        qWarning() << "Failed to save tool:" << m_currentToolId;
    }
}

void ToolManagementDialog::connectParameterSignals() {
    // Connect tool type selector
    if (m_toolTypeCombo) {
        connect(m_toolTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ToolManagementDialog::onToolTypeChanged);
    }
    
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
    
    // Tool Capabilities Parameters (moved to Tool Info tab)
    if (m_internalThreadingCheck) {
        connect(m_internalThreadingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_internalBoringCheck) {
        connect(m_internalBoringCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_partingGroovingCheck) {
        connect(m_partingGroovingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_externalThreadingCheck) {
        connect(m_externalThreadingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_longitudinalTurningCheck) {
        connect(m_longitudinalTurningCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_facingCheck) {
        connect(m_facingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_chamferingCheck) {
        connect(m_chamferingCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onToolInfoChanged);
    }
    
    // Cutting Data Parameters
    if (m_constantSurfaceSpeedCheck) {
        connect(m_constantSurfaceSpeedCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
        connect(m_constantSurfaceSpeedCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onConstantSurfaceSpeedToggled);
    }
    if (m_surfaceSpeedSpin) {
        connect(m_surfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_spindleRPMSpin) {
        connect(m_spindleRPMSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ToolManagementDialog::onCuttingDataChanged);
    }
    if (m_feedPerRevolutionCheck) {
        connect(m_feedPerRevolutionCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onCuttingDataChanged);
        connect(m_feedPerRevolutionCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onFeedPerRevolutionToggled);
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

// Load tool data from tool ID
void ToolManagementDialog::loadToolData(const QString& toolId) {
    qDebug() << "Loading tool data for:" << toolId;
    
    // Try to load from database first
    if (loadToolAssemblyFromDatabase(toolId)) {
        qDebug() << "Successfully loaded tool from database:" << toolId;
        
        // loadToolParametersIntoFields will handle tool type sync and UI updates
        
    } else {
        qDebug() << "Tool not found in database, creating new assembly:" << toolId;
        // Create a new tool assembly for this ID
        m_currentToolAssembly = ToolAssembly();
        m_currentToolAssembly.id = toolId.toStdString();
        m_currentToolAssembly.name = QString("Tool %1").arg(toolId).toStdString();
        m_currentToolAssembly.toolType = m_currentToolType;
        
        // Setup comprehensive default parameters
        setupDefaultToolParameters(m_currentToolType);
    }
    
    // Load the data into UI fields (this will handle tool type syncing)
    loadToolParametersIntoFields(m_currentToolAssembly);
}

void ToolManagementDialog::initializeNewTool(ToolType toolType) {
    m_currentToolType = toolType;
    qDebug() << "Initializing new tool of type:" << static_cast<int>(toolType);
    
    // Initialize a new tool assembly with defaults
    m_currentToolAssembly = ToolAssembly();
    m_currentToolAssembly.toolType = toolType;
    m_currentToolAssembly.isActive = true;
    
    // Generate a proper unique ID for new tools based on tool type and timestamp
    QString typePrefix = getToolTypePrefix(toolType);
    QString uniqueId = generateUniqueToolId(typePrefix);
    m_currentToolId = uniqueId;
    m_currentToolAssembly.id = m_currentToolId.toStdString();
    m_currentToolAssembly.name = QString("New %1 Tool").arg(formatToolType(toolType)).toStdString();
    
    // Setup comprehensive default parameters
    setupDefaultToolParameters(toolType);
    
    // Update UI based on tool type
    updateToolTypeSpecificUI();
}

QString ToolManagementDialog::getToolTypePrefix(ToolType toolType) const {
    switch (toolType) {
        case ToolType::GENERAL_TURNING: return "GT";
        case ToolType::THREADING: return "TH";
        case ToolType::GROOVING: return "GR"; 
        case ToolType::PARTING: return "PT";
        case ToolType::BORING: return "BR";
        case ToolType::FORM_TOOL: return "FT";
        case ToolType::LIVE_TOOLING: return "LT";
        default: return "CT"; // Custom Tool
    }
}

QString ToolManagementDialog::generateUniqueToolId(const QString& prefix) const {
    // Load existing database to check for existing IDs
    QString dbPath = getToolAssemblyDatabasePath();
    QSet<QString> existingIds;
    
    QFile file(dbPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject database = doc.object();
            QJsonArray toolsArray = database["tools"].toArray();
            
            for (const QJsonValue& toolValue : toolsArray) {
                QJsonObject toolObj = toolValue.toObject();
                QString existingId = toolObj["id"].toString();
                if (!existingId.isEmpty()) {  // Skip empty IDs
                    existingIds.insert(existingId);
                }
            }
        }
    }
    
    // Generate unique ID with format: PREFIX_YYYYMMDD_NNN (e.g., GT_20241215_001)
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString baseId = QString("%1_%2").arg(prefix, dateStr);
    
    // Find next available sequence number
    int sequence = 1;
    QString candidateId;
    do {
        candidateId = QString("%1_%2").arg(baseId).arg(sequence, 3, 10, QChar('0'));
        sequence++;
    } while (existingIds.contains(candidateId) && sequence <= 999);
    
    if (sequence > 999) {
        // Fallback to timestamp-based ID if we've exhausted sequence numbers
        qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        candidateId = QString("%1_%2").arg(prefix).arg(timestamp);
        
        // Ensure this fallback ID is also unique
        while (existingIds.contains(candidateId)) {
            timestamp++;
            candidateId = QString("%1_%2").arg(prefix).arg(timestamp);
        }
    }
    
    qDebug() << "Generated unique tool ID:" << candidateId;
    qDebug() << "Checked against" << existingIds.size() << "existing IDs";
    return candidateId;
}

bool ToolManagementDialog::validateCurrentTool() {
    // TODO: Implement validation logic
    return true;
}

void ToolManagementDialog::updateToolAssemblyFromFields() {
    if (!validateCurrentTool()) {
        qDebug() << "Tool validation failed, not updating assembly";
        return;
    }
    
    // Update tool type from combo box
    if (m_toolTypeCombo) {
        ToolType selectedType = static_cast<ToolType>(m_toolTypeCombo->currentData().toInt());
        m_currentToolType = selectedType;
        m_currentToolAssembly.toolType = selectedType;
    }
    
    // Update based on current tool type
    switch (m_currentToolType) {
        case ToolType::GENERAL_TURNING:
        case ToolType::BORING:
        case ToolType::FORM_TOOL:
            updateGeneralTurningInsertFromFields();
            break;
        case ToolType::THREADING:
            updateThreadingInsertFromFields();
            break;
        case ToolType::GROOVING:
        case ToolType::PARTING:
            updateGroovingInsertFromFields();
            break;
        default:
            qDebug() << "Unknown tool type for update:" << static_cast<int>(m_currentToolType);
            break;
    }
    
    // Always update holder and cutting data
    updateHolderDataFromFields();
    updateCuttingDataFromFields();
    updateToolInfoFromFields();
}

void ToolManagementDialog::loadToolParametersIntoFields(const ToolAssembly& assembly) {
    // Update current tool type and sync combo box
    m_currentToolType = assembly.toolType;
    if (m_toolTypeCombo) {
        m_toolTypeCombo->blockSignals(true);
        setComboBoxByValue(m_toolTypeCombo, static_cast<int>(assembly.toolType));
        m_toolTypeCombo->blockSignals(false);
    }
    
    // Update UI based on tool type
    updateToolTypeSpecificUI();
    
    // Load tool information
    if (m_toolNameEdit) {
        m_toolNameEdit->setText(QString::fromStdString(assembly.name));
    }
    if (m_toolNumberEdit) {
        m_toolNumberEdit->setText(QString::fromStdString(assembly.toolNumber));
    }
    if (m_turretPositionSpin) {
        m_turretPositionSpin->setValue(assembly.turretPosition);
    }
    if (m_isActiveCheck) {
        m_isActiveCheck->setChecked(assembly.isActive);
    }
    if (m_toolOffsetXSpin) {
        m_toolOffsetXSpin->setValue(assembly.toolOffset_X);
    }
    if (m_toolOffsetZSpin) {
        m_toolOffsetZSpin->setValue(assembly.toolOffset_Z);
    }
    if (m_toolLengthOffsetSpin) {
        m_toolLengthOffsetSpin->setValue(assembly.toolLengthOffset);
    }
    if (m_toolRadiusOffsetSpin) {
        m_toolRadiusOffsetSpin->setValue(assembly.toolRadiusOffset);
    }
    if (m_notesEdit) {
        m_notesEdit->setPlainText(QString::fromStdString(assembly.notes));
    }
    
    // Load tool capabilities
    if (m_internalThreadingCheck) {
        m_internalThreadingCheck->setChecked(assembly.internalThreading);
    }
    if (m_internalBoringCheck) {
        m_internalBoringCheck->setChecked(assembly.internalBoring);
    }
    if (m_partingGroovingCheck) {
        m_partingGroovingCheck->setChecked(assembly.partingGrooving);
    }
    if (m_externalThreadingCheck) {
        m_externalThreadingCheck->setChecked(assembly.externalThreading);
    }
    if (m_longitudinalTurningCheck) {
        m_longitudinalTurningCheck->setChecked(assembly.longitudinalTurning);
    }
    if (m_facingCheck) {
        m_facingCheck->setChecked(assembly.facing);
    }
    if (m_chamferingCheck) {
        m_chamferingCheck->setChecked(assembly.chamfering);
    }
    
    // Load cutting data
    loadCuttingDataParameters(assembly.cuttingData);
    
    // Load holder parameters
    if (assembly.holder) {
        loadHolderParameters(*assembly.holder);
    }
    
    // Load insert parameters based on tool type
    switch (assembly.toolType) {
        case ToolType::GENERAL_TURNING:
            if (assembly.turningInsert) {
                loadGeneralTurningInsertParameters(*assembly.turningInsert);
            }
            break;
        case ToolType::THREADING:
            if (assembly.threadingInsert) {
                loadThreadingInsertParameters(*assembly.threadingInsert);
            }
            break;
        case ToolType::GROOVING:
            if (assembly.groovingInsert) {
                loadGroovingInsertParameters(*assembly.groovingInsert);
            }
            break;
        default:
            qDebug() << "Unknown tool type for loading:" << static_cast<int>(assembly.toolType);
            break;
    }
}

// Tab creation methods - implement complete parameter sets
QWidget* ToolManagementDialog::createInsertPropertiesTab() {
    auto widget = new QWidget();
    auto mainLayout = new QVBoxLayout(widget);
    
    // Create individual insert type panels
    createGeneralTurningPanel();
    createThreadingPanel();
    createGroovingPanel();
    
    // Add all panels to the main layout (initially hidden)
    mainLayout->addWidget(m_turningInsertTab);
    mainLayout->addWidget(m_threadingInsertTab);
    mainLayout->addWidget(m_groovingInsertTab);
    
    // Initially hide all tabs and show the one for the current tool type
    hideAllInsertTabs();
    showToolTypeSpecificTabs(m_currentToolType);
    
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
    
    // Tool Capabilities Group (moved from Tool Holder tab)
    auto capabilitiesGroup = new QGroupBox("Tool Capabilities");
    auto capLayout = new QFormLayout(capabilitiesGroup);
    
    m_internalThreadingCheck = new QCheckBox();
    capLayout->addRow("Internal Threading:", m_internalThreadingCheck);
    
    m_internalBoringCheck = new QCheckBox();
    capLayout->addRow("Internal Boring:", m_internalBoringCheck);
    
    m_partingGroovingCheck = new QCheckBox();
    capLayout->addRow("Parting/Grooving:", m_partingGroovingCheck);
    
    m_externalThreadingCheck = new QCheckBox();
    capLayout->addRow("External Threading:", m_externalThreadingCheck);
    
    m_longitudinalTurningCheck = new QCheckBox();
    capLayout->addRow("Longitudinal Turning:", m_longitudinalTurningCheck);
    
    m_facingCheck = new QCheckBox();
    capLayout->addRow("Facing:", m_facingCheck);
    
    m_chamferingCheck = new QCheckBox();
    capLayout->addRow("Chamfering:", m_chamferingCheck);
    
    m_toolInfoLayout->addRow(capabilitiesGroup);
    
    return widget;
}

// 3D visualization methods
void ToolManagementDialog::generate3DToolGeometry() {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        qDebug() << "3D Viewer not ready, skipping geometry generation";
        return;
    }

    clearPreviousToolGeometry();
    
    try {
        // Generate tool geometry based on current tool assembly
        TopoDS_Shape assembledTool = createAssembledToolGeometry();
        
        if (!assembledTool.IsNull()) {
            m_currentToolGeometry = assembledTool;
            
            // Create AIS shape for display
            m_currentAssembledShape = new AIS_Shape(assembledTool);
            
            // Apply material and color
            applyMaterialToShape(m_currentAssembledShape, "tool");
            
            // Display in viewer
            auto context = m_3dViewer->getContext();
            if (!context.IsNull()) {
                context->Display(m_currentAssembledShape, Standard_False);
                updateVisualizationMode(m_currentVisualizationMode);
                
                // Fit view to show the tool
                fitViewToTool();
                
                emit toolGeometryUpdated(assembledTool);
                qDebug() << "Tool geometry generated and displayed successfully";
            }
        }
    } catch (const Standard_Failure& e) {
        qWarning() << "Failed to generate tool geometry:" << e.GetMessageString();
    } catch (const std::exception& e) {
        qWarning() << "Failed to generate tool geometry:" << e.what();
    }
}

void ToolManagementDialog::updateRealTime3DVisualization() {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    // Update the visualization in real-time as parameters change
    generate3DToolGeometry();
}

void ToolManagementDialog::updateToolVisualization() {
    generate3DToolGeometry();
    updateRealTime3DVisualization();
}

void ToolManagementDialog::setupViewControls() {
    m_viewControlsGroup = new QGroupBox("View Controls");
    auto controlsLayout = new QVBoxLayout(m_viewControlsGroup);
    
    // Visualization mode
    auto modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel("Mode:"));
    m_visualizationModeCombo = new QComboBox();
    m_visualizationModeCombo->addItem("Wireframe", 0);
    m_visualizationModeCombo->addItem("Shaded", 1);
    m_visualizationModeCombo->addItem("Shaded + Edges", 2);
    m_visualizationModeCombo->setCurrentIndex(1); // Default to shaded
    modeLayout->addWidget(m_visualizationModeCombo);
    controlsLayout->addLayout(modeLayout);
    
    // View mode buttons
    auto viewModeLayout = new QGridLayout();
    m_wireframeButton = new QPushButton("Wireframe");
    m_shadedButton = new QPushButton("Shaded");
    m_shadedEdgesButton = new QPushButton("Shaded+Edges");
    
    m_wireframeButton->setCheckable(true);
    m_shadedButton->setCheckable(true);
    m_shadedEdgesButton->setCheckable(true);
    m_shadedButton->setChecked(true); // Default
    
    viewModeLayout->addWidget(m_wireframeButton, 0, 0);
    viewModeLayout->addWidget(m_shadedButton, 0, 1);
    viewModeLayout->addWidget(m_shadedEdgesButton, 0, 2);
    controlsLayout->addLayout(viewModeLayout);
    
    // Standard views
    auto standardViewsLayout = new QGridLayout();
    m_isometricViewButton = new QPushButton("Isometric");
    m_frontViewButton = new QPushButton("Front");
    m_topViewButton = new QPushButton("Top");
    m_rightViewButton = new QPushButton("Right");
    
    standardViewsLayout->addWidget(m_isometricViewButton, 0, 0);
    standardViewsLayout->addWidget(m_frontViewButton, 0, 1);
    standardViewsLayout->addWidget(m_topViewButton, 1, 0);
    standardViewsLayout->addWidget(m_rightViewButton, 1, 1);
    controlsLayout->addLayout(standardViewsLayout);
    
    // View control buttons
    auto buttonLayout = new QHBoxLayout();
    m_fitViewButton = new QPushButton("Fit View");
    m_resetViewButton = new QPushButton("Reset View");
    buttonLayout->addWidget(m_fitViewButton);
    buttonLayout->addWidget(m_resetViewButton);
    controlsLayout->addLayout(buttonLayout);
    
    // Zoom control
    auto zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(new QLabel("Zoom:"));
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 500); // 10% to 500%
    m_zoomSlider->setValue(100); // 100% default
    m_zoomLabel = new QLabel("100%");
    zoomLayout->addWidget(m_zoomSlider);
    zoomLayout->addWidget(m_zoomLabel);
    controlsLayout->addLayout(zoomLayout);
    
    // Display options
    m_showDimensionsCheck = new QCheckBox("Show Dimensions");
    m_showAnnotationsCheck = new QCheckBox("Show Annotations");
    controlsLayout->addWidget(m_showDimensionsCheck);
    controlsLayout->addWidget(m_showAnnotationsCheck);
    
    m_visualizationLayout->addWidget(m_viewControlsGroup);
    
    // Connect signals
    connect(m_visualizationModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementDialog::onVisualizationModeChanged);
    connect(m_fitViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onFitViewClicked);
    connect(m_resetViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onResetViewClicked);
    connect(m_wireframeButton, &QPushButton::clicked, this, &ToolManagementDialog::onWireframeClicked);
    connect(m_shadedButton, &QPushButton::clicked, this, &ToolManagementDialog::onShadedClicked);
    connect(m_shadedEdgesButton, &QPushButton::clicked, this, &ToolManagementDialog::onShadedWithEdgesClicked);
    connect(m_isometricViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onIsometricViewClicked);
    connect(m_frontViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onFrontViewClicked);
    connect(m_topViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onTopViewClicked);
    connect(m_rightViewButton, &QPushButton::clicked, this, &ToolManagementDialog::onRightViewClicked);
    connect(m_showDimensionsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onShowDimensionsChanged);
    connect(m_showAnnotationsCheck, &QCheckBox::toggled, this, &ToolManagementDialog::onShowAnnotationsChanged);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ToolManagementDialog::onZoomChanged);
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

void ToolManagementDialog::onConstantSurfaceSpeedToggled(bool enabled) {
    if (!m_surfaceSpeedSpin || !m_spindleRPMSpin) {
        return;
    }
    
    // If Constant Surface Speed is ON → hide Spindle RPM, show Surface Speed
    // If Constant Surface Speed is OFF → hide Surface Speed, show Spindle RPM
    m_surfaceSpeedSpin->setVisible(enabled);
    m_spindleRPMSpin->setVisible(!enabled);
    
    // Find and update the corresponding labels
    // We need to search through the speed group's form layout
    auto speedGroup = m_surfaceSpeedSpin->parentWidget();
    while (speedGroup && !speedGroup->objectName().contains("Speed") && speedGroup->parentWidget()) {
        speedGroup = speedGroup->parentWidget();
    }
    
    if (speedGroup) {
        auto speedLayout = speedGroup->findChild<QFormLayout*>();
        if (speedLayout) {
            // Find the surface speed and spindle RPM rows
            for (int i = 0; i < speedLayout->rowCount(); ++i) {
                auto item = speedLayout->itemAt(i, QFormLayout::FieldRole);
                if (item && item->widget()) {
                    if (item->widget() == m_surfaceSpeedSpin) {
                        auto labelItem = speedLayout->itemAt(i, QFormLayout::LabelRole);
                        if (labelItem && labelItem->widget()) {
                            labelItem->widget()->setVisible(enabled);
                        }
                    } else if (item->widget() == m_spindleRPMSpin) {
                        auto labelItem = speedLayout->itemAt(i, QFormLayout::LabelRole);
                        if (labelItem && labelItem->widget()) {
                            labelItem->widget()->setVisible(!enabled);
                        }
                    }
                }
            }
        }
    }
    
    markAsModified();
}

void ToolManagementDialog::onFeedPerRevolutionToggled(bool enabled) {
    // Update units for feed rate controls
    updateFeedRateUnits(enabled);
    markAsModified();
}

void ToolManagementDialog::onVisualizationModeChanged(int mode) {
    m_currentVisualizationMode = mode;
    updateVisualizationMode(mode);
}

void ToolManagementDialog::onFitViewClicked() {
    fitViewToTool();
}

void ToolManagementDialog::onResetViewClicked() {
    resetCameraPosition();
    m_zoomSlider->setValue(100);
    m_zoomLabel->setText("100%");
}

void ToolManagementDialog::onShowDimensionsChanged(bool show) {
    m_showDimensions = show;
    // TODO: Implement dimension overlay
    qDebug() << "Show dimensions:" << show;
}

void ToolManagementDialog::onShowAnnotationsChanged(bool show) {
    m_showAnnotations = show;
    // TODO: Implement annotation overlay
    qDebug() << "Show annotations:" << show;
}

void ToolManagementDialog::onZoomChanged(int value) {
    m_currentZoomLevel = value / 100.0;
    m_zoomLabel->setText(QString("%1%").arg(value));
    
    if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
        auto context = m_3dViewer->getContext();
        if (!context.IsNull() && !context->CurrentViewer().IsNull()) {
            auto view = context->CurrentViewer()->ActiveViews().First();
            if (!view.IsNull()) {
                view->SetZoom(m_currentZoomLevel);
                view->Redraw();
            }
        }
    }
}

// New slot implementations for view controls
void ToolManagementDialog::onWireframeClicked() {
    m_wireframeButton->setChecked(true);
    m_shadedButton->setChecked(false);
    m_shadedEdgesButton->setChecked(false);
    m_visualizationModeCombo->setCurrentIndex(0);
    updateVisualizationMode(0);
}

void ToolManagementDialog::onShadedClicked() {
    m_wireframeButton->setChecked(false);
    m_shadedButton->setChecked(true);
    m_shadedEdgesButton->setChecked(false);
    m_visualizationModeCombo->setCurrentIndex(1);
    updateVisualizationMode(1);
}

void ToolManagementDialog::onShadedWithEdgesClicked() {
    m_wireframeButton->setChecked(false);
    m_shadedButton->setChecked(false);
    m_shadedEdgesButton->setChecked(true);
    m_visualizationModeCombo->setCurrentIndex(2);
    updateVisualizationMode(2);
}

void ToolManagementDialog::onIsometricViewClicked() {
    setStandardView(gp_Dir(1, 1, 1), gp_Dir(0, 0, 1));
}

void ToolManagementDialog::onFrontViewClicked() {
    setStandardView(gp_Dir(0, -1, 0), gp_Dir(0, 0, 1));
}

void ToolManagementDialog::onTopViewClicked() {
    setStandardView(gp_Dir(0, 0, 1), gp_Dir(0, 1, 0));
}

void ToolManagementDialog::onRightViewClicked() {
    setStandardView(gp_Dir(1, 0, 0), gp_Dir(0, 0, 1));
}

void ToolManagementDialog::updateViewControlsState() {
    if (!m_viewControlsGroup) {
        return;
    }
    
    // Update button states based on current visualization mode
    switch (m_currentVisualizationMode) {
        case 0: // Wireframe
            if (m_wireframeButton) m_wireframeButton->setChecked(true);
            if (m_shadedButton) m_shadedButton->setChecked(false);
            if (m_shadedEdgesButton) m_shadedEdgesButton->setChecked(false);
            break;
        case 1: // Shaded
            if (m_wireframeButton) m_wireframeButton->setChecked(false);
            if (m_shadedButton) m_shadedButton->setChecked(true);
            if (m_shadedEdgesButton) m_shadedEdgesButton->setChecked(false);
            break;
        case 2: // Shaded + Edges
            if (m_wireframeButton) m_wireframeButton->setChecked(false);
            if (m_shadedButton) m_shadedButton->setChecked(false);
            if (m_shadedEdgesButton) m_shadedEdgesButton->setChecked(true);
            break;
    }
    
    // Update combo box if it doesn't match
    if (m_visualizationModeCombo && m_visualizationModeCombo->currentIndex() != m_currentVisualizationMode) {
        m_visualizationModeCombo->blockSignals(true);
        m_visualizationModeCombo->setCurrentIndex(m_currentVisualizationMode);
        m_visualizationModeCombo->blockSignals(false);
    }
    
    // Update zoom label
    if (m_zoomLabel && m_zoomSlider) {
        m_zoomLabel->setText(QString("%1%").arg(m_zoomSlider->value()));
    }
    
    // Update checkbox states
    if (m_showDimensionsCheck) {
        m_showDimensionsCheck->setChecked(m_showDimensions);
    }
    if (m_showAnnotationsCheck) {
        m_showAnnotationsCheck->setChecked(m_showAnnotations);
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

void ToolManagementDialog::onToolTypeChanged(int index) {
    if (index < 0) return;
    
    // Get the selected tool type
    ToolType newToolType = static_cast<ToolType>(m_toolTypeCombo->itemData(index).toInt());
    
    // Only proceed if tool type actually changed
    if (newToolType == m_currentToolType) {
        return;
    }
    
    // Warn the user about losing configured parameters
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Change Tool Type", 
        QString("Changing the tool type from %1 to %2 will discard all currently configured parameters. Do you want to proceed?")
            .arg(formatToolType(m_currentToolType))
            .arg(formatToolType(newToolType)),
        QMessageBox::Yes | QMessageBox::No, 
        QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        // User cancelled, revert combo box selection
        m_toolTypeCombo->blockSignals(true);
        setComboBoxByValue(m_toolTypeCombo, static_cast<int>(m_currentToolType));
        m_toolTypeCombo->blockSignals(false);
        return;
    }
    
    qDebug() << "Tool type changed from" << static_cast<int>(m_currentToolType) 
             << "to" << static_cast<int>(newToolType);
    
    // Update current tool type
    m_currentToolType = newToolType;
    m_currentToolAssembly.toolType = newToolType;
    
    // Show/hide appropriate UI elements
    showToolTypeSpecificTabs(newToolType);
    
    // Setup default parameters for the new tool type
    setupDefaultToolParameters(newToolType);
    
    // Mark as modified to trigger auto-save
    markAsModified();
}

void ToolManagementDialog::hideAllInsertTabs() {
    if (m_turningInsertTab) m_turningInsertTab->setVisible(false);
    if (m_threadingInsertTab) m_threadingInsertTab->setVisible(false);
    if (m_groovingInsertTab) m_groovingInsertTab->setVisible(false);
}

void ToolManagementDialog::showToolTypeSpecificTabs(ToolType toolType) {
    // Hide all tabs first
    hideAllInsertTabs();
    
    // Show the appropriate tab based on tool type
    switch (toolType) {
        case ToolType::GENERAL_TURNING:
        case ToolType::BORING:
        case ToolType::FORM_TOOL:
            if (m_turningInsertTab) m_turningInsertTab->setVisible(true);
            break;
            
        case ToolType::THREADING:
            if (m_threadingInsertTab) m_threadingInsertTab->setVisible(true);
            break;
            
        case ToolType::GROOVING:
        case ToolType::PARTING:
            if (m_groovingInsertTab) m_groovingInsertTab->setVisible(true);
            break;
            
        default:
            // Default to general turning
            if (m_turningInsertTab) m_turningInsertTab->setVisible(true);
            break;
    }
}

void ToolManagementDialog::setupDefaultToolParameters(ToolType toolType) {
    // Clear existing tool assembly data
    m_currentToolAssembly = ToolAssembly();
    m_currentToolAssembly.toolType = toolType;
    
    // Initialize with comprehensive default parameters based on tool type
    switch (toolType) {
        case ToolType::GENERAL_TURNING:
            setupGeneralTurningDefaults();
            break;
        case ToolType::BORING:
            setupBoringDefaults();
            break;
        case ToolType::THREADING:
            setupThreadingDefaults();
            break;
        case ToolType::GROOVING:
            setupGroovingDefaults();
            break;
        case ToolType::PARTING:
            setupPartingDefaults();
            break;
        case ToolType::FORM_TOOL:
            setupFormToolDefaults();
            break;
        default:
            setupGeneralTurningDefaults();
            break;
    }
    
    // Set up tool capabilities based on tool type  
    setupCapabilitiesForToolType(toolType);
    
    // Always setup holder defaults
    setupHolderDefaults();
    setupCuttingDataDefaults(toolType);
    
    // Load the defaults into the UI fields
    loadToolParametersIntoFields(m_currentToolAssembly);
}

void ToolManagementDialog::updateToolTypeSpecificUI() {
    showToolTypeSpecificTabs(m_currentToolType);
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

void ToolManagementDialog::updateFeedRateUnits(bool feedPerRevolution) {
    QString unitSuffix = feedPerRevolution ? " mm/rev" : " mm/min";
    
    // Update cutting feedrate
    if (m_cuttingFeedrateSpin) {
        m_cuttingFeedrateSpin->setSuffix(unitSuffix);
        // Adjust ranges if needed - typically mm/rev values are much smaller
        if (feedPerRevolution) {
            m_cuttingFeedrateSpin->setRange(0.0, 10.0);
            m_cuttingFeedrateSpin->setDecimals(3);
        } else {
            m_cuttingFeedrateSpin->setRange(0.0, 1000.0);
            m_cuttingFeedrateSpin->setDecimals(1);
        }
    }
    
    // Update lead-in feedrate
    if (m_leadInFeedrateSpin) {
        m_leadInFeedrateSpin->setSuffix(unitSuffix);
        if (feedPerRevolution) {
            m_leadInFeedrateSpin->setRange(0.0, 10.0);
            m_leadInFeedrateSpin->setDecimals(3);
        } else {
            m_leadInFeedrateSpin->setRange(0.0, 1000.0);
            m_leadInFeedrateSpin->setDecimals(1);
        }
    }
    
    // Update lead-out feedrate
    if (m_leadOutFeedrateSpin) {
        m_leadOutFeedrateSpin->setSuffix(unitSuffix);
        if (feedPerRevolution) {
            m_leadOutFeedrateSpin->setRange(0.0, 10.0);
            m_leadOutFeedrateSpin->setDecimals(3);
        } else {
            m_leadOutFeedrateSpin->setRange(0.0, 1000.0);
            m_leadOutFeedrateSpin->setDecimals(1);
        }
    }
}

// Helper method to initialize tool assembly for a specific type
void ToolManagementDialog::initializeToolAssemblyForType(ToolType toolType) {
    switch (toolType) {
        case ToolType::GENERAL_TURNING:
            m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
            // Set reasonable defaults for general turning
            m_currentToolAssembly.turningInsert->inscribedCircle = 12.7; // 1/2 inch IC
            m_currentToolAssembly.turningInsert->thickness = 4.76;
            m_currentToolAssembly.turningInsert->cornerRadius = 0.8;
            m_currentToolAssembly.turningInsert->shape = InsertShape::DIAMOND_55;
            m_currentToolAssembly.turningInsert->reliefAngle = InsertReliefAngle::ANGLE_7;
            m_currentToolAssembly.turningInsert->tolerance = InsertTolerance::M_PRECISION;
            m_currentToolAssembly.turningInsert->material = InsertMaterial::COATED_CARBIDE;
            break;
        case ToolType::THREADING:
            m_currentToolAssembly.threadingInsert = std::make_shared<ThreadingInsert>();
            // Set reasonable defaults for threading
            m_currentToolAssembly.threadingInsert->thickness = 3.18;
            m_currentToolAssembly.threadingInsert->width = 6.0;
            m_currentToolAssembly.threadingInsert->minThreadPitch = 0.5;
            m_currentToolAssembly.threadingInsert->maxThreadPitch = 3.0;
            m_currentToolAssembly.threadingInsert->threadProfile = ThreadProfile::METRIC;
            m_currentToolAssembly.threadingInsert->threadProfileAngle = 60.0;
            break;
        case ToolType::GROOVING:
            m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
            // Set reasonable defaults for grooving
            m_currentToolAssembly.groovingInsert->thickness = 3.0;
            m_currentToolAssembly.groovingInsert->width = 3.0;
            m_currentToolAssembly.groovingInsert->grooveWidth = 3.0;
            break;
        default:
            break;
    }
    
    // Always create a holder
    m_currentToolAssembly.holder = std::make_shared<ToolHolder>();
    m_currentToolAssembly.holder->handOrientation = HandOrientation::RIGHT_HAND;
    m_currentToolAssembly.holder->clampingStyle = ClampingStyle::TOP_CLAMP;
    m_currentToolAssembly.holder->overallLength = 150.0;
    m_currentToolAssembly.holder->shankWidth = 20.0;
    m_currentToolAssembly.holder->shankHeight = 20.0;
    
    // Initialize cutting data with reasonable defaults
    m_currentToolAssembly.cuttingData.constantSurfaceSpeed = true;
    m_currentToolAssembly.cuttingData.surfaceSpeed = 200.0;
    m_currentToolAssembly.cuttingData.feedPerRevolution = true;
    m_currentToolAssembly.cuttingData.cuttingFeedrate = 0.2;
}

// Method to update tool info from fields
void ToolManagementDialog::updateToolInfoFromFields() {
    if (m_toolNameEdit) {
        m_currentToolAssembly.name = m_toolNameEdit->text().toStdString();
    }
    if (m_toolNumberEdit) {
        m_currentToolAssembly.toolNumber = m_toolNumberEdit->text().toStdString();
    }
    if (m_turretPositionSpin) {
        m_currentToolAssembly.turretPosition = m_turretPositionSpin->value();
    }
    if (m_isActiveCheck) {
        m_currentToolAssembly.isActive = m_isActiveCheck->isChecked();
    }
    if (m_toolOffsetXSpin) {
        m_currentToolAssembly.toolOffset_X = m_toolOffsetXSpin->value();
    }
    if (m_toolOffsetZSpin) {
        m_currentToolAssembly.toolOffset_Z = m_toolOffsetZSpin->value();
    }
    if (m_toolLengthOffsetSpin) {
        m_currentToolAssembly.toolLengthOffset = m_toolLengthOffsetSpin->value();
    }
    if (m_toolRadiusOffsetSpin) {
        m_currentToolAssembly.toolRadiusOffset = m_toolRadiusOffsetSpin->value();
    }
    if (m_notesEdit) {
        m_currentToolAssembly.notes = m_notesEdit->toPlainText().toStdString();
    }
    
    // Update tool capabilities
    if (m_internalThreadingCheck) {
        m_currentToolAssembly.internalThreading = m_internalThreadingCheck->isChecked();
    }
    if (m_internalBoringCheck) {
        m_currentToolAssembly.internalBoring = m_internalBoringCheck->isChecked();
    }
    if (m_partingGroovingCheck) {
        m_currentToolAssembly.partingGrooving = m_partingGroovingCheck->isChecked();
    }
    if (m_externalThreadingCheck) {
        m_currentToolAssembly.externalThreading = m_externalThreadingCheck->isChecked();
    }
    if (m_longitudinalTurningCheck) {
        m_currentToolAssembly.longitudinalTurning = m_longitudinalTurningCheck->isChecked();
    }
    if (m_facingCheck) {
        m_currentToolAssembly.facing = m_facingCheck->isChecked();
    }
    if (m_chamferingCheck) {
        m_currentToolAssembly.chamfering = m_chamferingCheck->isChecked();
    }
}

// Parameter loading methods
void ToolManagementDialog::loadGeneralTurningInsertParameters(const GeneralTurningInsert& insert) {
    if (m_isoCodeEdit) {
        m_isoCodeEdit->setText(QString::fromStdString(insert.isoCode));
    }
    if (m_insertShapeCombo) {
        setComboBoxByValue(m_insertShapeCombo, static_cast<int>(insert.shape));
    }
    if (m_reliefAngleCombo) {
        setComboBoxByValue(m_reliefAngleCombo, static_cast<int>(insert.reliefAngle));
    }
    if (m_toleranceCombo) {
        setComboBoxByValue(m_toleranceCombo, static_cast<int>(insert.tolerance));
    }
    if (m_materialCombo) {
        setComboBoxByValue(m_materialCombo, static_cast<int>(insert.material));
    }
    if (m_substrateCombo) {
        m_substrateCombo->setCurrentText(QString::fromStdString(insert.substrate));
    }
    if (m_coatingCombo) {
        m_coatingCombo->setCurrentText(QString::fromStdString(insert.coating));
    }
    if (m_inscribedCircleSpin) {
        m_inscribedCircleSpin->setValue(insert.inscribedCircle);
    }
    if (m_thicknessSpin) {
        m_thicknessSpin->setValue(insert.thickness);
    }
    if (m_cornerRadiusSpin) {
        m_cornerRadiusSpin->setValue(insert.cornerRadius);
    }
    if (m_cuttingEdgeLengthSpin) {
        m_cuttingEdgeLengthSpin->setValue(insert.cuttingEdgeLength);
    }
    if (m_widthSpin) {
        m_widthSpin->setValue(insert.width);
    }
    if (m_rakeAngleSpin) {
        m_rakeAngleSpin->setValue(insert.rake_angle);
    }
    if (m_inclinationAngleSpin) {
        m_inclinationAngleSpin->setValue(insert.inclination_angle);
    }
}

void ToolManagementDialog::loadThreadingInsertParameters(const ThreadingInsert& insert) {
    if (m_threadingISOCodeEdit) {
        m_threadingISOCodeEdit->setText(QString::fromStdString(insert.isoCode));
    }
    if (m_threadingShapeCombo) {
        setComboBoxByValue(m_threadingShapeCombo, static_cast<int>(insert.shape));
    }
    if (m_threadingToleranceCombo) {
        setComboBoxByValue(m_threadingToleranceCombo, static_cast<int>(insert.tolerance));
    }
    if (m_crossSectionEdit) {
        m_crossSectionEdit->setText(QString::fromStdString(insert.crossSection));
    }
    if (m_threadingMaterialCombo) {
        setComboBoxByValue(m_threadingMaterialCombo, static_cast<int>(insert.material));
    }
    if (m_threadingThicknessSpin) {
        m_threadingThicknessSpin->setValue(insert.thickness);
    }
    if (m_threadingWidthSpin) {
        m_threadingWidthSpin->setValue(insert.width);
    }
    if (m_minThreadPitchSpin) {
        m_minThreadPitchSpin->setValue(insert.minThreadPitch);
    }
    if (m_maxThreadPitchSpin) {
        m_maxThreadPitchSpin->setValue(insert.maxThreadPitch);
    }
    if (m_internalThreadsCheck) {
        m_internalThreadsCheck->setChecked(insert.internalThreads);
    }
    if (m_externalThreadsCheck) {
        m_externalThreadsCheck->setChecked(insert.externalThreads);
    }
    if (m_threadProfileCombo) {
        setComboBoxByValue(m_threadProfileCombo, static_cast<int>(insert.threadProfile));
    }
    if (m_threadProfileAngleSpin) {
        m_threadProfileAngleSpin->setValue(insert.threadProfileAngle);
    }
    if (m_threadTipTypeCombo) {
        setComboBoxByValue(m_threadTipTypeCombo, static_cast<int>(insert.threadTipType));
    }
    if (m_threadTipRadiusSpin) {
        m_threadTipRadiusSpin->setValue(insert.threadTipRadius);
    }
}

void ToolManagementDialog::loadGroovingInsertParameters(const GroovingInsert& insert) {
    if (m_groovingISOCodeEdit) {
        m_groovingISOCodeEdit->setText(QString::fromStdString(insert.isoCode));
    }
    if (m_groovingShapeCombo) {
        setComboBoxByValue(m_groovingShapeCombo, static_cast<int>(insert.shape));
    }
    if (m_groovingToleranceCombo) {
        setComboBoxByValue(m_groovingToleranceCombo, static_cast<int>(insert.tolerance));
    }
    if (m_groovingCrossSectionEdit) {
        m_groovingCrossSectionEdit->setText(QString::fromStdString(insert.crossSection));
    }
    if (m_groovingMaterialCombo) {
        setComboBoxByValue(m_groovingMaterialCombo, static_cast<int>(insert.material));
    }
    if (m_groovingThicknessSpin) {
        m_groovingThicknessSpin->setValue(insert.thickness);
    }
    if (m_groovingOverallLengthSpin) {
        m_groovingOverallLengthSpin->setValue(insert.overallLength);
    }
    if (m_groovingWidthSpin) {
        m_groovingWidthSpin->setValue(insert.width);
    }
    if (m_groovingCornerRadiusSpin) {
        m_groovingCornerRadiusSpin->setValue(insert.cornerRadius);
    }
    if (m_groovingHeadLengthSpin) {
        m_groovingHeadLengthSpin->setValue(insert.headLength);
    }
    if (m_grooveWidthSpin) {
        m_grooveWidthSpin->setValue(insert.grooveWidth);
    }
}

void ToolManagementDialog::loadHolderParameters(const ToolHolder& holder) {
    if (m_holderISOCodeEdit) {
        m_holderISOCodeEdit->setText(QString::fromStdString(holder.isoCode));
    }
    if (m_handOrientationCombo) {
        setComboBoxByValue(m_handOrientationCombo, static_cast<int>(holder.handOrientation));
    }
    if (m_clampingStyleCombo) {
        setComboBoxByValue(m_clampingStyleCombo, static_cast<int>(holder.clampingStyle));
    }
    if (m_cuttingWidthSpin) {
        m_cuttingWidthSpin->setValue(holder.cuttingWidth);
    }
    if (m_headLengthSpin) {
        m_headLengthSpin->setValue(holder.headLength);
    }
    if (m_overallLengthSpin) {
        m_overallLengthSpin->setValue(holder.overallLength);
    }
    if (m_shankWidthSpin) {
        m_shankWidthSpin->setValue(holder.shankWidth);
    }
    if (m_shankHeightSpin) {
        m_shankHeightSpin->setValue(holder.shankHeight);
    }
    if (m_roundShankCheck) {
        m_roundShankCheck->setChecked(holder.roundShank);
    }
    if (m_shankDiameterSpin) {
        m_shankDiameterSpin->setValue(holder.shankDiameter);
    }
    if (m_insertSeatAngleSpin) {
        m_insertSeatAngleSpin->setValue(holder.insertSeatAngle);
    }
    if (m_insertSetbackSpin) {
        m_insertSetbackSpin->setValue(holder.insertSetback);
    }
    if (m_sideAngleSpin) {
        m_sideAngleSpin->setValue(holder.sideAngle);
    }
    if (m_backAngleSpin) {
        m_backAngleSpin->setValue(holder.backAngle);
    }
}

void ToolManagementDialog::loadCuttingDataParameters(const CuttingData& cuttingData) {
    if (m_constantSurfaceSpeedCheck) {
        m_constantSurfaceSpeedCheck->setChecked(cuttingData.constantSurfaceSpeed);
    }
    if (m_surfaceSpeedSpin) {
        m_surfaceSpeedSpin->setValue(cuttingData.surfaceSpeed);
    }
    if (m_spindleRPMSpin) {
        m_spindleRPMSpin->setValue(cuttingData.spindleRPM);
    }
    if (m_feedPerRevolutionCheck) {
        m_feedPerRevolutionCheck->setChecked(cuttingData.feedPerRevolution);
    }
    if (m_cuttingFeedrateSpin) {
        m_cuttingFeedrateSpin->setValue(cuttingData.cuttingFeedrate);
    }
    if (m_leadInFeedrateSpin) {
        m_leadInFeedrateSpin->setValue(cuttingData.leadInFeedrate);
    }
    if (m_leadOutFeedrateSpin) {
        m_leadOutFeedrateSpin->setValue(cuttingData.leadOutFeedrate);
    }
    if (m_maxDepthOfCutSpin) {
        m_maxDepthOfCutSpin->setValue(cuttingData.maxDepthOfCut);
    }
    if (m_maxFeedrateSpin) {
        m_maxFeedrateSpin->setValue(cuttingData.maxFeedrate);
    }
    if (m_minSurfaceSpeedSpin) {
        m_minSurfaceSpeedSpin->setValue(cuttingData.minSurfaceSpeed);
    }
    if (m_maxSurfaceSpeedSpin) {
        m_maxSurfaceSpeedSpin->setValue(cuttingData.maxSurfaceSpeed);
    }
    if (m_floodCoolantCheck) {
        m_floodCoolantCheck->setChecked(cuttingData.floodCoolant);
    }
    if (m_mistCoolantCheck) {
        m_mistCoolantCheck->setChecked(cuttingData.mistCoolant);
    }
    if (m_preferredCoolantCombo) {
        setComboBoxByValue(m_preferredCoolantCombo, static_cast<int>(cuttingData.preferredCoolant));
    }
    if (m_coolantPressureSpin) {
        m_coolantPressureSpin->setValue(cuttingData.coolantPressure);
    }
    if (m_coolantFlowSpin) {
        m_coolantFlowSpin->setValue(cuttingData.coolantFlow);
    }
    
    // Apply UI logic based on current settings
    onConstantSurfaceSpeedToggled(cuttingData.constantSurfaceSpeed);
    onFeedPerRevolutionToggled(cuttingData.feedPerRevolution);
}

// Parameter updating methods - sync UI changes back to data
void ToolManagementDialog::updateGeneralTurningInsertFromFields() {
    if (!m_currentToolAssembly.turningInsert) {
        m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.turningInsert;
    
    if (m_isoCodeEdit) {
        insert.isoCode = m_isoCodeEdit->text().toStdString();
    }
    if (m_insertShapeCombo) {
        insert.shape = static_cast<InsertShape>(m_insertShapeCombo->currentData().toInt());
    }
    if (m_reliefAngleCombo) {
        insert.reliefAngle = static_cast<InsertReliefAngle>(m_reliefAngleCombo->currentData().toInt());
    }
    if (m_toleranceCombo) {
        insert.tolerance = static_cast<InsertTolerance>(m_toleranceCombo->currentData().toInt());
    }
    if (m_materialCombo) {
        insert.material = static_cast<InsertMaterial>(m_materialCombo->currentData().toInt());
    }
    if (m_substrateCombo) {
        insert.substrate = m_substrateCombo->currentText().toStdString();
    }
    if (m_coatingCombo) {
        insert.coating = m_coatingCombo->currentText().toStdString();
    }
    if (m_inscribedCircleSpin) {
        insert.inscribedCircle = m_inscribedCircleSpin->value();
    }
    if (m_thicknessSpin) {
        insert.thickness = m_thicknessSpin->value();
    }
    if (m_cornerRadiusSpin) {
        insert.cornerRadius = m_cornerRadiusSpin->value();
    }
    if (m_cuttingEdgeLengthSpin) {
        insert.cuttingEdgeLength = m_cuttingEdgeLengthSpin->value();
    }
    if (m_widthSpin) {
        insert.width = m_widthSpin->value();
    }
    if (m_rakeAngleSpin) {
        insert.rake_angle = m_rakeAngleSpin->value();
    }
    if (m_inclinationAngleSpin) {
        insert.inclination_angle = m_inclinationAngleSpin->value();
    }
}

void ToolManagementDialog::updateThreadingInsertFromFields() {
    if (!m_currentToolAssembly.threadingInsert) {
        m_currentToolAssembly.threadingInsert = std::make_shared<ThreadingInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.threadingInsert;
    
    if (m_threadingISOCodeEdit) {
        insert.isoCode = m_threadingISOCodeEdit->text().toStdString();
    }
    if (m_threadingShapeCombo) {
        insert.shape = static_cast<InsertShape>(m_threadingShapeCombo->currentData().toInt());
    }
    if (m_threadingToleranceCombo) {
        insert.tolerance = static_cast<InsertTolerance>(m_threadingToleranceCombo->currentData().toInt());
    }
    if (m_crossSectionEdit) {
        insert.crossSection = m_crossSectionEdit->text().toStdString();
    }
    if (m_threadingMaterialCombo) {
        insert.material = static_cast<InsertMaterial>(m_threadingMaterialCombo->currentData().toInt());
    }
    if (m_threadingThicknessSpin) {
        insert.thickness = m_threadingThicknessSpin->value();
    }
    if (m_threadingWidthSpin) {
        insert.width = m_threadingWidthSpin->value();
    }
    if (m_minThreadPitchSpin) {
        insert.minThreadPitch = m_minThreadPitchSpin->value();
    }
    if (m_maxThreadPitchSpin) {
        insert.maxThreadPitch = m_maxThreadPitchSpin->value();
    }
    if (m_internalThreadsCheck) {
        insert.internalThreads = m_internalThreadsCheck->isChecked();
    }
    if (m_externalThreadsCheck) {
        insert.externalThreads = m_externalThreadsCheck->isChecked();
    }
    if (m_threadProfileCombo) {
        insert.threadProfile = static_cast<ThreadProfile>(m_threadProfileCombo->currentData().toInt());
    }
    if (m_threadProfileAngleSpin) {
        insert.threadProfileAngle = m_threadProfileAngleSpin->value();
    }
    if (m_threadTipTypeCombo) {
        insert.threadTipType = static_cast<ThreadTipType>(m_threadTipTypeCombo->currentData().toInt());
    }
    if (m_threadTipRadiusSpin) {
        insert.threadTipRadius = m_threadTipRadiusSpin->value();
    }
}

void ToolManagementDialog::updateGroovingInsertFromFields() {
    if (!m_currentToolAssembly.groovingInsert) {
        m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.groovingInsert;
    
    if (m_groovingISOCodeEdit) {
        insert.isoCode = m_groovingISOCodeEdit->text().toStdString();
    }
    if (m_groovingShapeCombo) {
        insert.shape = static_cast<InsertShape>(m_groovingShapeCombo->currentData().toInt());
    }
    if (m_groovingToleranceCombo) {
        insert.tolerance = static_cast<InsertTolerance>(m_groovingToleranceCombo->currentData().toInt());
    }
    if (m_groovingCrossSectionEdit) {
        insert.crossSection = m_groovingCrossSectionEdit->text().toStdString();
    }
    if (m_groovingMaterialCombo) {
        insert.material = static_cast<InsertMaterial>(m_groovingMaterialCombo->currentData().toInt());
    }
    if (m_groovingThicknessSpin) {
        insert.thickness = m_groovingThicknessSpin->value();
    }
    if (m_groovingOverallLengthSpin) {
        insert.overallLength = m_groovingOverallLengthSpin->value();
    }
    if (m_groovingWidthSpin) {
        insert.width = m_groovingWidthSpin->value();
    }
    if (m_groovingCornerRadiusSpin) {
        insert.cornerRadius = m_groovingCornerRadiusSpin->value();
    }
    if (m_groovingHeadLengthSpin) {
        insert.headLength = m_groovingHeadLengthSpin->value();
    }
    if (m_grooveWidthSpin) {
        insert.grooveWidth = m_grooveWidthSpin->value();
    }
}

void ToolManagementDialog::updateHolderDataFromFields() {
    if (!m_currentToolAssembly.holder) {
        m_currentToolAssembly.holder = std::make_shared<ToolHolder>();
    }
    
    auto& holder = *m_currentToolAssembly.holder;
    
    if (m_holderISOCodeEdit) {
        holder.isoCode = m_holderISOCodeEdit->text().toStdString();
    }
    if (m_handOrientationCombo) {
        holder.handOrientation = static_cast<HandOrientation>(m_handOrientationCombo->currentData().toInt());
    }
    if (m_clampingStyleCombo) {
        holder.clampingStyle = static_cast<ClampingStyle>(m_clampingStyleCombo->currentData().toInt());
    }
    if (m_cuttingWidthSpin) {
        holder.cuttingWidth = m_cuttingWidthSpin->value();
    }
    if (m_headLengthSpin) {
        holder.headLength = m_headLengthSpin->value();
    }
    if (m_overallLengthSpin) {
        holder.overallLength = m_overallLengthSpin->value();
    }
    if (m_shankWidthSpin) {
        holder.shankWidth = m_shankWidthSpin->value();
    }
    if (m_shankHeightSpin) {
        holder.shankHeight = m_shankHeightSpin->value();
    }
    if (m_roundShankCheck) {
        holder.roundShank = m_roundShankCheck->isChecked();
    }
    if (m_shankDiameterSpin) {
        holder.shankDiameter = m_shankDiameterSpin->value();
    }
    if (m_insertSeatAngleSpin) {
        holder.insertSeatAngle = m_insertSeatAngleSpin->value();
    }
    if (m_insertSetbackSpin) {
        holder.insertSetback = m_insertSetbackSpin->value();
    }
    if (m_sideAngleSpin) {
        holder.sideAngle = m_sideAngleSpin->value();
    }
    if (m_backAngleSpin) {
        holder.backAngle = m_backAngleSpin->value();
    }
}

void ToolManagementDialog::updateCuttingDataFromFields() {
    if (m_constantSurfaceSpeedCheck) {
        m_currentToolAssembly.cuttingData.constantSurfaceSpeed = m_constantSurfaceSpeedCheck->isChecked();
    }
    if (m_surfaceSpeedSpin) {
        m_currentToolAssembly.cuttingData.surfaceSpeed = m_surfaceSpeedSpin->value();
    }
    if (m_spindleRPMSpin) {
        m_currentToolAssembly.cuttingData.spindleRPM = m_spindleRPMSpin->value();
    }
    if (m_feedPerRevolutionCheck) {
        m_currentToolAssembly.cuttingData.feedPerRevolution = m_feedPerRevolutionCheck->isChecked();
    }
    if (m_cuttingFeedrateSpin) {
        m_currentToolAssembly.cuttingData.cuttingFeedrate = m_cuttingFeedrateSpin->value();
    }
    if (m_leadInFeedrateSpin) {
        m_currentToolAssembly.cuttingData.leadInFeedrate = m_leadInFeedrateSpin->value();
    }
    if (m_leadOutFeedrateSpin) {
        m_currentToolAssembly.cuttingData.leadOutFeedrate = m_leadOutFeedrateSpin->value();
    }
    if (m_maxDepthOfCutSpin) {
        m_currentToolAssembly.cuttingData.maxDepthOfCut = m_maxDepthOfCutSpin->value();
    }
    if (m_maxFeedrateSpin) {
        m_currentToolAssembly.cuttingData.maxFeedrate = m_maxFeedrateSpin->value();
    }
    if (m_minSurfaceSpeedSpin) {
        m_currentToolAssembly.cuttingData.minSurfaceSpeed = m_minSurfaceSpeedSpin->value();
    }
    if (m_maxSurfaceSpeedSpin) {
        m_currentToolAssembly.cuttingData.maxSurfaceSpeed = m_maxSurfaceSpeedSpin->value();
    }
    if (m_floodCoolantCheck) {
        m_currentToolAssembly.cuttingData.floodCoolant = m_floodCoolantCheck->isChecked();
    }
    if (m_mistCoolantCheck) {
        m_currentToolAssembly.cuttingData.mistCoolant = m_mistCoolantCheck->isChecked();
    }
    if (m_preferredCoolantCombo) {
        m_currentToolAssembly.cuttingData.preferredCoolant = static_cast<CoolantType>(m_preferredCoolantCombo->currentData().toInt());
    }
    if (m_coolantPressureSpin) {
        m_currentToolAssembly.cuttingData.coolantPressure = m_coolantPressureSpin->value();
    }
    if (m_coolantFlowSpin) {
        m_currentToolAssembly.cuttingData.coolantFlow = m_coolantFlowSpin->value();
    }
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

// ============================================================================
// Tool Manager Integration and Persistence Implementation
// ============================================================================

void ToolManagementDialog::setToolManager(IntuiCAM::GUI::ToolManager* toolManager) {
    m_toolManager = toolManager;
}

QString ToolManagementDialog::getToolAssemblyDatabasePath() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("tool_assemblies.json");
}

bool ToolManagementDialog::saveToolAssemblyToDatabase() {
    QString dbPath = getToolAssemblyDatabasePath();
    
    // Load existing database
    QJsonObject database;
    QFile file(dbPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            database = doc.object();
        }
    }
    
    // Ensure tools array exists
    if (!database.contains("tools")) {
        database["tools"] = QJsonArray();
    }
    
    // For new tools, ensure we have a proper unique ID before saving
    if (m_isNewTool) {
        QString originalId = m_currentToolId;
        QString typePrefix = getToolTypePrefix(m_currentToolType);
        
        // Only generate a new ID if we don't have a valid one yet
        // This prevents regenerating IDs during auto-save of existing new tools
        bool needsNewId = originalId.isEmpty() || 
                         originalId.startsWith("NEW_") || 
                         originalId.startsWith("TEMP_") ||
                         originalId.contains("_undefined");
        
        if (needsNewId) {
            // Generate a fresh unique ID for truly new tools
            QString uniqueId = generateUniqueToolId(typePrefix);
            
            // Ensure we never save with an empty ID
            if (uniqueId.isEmpty()) {
                qWarning() << "Generated ID is empty, using fallback";
                uniqueId = QString("%1_%2").arg(typePrefix).arg(QDateTime::currentMSecsSinceEpoch());
            }
            
            qDebug() << "Generating new tool ID from" << originalId << "to" << uniqueId;
            m_currentToolId = uniqueId;
            m_currentToolAssembly.id = m_currentToolId.toStdString();
        } else {
            // Keep the existing valid ID for this new tool
            qDebug() << "Preserving existing valid ID for new tool:" << originalId;
            m_currentToolAssembly.id = m_currentToolId.toStdString();
        }
        
        qDebug() << "Final tool ID for new tool:" << m_currentToolId;
    }
    
    // Convert current tool assembly to JSON
    QJsonObject toolJson = toolAssemblyToJson(m_currentToolAssembly);
    
    // Update or add tool in database
    QJsonArray toolsArray = database["tools"].toArray();
    bool toolFound = false;
    QString toolIdToSave = QString::fromStdString(m_currentToolAssembly.id);
    
    // For existing tools, find and update
    if (!m_isNewTool) {
        for (int i = 0; i < toolsArray.size(); ++i) {
            QJsonObject existingTool = toolsArray[i].toObject();
            if (existingTool["id"].toString() == toolIdToSave) {
                toolsArray[i] = toolJson;
                toolFound = true;
                qDebug() << "Updated existing tool in database:" << toolIdToSave;
                break;
            }
        }
        
        if (!toolFound) {
            qWarning() << "Existing tool not found in database for update:" << toolIdToSave;
            // For safety, treat as new tool
            toolsArray.append(toolJson);
            qDebug() << "Added as new tool since existing tool not found:" << toolIdToSave;
        }
    } else {
        // For new tools, verify uniqueness and append
        for (int i = 0; i < toolsArray.size(); ++i) {
            QJsonObject existingTool = toolsArray[i].toObject();
            if (existingTool["id"].toString() == toolIdToSave) {
                qWarning() << "ID collision detected for new tool:" << toolIdToSave;
                // Generate a completely new unique ID to avoid collision
                QString newUniqueId = QString("%1_%2").arg(getToolTypePrefix(m_currentToolType))
                                                     .arg(QDateTime::currentMSecsSinceEpoch());
                m_currentToolId = newUniqueId;
                m_currentToolAssembly.id = m_currentToolId.toStdString();
                toolJson = toolAssemblyToJson(m_currentToolAssembly);
                qDebug() << "Generated new ID to avoid collision:" << newUniqueId;
                break;
            }
        }
        
        // Add new tool to the end of the array
        toolsArray.append(toolJson);
        qDebug() << "Added new tool to database:" << QString::fromStdString(m_currentToolAssembly.id) 
                 << "Total tools:" << toolsArray.size();
    }
    
    database["tools"] = toolsArray;
    database["version"] = "1.0";
    database["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    database["toolCount"] = toolsArray.size();
    
    // Save database with error checking
    QJsonDocument doc(database);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open tool assembly database for writing:" << dbPath;
        return false;
    }
    
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();
    
    if (bytesWritten == -1) {
        qWarning() << "Failed to write tool data to database";
        return false;
    }
    
    // After successfully saving a new tool for the first time, it's no longer "new"
    if (m_isNewTool) {
        m_isNewTool = false;
        qDebug() << "Tool is no longer considered 'new' after first successful save:" << QString::fromStdString(m_currentToolAssembly.id);
    }
    
    qDebug() << "Successfully saved tool assembly to database:" << QString::fromStdString(m_currentToolAssembly.id);
    qDebug() << "Database now contains" << toolsArray.size() << "tools";
    return true;
}

bool ToolManagementDialog::loadToolAssemblyFromDatabase(const QString& toolId) {
    QString dbPath = getToolAssemblyDatabasePath();
    
    QFile file(dbPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Tool assembly database not found:" << dbPath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse tool assembly database:" << error.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qWarning() << "Tool assembly database is not a valid JSON object";
        return false;
    }
    
    QJsonObject database = doc.object();
    if (!database.contains("tools")) {
        qDebug() << "No tools found in database";
        return false;
    }
    
    QJsonArray toolsArray = database["tools"].toArray();
    for (const QJsonValue& toolValue : toolsArray) {
        QJsonObject toolObj = toolValue.toObject();
        if (toolObj["id"].toString() == toolId) {
            m_currentToolAssembly = toolAssemblyFromJson(toolObj);
            qDebug() << "Loaded tool assembly from database:" << toolId;
            return true;
        }
    }
    
    qDebug() << "Tool not found in database:" << toolId;
    return false;
}

QJsonObject ToolManagementDialog::toolAssemblyToJson(const ToolAssembly& assembly) const {
    QJsonObject json;
    
    json["id"] = QString::fromStdString(assembly.id);
    json["name"] = QString::fromStdString(assembly.name);
    json["manufacturer"] = QString::fromStdString(assembly.manufacturer);
    json["toolType"] = static_cast<int>(assembly.toolType);
    
    // Tool positioning
    json["toolOffset_X"] = assembly.toolOffset_X;
    json["toolOffset_Z"] = assembly.toolOffset_Z;
    json["toolLengthOffset"] = assembly.toolLengthOffset;
    json["toolRadiusOffset"] = assembly.toolRadiusOffset;
    
    // Tool management
    json["toolNumber"] = QString::fromStdString(assembly.toolNumber);
    json["turretPosition"] = assembly.turretPosition;
    json["isActive"] = assembly.isActive;
    
    // Tool capabilities
    json["internalThreading"] = assembly.internalThreading;
    json["internalBoring"] = assembly.internalBoring;
    json["partingGrooving"] = assembly.partingGrooving;
    json["externalThreading"] = assembly.externalThreading;
    json["longitudinalTurning"] = assembly.longitudinalTurning;
    json["facing"] = assembly.facing;
    json["chamfering"] = assembly.chamfering;
    
    // Tool life management
    json["expectedLifeMinutes"] = assembly.expectedLifeMinutes;
    json["usageMinutes"] = assembly.usageMinutes;
    json["cycleCount"] = assembly.cycleCount;
    json["lastMaintenanceDate"] = QString::fromStdString(assembly.lastMaintenanceDate);
    json["nextMaintenanceDate"] = QString::fromStdString(assembly.nextMaintenanceDate);
    json["lastUsedDate"] = QString::fromStdString(assembly.lastUsedDate);
    
    // User properties
    json["notes"] = QString::fromStdString(assembly.notes);
    
    // Cutting data
    json["cuttingData"] = cuttingDataToJson(assembly.cuttingData);
    
    // Inserts based on tool type
    if (assembly.turningInsert) {
        json["turningInsert"] = generalTurningInsertToJson(*assembly.turningInsert);
    }
    if (assembly.threadingInsert) {
        json["threadingInsert"] = threadingInsertToJson(*assembly.threadingInsert);
    }
    if (assembly.groovingInsert) {
        json["groovingInsert"] = groovingInsertToJson(*assembly.groovingInsert);
    }
    
    // Tool holder
    if (assembly.holder) {
        json["holder"] = toolHolderToJson(*assembly.holder);
    }
    
    return json;
}

ToolAssembly ToolManagementDialog::toolAssemblyFromJson(const QJsonObject& json) const {
    ToolAssembly assembly;
    
    assembly.id = json["id"].toString().toStdString();
    assembly.name = json["name"].toString().toStdString();
    assembly.manufacturer = json["manufacturer"].toString().toStdString();
    assembly.toolType = static_cast<ToolType>(json["toolType"].toInt());
    
    // Tool positioning
    assembly.toolOffset_X = json["toolOffset_X"].toDouble();
    assembly.toolOffset_Z = json["toolOffset_Z"].toDouble();
    assembly.toolLengthOffset = json["toolLengthOffset"].toDouble();
    assembly.toolRadiusOffset = json["toolRadiusOffset"].toDouble();
    
    // Tool management
    assembly.toolNumber = json["toolNumber"].toString().toStdString();
    assembly.turretPosition = json["turretPosition"].toInt();
    assembly.isActive = json["isActive"].toBool();
    
    // Tool capabilities
    assembly.internalThreading = json["internalThreading"].toBool(false);
    assembly.internalBoring = json["internalBoring"].toBool(false);
    assembly.partingGrooving = json["partingGrooving"].toBool(false);
    assembly.externalThreading = json["externalThreading"].toBool(false);
    assembly.longitudinalTurning = json["longitudinalTurning"].toBool(true);
    assembly.facing = json["facing"].toBool(true);
    assembly.chamfering = json["chamfering"].toBool(false);
    
    // Tool life management
    assembly.expectedLifeMinutes = json["expectedLifeMinutes"].toDouble();
    assembly.usageMinutes = json["usageMinutes"].toDouble();
    assembly.cycleCount = json["cycleCount"].toInt();
    assembly.lastMaintenanceDate = json["lastMaintenanceDate"].toString().toStdString();
    assembly.nextMaintenanceDate = json["nextMaintenanceDate"].toString().toStdString();
    assembly.lastUsedDate = json["lastUsedDate"].toString().toStdString();
    
    // User properties
    assembly.notes = json["notes"].toString().toStdString();
    
    // Cutting data
    if (json.contains("cuttingData")) {
        assembly.cuttingData = cuttingDataFromJson(json["cuttingData"].toObject());
    }
    
    // Inserts based on what's available
    if (json.contains("turningInsert")) {
        assembly.turningInsert = std::make_shared<GeneralTurningInsert>(
            generalTurningInsertFromJson(json["turningInsert"].toObject()));
    }
    if (json.contains("threadingInsert")) {
        assembly.threadingInsert = std::make_shared<ThreadingInsert>(
            threadingInsertFromJson(json["threadingInsert"].toObject()));
    }
    if (json.contains("groovingInsert")) {
        assembly.groovingInsert = std::make_shared<GroovingInsert>(
            groovingInsertFromJson(json["groovingInsert"].toObject()));
    }
    
    // Tool holder
    if (json.contains("holder")) {
        assembly.holder = std::make_shared<ToolHolder>(
            toolHolderFromJson(json["holder"].toObject()));
    }
    
    return assembly;
}

QJsonObject ToolManagementDialog::generalTurningInsertToJson(const GeneralTurningInsert& insert) const {
    QJsonObject json;
    
    json["isoCode"] = QString::fromStdString(insert.isoCode);
    json["shape"] = static_cast<int>(insert.shape);
    json["reliefAngle"] = static_cast<int>(insert.reliefAngle);
    json["tolerance"] = static_cast<int>(insert.tolerance);
    json["sizeSpecifier"] = QString::fromStdString(insert.sizeSpecifier);
    
    json["inscribedCircle"] = insert.inscribedCircle;
    json["thickness"] = insert.thickness;
    json["cornerRadius"] = insert.cornerRadius;
    json["cuttingEdgeLength"] = insert.cuttingEdgeLength;
    json["width"] = insert.width;
    
    json["material"] = static_cast<int>(insert.material);
    json["substrate"] = QString::fromStdString(insert.substrate);
    json["coating"] = QString::fromStdString(insert.coating);
    json["manufacturer"] = QString::fromStdString(insert.manufacturer);
    json["partNumber"] = QString::fromStdString(insert.partNumber);
    
    json["rake_angle"] = insert.rake_angle;
    json["inclination_angle"] = insert.inclination_angle;
    
    json["name"] = QString::fromStdString(insert.name);
    json["vendor"] = QString::fromStdString(insert.vendor);
    json["productId"] = QString::fromStdString(insert.productId);
    json["productLink"] = QString::fromStdString(insert.productLink);
    json["notes"] = QString::fromStdString(insert.notes);
    json["isActive"] = insert.isActive;
    
    return json;
}

GeneralTurningInsert ToolManagementDialog::generalTurningInsertFromJson(const QJsonObject& json) const {
    GeneralTurningInsert insert;
    
    insert.isoCode = json["isoCode"].toString().toStdString();
    insert.shape = static_cast<InsertShape>(json["shape"].toInt());
    insert.reliefAngle = static_cast<InsertReliefAngle>(json["reliefAngle"].toInt());
    insert.tolerance = static_cast<InsertTolerance>(json["tolerance"].toInt());
    insert.sizeSpecifier = json["sizeSpecifier"].toString().toStdString();
    
    insert.inscribedCircle = json["inscribedCircle"].toDouble();
    insert.thickness = json["thickness"].toDouble();
    insert.cornerRadius = json["cornerRadius"].toDouble();
    insert.cuttingEdgeLength = json["cuttingEdgeLength"].toDouble();
    insert.width = json["width"].toDouble();
    
    insert.material = static_cast<InsertMaterial>(json["material"].toInt());
    insert.substrate = json["substrate"].toString().toStdString();
    insert.coating = json["coating"].toString().toStdString();
    insert.manufacturer = json["manufacturer"].toString().toStdString();
    insert.partNumber = json["partNumber"].toString().toStdString();
    
    insert.rake_angle = json["rake_angle"].toDouble();
    insert.inclination_angle = json["inclination_angle"].toDouble();
    
    insert.name = json["name"].toString().toStdString();
    insert.vendor = json["vendor"].toString().toStdString();
    insert.productId = json["productId"].toString().toStdString();
    insert.productLink = json["productLink"].toString().toStdString();
    insert.notes = json["notes"].toString().toStdString();
    insert.isActive = json["isActive"].toBool();
    
    return insert;
}

QJsonObject ToolManagementDialog::threadingInsertToJson(const ThreadingInsert& insert) const {
    QJsonObject json;
    
    json["isoCode"] = QString::fromStdString(insert.isoCode);
    json["shape"] = static_cast<int>(insert.shape);
    json["tolerance"] = static_cast<int>(insert.tolerance);
    json["crossSection"] = QString::fromStdString(insert.crossSection);
    json["material"] = static_cast<int>(insert.material);
    
    json["thickness"] = insert.thickness;
    json["width"] = insert.width;
    json["minThreadPitch"] = insert.minThreadPitch;
    json["maxThreadPitch"] = insert.maxThreadPitch;
    json["internalThreads"] = insert.internalThreads;
    json["externalThreads"] = insert.externalThreads;
    
    json["threadProfile"] = static_cast<int>(insert.threadProfile);
    json["threadProfileAngle"] = insert.threadProfileAngle;
    json["threadTipType"] = static_cast<int>(insert.threadTipType);
    json["threadTipRadius"] = insert.threadTipRadius;
    
    json["name"] = QString::fromStdString(insert.name);
    json["vendor"] = QString::fromStdString(insert.vendor);
    json["productId"] = QString::fromStdString(insert.productId);
    json["productLink"] = QString::fromStdString(insert.productLink);
    json["notes"] = QString::fromStdString(insert.notes);
    json["isActive"] = insert.isActive;
    
    return json;
}

ThreadingInsert ToolManagementDialog::threadingInsertFromJson(const QJsonObject& json) const {
    ThreadingInsert insert;
    
    insert.isoCode = json["isoCode"].toString().toStdString();
    insert.shape = static_cast<InsertShape>(json["shape"].toInt());
    insert.tolerance = static_cast<InsertTolerance>(json["tolerance"].toInt());
    insert.crossSection = json["crossSection"].toString().toStdString();
    insert.material = static_cast<InsertMaterial>(json["material"].toInt());
    
    insert.thickness = json["thickness"].toDouble();
    insert.width = json["width"].toDouble();
    insert.minThreadPitch = json["minThreadPitch"].toDouble();
    insert.maxThreadPitch = json["maxThreadPitch"].toDouble();
    insert.internalThreads = json["internalThreads"].toBool();
    insert.externalThreads = json["externalThreads"].toBool();
    
    insert.threadProfile = static_cast<ThreadProfile>(json["threadProfile"].toInt());
    insert.threadProfileAngle = json["threadProfileAngle"].toDouble();
    insert.threadTipType = static_cast<ThreadTipType>(json["threadTipType"].toInt());
    insert.threadTipRadius = json["threadTipRadius"].toDouble();
    
    insert.name = json["name"].toString().toStdString();
    insert.vendor = json["vendor"].toString().toStdString();
    insert.productId = json["productId"].toString().toStdString();
    insert.productLink = json["productLink"].toString().toStdString();
    insert.notes = json["notes"].toString().toStdString();
    insert.isActive = json["isActive"].toBool();
    
    return insert;
}

QJsonObject ToolManagementDialog::groovingInsertToJson(const GroovingInsert& insert) const {
    QJsonObject json;
    
    json["isoCode"] = QString::fromStdString(insert.isoCode);
    json["shape"] = static_cast<int>(insert.shape);
    json["tolerance"] = static_cast<int>(insert.tolerance);
    json["crossSection"] = QString::fromStdString(insert.crossSection);
    json["material"] = static_cast<int>(insert.material);
    
    json["thickness"] = insert.thickness;
    json["overallLength"] = insert.overallLength;
    json["width"] = insert.width;
    json["cornerRadius"] = insert.cornerRadius;
    json["headLength"] = insert.headLength;
    json["grooveWidth"] = insert.grooveWidth;
    
    json["name"] = QString::fromStdString(insert.name);
    json["vendor"] = QString::fromStdString(insert.vendor);
    json["productId"] = QString::fromStdString(insert.productId);
    json["productLink"] = QString::fromStdString(insert.productLink);
    json["notes"] = QString::fromStdString(insert.notes);
    json["isActive"] = insert.isActive;
    
    return json;
}

GroovingInsert ToolManagementDialog::groovingInsertFromJson(const QJsonObject& json) const {
    GroovingInsert insert;
    
    insert.isoCode = json["isoCode"].toString().toStdString();
    insert.shape = static_cast<InsertShape>(json["shape"].toInt());
    insert.tolerance = static_cast<InsertTolerance>(json["tolerance"].toInt());
    insert.crossSection = json["crossSection"].toString().toStdString();
    insert.material = static_cast<InsertMaterial>(json["material"].toInt());
    
    insert.thickness = json["thickness"].toDouble();
    insert.overallLength = json["overallLength"].toDouble();
    insert.width = json["width"].toDouble();
    insert.cornerRadius = json["cornerRadius"].toDouble();
    insert.headLength = json["headLength"].toDouble();
    insert.grooveWidth = json["grooveWidth"].toDouble();
    
    insert.name = json["name"].toString().toStdString();
    insert.vendor = json["vendor"].toString().toStdString();
    insert.productId = json["productId"].toString().toStdString();
    insert.productLink = json["productLink"].toString().toStdString();
    insert.notes = json["notes"].toString().toStdString();
    insert.isActive = json["isActive"].toBool();
    
    return insert;
}

QJsonObject ToolManagementDialog::toolHolderToJson(const ToolHolder& holder) const {
    QJsonObject json;
    
    json["isoCode"] = QString::fromStdString(holder.isoCode);
    json["handOrientation"] = static_cast<int>(holder.handOrientation);
    json["clampingStyle"] = static_cast<int>(holder.clampingStyle);
    
    json["cuttingWidth"] = holder.cuttingWidth;
    json["headLength"] = holder.headLength;
    json["overallLength"] = holder.overallLength;
    json["shankWidth"] = holder.shankWidth;
    json["shankHeight"] = holder.shankHeight;
    json["roundShank"] = holder.roundShank;
    json["shankDiameter"] = holder.shankDiameter;
    
    json["insertSeatAngle"] = holder.insertSeatAngle;
    json["insertSetback"] = holder.insertSetback;
    json["sideAngle"] = holder.sideAngle;
    json["backAngle"] = holder.backAngle;
    
    json["name"] = QString::fromStdString(holder.name);
    json["vendor"] = QString::fromStdString(holder.vendor);
    json["productId"] = QString::fromStdString(holder.productId);
    json["productLink"] = QString::fromStdString(holder.productLink);
    json["notes"] = QString::fromStdString(holder.notes);
    json["isActive"] = holder.isActive;
    
    return json;
}

ToolHolder ToolManagementDialog::toolHolderFromJson(const QJsonObject& json) const {
    ToolHolder holder;
    
    holder.isoCode = json["isoCode"].toString().toStdString();
    holder.handOrientation = static_cast<HandOrientation>(json["handOrientation"].toInt());
    holder.clampingStyle = static_cast<ClampingStyle>(json["clampingStyle"].toInt());
    
    holder.cuttingWidth = json["cuttingWidth"].toDouble();
    holder.headLength = json["headLength"].toDouble();
    holder.overallLength = json["overallLength"].toDouble();
    holder.shankWidth = json["shankWidth"].toDouble();
    holder.shankHeight = json["shankHeight"].toDouble();
    holder.roundShank = json["roundShank"].toBool();
    holder.shankDiameter = json["shankDiameter"].toDouble();
    
    holder.insertSeatAngle = json["insertSeatAngle"].toDouble();
    holder.insertSetback = json["insertSetback"].toDouble();
    holder.sideAngle = json["sideAngle"].toDouble();
    holder.backAngle = json["backAngle"].toDouble();
    
    holder.name = json["name"].toString().toStdString();
    holder.vendor = json["vendor"].toString().toStdString();
    holder.productId = json["productId"].toString().toStdString();
    holder.productLink = json["productLink"].toString().toStdString();
    holder.notes = json["notes"].toString().toStdString();
    holder.isActive = json["isActive"].toBool();
    
    return holder;
}

QJsonObject ToolManagementDialog::cuttingDataToJson(const CuttingData& cuttingData) const {
    QJsonObject json;
    
    json["constantSurfaceSpeed"] = cuttingData.constantSurfaceSpeed;
    json["surfaceSpeed"] = cuttingData.surfaceSpeed;
    json["spindleRPM"] = cuttingData.spindleRPM;
    
    json["feedPerRevolution"] = cuttingData.feedPerRevolution;
    json["cuttingFeedrate"] = cuttingData.cuttingFeedrate;
    json["leadInFeedrate"] = cuttingData.leadInFeedrate;
    json["leadOutFeedrate"] = cuttingData.leadOutFeedrate;
    
    json["maxDepthOfCut"] = cuttingData.maxDepthOfCut;
    json["maxFeedrate"] = cuttingData.maxFeedrate;
    json["minSurfaceSpeed"] = cuttingData.minSurfaceSpeed;
    json["maxSurfaceSpeed"] = cuttingData.maxSurfaceSpeed;
    
    json["floodCoolant"] = cuttingData.floodCoolant;
    json["mistCoolant"] = cuttingData.mistCoolant;
    json["preferredCoolant"] = static_cast<int>(cuttingData.preferredCoolant);
    json["coolantPressure"] = cuttingData.coolantPressure;
    json["coolantFlow"] = cuttingData.coolantFlow;
    
    return json;
}

CuttingData ToolManagementDialog::cuttingDataFromJson(const QJsonObject& json) const {
    CuttingData cuttingData;
    
    cuttingData.constantSurfaceSpeed = json["constantSurfaceSpeed"].toBool();
    cuttingData.surfaceSpeed = json["surfaceSpeed"].toDouble();
    cuttingData.spindleRPM = json["spindleRPM"].toDouble();
    
    cuttingData.feedPerRevolution = json["feedPerRevolution"].toBool();
    cuttingData.cuttingFeedrate = json["cuttingFeedrate"].toDouble();
    cuttingData.leadInFeedrate = json["leadInFeedrate"].toDouble();
    cuttingData.leadOutFeedrate = json["leadOutFeedrate"].toDouble();
    
    cuttingData.maxDepthOfCut = json["maxDepthOfCut"].toDouble();
    cuttingData.maxFeedrate = json["maxFeedrate"].toDouble();
    cuttingData.minSurfaceSpeed = json["minSurfaceSpeed"].toDouble();
    cuttingData.maxSurfaceSpeed = json["maxSurfaceSpeed"].toDouble();
    
    cuttingData.floodCoolant = json["floodCoolant"].toBool();
    cuttingData.mistCoolant = json["mistCoolant"].toBool();
    cuttingData.preferredCoolant = static_cast<CoolantType>(json["preferredCoolant"].toInt());
    cuttingData.coolantPressure = json["coolantPressure"].toDouble();
    cuttingData.coolantFlow = json["coolantFlow"].toDouble();
    
    return cuttingData;
}

// ============================================================================
// Comprehensive Default Parameter Setup Methods
// ============================================================================

void ToolManagementDialog::setupGeneralTurningDefaults() {
    // Create general turning insert with professional defaults
    m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
    auto& insert = *m_currentToolAssembly.turningInsert;
    
    // ISO designation for a common CNMG insert
    insert.isoCode = "CNMG120408";
    insert.shape = InsertShape::DIAMOND_80;
    insert.reliefAngle = InsertReliefAngle::ANGLE_7;
    insert.tolerance = InsertTolerance::M_PRECISION;
    insert.sizeSpecifier = "1204";
    
    // Physical dimensions (CNMG120408 specifications)
    insert.inscribedCircle = 12.7;    // 1/2 inch IC
    insert.thickness = 4.76;          // Standard thickness
    insert.cornerRadius = 0.8;        // 0.8mm corner radius
    insert.cuttingEdgeLength = 12.7;  // Effective cutting edge
    insert.width = 12.7;              // Insert width
    
    // Material properties - premium carbide with coating
    insert.material = InsertMaterial::COATED_CARBIDE;
    insert.substrate = "P25";         // ISO grade for steel machining
    insert.coating = "TiAlN";         // Premium coating
    insert.manufacturer = "Generic";
    insert.partNumber = "CNMG120408-PM";
    
    // Cutting geometry
    insert.rake_angle = -7.0;         // Negative rake for stability
    insert.inclination_angle = 0.0;   // Standard inclination
    
    // User properties
    insert.name = "General Turning Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "GT-001";
    insert.productLink = "";
    insert.notes = "Standard general turning insert for steel and cast iron";
    insert.isActive = true;
}

void ToolManagementDialog::setupBoringDefaults() {
    // Boring tools use general turning inserts with specific geometry
    m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
    auto& insert = *m_currentToolAssembly.turningInsert;
    
    // ISO designation for a smaller boring insert
    insert.isoCode = "CCMT09T304";
    insert.shape = InsertShape::DIAMOND_80;
    insert.reliefAngle = InsertReliefAngle::ANGLE_7;
    insert.tolerance = InsertTolerance::M_PRECISION;
    insert.sizeSpecifier = "09T3";
    
    // Physical dimensions (smaller for boring operations)
    insert.inscribedCircle = 9.525;   // 3/8 inch IC
    insert.thickness = 3.97;          // Thinner for bore access
    insert.cornerRadius = 0.4;        // Smaller radius for precision
    insert.cuttingEdgeLength = 9.525;
    insert.width = 9.525;
    
    // Material properties
    insert.material = InsertMaterial::COATED_CARBIDE;
    insert.substrate = "P15";         // Harder grade for precision
    insert.coating = "TiAlN";
    insert.manufacturer = "Generic";
    insert.partNumber = "CCMT09T304-PM";
    
    // Cutting geometry optimized for boring
    insert.rake_angle = -5.0;         // Less negative for better finish
    insert.inclination_angle = 0.0;
    
    // User properties
    insert.name = "Boring Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "BR-001";
    insert.productLink = "";
    insert.notes = "Precision boring insert for internal operations";
    insert.isActive = true;
}

void ToolManagementDialog::setupThreadingDefaults() {
    // Create threading insert with metric threading defaults
    m_currentToolAssembly.threadingInsert = std::make_shared<ThreadingInsert>();
    auto& insert = *m_currentToolAssembly.threadingInsert;
    
    // ISO threading insert designation
    insert.isoCode = "16ER1.0ISO";
    insert.isoShape = InsertShape::RHOMBUS_80;
    insert.shape = InsertShape::RHOMBUS_80;
    insert.tolerance = InsertTolerance::B_PRECISION;
    insert.crossSection = "16ER";
    insert.material = InsertMaterial::COATED_CARBIDE;
    
    // Threading specific dimensions
    insert.thickness = 3.18;          // Standard threading insert thickness
    insert.width = 6.0;               // Insert width
    insert.minThreadPitch = 0.5;      // 0.5mm minimum pitch
    insert.maxThreadPitch = 3.0;      // 3.0mm maximum pitch
    insert.internalThreads = true;    // Can do internal threads
    insert.externalThreads = true;    // Can do external threads
    
    // Threading profile settings
    insert.threadProfile = ThreadProfile::METRIC;
    insert.threadProfileAngle = 60.0;  // 60° metric thread
    insert.threadTipType = ThreadTipType::SHARP_POINT;
    insert.threadTipRadius = 0.0;      // Sharp tip for metric threads
    
    // User properties
    insert.name = "Metric Threading Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "TH-001";
    insert.productLink = "";
    insert.notes = "Metric threading insert for M0.5-M3.0 threads";
    insert.isActive = true;
}

void ToolManagementDialog::setupGroovingDefaults() {
    // Create grooving insert with standard grooving specifications
    m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
    auto& insert = *m_currentToolAssembly.groovingInsert;
    
    // ISO grooving insert designation
    insert.isoCode = "N123G2-0300-0003-GM";
    insert.shape = InsertShape::SQUARE;
    insert.tolerance = InsertTolerance::M_PRECISION;
    insert.crossSection = "N123";
    insert.material = InsertMaterial::COATED_CARBIDE;
    
    // Grooving specific dimensions
    insert.thickness = 2.38;          // Standard grooving thickness
    insert.overallLength = 12.0;      // Insert length
    insert.width = 3.0;               // 3mm grooving width
    insert.cornerRadius = 0.05;       // Very small radius for sharp corners
    insert.headLength = 8.0;          // Cutting head length
    insert.grooveWidth = 3.0;         // 3mm groove width
    
    // User properties
    insert.name = "3mm Grooving Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "GR-001";
    insert.productLink = "";
    insert.notes = "Standard 3mm grooving insert for external grooving";
    insert.isActive = true;
}

void ToolManagementDialog::setupPartingDefaults() {
    // Parting tools use grooving inserts with parting-specific settings
    m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
    auto& insert = *m_currentToolAssembly.groovingInsert;
    
    // ISO parting insert designation (wider than grooving)
    insert.isoCode = "N123J2-0500-0005-PM";
    insert.shape = InsertShape::SQUARE;
    insert.tolerance = InsertTolerance::M_PRECISION;
    insert.crossSection = "N123";
    insert.material = InsertMaterial::COATED_CARBIDE;
    
    // Parting specific dimensions
    insert.thickness = 2.38;          // Standard thickness
    insert.overallLength = 16.0;      // Longer for parting operations
    insert.width = 5.0;               // 5mm parting width
    insert.cornerRadius = 0.0;        // Sharp corners for clean parting
    insert.headLength = 12.0;         // Longer cutting head
    insert.grooveWidth = 5.0;         // 5mm parting width
    
    // User properties
    insert.name = "5mm Parting Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "PT-001";
    insert.productLink = "";
    insert.notes = "Heavy-duty 5mm parting insert for cutoff operations";
    insert.isActive = true;
}

void ToolManagementDialog::setupFormToolDefaults() {
    // Form tools use general turning inserts with custom geometry
    m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
    auto& insert = *m_currentToolAssembly.turningInsert;
    
    // ISO designation for a round insert (common for form tools)
    insert.isoCode = "RPMX1204M0";
    insert.shape = InsertShape::ROUND;
    insert.reliefAngle = InsertReliefAngle::ANGLE_11;
    insert.tolerance = InsertTolerance::M_PRECISION;
    insert.sizeSpecifier = "1204";
    
    // Physical dimensions (round insert)
    insert.inscribedCircle = 12.7;    // 1/2 inch diameter
    insert.thickness = 4.76;          // Standard thickness
    insert.cornerRadius = 6.35;       // Half diameter for round insert
    insert.cuttingEdgeLength = 12.7;
    insert.width = 12.7;
    
    // Material properties
    insert.material = InsertMaterial::COATED_CARBIDE;
    insert.substrate = "P30";         // Tougher grade for forming
    insert.coating = "TiCN";
    insert.manufacturer = "Generic";
    insert.partNumber = "RPMX1204M0-PM";
    
    // Cutting geometry for forming
    insert.rake_angle = -5.0;         // Moderate negative rake
    insert.inclination_angle = 0.0;
    
    // User properties
    insert.name = "Round Form Tool Insert";
    insert.vendor = "Tool Supplier";
    insert.productId = "FT-001";
    insert.productLink = "";
    insert.notes = "Round insert for custom forming operations";
    insert.isActive = true;
}

void ToolManagementDialog::setupHolderDefaults() {
    // Create tool holder with standard specifications
    m_currentToolAssembly.holder = std::make_shared<ToolHolder>();
    auto& holder = *m_currentToolAssembly.holder;
    
    // ISO holder identification
    holder.isoCode = "MCLNR2020K12";
    holder.handOrientation = HandOrientation::RIGHT_HAND;
    holder.clampingStyle = ClampingStyle::TOP_CLAMP;
    
    // Physical dimensions (20x20mm standard holder)
    holder.cuttingWidth = 20.0;       // 20mm cutting width
    holder.headLength = 32.0;         // Standard head length
    holder.overallLength = 150.0;     // 150mm total length
    holder.shankWidth = 20.0;         // 20mm square shank
    holder.shankHeight = 20.0;        // 20mm square shank
    holder.roundShank = false;        // Square shank
    holder.isRoundShank = false;      // Alternative name
    holder.shankDiameter = 20.0;      // Equivalent diameter
    
    // Insert seat geometry
    holder.insertSeatAngle = 95.0;    // 95° seat angle
    holder.insertSetback = 2.0;       // 2mm setback from nose
    holder.sideAngle = 15.0;          // 15° side cutting edge angle
    holder.backAngle = 5.0;           // 5° back angle
    
    // Compatible inserts
    holder.compatibleInserts = {"CNMG120408", "CNMG120412", "CNMG120404"};
    
    // Holder capabilities
    holder.isInternal = false;        // External operations
    holder.isGrooving = false;        // Not for grooving
    holder.isThreading = false;       // Not for threading
    
    // User properties
    holder.name = "Standard Right-Hand Holder";
    holder.vendor = "Tool Supplier";
    holder.productId = "MH-001";
    holder.productLink = "";
    holder.notes = "Standard 20x20mm right-hand turning holder";
    holder.isActive = true;
}

void ToolManagementDialog::setupCapabilitiesForToolType(ToolType toolType) {
    // Set default capabilities based on tool type
    switch (toolType) {
        case ToolType::GENERAL_TURNING:
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = false;
            m_currentToolAssembly.partingGrooving = false;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = true;
            m_currentToolAssembly.facing = true;
            m_currentToolAssembly.chamfering = true;
            break;
        case ToolType::BORING:
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = true;
            m_currentToolAssembly.partingGrooving = false;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = true;
            m_currentToolAssembly.facing = false;
            m_currentToolAssembly.chamfering = true;
            break;
        case ToolType::THREADING:
            m_currentToolAssembly.internalThreading = true;
            m_currentToolAssembly.internalBoring = false;
            m_currentToolAssembly.partingGrooving = false;
            m_currentToolAssembly.externalThreading = true;
            m_currentToolAssembly.longitudinalTurning = false;
            m_currentToolAssembly.facing = false;
            m_currentToolAssembly.chamfering = false;
            break;
        case ToolType::GROOVING:
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = false;
            m_currentToolAssembly.partingGrooving = true;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = false;
            m_currentToolAssembly.facing = false;
            m_currentToolAssembly.chamfering = false;
            break;
        case ToolType::PARTING:
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = false;
            m_currentToolAssembly.partingGrooving = true;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = false;
            m_currentToolAssembly.facing = false;
            m_currentToolAssembly.chamfering = false;
            break;
        case ToolType::FORM_TOOL:
            // Form tools can have custom capabilities - enable all by default
            m_currentToolAssembly.internalThreading = true;
            m_currentToolAssembly.internalBoring = true;
            m_currentToolAssembly.partingGrooving = true;
            m_currentToolAssembly.externalThreading = true;
            m_currentToolAssembly.longitudinalTurning = true;
            m_currentToolAssembly.facing = true;
            m_currentToolAssembly.chamfering = true;
            break;
        case ToolType::LIVE_TOOLING:
            // Live tooling capabilities depend on specific tooling
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = true;
            m_currentToolAssembly.partingGrooving = false;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = false;
            m_currentToolAssembly.facing = true;
            m_currentToolAssembly.chamfering = false;
            break;
        default:
            // Default: basic turning capabilities
            m_currentToolAssembly.internalThreading = false;
            m_currentToolAssembly.internalBoring = false;
            m_currentToolAssembly.partingGrooving = false;
            m_currentToolAssembly.externalThreading = false;
            m_currentToolAssembly.longitudinalTurning = true;
            m_currentToolAssembly.facing = true;
            m_currentToolAssembly.chamfering = false;
            break;
    }
}

void ToolManagementDialog::setupCuttingDataDefaults(ToolType toolType) {
    // Initialize cutting data with tool type specific defaults
    auto& cutting = m_currentToolAssembly.cuttingData;
    
    // Speed control defaults
    cutting.constantSurfaceSpeed = true;
    cutting.feedPerRevolution = true;
    
    switch (toolType) {
        case ToolType::GENERAL_TURNING:
            cutting.surfaceSpeed = 200.0;        // 200 m/min for steel
            cutting.cuttingFeedrate = 0.3;       // 0.3 mm/rev
            cutting.leadInFeedrate = 0.1;        // 0.1 mm/rev
            cutting.leadOutFeedrate = 0.1;       // 0.1 mm/rev
            cutting.maxDepthOfCut = 3.0;         // 3mm max depth
            break;
            
        case ToolType::BORING:
            cutting.surfaceSpeed = 150.0;        // Conservative for accuracy
            cutting.cuttingFeedrate = 0.2;       // Finer feed for precision
            cutting.leadInFeedrate = 0.05;       // Very slow entry
            cutting.leadOutFeedrate = 0.05;      // Very slow exit
            cutting.maxDepthOfCut = 1.0;         // 1mm max depth
            break;
            
        case ToolType::THREADING:
            cutting.surfaceSpeed = 80.0;         // Slow for threading
            cutting.cuttingFeedrate = 1.5;       // 1.5mm pitch example
            cutting.leadInFeedrate = 1.5;        // Same as thread pitch
            cutting.leadOutFeedrate = 1.5;       // Same as thread pitch
            cutting.maxDepthOfCut = 0.5;         // 0.5mm per pass
            break;
            
        case ToolType::GROOVING:
            cutting.surfaceSpeed = 120.0;        // Moderate speed
            cutting.cuttingFeedrate = 0.1;       // Slow radial feed
            cutting.leadInFeedrate = 0.05;       // Very slow entry
            cutting.leadOutFeedrate = 0.05;      // Very slow exit
            cutting.maxDepthOfCut = 1.0;         // 1mm per pass
            break;
            
        case ToolType::PARTING:
            cutting.surfaceSpeed = 100.0;        // Conservative for parting
            cutting.cuttingFeedrate = 0.08;      // Very slow feed
            cutting.leadInFeedrate = 0.03;       // Ultra-slow entry
            cutting.leadOutFeedrate = 0.03;      // Ultra-slow exit
            cutting.maxDepthOfCut = 0.5;         // 0.5mm per pass
            break;
            
        case ToolType::FORM_TOOL:
            cutting.surfaceSpeed = 150.0;        // Moderate speed
            cutting.cuttingFeedrate = 0.15;      // Fine feed for forming
            cutting.leadInFeedrate = 0.05;       // Slow entry
            cutting.leadOutFeedrate = 0.05;      // Slow exit
            cutting.maxDepthOfCut = 0.5;         // Small depth per pass
            break;
            
        default:
            cutting.surfaceSpeed = 200.0;
            cutting.cuttingFeedrate = 0.2;
            cutting.leadInFeedrate = 0.1;
            cutting.leadOutFeedrate = 0.1;
            cutting.maxDepthOfCut = 2.0;
            break;
    }
    
    // Common defaults for all tool types
    cutting.spindleRPM = 1000.0;              // Default RPM when not using CSS
    cutting.maxFeedrate = 1000.0;             // 1000 mm/min maximum
    cutting.minSurfaceSpeed = 50.0;           // 50 m/min minimum
    cutting.maxSurfaceSpeed = 500.0;          // 500 m/min maximum
    
    // Coolant settings
    cutting.floodCoolant = true;              // Flood coolant enabled
    cutting.mistCoolant = false;              // Mist coolant disabled
    cutting.preferredCoolant = CoolantType::FLOOD;
    cutting.coolantType = CoolantType::FLOOD; // Alternative name
    cutting.coolantPressure = 5.0;            // 5 bar pressure
    cutting.coolantFlow = 10.0;               // 10 L/min flow rate
} 

// ============================================================================
// 3D Tool Geometry Generation Methods
// ============================================================================

TopoDS_Shape ToolManagementDialog::createAssembledToolGeometry() {
    try {
        TopoDS_Shape insertShape = createInsertGeometry();
        TopoDS_Shape holderShape = createHolderGeometry();
        
        if (insertShape.IsNull() && holderShape.IsNull()) {
            return TopoDS_Shape(); // Return null shape if nothing to display
        }
        
        // If we have both insert and holder, combine them
        if (!insertShape.IsNull() && !holderShape.IsNull()) {
            try {
                BRepAlgoAPI_Fuse fuser(holderShape, insertShape);
                if (fuser.IsDone()) {
                    return fuser.Shape();
                }
            } catch (const Standard_Failure&) {
                // If fusion fails, just return the holder
                return holderShape;
            }
        }
        
        // Return whichever shape is available
        return !insertShape.IsNull() ? insertShape : holderShape;
        
    } catch (const Standard_Failure& e) {
        qWarning() << "Failed to create assembled tool geometry:" << e.GetMessageString();
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createInsertGeometry() {
    try {
        switch (m_currentToolType) {
            case ToolType::GENERAL_TURNING:
            case ToolType::BORING:
            case ToolType::FORM_TOOL:
                if (m_currentToolAssembly.turningInsert) {
                    auto& insert = *m_currentToolAssembly.turningInsert;
                    switch (insert.shape) {
                        case InsertShape::SQUARE:
                            return createSquareInsert(insert.inscribedCircle, insert.thickness, insert.cornerRadius);
                        case InsertShape::TRIANGLE:
                            return createTriangleInsert(insert.inscribedCircle, insert.thickness, insert.cornerRadius);
                        case InsertShape::DIAMOND_80:
                        case InsertShape::DIAMOND_55:
                            return createDiamondInsert(insert.inscribedCircle, insert.thickness, insert.cornerRadius);
                        case InsertShape::ROUND:
                            return createRoundInsert(insert.inscribedCircle, insert.thickness);
                        default:
                            return createSquareInsert(insert.inscribedCircle, insert.thickness, insert.cornerRadius);
                    }
                }
                break;
                
            case ToolType::THREADING:
                if (m_currentToolAssembly.threadingInsert) {
                    auto& insert = *m_currentToolAssembly.threadingInsert;
                    return createThreadingInsert(insert.thickness, insert.width, 16.0); // Standard length
                }
                break;
                
            case ToolType::GROOVING:
            case ToolType::PARTING:
                if (m_currentToolAssembly.groovingInsert) {
                    auto& insert = *m_currentToolAssembly.groovingInsert;
                    return createGroovingInsert(insert.thickness, insert.width, insert.overallLength, insert.grooveWidth);
                }
                break;
        }
    } catch (const Standard_Failure& e) {
        qWarning() << "Failed to create insert geometry:" << e.GetMessageString();
    }
    
    return TopoDS_Shape();
}

TopoDS_Shape ToolManagementDialog::createHolderGeometry() {
    try {
        if (m_currentToolAssembly.holder) {
            auto& holder = *m_currentToolAssembly.holder;
            if (holder.roundShank || holder.isRoundShank) {
                return createCylindricalHolder(holder.shankDiameter, holder.overallLength);
            } else {
                return createRectangularHolder(holder.overallLength, holder.shankWidth, holder.shankHeight);
            }
        }
    } catch (const Standard_Failure& e) {
        qWarning() << "Failed to create holder geometry:" << e.GetMessageString();
    }
    
    return TopoDS_Shape();
}

// Insert geometry creation methods
TopoDS_Shape ToolManagementDialog::createSquareInsert(double inscribedCircle, double thickness, double cornerRadius) {
    try {
        double halfSize = inscribedCircle / 2.0;
        
        // Create base square
        BRepPrimAPI_MakeBox boxMaker(gp_Pnt(-halfSize, -halfSize, 0), gp_Pnt(halfSize, halfSize, thickness));
        TopoDS_Shape baseBox = boxMaker.Shape();
        
        if (cornerRadius > 0.001) {
            // TODO: Add corner radius using BRepFilletAPI_MakeFillet
            // For now, return the basic square
        }
        
        return baseBox;
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createTriangleInsert(double inscribedCircle, double thickness, double cornerRadius) {
    try {
        double radius = inscribedCircle / 2.0;
        double side = radius * 2.0 * sqrt(3.0); // Equilateral triangle side length
        
        // For simplicity, create a square for now
        // TODO: Implement proper triangular insert using BRepBuilderAPI_MakePolygon
        return createSquareInsert(inscribedCircle, thickness, cornerRadius);
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createDiamondInsert(double inscribedCircle, double thickness, double cornerRadius) {
    try {
        double halfDiag = inscribedCircle / 1.414; // Diamond diagonal
        
        // For simplicity, create a square rotated 45 degrees
        // TODO: Implement proper diamond shape
        return createSquareInsert(inscribedCircle, thickness, cornerRadius);
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createRoundInsert(double inscribedCircle, double thickness) {
    try {
        double radius = inscribedCircle / 2.0;
        
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        BRepPrimAPI_MakeCylinder cylinderMaker(axis, radius, thickness);
        
        return cylinderMaker.Shape();
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createThreadingInsert(double thickness, double width, double length) {
    try {
        // Create a rectangular threading insert
        BRepPrimAPI_MakeBox boxMaker(gp_Pnt(0, 0, 0), gp_Pnt(length, width, thickness));
        TopoDS_Shape baseBox = boxMaker.Shape();
        
        // TODO: Add threading profile features
        return baseBox;
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createGroovingInsert(double thickness, double width, double length, double grooveWidth) {
    try {
        // Create the main body
        BRepPrimAPI_MakeBox bodyMaker(gp_Pnt(0, 0, 0), gp_Pnt(length, width, thickness));
        TopoDS_Shape body = bodyMaker.Shape();
        
        // TODO: Add groove cutting edge details
        return body;
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

// Tool holder geometry creation methods
TopoDS_Shape ToolManagementDialog::createRectangularHolder(double length, double width, double height) {
    try {
        // Create main holder body
        BRepPrimAPI_MakeBox holderMaker(gp_Pnt(0, 0, 0), gp_Pnt(length, width, height));
        TopoDS_Shape holder = holderMaker.Shape();
        
        // Create insert pocket (simplified)
        double pocketDepth = height * 0.3;
        double pocketWidth = width * 0.8;
        double pocketLength = length * 0.2;
        
        BRepPrimAPI_MakeBox pocketMaker(
            gp_Pnt(length - pocketLength, (width - pocketWidth) / 2.0, height - pocketDepth),
            gp_Pnt(length, (width + pocketWidth) / 2.0, height)
        );
        
        TopoDS_Shape pocket = pocketMaker.Shape();
        
        // Cut the pocket from the holder
        try {
            BRepAlgoAPI_Cut cutter(holder, pocket);
            if (cutter.IsDone()) {
                return cutter.Shape();
            }
        } catch (const Standard_Failure&) {
            // If cutting fails, return basic holder
        }
        
        return holder;
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

TopoDS_Shape ToolManagementDialog::createCylindricalHolder(double diameter, double length) {
    try {
        double radius = diameter / 2.0;
        
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)); // Along X-axis
        BRepPrimAPI_MakeCylinder cylinderMaker(axis, radius, length);
        
        return cylinderMaker.Shape();
    } catch (const Standard_Failure&) {
        return TopoDS_Shape();
    }
}

// Visualization helper methods
void ToolManagementDialog::updateVisualizationMode(int mode) {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    auto context = m_3dViewer->getContext();
    if (context.IsNull()) {
        return;
    }
    
    // Apply display mode to all displayed tool shapes
    if (!m_currentAssembledShape.IsNull()) {
        switch (mode) {
            case 0: // Wireframe
                context->SetDisplayMode(m_currentAssembledShape, AIS_WireFrame, Standard_False);
                break;
            case 1: // Shaded
                context->SetDisplayMode(m_currentAssembledShape, AIS_Shaded, Standard_False);
                break;
            case 2: // Shaded with edges
                context->SetDisplayMode(m_currentAssembledShape, AIS_Shaded, Standard_False);
                // TODO: Add edge display overlay
                break;
        }
        
        context->Redisplay(m_currentAssembledShape, Standard_False);
        context->UpdateCurrentViewer();
    }
}

void ToolManagementDialog::applyMaterialToShape(Handle(AIS_Shape) aisShape, const QString& materialType) {
    if (aisShape.IsNull()) {
        return;
    }
    
    Graphic3d_MaterialAspect material;
    Quantity_Color color;
    
    if (materialType == "tool") {
        // Tool steel appearance
        material = Graphic3d_MaterialAspect(Graphic3d_NOM_STEEL);
        color = Quantity_Color(0.7, 0.7, 0.8, Quantity_TOC_RGB); // Steel blue-gray
    } else if (materialType == "insert") {
        // Carbide insert appearance
        material = Graphic3d_MaterialAspect(Graphic3d_NOM_METALIZED);
        color = Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB); // Dark gray
    } else {
        // Default material
        material = Graphic3d_MaterialAspect(Graphic3d_NOM_DEFAULT);
        color = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB); // Medium gray
    }
    
    auto drawer = aisShape->Attributes();
    drawer->SetFaceBoundaryDraw(Standard_True);
    drawer->SetColor(color);
    
    aisShape->SetMaterial(material);
    aisShape->SetColor(color);
}

void ToolManagementDialog::clearPreviousToolGeometry() {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    auto context = m_3dViewer->getContext();
    if (context.IsNull()) {
        return;
    }
    
    // Remove previous tool geometry
    if (!m_currentAssembledShape.IsNull()) {
        context->Remove(m_currentAssembledShape, Standard_False);
        m_currentAssembledShape.Nullify();
    }
    
    if (!m_currentInsertShape.IsNull()) {
        context->Remove(m_currentInsertShape, Standard_False);
        m_currentInsertShape.Nullify();
    }
    
    if (!m_currentHolderShape.IsNull()) {
        context->Remove(m_currentHolderShape, Standard_False);
        m_currentHolderShape.Nullify();
    }
    
    context->UpdateCurrentViewer();
}

// View control helper methods
void ToolManagementDialog::setStandardView(const gp_Dir& viewDirection, const gp_Dir& upDirection) {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    auto context = m_3dViewer->getContext();
    if (context.IsNull() || context->CurrentViewer().IsNull()) {
        return;
    }
    
    auto view = context->CurrentViewer()->ActiveViews().First();
    if (!view.IsNull()) {
        // Use the correct V3d_View API for setting view direction
        gp_Pnt eye = gp_Pnt(0, 0, 0).Translated(gp_Vec(viewDirection.Reversed().XYZ() * 100.0));
        gp_Pnt at = gp_Pnt(0, 0, 0);
        view->SetAt(at.X(), at.Y(), at.Z());
        view->SetEye(eye.X(), eye.Y(), eye.Z());
        view->SetUp(upDirection.X(), upDirection.Y(), upDirection.Z());
        view->FitAll();
        view->Redraw();
    }
}

void ToolManagementDialog::resetCameraPosition() {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    auto context = m_3dViewer->getContext();
    if (context.IsNull() || context->CurrentViewer().IsNull()) {
        return;
    }
    
    auto view = context->CurrentViewer()->ActiveViews().First();
    if (!view.IsNull()) {
        view->Reset();
        view->FitAll();
        view->Redraw();
    }
}

void ToolManagementDialog::fitViewToTool() {
    if (!m_3dViewer || !m_3dViewer->isViewerInitialized()) {
        return;
    }
    
    auto context = m_3dViewer->getContext();
    if (context.IsNull() || context->CurrentViewer().IsNull()) {
        return;
    }
    
    auto view = context->CurrentViewer()->ActiveViews().First();
    if (!view.IsNull()) {
        view->FitAll();
        view->Redraw();
    }
} 