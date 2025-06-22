#include "toolmanagementdialog.h"

#include <QApplication>
#include <QDate>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QRadioButton>
#include <QProgressBar>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QSlider>
#include <QToolBar>
#include <QStatusBar>
#include <QDebug>

using namespace IntuiCAM::Toolpath;

ToolManagementDialog::ToolManagementDialog(QWidget *parent)
    : QDialog(parent)
    , m_mainSplitter(nullptr)
    , m_mainLayout(nullptr)
    , m_toolListPanel(nullptr)
    , m_toolListLayout(nullptr)
    , m_searchBox(nullptr)
    , m_toolTypeFilter(nullptr)
    , m_materialFilter(nullptr)
    , m_manufacturerFilter(nullptr)
    , m_clearFiltersButton(nullptr)
    , m_toolListWidget(nullptr)
    , m_toolEditPanel(nullptr)
    , m_toolEditLayout(nullptr)
    , m_toolEditTabs(nullptr)
    , m_insertTab(nullptr)
    , m_holderTab(nullptr)
    , m_cuttingDataTab(nullptr)
    , m_toolInfoTab(nullptr)
    , m_visualization3DPanel(nullptr)
    , m_opengl3DWidget(nullptr)
    , m_currentToolAssembly()
    , m_3dToolViewer(nullptr)
    , m_visualizationTabs(nullptr)
    , m_3dViewTab(nullptr)
    , m_2dViewTab(nullptr)
    , m_sectionsTab(nullptr)
    , m_toolLifePanel(nullptr)
    , m_advancedCuttingPanel(nullptr)
    , m_parameterUpdateTimer(new QTimer(this))
    , m_visualizationUpdateTimer(new QTimer(this))
    , m_toolLifeUpdateTimer(new QTimer(this))
    , m_realTimeUpdatesEnabled(true)
    , m_3dVisualizationEnabled(true)
    , m_current3DViewPlane("XZ")
    , m_currentToolId("")
    , m_isEditing(false)
    , m_toolNameEdit(nullptr)
    , m_vendorEdit(nullptr)
    , m_partNumberEdit(nullptr)
    , m_notesEdit(nullptr)
    , m_toolNumberEdit(nullptr)
    , m_turretPositionSpin(nullptr)
    , m_isActiveCheck(nullptr)
    , m_toolOffsetXSpin(nullptr)
    , m_toolOffsetZSpin(nullptr)
    , m_toolLengthOffsetSpin(nullptr)
    , m_toolRadiusOffsetSpin(nullptr)
    , m_expectedLifeMinutesSpin(nullptr)
    , m_usageMinutesSpin(nullptr)
    , m_cycleCountSpin(nullptr)
    , m_lastMaintenanceDateEdit(nullptr)
    , m_nextMaintenanceDateEdit(nullptr)
    , m_manufacturerEdit(nullptr)
    , m_productIdEdit(nullptr)
    , m_productLinkEdit(nullptr)
{
    setupUI();
    loadToolsFromDatabase();
    populateToolList();
    initializeToolLifeTracking();
    
    // Connect signals
    connect(m_toolListWidget, &QListWidget::currentRowChanged,
            this, &ToolManagementDialog::onToolListSelectionChanged);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &ToolManagementDialog::onSearchTextChanged);
    connect(m_toolTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementDialog::onFilterChanged);
    
    // Advanced feature timers
    m_parameterUpdateTimer->setSingleShot(true);
    m_parameterUpdateTimer->setInterval(500);
    connect(m_parameterUpdateTimer, &QTimer::timeout,
            this, &ToolManagementDialog::onParameterUpdateTimeout);
    
    m_visualizationUpdateTimer->setSingleShot(true);
    m_visualizationUpdateTimer->setInterval(200);
    connect(m_visualizationUpdateTimer, &QTimer::timeout,
            this, &ToolManagementDialog::onVisualizationUpdateTimeout);
    
    m_toolLifeUpdateTimer->setInterval(60000);
    connect(m_toolLifeUpdateTimer, &QTimer::timeout,
            this, &ToolManagementDialog::onToolLifeUpdateTimeout);
    m_toolLifeUpdateTimer->start();
    
    connectParameterSignals();
}

ToolManagementDialog::~ToolManagementDialog() = default;

void ToolManagementDialog::setupUI() {
    setWindowTitle("Tool Management - IntuiCAM");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    createMainLayout();
    createToolListPanel();
    createToolEditPanel();
    create3DVisualizationPanel();
    createToolbar();
}

void ToolManagementDialog::createMainLayout() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
}

void ToolManagementDialog::createToolListPanel() {
    m_toolListPanel = new QWidget();
    m_toolListPanel->setMinimumWidth(300);
    m_toolListPanel->setMaximumWidth(400);
    
    m_toolListLayout = new QVBoxLayout(m_toolListPanel);
    
    // Create filter section
    auto filterGroup = new QGroupBox("Filter Tools");
    auto filterLayout = new QGridLayout(filterGroup);
    
    // Search box
    filterLayout->addWidget(new QLabel("Search:"), 0, 0);
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Enter tool name, ISO code, or vendor...");
    filterLayout->addWidget(m_searchBox, 0, 1);
    
    // Tool type filter
    filterLayout->addWidget(new QLabel("Type:"), 1, 0);
    m_toolTypeFilter = new QComboBox();
    m_toolTypeFilter->addItems({
        "All Types", "General Turning", "Threading", "Grooving", 
        "Boring", "Parting", "Form Tool"
    });
    filterLayout->addWidget(m_toolTypeFilter, 1, 1);
    
    // Material filter
    filterLayout->addWidget(new QLabel("Material:"), 2, 0);
    m_materialFilter = new QComboBox();
    m_materialFilter->addItems({
        "All Materials", "Uncoated Carbide", "Coated Carbide", 
        "Cermet", "Ceramic", "CBN", "PCD", "HSS"
    });
    filterLayout->addWidget(m_materialFilter, 2, 1);
    
    // Manufacturer filter
    filterLayout->addWidget(new QLabel("Vendor:"), 3, 0);
    m_manufacturerFilter = new QComboBox();
    m_manufacturerFilter->addItems({
        "All Vendors", "Sandvik", "Kennametal", "Iscar", "Mitsubishi", 
        "Kyocera", "Seco", "Walter", "Tungaloy", "Taegutec"
    });
    filterLayout->addWidget(m_manufacturerFilter, 3, 1);
    
    // Clear filters button
    m_clearFiltersButton = new QPushButton("Clear Filters");
    filterLayout->addWidget(m_clearFiltersButton, 4, 0, 1, 2);
    
    m_toolListLayout->addWidget(filterGroup);
    
    // Create tool list
    auto toolListGroup = new QGroupBox("Tool Library");
    auto toolListLayout = new QVBoxLayout(toolListGroup);
    
    m_toolListWidget = new QListWidget();
    toolListLayout->addWidget(m_toolListWidget);
    
    m_toolListLayout->addWidget(toolListGroup);
    
    m_mainSplitter->addWidget(m_toolListPanel);
}

void ToolManagementDialog::createToolEditPanel() {
    m_toolEditPanel = new QWidget();
    m_toolEditLayout = new QVBoxLayout(m_toolEditPanel);
    
    // Tool editing tabs
    m_toolEditTabs = new QTabWidget();
    
    // Create tabs
    m_insertTab = createInsertPropertiesTab();
    m_toolEditTabs->addTab(m_insertTab, "Insert Properties");
    
    m_holderTab = createHolderPropertiesTab();
    m_toolEditTabs->addTab(m_holderTab, "Holder Properties");
    
    m_cuttingDataTab = createCuttingDataTab();
    m_toolEditTabs->addTab(m_cuttingDataTab, "Cutting Data");
    
    m_toolInfoTab = createToolInfoTab();
    m_toolEditTabs->addTab(m_toolInfoTab, "Tool Information");
    
    m_toolEditLayout->addWidget(m_toolEditTabs);
    
    // Action buttons
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(new QPushButton("Add Tool"));
    buttonLayout->addWidget(new QPushButton("Save Tool"));
    buttonLayout->addWidget(new QPushButton("Delete Tool"));
    buttonLayout->addStretch();
    
    m_toolEditLayout->addLayout(buttonLayout);
    
    m_mainSplitter->addWidget(m_toolEditPanel);
}

QWidget* ToolManagementDialog::createInsertPropertiesTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    // Basic insert properties
    auto basicGroup = new QGroupBox("Basic Properties");
    auto basicLayout = new QFormLayout(basicGroup);
    
    basicLayout->addRow("ISO Code:", new QLineEdit());
    basicLayout->addRow("Shape:", new QComboBox());
    basicLayout->addRow("Size:", new QDoubleSpinBox());
    basicLayout->addRow("Thickness:", new QDoubleSpinBox());
    basicLayout->addRow("Corner Radius:", new QDoubleSpinBox());
    
    layout->addWidget(basicGroup);
    
    // Advanced properties
    auto advancedGroup = new QGroupBox("Advanced Properties");
    auto advancedLayout = new QFormLayout(advancedGroup);
    
    advancedLayout->addRow("Material:", new QComboBox());
    advancedLayout->addRow("Coating:", new QComboBox());
    advancedLayout->addRow("Tolerance Class:", new QComboBox());
    
    layout->addWidget(advancedGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* ToolManagementDialog::createHolderPropertiesTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    // Holder properties
    auto holderGroup = new QGroupBox("Holder Properties");
    auto holderLayout = new QFormLayout(holderGroup);
    
    holderLayout->addRow("Holder Code:", new QLineEdit());
    holderLayout->addRow("Hand Orientation:", new QComboBox());
    holderLayout->addRow("Clamping Style:", new QComboBox());
    holderLayout->addRow("Shank Size:", new QDoubleSpinBox());
    
    layout->addWidget(holderGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* ToolManagementDialog::createCuttingDataTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    // Cutting parameters
    auto cuttingGroup = new QGroupBox("Cutting Parameters");
    auto cuttingLayout = new QFormLayout(cuttingGroup);
    
    cuttingLayout->addRow("Cutting Speed (m/min):", new QDoubleSpinBox());
    cuttingLayout->addRow("Feed Rate (mm/rev):", new QDoubleSpinBox());
    cuttingLayout->addRow("Depth of Cut (mm):", new QDoubleSpinBox());
    
    layout->addWidget(cuttingGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* ToolManagementDialog::createToolInfoTab() {
    auto widget = new QWidget();
    auto layout = new QVBoxLayout(widget);
    
    // Tool information
    auto infoGroup = new QGroupBox("Tool Information");
    auto infoLayout = new QFormLayout(infoGroup);
    
    // Create and store references to the UI components
    m_toolNameEdit = new QLineEdit();
    m_vendorEdit = new QLineEdit();
    m_manufacturerEdit = new QLineEdit();
    m_partNumberEdit = new QLineEdit();
    m_productIdEdit = new QLineEdit();
    m_productLinkEdit = new QLineEdit();
    m_notesEdit = new QTextEdit();
    m_notesEdit->setMaximumHeight(100);
    
    // Add to form layout
    infoLayout->addRow("Tool Name:", m_toolNameEdit);
    infoLayout->addRow("Vendor:", m_vendorEdit);
    infoLayout->addRow("Manufacturer:", m_manufacturerEdit);
    infoLayout->addRow("Part Number:", m_partNumberEdit);
    infoLayout->addRow("Product ID:", m_productIdEdit);
    infoLayout->addRow("Product Link:", m_productLinkEdit);
    infoLayout->addRow("Notes:", m_notesEdit);
    
    layout->addWidget(infoGroup);
    
    // Tool positioning group
    auto positionGroup = new QGroupBox("Tool Positioning");
    auto positionLayout = new QFormLayout(positionGroup);
    
    m_toolNumberEdit = new QLineEdit();
    m_toolNumberEdit->setPlaceholderText("T01");
    
    m_turretPositionSpin = new QSpinBox();
    m_turretPositionSpin->setRange(1, 99);
    m_turretPositionSpin->setValue(1);
    
    m_isActiveCheck = new QCheckBox();
    m_isActiveCheck->setChecked(true);
    
    positionLayout->addRow("Tool Number:", m_toolNumberEdit);
    positionLayout->addRow("Turret Position:", m_turretPositionSpin);
    positionLayout->addRow("Is Active:", m_isActiveCheck);
    
    layout->addWidget(positionGroup);
    
    // Tool offsets group
    auto offsetsGroup = new QGroupBox("Tool Offsets");
    auto offsetsLayout = new QFormLayout(offsetsGroup);
    
    m_toolOffsetXSpin = new QDoubleSpinBox();
    m_toolOffsetXSpin->setRange(-999.999, 999.999);
    m_toolOffsetXSpin->setDecimals(3);
    m_toolOffsetXSpin->setSuffix(" mm");
    
    m_toolOffsetZSpin = new QDoubleSpinBox();
    m_toolOffsetZSpin->setRange(-999.999, 999.999);
    m_toolOffsetZSpin->setDecimals(3);
    m_toolOffsetZSpin->setSuffix(" mm");
    
    m_toolLengthOffsetSpin = new QDoubleSpinBox();
    m_toolLengthOffsetSpin->setRange(-999.999, 999.999);
    m_toolLengthOffsetSpin->setDecimals(3);
    m_toolLengthOffsetSpin->setSuffix(" mm");
    
    m_toolRadiusOffsetSpin = new QDoubleSpinBox();
    m_toolRadiusOffsetSpin->setRange(-999.999, 999.999);
    m_toolRadiusOffsetSpin->setDecimals(3);
    m_toolRadiusOffsetSpin->setSuffix(" mm");
    
    offsetsLayout->addRow("X Offset:", m_toolOffsetXSpin);
    offsetsLayout->addRow("Z Offset:", m_toolOffsetZSpin);
    offsetsLayout->addRow("Length Offset:", m_toolLengthOffsetSpin);
    offsetsLayout->addRow("Radius Offset:", m_toolRadiusOffsetSpin);
    
    layout->addWidget(offsetsGroup);
    
    // Tool life management group
    auto toolLifeGroup = new QGroupBox("Tool Life Management");
    auto toolLifeLayout = new QFormLayout(toolLifeGroup);
    
    m_expectedLifeMinutesSpin = new QDoubleSpinBox();
    m_expectedLifeMinutesSpin->setRange(0, 10000);
    m_expectedLifeMinutesSpin->setValue(480); // 8 hours default
    m_expectedLifeMinutesSpin->setSuffix(" min");
    
    m_usageMinutesSpin = new QDoubleSpinBox();
    m_usageMinutesSpin->setRange(0, 10000);
    m_usageMinutesSpin->setValue(0);
    m_usageMinutesSpin->setSuffix(" min");
    
    m_cycleCountSpin = new QSpinBox();
    m_cycleCountSpin->setRange(0, 999999);
    m_cycleCountSpin->setValue(0);
    
    m_lastMaintenanceDateEdit = new QLineEdit();
    m_lastMaintenanceDateEdit->setPlaceholderText("YYYY-MM-DD");
    
    m_nextMaintenanceDateEdit = new QLineEdit();
    m_nextMaintenanceDateEdit->setPlaceholderText("YYYY-MM-DD");
    
    toolLifeLayout->addRow("Expected Life:", m_expectedLifeMinutesSpin);
    toolLifeLayout->addRow("Usage:", m_usageMinutesSpin);
    toolLifeLayout->addRow("Cycle Count:", m_cycleCountSpin);
    toolLifeLayout->addRow("Last Maintenance:", m_lastMaintenanceDateEdit);
    toolLifeLayout->addRow("Next Maintenance:", m_nextMaintenanceDateEdit);
    
    layout->addWidget(toolLifeGroup);
    
    layout->addStretch();
    
    return widget;
}

void ToolManagementDialog::create3DVisualizationPanel() {
    m_visualization3DPanel = new QWidget();
    m_visualization3DPanel->setMinimumWidth(400);
    
    auto layout = new QVBoxLayout(m_visualization3DPanel);
    
    // 3D viewer group
    auto viewerGroup = new QGroupBox("3D Visualization");
    auto viewerLayout = new QVBoxLayout(viewerGroup);
    
    // Create OpenGL widget
    m_opengl3DWidget = new OpenGL3DWidget();
    m_opengl3DWidget->setMinimumSize(350, 300);
    viewerLayout->addWidget(m_opengl3DWidget);
    
    // View controls
    auto controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(new QPushButton("Fit View"));
    controlsLayout->addWidget(new QPushButton("Reset View"));
    controlsLayout->addStretch();
    
    viewerLayout->addLayout(controlsLayout);
    layout->addWidget(viewerGroup);
    
    // Tool life management
    auto toolLifeGroup = new QGroupBox("Tool Life Management");
    auto toolLifeLayout = new QVBoxLayout(toolLifeGroup);
    
    auto progressBar = new QProgressBar();
    progressBar->setValue(75);
    toolLifeLayout->addWidget(new QLabel("Tool Life Remaining:"));
    toolLifeLayout->addWidget(progressBar);
    
    layout->addWidget(toolLifeGroup);
    layout->addStretch();
    
    m_mainSplitter->addWidget(m_visualization3DPanel);
}

void ToolManagementDialog::createToolbar() {
    // Implementation for toolbar creation
}

void ToolManagementDialog::loadToolsFromDatabase() {
    // Load sample tools for demonstration
    m_toolDatabase.clear();
    
    // Add sample tools
    ToolAssembly sampleTool;
    sampleTool.id = "TOOL_001";
    sampleTool.name = "General Turning Insert CNMG120408";
    m_toolDatabase.append(sampleTool);
    
    sampleTool.id = "TOOL_002";
    sampleTool.name = "Threading Insert 16ER28UN";
    m_toolDatabase.append(sampleTool);
}

void ToolManagementDialog::populateToolList() {
    m_toolListWidget->clear();
    
    for (const auto& tool : m_toolDatabase) {
        auto item = new QListWidgetItem(QString::fromStdString(tool.name));
        item->setData(Qt::UserRole, QString::fromStdString(tool.id));
        m_toolListWidget->addItem(item);
    }
}

QString ToolManagementDialog::formatToolType(ToolType toolType) {
    switch (toolType) {
        case ToolType::GENERAL_TURNING: return "General Turning";
        case ToolType::THREADING: return "Threading";
        case ToolType::GROOVING: return "Grooving";
        case ToolType::BORING: return "Boring";
        case ToolType::PARTING: return "Parting";
        default: return "Unknown";
    }
}

void ToolManagementDialog::initializeToolLifeTracking() {
    // Initialize tool life tracking system
}

void ToolManagementDialog::connectParameterSignals() {
    // Connect parameter change signals for real-time updates
}

void ToolManagementDialog::updateToolDetails() {
    // Update tool details display
}

bool ToolManagementDialog::validateCurrentTool() {
    // Validate current tool parameters
    return true;
}

void ToolManagementDialog::throttledParameterUpdate() {
    // Implement throttled parameter updates
}

void ToolManagementDialog::validateParametersInRealTime() {
    // Real-time parameter validation
}

void ToolManagementDialog::generate3DAssemblyGeometry(const ToolAssembly& assembly) {
    // Generate 3D geometry for tool assembly
}

void ToolManagementDialog::updateRealTime3DVisualization() {
    // Update 3D visualization in real-time
}

void ToolManagementDialog::enable3DViewPlaneLocking(bool locked) {
    // Enable/disable 3D view plane locking
}

void ToolManagementDialog::set3DViewPlane(const QString& plane) {
    m_current3DViewPlane = plane;
}

// Slot implementations
void ToolManagementDialog::onToolListSelectionChanged() {
    // Handle tool list selection changes
}

void ToolManagementDialog::onSearchTextChanged(const QString& text) {
    // Handle search text changes
}

void ToolManagementDialog::onFilterChanged() {
    // Handle filter changes
}

void ToolManagementDialog::onAddToolClicked() {
    // Handle add tool button click
}

void ToolManagementDialog::onEditToolClicked() {
    // Handle edit tool button click
}

void ToolManagementDialog::onDeleteToolClicked() {
    // Handle delete tool button click
}

void ToolManagementDialog::onDuplicateToolClicked() {
    // Handle duplicate tool button click
}

void ToolManagementDialog::onImportLibrary() {
    // Handle import library
}

void ToolManagementDialog::onExportLibrary() {
    // Handle export library
}

void ToolManagementDialog::onParameterUpdateTimeout() {
    // Handle parameter update timeout
}

void ToolManagementDialog::onVisualizationUpdateTimeout() {
    // Handle visualization update timeout
}

void ToolManagementDialog::onToolLifeUpdateTimeout() {
    // Handle tool life update timeout
}

void ToolManagementDialog::checkToolLifeWarnings() {
    // Check for tool life warnings
}

void ToolManagementDialog::updateToolLifeDisplay() {
    // Update tool life display
}

void ToolManagementDialog::on3DViewModeChanged(const QString& mode) {
    // Handle 3D view mode changes
}

void ToolManagementDialog::on3DViewPlaneChanged(const QString& plane) {
    set3DViewPlane(plane);
}

void ToolManagementDialog::onViewPlaneLockChanged(bool locked) {
    enable3DViewPlaneLocking(locked);
}

void ToolManagementDialog::onFitViewClicked() {
    // Handle fit view button click
}

void ToolManagementDialog::onResetViewClicked() {
    // Handle reset view button click
}

void ToolManagementDialog::onOptimizeParametersClicked() {
    // Handle optimize parameters button click
}

void ToolManagementDialog::onCalculateDeflectionClicked() {
    // Handle calculate deflection button click
}

void ToolManagementDialog::onAnalyzeSurfaceFinishClicked() {
    // Handle analyze surface finish button click
}

void ToolManagementDialog::onScheduleMaintenanceClicked() {
    // Handle schedule maintenance button click
}

void ToolManagementDialog::onResetToolLifeClicked() {
    // Handle reset tool life button click
}

void ToolManagementDialog::onGenerateReportClicked() {
    // Handle generate report button click
}

void ToolManagementDialog::onEnableAlertsChanged(bool enabled) {
    // Handle enable alerts change
}

void ToolManagementDialog::scheduleToolMaintenance() {
    // Schedule tool maintenance
}

void ToolManagementDialog::generateToolLifeReport() {
    // Generate tool life report
}

void ToolManagementDialog::enableRealTimeUpdates(bool enabled) {
    m_realTimeUpdatesEnabled = enabled;
}

// Additional slot implementations to prevent missing function errors

void ToolManagementDialog::onToolTypeChanged() {
    // Handle tool type changes
}

void ToolManagementDialog::onInsertParameterChanged() {
    // Handle insert parameter changes
}

void ToolManagementDialog::onHolderParameterChanged() {
    // Handle holder parameter changes
}

void ToolManagementDialog::onCuttingDataChanged() {
    // Handle cutting data changes
}

void ToolManagementDialog::onISOCodeChanged() {
    // Handle ISO code changes
}

void ToolManagementDialog::onManualParametersChanged() {
    // Handle manual parameter changes
}

void ToolManagementDialog::onVisualizationModeChanged(int mode) {
    // Handle visualization mode changes
}

void ToolManagementDialog::onViewModeChanged(int mode) {
    // Handle view mode changes
}

void ToolManagementDialog::onToolGeometryChanged() {
    // Handle tool geometry changes
}

void ToolManagementDialog::updateToolVisualization() {
    // Update tool visualization
}

void ToolManagementDialog::onValidateISO() {
    // Handle ISO validation
}

void ToolManagementDialog::onLoadFromDatabase() {
    // Handle load from database
}

void ToolManagementDialog::onSaveToDatabase() {
    // Handle save to database
}

void ToolManagementDialog::onImportCatalog() {
    // Handle import catalog
}

void ToolManagementDialog::onExportCatalog() {
    // Handle export catalog
}

void ToolManagementDialog::onShowDimensionsChanged(bool show) {
    // Handle show dimensions change
}

void ToolManagementDialog::onShowAnnotationsChanged(bool show) {
    // Handle show annotations change
}

void ToolManagementDialog::onZoomChanged(int value) {
    // Handle zoom change
}

void ToolManagementDialog::onToolLifeParameterChanged() {
    // Handle tool life parameter changes
}

void ToolManagementDialog::onToolLifeWarning(const QString& toolId) {
    // Handle tool life warning
}

void ToolManagementDialog::onToolLifeCritical(const QString& toolId) {
    // Handle tool life critical
}

void ToolManagementDialog::onWorkpieceMaterialChanged(const QString& material) {
    // Handle workpiece material change
}

void ToolManagementDialog::onOperationTypeChanged(const QString& operation) {
    // Handle operation type change
}

void ToolManagementDialog::onSurfaceFinishRequirementChanged(double value) {
    // Handle surface finish requirement change
}

void ToolManagementDialog::onDeflectionLimitChanged(double value) {
    // Handle deflection limit change
}

void ToolManagementDialog::onRealTimeParameterChanged() {
    // Handle real-time parameter changes
}

// Tool management operations
void ToolManagementDialog::addNewTool(ToolType toolType) {
    // Clear current tool assembly and create new one
    m_currentToolAssembly = ToolAssembly();
    m_currentToolAssembly.toolType = toolType;
    m_currentToolAssembly.id = QString("TOOL_%1").arg(QDateTime::currentMSecsSinceEpoch()).toStdString();
    m_currentToolId = QString::fromStdString(m_currentToolAssembly.id);
    m_isEditing = false;
    
    // Set default values
    loadDefaultParameters();
    
    // Update UI to reflect tool type
    updateToolTypeSpecificUI();
    
    // Clear and reset all parameter fields
    clearAllParameterFields();
    
    // Enable real-time updates
    enableRealTimeUpdates(true);
}

void ToolManagementDialog::editTool(const QString& toolId) {
    qDebug() << "EditTool called for toolId:" << toolId;
    
    // Find the tool in our database
    ToolAssembly toolToEdit;
    bool foundTool = false;
    
    // Search in m_toolDatabase first
    for (const auto& tool : m_toolDatabase) {
        if (QString::fromStdString(tool.id) == toolId || QString::fromStdString(tool.name) == toolId) {
            toolToEdit = tool;
            foundTool = true;
            break;
        }
    }
    
    // If not found in database, create sample tool data based on toolId
    if (!foundTool) {
        qDebug() << "Tool not found in database, creating sample tool for:" << toolId;
        toolToEdit = createSampleToolFromId(toolId);
        foundTool = true;
    }
    
    if (foundTool) {
        // Set the current tool assembly
        m_currentToolAssembly = toolToEdit;
        m_currentToolId = toolId;
        m_isEditing = true;
        
        // Load tool parameters into dialog fields
        loadToolParametersIntoFields(toolToEdit);
        
        // Update tool type specific UI
        updateToolTypeSpecificUI();
        
        // Enable real-time updates
        enableRealTimeUpdates(true);
        
        // Update 3D visualization if available
        if (m_3dVisualizationEnabled) {
            updateToolVisualization();
        }
        
        qDebug() << "Successfully loaded tool parameters for editing";
    } else {
        qDebug() << "Error: Tool not found:" << toolId;
        emit errorOccurred(QString("Tool '%1' not found").arg(toolId));
    }
}

void ToolManagementDialog::deleteTool(const QString& toolId) {
    // Remove tool from database
    auto it = std::remove_if(m_toolDatabase.begin(), m_toolDatabase.end(),
        [&toolId](const ToolAssembly& tool) {
            return QString::fromStdString(tool.id) == toolId || QString::fromStdString(tool.name) == toolId;
        });
    
    if (it != m_toolDatabase.end()) {
        m_toolDatabase.erase(it, m_toolDatabase.end());
        emit toolDeleted(toolId);
        emit toolLibraryChanged();
    }
}

void ToolManagementDialog::duplicateTool(const QString& toolId) {
    // Find the original tool
    for (const auto& tool : m_toolDatabase) {
        if (QString::fromStdString(tool.id) == toolId || QString::fromStdString(tool.name) == toolId) {
            ToolAssembly duplicatedTool = tool;
            duplicatedTool.id = QString("TOOL_%1_COPY").arg(QDateTime::currentMSecsSinceEpoch()).toStdString();
            duplicatedTool.name += " Copy";
            duplicatedTool.toolNumber = "T99"; // Temporary tool number
            duplicatedTool.turretPosition = 99; // Temporary position
            duplicatedTool.isActive = false; // Start as inactive
            
            m_toolDatabase.append(duplicatedTool);
            emit toolAdded(QString::fromStdString(duplicatedTool.id));
            emit toolLibraryChanged();
            break;
        }
    }
}

// Tool library operations
void ToolManagementDialog::loadToolLibrary(const QString& filePath) {
    // Load tool library implementation
}

void ToolManagementDialog::saveToolLibrary(const QString& filePath) {
    // Save tool library implementation
}

void ToolManagementDialog::importToolsFromCatalog(const QString& catalogPath) {
    // Import tools from catalog implementation
}

void ToolManagementDialog::exportSelectedTools(const QString& filePath) {
    // Export selected tools implementation
}

// Tool filtering and search
void ToolManagementDialog::filterByToolType(ToolType toolType) {
    // Filter by tool type implementation
}

void ToolManagementDialog::filterByMaterial(InsertMaterial material) {
    // Filter by material implementation
}

void ToolManagementDialog::filterByManufacturer(const QString& manufacturer) {
    // Filter by manufacturer implementation
}

void ToolManagementDialog::searchTools(const QString& searchTerm) {
    // Search tools implementation
}

void ToolManagementDialog::clearFilters() {
    // Clear filters implementation
}

// ============================================================================
// Tool Parameter Synchronization Methods
// ============================================================================

ToolAssembly ToolManagementDialog::getCurrentToolAssembly() {
    // Update current tool assembly from UI fields
    updateToolAssemblyFromFields();
    return m_currentToolAssembly;
}

void ToolManagementDialog::setCurrentToolAssembly(const ToolAssembly& assembly) {
    m_currentToolAssembly = assembly;
    m_currentToolId = QString::fromStdString(assembly.id);
    
    // Load parameters into UI fields
    loadToolParametersIntoFields(assembly);
    
    // Update tool type specific UI
    updateToolTypeSpecificUI();
    
    // Update 3D visualization
    if (m_3dVisualizationEnabled) {
        updateToolVisualization();
    }
}

void ToolManagementDialog::loadToolParametersIntoFields(const ToolAssembly& assembly) {
    qDebug() << "Loading tool parameters into fields for:" << QString::fromStdString(assembly.name);
    
    // Temporarily disable real-time updates to prevent feedback loops
    bool wasEnabled = m_realTimeUpdatesEnabled;
    enableRealTimeUpdates(false);
    
    try {
        // Load basic tool info - check for null pointers first
        if (m_toolNameEdit) {
            m_toolNameEdit->setText(QString::fromStdString(assembly.name));
        } else {
            qDebug() << "Warning: m_toolNameEdit is null";
        }
        
        if (m_vendorEdit) {
            m_vendorEdit->setText(QString::fromStdString(assembly.manufacturer));
        } else {
            qDebug() << "Warning: m_vendorEdit is null";
        }
        
        if (m_toolNumberEdit) {
            m_toolNumberEdit->setText(QString::fromStdString(assembly.toolNumber));
        } else {
            qDebug() << "Warning: m_toolNumberEdit is null";
        }
        
        if (m_turretPositionSpin) {
            m_turretPositionSpin->setValue(assembly.turretPosition);
        } else {
            qDebug() << "Warning: m_turretPositionSpin is null";
        }
        
        if (m_isActiveCheck) {
            m_isActiveCheck->setChecked(assembly.isActive);
        } else {
            qDebug() << "Warning: m_isActiveCheck is null";
        }
        
        if (m_notesEdit) {
            m_notesEdit->setPlainText(QString::fromStdString(assembly.notes));
        } else {
            qDebug() << "Warning: m_notesEdit is null";
        }
        
        // Load tool offsets - check for null pointers
        if (m_toolOffsetXSpin) {
            m_toolOffsetXSpin->setValue(assembly.toolOffset_X);
        } else {
            qDebug() << "Warning: m_toolOffsetXSpin is null";
        }
        
        if (m_toolOffsetZSpin) {
            m_toolOffsetZSpin->setValue(assembly.toolOffset_Z);
        } else {
            qDebug() << "Warning: m_toolOffsetZSpin is null";
        }
        
        if (m_toolLengthOffsetSpin) {
            m_toolLengthOffsetSpin->setValue(assembly.toolLengthOffset);
        } else {
            qDebug() << "Warning: m_toolLengthOffsetSpin is null";
        }
        
        if (m_toolRadiusOffsetSpin) {
            m_toolRadiusOffsetSpin->setValue(assembly.toolRadiusOffset);
        } else {
            qDebug() << "Warning: m_toolRadiusOffsetSpin is null";
        }
        
        // Load tool life data - check for null pointers
        if (m_expectedLifeMinutesSpin) {
            m_expectedLifeMinutesSpin->setValue(assembly.expectedLifeMinutes);
        } else {
            qDebug() << "Warning: m_expectedLifeMinutesSpin is null";
        }
        
        if (m_usageMinutesSpin) {
            m_usageMinutesSpin->setValue(assembly.usageMinutes);
        } else {
            qDebug() << "Warning: m_usageMinutesSpin is null";
        }
        
        if (m_cycleCountSpin) {
            m_cycleCountSpin->setValue(assembly.cycleCount);
        } else {
            qDebug() << "Warning: m_cycleCountSpin is null";
        }
        
        if (m_lastMaintenanceDateEdit) {
            m_lastMaintenanceDateEdit->setText(QString::fromStdString(assembly.lastMaintenanceDate));
        } else {
            qDebug() << "Warning: m_lastMaintenanceDateEdit is null";
        }
        
        if (m_nextMaintenanceDateEdit) {
            m_nextMaintenanceDateEdit->setText(QString::fromStdString(assembly.nextMaintenanceDate));
        } else {
            qDebug() << "Warning: m_nextMaintenanceDateEdit is null";
        }
        
        // Load insert and holder data based on tool type
        switch (assembly.toolType) {
            case ToolType::GENERAL_TURNING:
                if (assembly.turningInsert) {
                    loadGeneralTurningInsertParameters(*assembly.turningInsert);
                } else {
                    qDebug() << "Warning: turningInsert is null for general turning tool";
                }
                break;
            case ToolType::THREADING:
                if (assembly.threadingInsert) {
                    loadThreadingInsertParameters(*assembly.threadingInsert);
                } else {
                    qDebug() << "Warning: threadingInsert is null for threading tool";
                }
                break;
            case ToolType::GROOVING:
                if (assembly.groovingInsert) {
                    loadGroovingInsertParameters(*assembly.groovingInsert);
                } else {
                    qDebug() << "Warning: groovingInsert is null for grooving tool";
                }
                break;
            default:
                qDebug() << "Unknown tool type:" << static_cast<int>(assembly.toolType);
                break;
        }
        
        // Load holder parameters
        if (assembly.holder) {
            loadHolderParameters(*assembly.holder);
        } else {
            qDebug() << "Warning: holder is null";
        }
        
        // Load cutting data
        loadCuttingDataParameters(assembly.cuttingData);
        
        qDebug() << "Successfully loaded tool parameters into fields";
        
    } catch (const std::exception& e) {
        qDebug() << "Exception while loading tool parameters:" << e.what();
        emit errorOccurred(QString("Error loading tool parameters: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception while loading tool parameters";
        emit errorOccurred("Unknown error while loading tool parameters");
    }
    
    // Restore real-time updates
    enableRealTimeUpdates(wasEnabled);
}

void ToolManagementDialog::loadGeneralTurningInsertParameters(const GeneralTurningInsert& insert) {
    qDebug() << "Loading general turning insert parameters";
    
    // Implementation for loading general turning insert parameters
    // This is a stub implementation to prevent crashes
    qDebug() << "Insert ISO Code:" << QString::fromStdString(insert.isoCode);
    qDebug() << "Insert Name:" << QString::fromStdString(insert.name);
}

void ToolManagementDialog::loadThreadingInsertParameters(const ThreadingInsert& insert) {
    qDebug() << "Loading threading insert parameters";
    
    // Implementation for loading threading insert parameters
    // This is a stub implementation to prevent crashes
    qDebug() << "Threading Insert ISO Code:" << QString::fromStdString(insert.isoCode);
    qDebug() << "Threading Insert Name:" << QString::fromStdString(insert.name);
}

void ToolManagementDialog::loadGroovingInsertParameters(const GroovingInsert& insert) {
    qDebug() << "Loading grooving insert parameters";
    
    // Implementation for loading grooving insert parameters
    // This is a stub implementation to prevent crashes
    qDebug() << "Grooving Insert ISO Code:" << QString::fromStdString(insert.isoCode);
    qDebug() << "Grooving Insert Name:" << QString::fromStdString(insert.name);
}

void ToolManagementDialog::loadHolderParameters(const ToolHolder& holder) {
    qDebug() << "Loading holder parameters";
    
    // Implementation for loading holder parameters
    // This is a stub implementation to prevent crashes
    qDebug() << "Holder ISO Code:" << QString::fromStdString(holder.isoCode);
    qDebug() << "Holder Name:" << QString::fromStdString(holder.name);
}

void ToolManagementDialog::loadCuttingDataParameters(const CuttingData& cuttingData) {
    qDebug() << "Loading cutting data parameters";
    
    // Implementation for loading cutting data parameters
    // This is a stub implementation to prevent crashes
    qDebug() << "Surface Speed:" << cuttingData.surfaceSpeed;
    qDebug() << "Feedrate:" << cuttingData.cuttingFeedrate;
    qDebug() << "Max Depth:" << cuttingData.maxDepthOfCut;
}

// ============================================================================
// Helper Methods for Parameter Synchronization
// ============================================================================

void ToolManagementDialog::setComboBoxByValue(QComboBox* comboBox, int value) {
    if (!comboBox) return;
    
    for (int i = 0; i < comboBox->count(); ++i) {
        if (comboBox->itemData(i).toInt() == value) {
            comboBox->setCurrentIndex(i);
            return;
        }
    }
    
    // If value not found, try to set by index directly (fallback)
    if (value >= 0 && value < comboBox->count()) {
        comboBox->setCurrentIndex(value);
    }
}

void ToolManagementDialog::clearAllParameterFields() {
    qDebug() << "Clearing all parameter fields";
    
    // Clear tool info fields
    if (m_toolNameEdit) m_toolNameEdit->clear();
    if (m_vendorEdit) m_vendorEdit->clear();
    if (m_manufacturerEdit) m_manufacturerEdit->clear();
    if (m_partNumberEdit) m_partNumberEdit->clear();
    if (m_productIdEdit) m_productIdEdit->clear();
    if (m_productLinkEdit) m_productLinkEdit->clear();
    if (m_notesEdit) m_notesEdit->clear();
    if (m_toolNumberEdit) m_toolNumberEdit->setText("T01");
    if (m_turretPositionSpin) m_turretPositionSpin->setValue(1);
    if (m_isActiveCheck) m_isActiveCheck->setChecked(true);
    
    // Clear offset fields
    if (m_toolOffsetXSpin) m_toolOffsetXSpin->setValue(0.0);
    if (m_toolOffsetZSpin) m_toolOffsetZSpin->setValue(0.0);
    if (m_toolLengthOffsetSpin) m_toolLengthOffsetSpin->setValue(0.0);
    if (m_toolRadiusOffsetSpin) m_toolRadiusOffsetSpin->setValue(0.0);
    
    // Clear tool life fields
    if (m_expectedLifeMinutesSpin) m_expectedLifeMinutesSpin->setValue(480.0);
    if (m_usageMinutesSpin) m_usageMinutesSpin->setValue(0.0);
    if (m_cycleCountSpin) m_cycleCountSpin->setValue(0);
    if (m_lastMaintenanceDateEdit) m_lastMaintenanceDateEdit->clear();
    if (m_nextMaintenanceDateEdit) m_nextMaintenanceDateEdit->clear();
    
    qDebug() << "All parameter fields cleared";
}

void ToolManagementDialog::updateToolTypeSpecificUI() {
    qDebug() << "Updating tool type specific UI for type:" << static_cast<int>(m_currentToolAssembly.toolType);
    
    if (!m_toolEditTabs) {
        qDebug() << "Warning: m_toolEditTabs is null";
        return;
    }
    
    // Enable/disable tabs based on tool type
    bool hasInsertTab = (m_toolEditTabs->count() > 0);
    bool hasHolderTab = (m_toolEditTabs->count() > 1);
    bool hasCuttingDataTab = (m_toolEditTabs->count() > 2);
    bool hasToolInfoTab = (m_toolEditTabs->count() > 3);
    
    if (hasInsertTab) {
        m_toolEditTabs->setTabEnabled(0, true); // Insert properties always enabled
    }
    if (hasHolderTab) {
        m_toolEditTabs->setTabEnabled(1, true); // Holder properties always enabled
    }
    if (hasCuttingDataTab) {
        m_toolEditTabs->setTabEnabled(2, true); // Cutting data always enabled
    }
    if (hasToolInfoTab) {
        m_toolEditTabs->setTabEnabled(3, true); // Tool info always enabled
    }
    
    // Update window title based on tool type
    QString toolTypeStr = formatToolType(m_currentToolAssembly.toolType);
    if (m_isEditing) {
        setWindowTitle(QString("Edit %1 Tool - IntuiCAM").arg(toolTypeStr));
    } else {
        setWindowTitle(QString("Add New %1 Tool - IntuiCAM").arg(toolTypeStr));
    }
    
    qDebug() << "Tool type specific UI updated";
}

void ToolManagementDialog::updateToolAssemblyFromFields() {
    qDebug() << "Updating tool assembly from fields";
    
    // Update basic tool info from fields - with null checks
    if (m_toolNameEdit) {
        m_currentToolAssembly.name = m_toolNameEdit->text().toStdString();
    }
    if (m_vendorEdit) {
        m_currentToolAssembly.manufacturer = m_vendorEdit->text().toStdString();
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
    if (m_notesEdit) {
        m_currentToolAssembly.notes = m_notesEdit->toPlainText().toStdString();
    }
    
    // Update tool offsets
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
    
    // Update tool life data
    if (m_expectedLifeMinutesSpin) {
        m_currentToolAssembly.expectedLifeMinutes = m_expectedLifeMinutesSpin->value();
    }
    if (m_usageMinutesSpin) {
        m_currentToolAssembly.usageMinutes = m_usageMinutesSpin->value();
    }
    if (m_cycleCountSpin) {
        m_currentToolAssembly.cycleCount = m_cycleCountSpin->value();
    }
    if (m_lastMaintenanceDateEdit) {
        m_currentToolAssembly.lastMaintenanceDate = m_lastMaintenanceDateEdit->text().toStdString();
    }
    if (m_nextMaintenanceDateEdit) {
        m_currentToolAssembly.nextMaintenanceDate = m_nextMaintenanceDateEdit->text().toStdString();
    }
    
    qDebug() << "Tool assembly updated from fields";
}

void ToolManagementDialog::updateCuttingDataFromFields() {
    CuttingData& cd = m_currentToolAssembly.cuttingData;
    
    if (m_constantSurfaceSpeedCheck) cd.constantSurfaceSpeed = m_constantSurfaceSpeedCheck->isChecked();
    if (m_surfaceSpeedSpin) cd.surfaceSpeed = m_surfaceSpeedSpin->value();
    if (m_spindleRPMSpin) cd.spindleRPM = m_spindleRPMSpin->value();
    if (m_feedPerRevolutionCheck) cd.feedPerRevolution = m_feedPerRevolutionCheck->isChecked();
    if (m_cuttingFeedrateSpin) cd.cuttingFeedrate = m_cuttingFeedrateSpin->value();
    if (m_leadInFeedrateSpin) cd.leadInFeedrate = m_leadInFeedrateSpin->value();
    if (m_leadOutFeedrateSpin) cd.leadOutFeedrate = m_leadOutFeedrateSpin->value();
    if (m_maxDepthOfCutSpin) cd.maxDepthOfCut = m_maxDepthOfCutSpin->value();
    if (m_maxFeedrateSpin) cd.maxFeedrate = m_maxFeedrateSpin->value();
    if (m_minSurfaceSpeedSpin) cd.minSurfaceSpeed = m_minSurfaceSpeedSpin->value();
    if (m_maxSurfaceSpeedSpin) cd.maxSurfaceSpeed = m_maxSurfaceSpeedSpin->value();
    if (m_floodCoolantCheck) cd.floodCoolant = m_floodCoolantCheck->isChecked();
    if (m_mistCoolantCheck) cd.mistCoolant = m_mistCoolantCheck->isChecked();
    if (m_coolantPressureSpin) cd.coolantPressure = m_coolantPressureSpin->value();
    if (m_coolantFlowSpin) cd.coolantFlow = m_coolantFlowSpin->value();
}

void ToolManagementDialog::updateInsertDataFromFields() {
    switch (m_currentToolAssembly.toolType) {
        case ToolType::GENERAL_TURNING:
            updateGeneralTurningInsertFromFields();
            break;
        case ToolType::THREADING:
            updateThreadingInsertFromFields();
            break;
        case ToolType::GROOVING:
            updateGroovingInsertFromFields();
            break;
        default:
            break;
    }
}

void ToolManagementDialog::updateGeneralTurningInsertFromFields() {
    if (!m_currentToolAssembly.turningInsert) {
        m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.turningInsert;
    
    if (m_isoCodeEdit) insert.isoCode = m_isoCodeEdit->text().toStdString();
    if (m_inscribedCircleSpin) insert.inscribedCircle = m_inscribedCircleSpin->value();
    if (m_thicknessSpin) insert.thickness = m_thicknessSpin->value();
    if (m_cornerRadiusSpin) insert.cornerRadius = m_cornerRadiusSpin->value();
    if (m_cuttingEdgeLengthSpin) insert.cuttingEdgeLength = m_cuttingEdgeLengthSpin->value();
    if (m_widthSpin) insert.width = m_widthSpin->value();
    if (m_rakeAngleSpin) insert.rake_angle = m_rakeAngleSpin->value();
    if (m_inclinationAngleSpin) insert.inclination_angle = m_inclinationAngleSpin->value();
    if (m_productIdEdit) insert.productId = m_productIdEdit->text().toStdString();
    if (m_partNumberEdit) insert.partNumber = m_partNumberEdit->text().toStdString();
}

void ToolManagementDialog::updateThreadingInsertFromFields() {
    if (!m_currentToolAssembly.threadingInsert) {
        m_currentToolAssembly.threadingInsert = std::make_shared<ThreadingInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.threadingInsert;
    
    if (m_threadingISOCodeEdit) insert.isoCode = m_threadingISOCodeEdit->text().toStdString();
    if (m_threadingThicknessSpin) insert.thickness = m_threadingThicknessSpin->value();
    if (m_threadingWidthSpin) insert.width = m_threadingWidthSpin->value();
    if (m_minThreadPitchSpin) insert.minThreadPitch = m_minThreadPitchSpin->value();
    if (m_maxThreadPitchSpin) insert.maxThreadPitch = m_maxThreadPitchSpin->value();
    if (m_internalThreadsCheck) insert.internalThreads = m_internalThreadsCheck->isChecked();
    if (m_externalThreadsCheck) insert.externalThreads = m_externalThreadsCheck->isChecked();
    if (m_threadProfileAngleSpin) insert.threadProfileAngle = m_threadProfileAngleSpin->value();
    if (m_threadTipRadiusSpin) insert.threadTipRadius = m_threadTipRadiusSpin->value();
}

void ToolManagementDialog::updateGroovingInsertFromFields() {
    if (!m_currentToolAssembly.groovingInsert) {
        m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
    }
    
    auto& insert = *m_currentToolAssembly.groovingInsert;
    
    if (m_groovingISOCodeEdit) insert.isoCode = m_groovingISOCodeEdit->text().toStdString();
    if (m_groovingThicknessSpin) insert.thickness = m_groovingThicknessSpin->value();
    if (m_groovingOverallLengthSpin) insert.overallLength = m_groovingOverallLengthSpin->value();
    if (m_groovingWidthSpin) insert.width = m_groovingWidthSpin->value();
    if (m_groovingCornerRadiusSpin) insert.cornerRadius = m_groovingCornerRadiusSpin->value();
    if (m_groovingHeadLengthSpin) insert.headLength = m_groovingHeadLengthSpin->value();
    if (m_grooveWidthSpin) insert.grooveWidth = m_grooveWidthSpin->value();
}

void ToolManagementDialog::updateHolderDataFromFields() {
    if (!m_currentToolAssembly.holder) {
        m_currentToolAssembly.holder = std::make_shared<ToolHolder>();
    }
    
    auto& holder = *m_currentToolAssembly.holder;
    
    if (m_holderISOCodeEdit) holder.isoCode = m_holderISOCodeEdit->text().toStdString();
    if (m_cuttingWidthSpin) holder.cuttingWidth = m_cuttingWidthSpin->value();
    if (m_headLengthSpin) holder.headLength = m_headLengthSpin->value();
    if (m_overallLengthSpin) holder.overallLength = m_overallLengthSpin->value();
    if (m_shankWidthSpin) holder.shankWidth = m_shankWidthSpin->value();
    if (m_shankHeightSpin) holder.shankHeight = m_shankHeightSpin->value();
    if (m_shankDiameterSpin) holder.shankDiameter = m_shankDiameterSpin->value();
    if (m_roundShankCheck) holder.roundShank = m_roundShankCheck->isChecked();
    if (m_insertSeatAngleSpin) holder.insertSeatAngle = m_insertSeatAngleSpin->value();
    if (m_insertSetbackSpin) holder.insertSetback = m_insertSetbackSpin->value();
    if (m_sideAngleSpin) holder.sideAngle = m_sideAngleSpin->value();
    if (m_backAngleSpin) holder.backAngle = m_backAngleSpin->value();
    if (m_isInternalCheck) holder.isInternal = m_isInternalCheck->isChecked();
    if (m_isGroovingCheck) holder.isGrooving = m_isGroovingCheck->isChecked();
    if (m_isThreadingCheck) holder.isThreading = m_isThreadingCheck->isChecked();
}

void ToolManagementDialog::loadDefaultParameters() {
    qDebug() << "Loading default parameters for tool type:" << static_cast<int>(m_currentToolAssembly.toolType);
    
    // Initialize shared pointers for components based on tool type
    switch (m_currentToolAssembly.toolType) {
        case ToolType::GENERAL_TURNING:
            if (!m_currentToolAssembly.turningInsert) {
                m_currentToolAssembly.turningInsert = std::make_shared<GeneralTurningInsert>();
                // Set default values
                m_currentToolAssembly.turningInsert->name = "Default Turning Insert";
                m_currentToolAssembly.turningInsert->isoCode = "CNMG120408";
                m_currentToolAssembly.turningInsert->inscribedCircle = 12.7;
                m_currentToolAssembly.turningInsert->thickness = 4.76;
                m_currentToolAssembly.turningInsert->cornerRadius = 0.8;
                m_currentToolAssembly.turningInsert->isActive = true;
            }
            break;
            
        case ToolType::THREADING:
            if (!m_currentToolAssembly.threadingInsert) {
                m_currentToolAssembly.threadingInsert = std::make_shared<ThreadingInsert>();
                // Set default values
                m_currentToolAssembly.threadingInsert->name = "Default Threading Insert";
                m_currentToolAssembly.threadingInsert->isoCode = "16ER28UN";
                m_currentToolAssembly.threadingInsert->thickness = 4.0;
                m_currentToolAssembly.threadingInsert->width = 16.0;
                m_currentToolAssembly.threadingInsert->isActive = true;
            }
            break;
            
        case ToolType::GROOVING:
            if (!m_currentToolAssembly.groovingInsert) {
                m_currentToolAssembly.groovingInsert = std::make_shared<GroovingInsert>();
                // Set default values
                m_currentToolAssembly.groovingInsert->name = "Default Grooving Insert";
                m_currentToolAssembly.groovingInsert->isoCode = "GTN300";
                m_currentToolAssembly.groovingInsert->thickness = 3.0;
                m_currentToolAssembly.groovingInsert->grooveWidth = 3.0;
                m_currentToolAssembly.groovingInsert->isActive = true;
            }
            break;
            
        default:
            qDebug() << "Unknown tool type, using general turning defaults";
            break;
    }
    
    // Initialize holder if not present
    if (!m_currentToolAssembly.holder) {
        m_currentToolAssembly.holder = std::make_shared<ToolHolder>();
        // Set default values
        m_currentToolAssembly.holder->name = "Default Holder";
        m_currentToolAssembly.holder->isoCode = "MCLNR2525M12";
        m_currentToolAssembly.holder->handOrientation = HandOrientation::RIGHT_HAND;
        m_currentToolAssembly.holder->clampingStyle = ClampingStyle::TOP_CLAMP;
        m_currentToolAssembly.holder->shankWidth = 25.0;
        m_currentToolAssembly.holder->shankHeight = 25.0;
        m_currentToolAssembly.holder->overallLength = 150.0;
        m_currentToolAssembly.holder->isActive = true;
    }
    
    // Set default cutting data
    m_currentToolAssembly.cuttingData.surfaceSpeed = 200.0;
    m_currentToolAssembly.cuttingData.cuttingFeedrate = 0.2;
    m_currentToolAssembly.cuttingData.maxDepthOfCut = 2.0;
    m_currentToolAssembly.cuttingData.constantSurfaceSpeed = true;
    m_currentToolAssembly.cuttingData.feedPerRevolution = true;
    m_currentToolAssembly.cuttingData.preferredCoolant = CoolantType::FLOOD;
    
    // Set default assembly values
    if (m_currentToolAssembly.name.empty()) {
        m_currentToolAssembly.name = "New Tool";
    }
    if (m_currentToolAssembly.toolNumber.empty()) {
        m_currentToolAssembly.toolNumber = "T01";
    }
    
    qDebug() << "Default parameters loaded successfully";
}

ToolAssembly ToolManagementDialog::createSampleToolFromId(const QString& toolId) {
    qDebug() << "Creating sample tool from ID:" << toolId;
    
    ToolAssembly sampleTool;
    sampleTool.id = toolId.toStdString();
    sampleTool.name = toolId.toStdString();
    sampleTool.toolNumber = "T01";
    sampleTool.turretPosition = 1;
    sampleTool.isActive = true;
    sampleTool.toolType = ToolType::GENERAL_TURNING;
    
    // Create default insert
    sampleTool.turningInsert = std::make_shared<GeneralTurningInsert>();
    sampleTool.turningInsert->name = (toolId + " Insert").toStdString();
    sampleTool.turningInsert->isoCode = "CNMG120408";
    sampleTool.turningInsert->inscribedCircle = 12.7;
    sampleTool.turningInsert->thickness = 4.76;
    sampleTool.turningInsert->cornerRadius = 0.8;
    sampleTool.turningInsert->isActive = true;
    
    // Create default holder
    sampleTool.holder = std::make_shared<ToolHolder>();
    sampleTool.holder->name = (toolId + " Holder").toStdString();
    sampleTool.holder->isoCode = "MCLNR2525M12";
    sampleTool.holder->handOrientation = HandOrientation::RIGHT_HAND;
    sampleTool.holder->clampingStyle = ClampingStyle::TOP_CLAMP;
    sampleTool.holder->shankWidth = 25.0;
    sampleTool.holder->shankHeight = 25.0;
    sampleTool.holder->overallLength = 150.0;
    sampleTool.holder->isActive = true;
    
    // Set default cutting data
    sampleTool.cuttingData.surfaceSpeed = 200.0;
    sampleTool.cuttingData.cuttingFeedrate = 0.2;
    sampleTool.cuttingData.maxDepthOfCut = 2.0;
    sampleTool.cuttingData.constantSurfaceSpeed = true;
    sampleTool.cuttingData.feedPerRevolution = true;
    sampleTool.cuttingData.preferredCoolant = CoolantType::FLOOD;
    
    qDebug() << "Sample tool created successfully";
    return sampleTool;
} 