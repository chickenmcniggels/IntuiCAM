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
    if (m_toolNameEdit) {
        connect(m_toolNameEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_vendorEdit) {
        connect(m_vendorEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onToolInfoChanged);
    }
    if (m_isoCodeEdit) {
        connect(m_isoCodeEdit, &QLineEdit::textChanged, this, &ToolManagementDialog::onISOCodeChanged);
    }
    // Add more connections as needed for other parameters
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

// Tab creation methods - implement as stubs for now
QWidget* ToolManagementDialog::createInsertPropertiesTab() {
    auto widget = new QWidget();
    auto layout = new QFormLayout(widget);
    
    // Basic insert properties
    m_isoCodeEdit = new QLineEdit();
    layout->addRow("ISO Code:", m_isoCodeEdit);
    
    // Add more controls as needed
    return widget;
}

QWidget* ToolManagementDialog::createHolderPropertiesTab() {
    auto widget = new QWidget();
    auto layout = new QFormLayout(widget);
    
    // Holder properties
    m_holderISOCodeEdit = new QLineEdit();
    layout->addRow("Holder ISO Code:", m_holderISOCodeEdit);
    
    // Add more controls as needed
    return widget;
}

QWidget* ToolManagementDialog::createCuttingDataTab() {
    auto widget = new QWidget();
    auto layout = new QFormLayout(widget);
    
    // Cutting data
    m_surfaceSpeedSpin = new QDoubleSpinBox();
    m_surfaceSpeedSpin->setRange(0.0, 10000.0);
    m_surfaceSpeedSpin->setSuffix(" m/min");
    layout->addRow("Surface Speed:", m_surfaceSpeedSpin);
    
    // Add more controls as needed
    return widget;
}

QWidget* ToolManagementDialog::createToolInfoTab() {
    auto widget = new QWidget();
    auto layout = new QFormLayout(widget);
    
    // Tool info
    m_toolNameEdit = new QLineEdit();
    layout->addRow("Tool Name:", m_toolNameEdit);
    
    m_vendorEdit = new QLineEdit();
    layout->addRow("Vendor:", m_vendorEdit);
    
    // Add more controls as needed
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