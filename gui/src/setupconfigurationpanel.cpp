#include "setupconfigurationpanel.h"
#include "materialmanager.h"
#include "toolmanager.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QHeaderView>
#include <QDir>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QProgressBar>
#include <QListWidget>
#include <QListWidgetItem>

namespace IntuiCAM {
namespace GUI {

using namespace IntuiCAM::GUI;

// Define the static const members
const QStringList MATERIAL_NAMES = {
    "Aluminum 6061-T6",
    "Aluminum 7075-T6",
    "Steel 1018",
    "Steel 4140",
    "Stainless Steel 316",
    "Stainless Steel 304",
    "Brass C360",
    "Bronze",
    "Titanium Grade 5",
    "Plastic - ABS",
    "Plastic - Delrin (POM)",
    "Custom Material"
};

const QStringList SURFACE_FINISH_NAMES = {
    "Rough (32 μm Ra)",
    "Medium (16 μm Ra)",
    "Fine (8 μm Ra)",
    "Smooth (4 μm Ra)",
    "Polished (2 μm Ra)",
    "Mirror (1 μm Ra)"
};

const QStringList OPERATION_ORDER = {
    "Contouring",
    "Threading",
    "Chamfering",
    "Parting"
};

SetupConfigurationPanel::SetupConfigurationPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_partTab(nullptr)
    , m_operationsTabWidget(nullptr)
    , m_contouringTab(nullptr)
    , m_threadingTab(nullptr)
    , m_chamferingTab(nullptr)
    , m_partingTab(nullptr)
    , m_materialManager(nullptr)
    , m_toolManager(nullptr)
    , m_materialPropertiesLabel(nullptr)
    , m_contouringEnabledCheck(nullptr)
    , m_contouringParamsButton(nullptr)
    , m_threadingEnabledCheck(nullptr)
    , m_threadingParamsButton(nullptr)
    , m_chamferingEnabledCheck(nullptr)
    , m_chamferingParamsButton(nullptr)
    , m_partingEnabledCheck(nullptr)
    , m_partingParamsButton(nullptr)
{
    setupUI();
    setupConnections();
    applyTabStyling();
}

SetupConfigurationPanel::~SetupConfigurationPanel()
{
    // Qt handles cleanup automatically
}

void SetupConfigurationPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);

    // Create top part setup section
    m_partTab = new QWidget();
    setupPartTab();
    m_mainLayout->addWidget(m_partTab);

    // Create operations tab widget
    m_operationsTabWidget = new QTabWidget();
    m_operationsTabWidget->setTabPosition(QTabWidget::North);
    m_operationsTabWidget->setDocumentMode(true);

    // Create operation tabs
    m_contouringTab = new QWidget();
    m_threadingTab = new QWidget();
    m_chamferingTab = new QWidget();
    m_partingTab = new QWidget();

    // Setup individual operation tabs
    setupMachiningTab();

    // Add tabs to widget
    m_operationsTabWidget->addTab(m_contouringTab, "Contouring");
    m_operationsTabWidget->addTab(m_threadingTab, "Threading");
    m_operationsTabWidget->addTab(m_chamferingTab, "Chamfering");
    m_operationsTabWidget->addTab(m_partingTab, "Parting");

    // Add operations tab widget to main layout
    m_mainLayout->addWidget(m_operationsTabWidget);

    
}

void SetupConfigurationPanel::setupPartTab()
{
    QVBoxLayout* partTabLayout = new QVBoxLayout(m_partTab);
    partTabLayout->setContentsMargins(12, 12, 12, 12);
    partTabLayout->setSpacing(16);

    // Part Setup Group
    m_partSetupGroup = new QGroupBox("Part Setup");
    m_partSetupLayout = new QVBoxLayout(m_partSetupGroup);
    m_partSetupLayout->setSpacing(8);

    // STEP file selection
    m_stepFileLayout = new QHBoxLayout();
    QLabel* stepFileLabel = new QLabel("STEP File:");
    stepFileLabel->setMinimumWidth(120);
    m_stepFileEdit = new QLineEdit();
    m_stepFileEdit->setPlaceholderText("Select STEP file...");
    m_stepFileEdit->setReadOnly(true);
    m_browseButton = new QPushButton("Browse...");
    m_browseButton->setMaximumWidth(80);
    
    m_stepFileLayout->addWidget(stepFileLabel);
    m_stepFileLayout->addWidget(m_stepFileEdit);
    m_stepFileLayout->addWidget(m_browseButton);
    m_partSetupLayout->addLayout(m_stepFileLayout);

    // Manual rotational axis selection
    m_manualAxisButton = new QPushButton("Select Rotational Axis from 3D View");
    m_manualAxisButton->setMinimumHeight(32);
    m_manualAxisButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1565C0;"
        "}"
    );
    
    m_axisInfoLabel = new QLabel("Click the button above, then select a cylindrical surface or circular edge in the 3D view");
    m_axisInfoLabel->setStyleSheet("color: #666; font-size: 11px;");
    m_axisInfoLabel->setWordWrap(true);
    
    m_partSetupLayout->addWidget(m_manualAxisButton);
    m_partSetupLayout->addWidget(m_axisInfoLabel);
    
    // Part positioning controls
    m_distanceLayout = new QHBoxLayout();
    m_distanceLabel = new QLabel("Distance to Chuck:");
    m_distanceLabel->setMinimumWidth(120);
    m_distanceSlider = new QSlider(Qt::Horizontal);
    m_distanceSlider->setRange(0, 100);
    m_distanceSlider->setValue(25);
    m_distanceSpinBox = new QDoubleSpinBox();
    m_distanceSpinBox->setRange(0.0, 100.0);
    m_distanceSpinBox->setValue(25.0);
    m_distanceSpinBox->setSuffix(" mm");
    m_distanceSpinBox->setDecimals(1);
    m_distanceSpinBox->setMaximumWidth(80);
    
    m_distanceLayout->addWidget(m_distanceLabel);
    m_distanceLayout->addWidget(m_distanceSlider);
    m_distanceLayout->addWidget(m_distanceSpinBox);
    m_partSetupLayout->addLayout(m_distanceLayout);
    
    // Orientation flip checkbox
    m_flipOrientationCheckBox = new QCheckBox("Flip Part Orientation (180°)");
    m_flipOrientationCheckBox->setChecked(false);
    m_partSetupLayout->addWidget(m_flipOrientationCheckBox);

    partTabLayout->addWidget(m_partSetupGroup);

    // Material Settings Group
    m_materialGroup = new QGroupBox("Material Settings");
    m_materialLayout = new QVBoxLayout(m_materialGroup);
    m_materialLayout->setSpacing(8);

    // Material type
    m_materialTypeLayout = new QHBoxLayout();
    m_materialTypeLabel = new QLabel("Material Type:");
    m_materialTypeLabel->setMinimumWidth(120);
    m_materialTypeCombo = new QComboBox();
    m_materialTypeCombo->addItems(MATERIAL_NAMES);
    m_materialTypeCombo->setCurrentText("Aluminum 6061-T6");
    
    m_materialTypeLayout->addWidget(m_materialTypeLabel);
    m_materialTypeLayout->addWidget(m_materialTypeCombo);
    m_materialTypeLayout->addStretch();
    m_materialLayout->addLayout(m_materialTypeLayout);

    // Raw material diameter
    m_rawDiameterLayout = new QHBoxLayout();
    m_rawDiameterLabel = new QLabel("Raw Diameter:");
    m_rawDiameterLabel->setMinimumWidth(120);
    m_rawDiameterSpin = new QDoubleSpinBox();
    m_rawDiameterSpin->setRange(5.0, 500.0);
    m_rawDiameterSpin->setValue(50.0);
    m_rawDiameterSpin->setSuffix(" mm");
    m_rawDiameterSpin->setDecimals(1);
    
    m_rawDiameterLayout->addWidget(m_rawDiameterLabel);
    m_rawDiameterLayout->addWidget(m_rawDiameterSpin);
    m_rawDiameterLayout->addStretch();
    m_materialLayout->addLayout(m_rawDiameterLayout);

    // Raw material length (auto-calculated, display only)
    m_rawLengthLabel = new QLabel("Raw Length: Auto-calculated");
    m_rawLengthLabel->setStyleSheet("color: #666; font-size: 11px;");
    m_materialLayout->addWidget(m_rawLengthLabel);

    // Material properties display
    m_materialPropertiesLabel = new QLabel("Material properties will be shown here");
    m_materialPropertiesLabel->setStyleSheet("color: #666; font-size: 11px; padding: 8px; background: #f5f5f5; border-radius: 4px;");
    m_materialPropertiesLabel->setWordWrap(true);
    m_materialLayout->addWidget(m_materialPropertiesLabel);

    // Material details button
    m_materialDetailsButton = new QPushButton("Material Details & Cutting Parameters");
    m_materialDetailsButton->setMaximumHeight(28);
    m_materialDetailsButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
    );
    m_materialLayout->addWidget(m_materialDetailsButton);

    partTabLayout->addWidget(m_materialGroup);

    partTabLayout->addStretch();
}

void SetupConfigurationPanel::setupMachiningTab()
{
    // --- Contouring Tab ---
    QVBoxLayout* contourLayout = new QVBoxLayout(m_contouringTab);
    contourLayout->setContentsMargins(12, 12, 12, 12);
    contourLayout->setSpacing(16);

    QHBoxLayout* contourEnableLayout = new QHBoxLayout();
    m_contouringEnabledCheck = new QCheckBox("Enable Contouring");
    m_contouringParamsButton = new QPushButton("Parameters...");
    m_contouringParamsButton->setEnabled(false);
    contourEnableLayout->addWidget(m_contouringEnabledCheck);
    contourEnableLayout->addStretch();
    contourEnableLayout->addWidget(m_contouringParamsButton);
    contourLayout->addLayout(contourEnableLayout);

    m_machiningParamsGroup = new QGroupBox("Machining Parameters");
    m_machiningParamsLayout = new QVBoxLayout(m_machiningParamsGroup);
    m_machiningParamsLayout->setSpacing(8);

    // Facing allowance
    m_facingAllowanceLayout = new QHBoxLayout();
    m_facingAllowanceLabel = new QLabel("Facing Allowance:");
    m_facingAllowanceLabel->setMinimumWidth(140);
    m_facingAllowanceSpin = new QDoubleSpinBox();
    m_facingAllowanceSpin->setRange(0.0, 50.0);
    m_facingAllowanceSpin->setValue(10.0);
    m_facingAllowanceSpin->setSuffix(" mm");
    m_facingAllowanceSpin->setDecimals(1);
    
    m_facingAllowanceLayout->addWidget(m_facingAllowanceLabel);
    m_facingAllowanceLayout->addWidget(m_facingAllowanceSpin);
    m_facingAllowanceLayout->addStretch();
    m_machiningParamsLayout->addLayout(m_facingAllowanceLayout);

    // Roughing allowance
    m_roughingAllowanceLayout = new QHBoxLayout();
    m_roughingAllowanceLabel = new QLabel("Roughing Allowance:");
    m_roughingAllowanceLabel->setMinimumWidth(140);
    m_roughingAllowanceSpin = new QDoubleSpinBox();
    m_roughingAllowanceSpin->setRange(0.0, 5.0);
    m_roughingAllowanceSpin->setValue(1.0);
    m_roughingAllowanceSpin->setSuffix(" mm");
    m_roughingAllowanceSpin->setDecimals(2);
    
    m_roughingAllowanceLayout->addWidget(m_roughingAllowanceLabel);
    m_roughingAllowanceLayout->addWidget(m_roughingAllowanceSpin);
    m_roughingAllowanceLayout->addStretch();
    m_machiningParamsLayout->addLayout(m_roughingAllowanceLayout);

    // Finishing allowance
    m_finishingAllowanceLayout = new QHBoxLayout();
    m_finishingAllowanceLabel = new QLabel("Finishing Allowance:");
    m_finishingAllowanceLabel->setMinimumWidth(140);
    m_finishingAllowanceSpin = new QDoubleSpinBox();
    m_finishingAllowanceSpin->setRange(0.0, 2.0);
    m_finishingAllowanceSpin->setValue(0.2);
    m_finishingAllowanceSpin->setSuffix(" mm");
    m_finishingAllowanceSpin->setDecimals(2);
    
    m_finishingAllowanceLayout->addWidget(m_finishingAllowanceLabel);
    m_finishingAllowanceLayout->addWidget(m_finishingAllowanceSpin);
    m_finishingAllowanceLayout->addStretch();
    m_machiningParamsLayout->addLayout(m_finishingAllowanceLayout);

    // Parting width
    m_partingWidthLayout = new QHBoxLayout();
    m_partingWidthLabel = new QLabel("Parting Width:");
    m_partingWidthLabel->setMinimumWidth(140);
    m_partingWidthSpin = new QDoubleSpinBox();
    m_partingWidthSpin->setRange(1.0, 5.0);
    m_partingWidthSpin->setValue(3.0);
    m_partingWidthSpin->setSuffix(" mm");
    m_partingWidthSpin->setDecimals(1);
    
    m_partingWidthLayout->addWidget(m_partingWidthLabel);
    m_partingWidthLayout->addWidget(m_partingWidthSpin);
    m_partingWidthLayout->addStretch();
    m_machiningParamsLayout->addLayout(m_partingWidthLayout);

    contourLayout->addWidget(m_machiningParamsGroup);

    // Quality Group
    m_qualityGroup = new QGroupBox("Quality Settings");
    m_qualityLayout = new QVBoxLayout(m_qualityGroup);
    m_qualityLayout->setSpacing(8);

    // Surface finish
    m_surfaceFinishLayout = new QHBoxLayout();
    m_surfaceFinishLabel = new QLabel("Surface Finish:");
    m_surfaceFinishLabel->setMinimumWidth(140);
    m_surfaceFinishCombo = new QComboBox();
    
    // Add surface finish options
    QStringList SURFACE_FINISH_NAMES = {
        "Rough (32 μm Ra)",
        "Medium (16 μm Ra)",
        "Fine (8 μm Ra)",
        "Smooth (4 μm Ra)",
        "Polished (2 μm Ra)",
        "Mirror (1 μm Ra)"
    };
    
    m_surfaceFinishCombo->addItems(SURFACE_FINISH_NAMES);
    m_surfaceFinishCombo->setCurrentText("Medium (16 μm Ra)");
    
    m_surfaceFinishLayout->addWidget(m_surfaceFinishLabel);
    m_surfaceFinishLayout->addWidget(m_surfaceFinishCombo);
    m_surfaceFinishLayout->addStretch();
    m_qualityLayout->addLayout(m_surfaceFinishLayout);

    // Tolerance
    m_toleranceLayout = new QHBoxLayout();
    m_toleranceLabel = new QLabel("Tolerance:");
    m_toleranceLabel->setMinimumWidth(140);
    m_toleranceSpin = new QDoubleSpinBox();
    m_toleranceSpin->setRange(0.001, 1.0);
    m_toleranceSpin->setValue(0.1);
    m_toleranceSpin->setSuffix(" mm");
    m_toleranceSpin->setDecimals(3);
    
    m_toleranceLayout->addWidget(m_toleranceLabel);
    m_toleranceLayout->addWidget(m_toleranceSpin);
    m_toleranceLayout->addStretch();
    m_qualityLayout->addLayout(m_toleranceLayout);

    contourLayout->addWidget(m_qualityGroup);

    QGroupBox* contourToolsGroup = new QGroupBox("Recommended Tools");
    QVBoxLayout* contourToolsLayout = new QVBoxLayout(contourToolsGroup);
    QListWidget* contourToolsList = new QListWidget();
    contourToolsLayout->addWidget(contourToolsList);
    contourLayout->addWidget(contourToolsGroup);
    m_operationToolLists.insert("contouring", contourToolsList);

    contourLayout->addStretch();

    // --- Threading Tab ---
    QVBoxLayout* threadingLayout = new QVBoxLayout(m_threadingTab);
    threadingLayout->setContentsMargins(12, 12, 12, 12);
    threadingLayout->setSpacing(16);
    QHBoxLayout* threadEnableLayout = new QHBoxLayout();
    m_threadingEnabledCheck = new QCheckBox("Enable Threading");
    m_threadingParamsButton = new QPushButton("Parameters...");
    m_threadingParamsButton->setEnabled(false);
    threadEnableLayout->addWidget(m_threadingEnabledCheck);
    threadEnableLayout->addStretch();
    threadEnableLayout->addWidget(m_threadingParamsButton);
    threadingLayout->addLayout(threadEnableLayout);
    QLabel* threadingInfo = new QLabel("Select faces to thread in the 3D view.");
    threadingInfo->setWordWrap(true);
    threadingLayout->addWidget(threadingInfo);

    QGroupBox* threadingToolsGroup = new QGroupBox("Recommended Tools");
    QVBoxLayout* threadingToolsLayout = new QVBoxLayout(threadingToolsGroup);
    QListWidget* threadingToolsList = new QListWidget();
    threadingToolsLayout->addWidget(threadingToolsList);
    threadingLayout->addWidget(threadingToolsGroup);
    m_operationToolLists.insert("threading", threadingToolsList);

    threadingLayout->addStretch();

    // --- Chamfering Tab ---
    QVBoxLayout* chamferLayout = new QVBoxLayout(m_chamferingTab);
    chamferLayout->setContentsMargins(12, 12, 12, 12);
    chamferLayout->setSpacing(16);
    QHBoxLayout* chamferEnableLayout = new QHBoxLayout();
    m_chamferingEnabledCheck = new QCheckBox("Enable Chamfering");
    m_chamferingParamsButton = new QPushButton("Parameters...");
    m_chamferingParamsButton->setEnabled(false);
    chamferEnableLayout->addWidget(m_chamferingEnabledCheck);
    chamferEnableLayout->addStretch();
    chamferEnableLayout->addWidget(m_chamferingParamsButton);
    chamferLayout->addLayout(chamferEnableLayout);
    QLabel* chamferInfo = new QLabel("Select edges to chamfer in the 3D view.");
    chamferInfo->setWordWrap(true);
    chamferLayout->addWidget(chamferInfo);

    QGroupBox* chamferToolsGroup = new QGroupBox("Recommended Tools");
    QVBoxLayout* chamferToolsLayout = new QVBoxLayout(chamferToolsGroup);
    QListWidget* chamferToolsList = new QListWidget();
    chamferToolsLayout->addWidget(chamferToolsList);
    chamferLayout->addWidget(chamferToolsGroup);
    m_operationToolLists.insert("chamfering", chamferToolsList);

    chamferLayout->addStretch();

    // --- Parting Tab ---
    QVBoxLayout* partLayout = new QVBoxLayout(m_partingTab);
    partLayout->setContentsMargins(12, 12, 12, 12);
    partLayout->setSpacing(16);
    QHBoxLayout* partEnableLayout = new QHBoxLayout();
    m_partingEnabledCheck = new QCheckBox("Enable Parting");
    m_partingParamsButton = new QPushButton("Parameters...");
    m_partingParamsButton->setEnabled(false);
    partEnableLayout->addWidget(m_partingEnabledCheck);
    partEnableLayout->addStretch();
    partEnableLayout->addWidget(m_partingParamsButton);
    partLayout->addLayout(partEnableLayout);
    m_partingWidthLayout = new QHBoxLayout();
    m_partingWidthLabel = new QLabel("Parting Width:");
    m_partingWidthLabel->setMinimumWidth(140);
    m_partingWidthSpin = new QDoubleSpinBox();
    m_partingWidthSpin->setRange(1.0, 5.0);
    m_partingWidthSpin->setValue(3.0);
    m_partingWidthSpin->setSuffix(" mm");
    m_partingWidthSpin->setDecimals(1);
    m_partingWidthLayout->addWidget(m_partingWidthLabel);
    m_partingWidthLayout->addWidget(m_partingWidthSpin);
    m_partingWidthLayout->addStretch();
    partLayout->addLayout(m_partingWidthLayout);

    QGroupBox* partingToolsGroup = new QGroupBox("Recommended Tools");
    QVBoxLayout* partingToolsLayout = new QVBoxLayout(partingToolsGroup);
    QListWidget* partingToolsList = new QListWidget();
    partingToolsLayout->addWidget(partingToolsList);
    partLayout->addWidget(partingToolsGroup);
    m_operationToolLists.insert("parting", partingToolsList);

    partLayout->addStretch();
}

void SetupConfigurationPanel::setupConnections()
{
    // Part tab connections
    connect(m_browseButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onBrowseStepFile);
    connect(m_manualAxisButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onManualAxisSelectionClicked);
    connect(m_materialTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SetupConfigurationPanel::onMaterialChanged);
    
    // Material and tool management connections
    if (m_materialDetailsButton) {
        connect(m_materialDetailsButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onToolSelectionRequested);
    }
    connect(m_rawDiameterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    
    // Part positioning connections
    connect(m_distanceSlider, &QSlider::valueChanged, this, [this](int value) {
        m_distanceSpinBox->setValue(static_cast<double>(value));
    });
    connect(m_distanceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        m_distanceSlider->setValue(static_cast<int>(value));
        emit distanceToChuckChanged(value);
        emit configurationChanged();
    });
    connect(m_flipOrientationCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit orientationFlipped(checked);
        emit configurationChanged();
    });

    // Machining tab connections
    connect(m_facingAllowanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    connect(m_roughingAllowanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    connect(m_finishingAllowanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    connect(m_partingWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    
    connect(m_surfaceFinishCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SetupConfigurationPanel::onConfigurationChanged);
    connect(m_toleranceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SetupConfigurationPanel::onConfigurationChanged);

    // Operation controls - only connect if widgets exist
    if (m_contouringEnabledCheck) {
        connect(m_contouringEnabledCheck, &QCheckBox::toggled, this, &SetupConfigurationPanel::onOperationToggled);
    }
    if (m_threadingEnabledCheck) {
        connect(m_threadingEnabledCheck, &QCheckBox::toggled, this, &SetupConfigurationPanel::onOperationToggled);
    }
    if (m_chamferingEnabledCheck) {
        connect(m_chamferingEnabledCheck, &QCheckBox::toggled, this, &SetupConfigurationPanel::onOperationToggled);
    }
    if (m_partingEnabledCheck) {
        connect(m_partingEnabledCheck, &QCheckBox::toggled, this, &SetupConfigurationPanel::onOperationToggled);
    }

    if (m_contouringParamsButton) {
        connect(m_contouringParamsButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onOperationParametersClicked);
    }
    if (m_threadingParamsButton) {
        connect(m_threadingParamsButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onOperationParametersClicked);
    }
    if (m_chamferingParamsButton) {
        connect(m_chamferingParamsButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onOperationParametersClicked);
    }
    if (m_partingParamsButton) {
        connect(m_partingParamsButton, &QPushButton::clicked, this, &SetupConfigurationPanel::onOperationParametersClicked);
    }
}

void SetupConfigurationPanel::applyTabStyling()
{
    // Apply modern styling to tab widget
    m_operationsTabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #c0c0c0;
            background-color: #f0f0f0;
        }
        QTabBar::tab {
            background-color: #e0e0e0;
            border: 1px solid #c0c0c0;
            border-bottom-color: #c0c0c0;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            min-width: 80px;
            padding: 8px 12px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #f0f0f0;
            border-bottom-color: #f0f0f0;
        }
        QTabBar::tab:hover {
            background-color: #e8e8e8;
        }
    )");

    // Style the group boxes with modern colors
    QString groupBoxStyle = R"(
        QGroupBox {
            font-weight: bold;
            border: 2px solid %1;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            background-color: rgba(%2, 0.05);
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            color: %1;
            background-color: #f0f0f0;
            border-radius: 4px;
        }
    )";

    m_partSetupGroup->setStyleSheet(groupBoxStyle.arg("#2196F3", "33, 150, 243"));
    m_materialGroup->setStyleSheet(groupBoxStyle.arg("#4CAF50", "76, 175, 80"));
    m_machiningParamsGroup->setStyleSheet(groupBoxStyle.arg("#f44336", "244, 67, 54"));
    m_qualityGroup->setStyleSheet(groupBoxStyle.arg("#607D8B", "96, 125, 139"));
}

// Getter implementations
QString SetupConfigurationPanel::getStepFilePath() const
{
    return m_stepFileEdit->text();
}

MaterialType SetupConfigurationPanel::getMaterialType() const
{
    return static_cast<MaterialType>(m_materialTypeCombo->currentIndex());
}

double SetupConfigurationPanel::getRawDiameter() const
{
    return m_rawDiameterSpin->value();
}

double SetupConfigurationPanel::getDistanceToChuck() const
{
    return m_distanceSpinBox->value();
}

bool SetupConfigurationPanel::isOrientationFlipped() const
{
    return m_flipOrientationCheckBox->isChecked();
}

double SetupConfigurationPanel::getFacingAllowance() const
{
    return m_facingAllowanceSpin->value();
}

double SetupConfigurationPanel::getRoughingAllowance() const
{
    return m_roughingAllowanceSpin->value();
}

double SetupConfigurationPanel::getFinishingAllowance() const
{
    return m_finishingAllowanceSpin->value();
}

double SetupConfigurationPanel::getPartingWidth() const
{
    return m_partingWidthSpin->value();
}

SurfaceFinish SetupConfigurationPanel::getSurfaceFinish() const
{
    return static_cast<SurfaceFinish>(m_surfaceFinishCombo->currentIndex());
}

double SetupConfigurationPanel::getTolerance() const
{
    return m_toleranceSpin->value();
}

bool SetupConfigurationPanel::isOperationEnabled(const QString& operationName) const
{
    if (operationName == "Contouring") return m_contouringEnabledCheck ? m_contouringEnabledCheck->isChecked() : false;
    if (operationName == "Threading") return m_threadingEnabledCheck ? m_threadingEnabledCheck->isChecked() : false;
    if (operationName == "Chamfering") return m_chamferingEnabledCheck ? m_chamferingEnabledCheck->isChecked() : false;
    if (operationName == "Parting") return m_partingEnabledCheck ? m_partingEnabledCheck->isChecked() : false;
    return false;
}

OperationConfig SetupConfigurationPanel::getOperationConfig(const QString& operationName) const
{
    OperationConfig config;
    config.enabled = isOperationEnabled(operationName);
    config.name = operationName;
    config.description = QString("%1 operation").arg(operationName);
    return config;
}

// Setter implementations
void SetupConfigurationPanel::setStepFilePath(const QString& path)
{
    m_stepFileEdit->setText(path);
}

void SetupConfigurationPanel::setMaterialType(MaterialType type)
{
    m_materialTypeCombo->setCurrentIndex(static_cast<int>(type));
}

void SetupConfigurationPanel::setRawDiameter(double diameter)
{
    m_rawDiameterSpin->setValue(diameter);
}

void SetupConfigurationPanel::setDistanceToChuck(double distance)
{
    m_distanceSlider->setValue(static_cast<int>(distance));
    m_distanceSpinBox->setValue(distance);
}

void SetupConfigurationPanel::setOrientationFlipped(bool flipped)
{
    m_flipOrientationCheckBox->setChecked(flipped);
}

void SetupConfigurationPanel::updateRawMaterialLength(double length)
{
    m_rawLengthLabel->setText(QString("Raw Length: %1 mm (auto-calculated)").arg(length, 0, 'f', 1));
}

void SetupConfigurationPanel::setFacingAllowance(double allowance)
{
    m_facingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setRoughingAllowance(double allowance)
{
    m_roughingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setFinishingAllowance(double allowance)
{
    m_finishingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setPartingWidth(double width)
{
    m_partingWidthSpin->setValue(width);
}

void SetupConfigurationPanel::setSurfaceFinish(SurfaceFinish finish)
{
    m_surfaceFinishCombo->setCurrentIndex(static_cast<int>(finish));
}

void SetupConfigurationPanel::setTolerance(double tolerance)
{
    m_toleranceSpin->setValue(tolerance);
}

void SetupConfigurationPanel::setOperationEnabled(const QString& operationName, bool enabled)
{
    if (operationName == "Contouring") m_contouringEnabledCheck->setChecked(enabled);
    else if (operationName == "Threading") m_threadingEnabledCheck->setChecked(enabled);
    else if (operationName == "Chamfering") m_chamferingEnabledCheck->setChecked(enabled);
    else if (operationName == "Parting") m_partingEnabledCheck->setChecked(enabled);
}

void SetupConfigurationPanel::updateAxisInfo(const QString& info)
{
    m_axisInfoLabel->setText(info);
}

// Slot implementations
void SetupConfigurationPanel::onBrowseStepFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select STEP File"), QString(),
        tr("STEP Files (*.step *.stp);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        m_stepFileEdit->setText(fileName);
        emit stepFileSelected(fileName);
        emit configurationChanged();
    }
}

void SetupConfigurationPanel::onManualAxisSelectionClicked()
{
    emit manualAxisSelectionRequested();
    m_axisInfoLabel->setText("Selection mode enabled - click on a cylindrical surface or circular edge in the 3D view");
}

void SetupConfigurationPanel::onConfigurationChanged()
{
    emit configurationChanged();
    
    // Emit specific signals for certain changes
    QObject* sender = this->sender();
    if (sender == m_materialTypeCombo) {
        emit materialTypeChanged(getMaterialType());
    }
    if (sender == m_rawDiameterSpin) {
        emit rawMaterialDiameterChanged(getRawDiameter());
    }
}

void SetupConfigurationPanel::onOperationToggled()
{
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
    if (checkBox) {
        QString operationName = checkBox->text();
        bool enabled = checkBox->isChecked();
        emit operationToggled(operationName, enabled);
        emit configurationChanged();
        if (enabled) {
            emit automaticToolpathGenerationRequested();
        }
    }
}

void SetupConfigurationPanel::onOperationParametersClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString operationName;
        if (button == m_contouringParamsButton) operationName = "Contouring";
        else if (button == m_threadingParamsButton) operationName = "Threading";
        else if (button == m_chamferingParamsButton) operationName = "Chamfering";
        else if (button == m_partingParamsButton) operationName = "Parting";
        
        if (!operationName.isEmpty()) {
            emit operationParametersRequested(operationName);
        }
    }
}


void SetupConfigurationPanel::setMaterialManager(MaterialManager* materialManager)
{
    m_materialManager = materialManager;
    if (m_materialManager) {
        // Update material combo box with materials from database
        m_materialTypeCombo->clear();
        QStringList materialNames = m_materialManager->getAllMaterialNames();
        for (const QString& name : materialNames) {
            MaterialProperties props = m_materialManager->getMaterialProperties(name);
            m_materialTypeCombo->addItem(props.displayName, name);
        }
        
        // Connect to material database changes
        connect(m_materialManager, &MaterialManager::materialAdded,
                this, &SetupConfigurationPanel::updateMaterialProperties);
        connect(m_materialManager, &MaterialManager::materialUpdated,
                this, &SetupConfigurationPanel::updateMaterialProperties);
        
        updateMaterialProperties();
    }
}

void SetupConfigurationPanel::setToolManager(ToolManager* toolManager)
{
    m_toolManager = toolManager;
    if (m_toolManager) {
        // Connect to tool database changes
        connect(m_toolManager, &ToolManager::toolAdded,
                this, &SetupConfigurationPanel::updateToolRecommendations);
        connect(m_toolManager, &ToolManager::toolUpdated,
                this, &SetupConfigurationPanel::updateToolRecommendations);
        
        updateToolRecommendations();
    }
}

QString SetupConfigurationPanel::getSelectedMaterialName() const
{
    if (m_materialTypeCombo->currentIndex() >= 0) {
        return m_materialTypeCombo->currentData().toString();
    }
    return QString();
}

QStringList SetupConfigurationPanel::getRecommendedTools() const
{
    QStringList toolIds;
    for (auto list : m_operationToolLists) {
        if (!list) continue;
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem* item = list->item(i);
            if (item && item->data(Qt::UserRole).isValid()) {
                toolIds.append(item->data(Qt::UserRole).toString());
            }
        }
    }
    return toolIds;
}

void SetupConfigurationPanel::updateMaterialProperties()
{
    if (!m_materialManager || !m_materialPropertiesLabel) {
        return;
    }
    
    QString materialName = getSelectedMaterialName();
    if (materialName.isEmpty()) {
        m_materialPropertiesLabel->setText("No material selected");
        return;
    }
    
    MaterialProperties props = m_materialManager->getMaterialProperties(materialName);
    if (props.name.isEmpty()) {
        m_materialPropertiesLabel->setText("Material properties not available");
        return;
    }
    
    QString propertiesText = QString(
        "Density: %1 kg/m³\n"
        "Machinability: %2/10\n"
        "Recommended Speed: %3 m/min\n"
        "Recommended Feed: %4 mm/rev"
    ).arg(props.density, 0, 'f', 0)
     .arg(props.machinabilityRating * 10, 0, 'f', 1)
     .arg(props.recommendedSurfaceSpeed, 0, 'f', 0)
     .arg(props.recommendedFeedRate, 0, 'f', 2);
    
    m_materialPropertiesLabel->setText(propertiesText);
    
    // Update tool recommendations when material changes
    updateToolRecommendations();
    
    emit materialSelectionChanged(materialName);
}

void SetupConfigurationPanel::updateToolRecommendations()
{
    if (!m_toolManager) {
        return;
    }

    for (auto list : m_operationToolLists) {
        if (list) list->clear();
    }
    
    QString materialName = getSelectedMaterialName();
    if (materialName.isEmpty()) {
        return;
    }
    
    double workpieceDiameter = getRawDiameter();
    double surfaceFinish = 16.0; // Default medium finish
    
    // Get surface finish target from UI
    SurfaceFinish finish = getSurfaceFinish();
    switch (finish) {
        case SurfaceFinish::Mirror_1Ra: surfaceFinish = 1.0; break;
        case SurfaceFinish::Polish_2Ra: surfaceFinish = 2.0; break;
        case SurfaceFinish::Smooth_4Ra: surfaceFinish = 4.0; break;
        case SurfaceFinish::Fine_8Ra: surfaceFinish = 8.0; break;
        case SurfaceFinish::Medium_16Ra: surfaceFinish = 16.0; break;
        case SurfaceFinish::Rough_32Ra: surfaceFinish = 32.0; break;
    }
    
    // Get tool recommendations for each enabled operation
    QStringList operations;
    if (isOperationEnabled("Contouring")) operations.append("contouring");
    if (isOperationEnabled("Threading")) operations.append("threading");
    if (isOperationEnabled("Chamfering")) operations.append("chamfering");
    if (isOperationEnabled("Parting")) operations.append("parting");
    
    QSet<QString> recommendedToolIds;
    
    for (const QString& operation : operations) {
        auto recommendations = m_toolManager->recommendTools(
            operation, materialName, workpieceDiameter, surfaceFinish);
        
        // Add top 2 recommendations for each operation
        int count = 0;
        for (const auto& rec : recommendations) {
            if (count >= 2) break;
            if (!recommendedToolIds.contains(rec.toolId)) {
                recommendedToolIds.insert(rec.toolId);
                
                CuttingTool tool = m_toolManager->getTool(rec.toolId);
                QString itemText = QString("%1 - %2 (%3)")
                    .arg(tool.name)
                    .arg(operation)
                    .arg(rec.suitabilityScore, 0, 'f', 2);
                
                QListWidgetItem* item = new QListWidgetItem(itemText);
                item->setData(Qt::UserRole, QVariant(rec.toolId));
                item->setToolTip(QString("Tool: %1\nOperation: %2\nSuitability: %3\nReason: %4")
                    .arg(tool.name).arg(operation).arg(rec.suitabilityScore, 0, 'f', 2).arg(rec.reason));

                QListWidget* targetList = m_operationToolLists.value(operation);
                if (targetList) {
                    targetList->addItem(item);
                }
                count++;
            }
        }
    }
    
    emit toolRecommendationsUpdated(QStringList(recommendedToolIds.begin(), recommendedToolIds.end()));
}

void SetupConfigurationPanel::onMaterialChanged()
{
    updateMaterialProperties();
    emit configurationChanged();
}

void SetupConfigurationPanel::onToolSelectionRequested()
{
    // This could open a tool selection dialog or provide more detailed tool information
    QStringList selectedTools = getRecommendedTools();
    if (!selectedTools.isEmpty()) {
        qDebug() << "Selected tools:" << selectedTools;
        // Here we could emit a signal to show detailed tool information
    }
}

void SetupConfigurationPanel::updateOperationControls()
{
    // Enable/disable parameter buttons based on operation checkboxes
    m_contouringParamsButton->setEnabled(m_contouringEnabledCheck->isChecked());
    m_threadingParamsButton->setEnabled(m_threadingEnabledCheck->isChecked());
    m_chamferingParamsButton->setEnabled(m_chamferingEnabledCheck->isChecked());
    m_partingParamsButton->setEnabled(m_partingEnabledCheck->isChecked());
}

// Utility method implementations
QString SetupConfigurationPanel::materialTypeToString(MaterialType type)
{
    if (static_cast<int>(type) < MATERIAL_NAMES.size()) {
        return MATERIAL_NAMES[static_cast<int>(type)];
    }
    return "Unknown";
}

MaterialType SetupConfigurationPanel::stringToMaterialType(const QString& typeStr)
{
    int index = MATERIAL_NAMES.indexOf(typeStr);
    return (index >= 0) ? static_cast<MaterialType>(index) : MaterialType::Custom;
}

QString SetupConfigurationPanel::surfaceFinishToString(SurfaceFinish finish)
{
    if (static_cast<int>(finish) < SURFACE_FINISH_NAMES.size()) {
        return SURFACE_FINISH_NAMES[static_cast<int>(finish)];
    }
    return "Unknown";
}

SurfaceFinish SetupConfigurationPanel::stringToSurfaceFinish(const QString& finishStr)
{
    int index = SURFACE_FINISH_NAMES.indexOf(finishStr);
    return (index >= 0) ? static_cast<SurfaceFinish>(index) : SurfaceFinish::Medium_16Ra;
}

} // namespace GUI
} // namespace IntuiCAM
