#include "setupconfigurationpanel.h"
#include "materialmanager.h"
#include "toolmanager.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace IntuiCAM {
namespace GUI {

using namespace IntuiCAM::GUI;

// Define the static const members
const QStringList MATERIAL_NAMES = {"Aluminum 6061-T6",
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
                                    "Custom Material"};

const QStringList SURFACE_FINISH_NAMES = {
    "Rough (32 μm Ra)", "Medium (16 μm Ra)",  "Fine (8 μm Ra)",
    "Smooth (4 μm Ra)", "Polished (2 μm Ra)", "Mirror (1 μm Ra)"};

const QStringList OPERATION_ORDER = {"Contouring", "Threading", "Chamfering",
                                     "Parting"};

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
    
    , m_contouringEnabledCheck(nullptr)
    , m_threadingEnabledCheck(nullptr)
    , m_chamferingEnabledCheck(nullptr)
    , m_partingEnabledCheck(nullptr)
{
    setupUI();
    setupConnections();
    applyTabStyling();

    // Ensure widgets respect the initial Advanced Mode state
    updateAdvancedMode();
}

SetupConfigurationPanel::~SetupConfigurationPanel() {
  // Qt handles cleanup automatically
}

void SetupConfigurationPanel::setupUI() {
  m_mainLayout = new QVBoxLayout(this);
  m_mainLayout->setSpacing(8);
  m_mainLayout->setContentsMargins(8, 8, 8, 8);

  // Create top part setup section
  m_partTab = new QWidget();
  setupPartTab();
  m_mainLayout->addWidget(m_partTab);

  // Advanced mode toggle
  QHBoxLayout *advLayout = new QHBoxLayout();
  m_advancedModeCheck = new QCheckBox("Advanced Mode");
  advLayout->addWidget(m_advancedModeCheck);
  advLayout->addStretch();
  m_mainLayout->addLayout(advLayout);
  connect(m_advancedModeCheck, &QCheckBox::toggled, this,
          &SetupConfigurationPanel::updateAdvancedMode);

  // Create operations tab widget
  m_operationsTabWidget = new QTabWidget();
  m_operationsTabWidget->setTabPosition(QTabWidget::North);
  m_operationsTabWidget->setDocumentMode(true);

  // Create operation tabs (content widgets)
  m_contouringTab = new QWidget();
  m_threadingTab = new QWidget();
  m_chamferingTab = new QWidget();
  m_partingTab = new QWidget();

  // Wrap each tab in a scroll area to allow scrolling when content exceeds
  // the available space. The scroll areas themselves are local to this
  // method, but the inner tab widgets are stored in member variables for
  // later access.
  QScrollArea *contourScroll = new QScrollArea();
  contourScroll->setWidgetResizable(true);
  contourScroll->setFrameShape(QFrame::NoFrame);
  contourScroll->setWidget(m_contouringTab);

  QScrollArea *threadingScroll = new QScrollArea();
  threadingScroll->setWidgetResizable(true);
  threadingScroll->setFrameShape(QFrame::NoFrame);
  threadingScroll->setWidget(m_threadingTab);

  QScrollArea *chamferScroll = new QScrollArea();
  chamferScroll->setWidgetResizable(true);
  chamferScroll->setFrameShape(QFrame::NoFrame);
  chamferScroll->setWidget(m_chamferingTab);

  QScrollArea *partingScroll = new QScrollArea();
  partingScroll->setWidgetResizable(true);
  partingScroll->setFrameShape(QFrame::NoFrame);
  partingScroll->setWidget(m_partingTab);

  // Setup individual operation tabs
  setupMachiningTab();

  // Add tabs to widget using the scroll areas created above
  m_operationsTabWidget->addTab(contourScroll, "Contouring");
  m_operationsTabWidget->addTab(threadingScroll, "Threading");
  m_operationsTabWidget->addTab(chamferScroll, "Chamfering");
  m_operationsTabWidget->addTab(partingScroll, "Parting");

  // Add operations tab widget to main layout
  m_mainLayout->addWidget(m_operationsTabWidget);
}

void SetupConfigurationPanel::setupPartTab() {
  QVBoxLayout *partTabLayout = new QVBoxLayout(m_partTab);
  partTabLayout->setContentsMargins(12, 12, 12, 12);
  partTabLayout->setSpacing(16);

  // Part Setup Group
  m_partSetupGroup = new QGroupBox("Part Setup");
  m_partSetupLayout = new QVBoxLayout(m_partSetupGroup);
  m_partSetupLayout->setSpacing(8);

  // STEP file selection
  m_stepFileLayout = new QHBoxLayout();
  QLabel *stepFileLabel = new QLabel("STEP File:");
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
  m_manualAxisButton->setStyleSheet("QPushButton {"
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
                                    "}");

  m_axisInfoLabel =
      new QLabel("Click the button above, then select a cylindrical surface or "
                 "circular edge in the 3D view");
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
  m_autoRawDiameterButton = new QPushButton("Auto");
  m_autoRawDiameterButton->setMaximumWidth(60);

  m_rawDiameterLayout->addWidget(m_rawDiameterLabel);
  m_rawDiameterLayout->addWidget(m_rawDiameterSpin);
  m_rawDiameterLayout->addWidget(m_autoRawDiameterButton);
  m_rawDiameterLayout->addStretch();
  m_materialLayout->addLayout(m_rawDiameterLayout);

  // Raw material length display
  m_rawLengthLabel = new QLabel("Raw material length required: 0.0 mm");
  m_rawLengthLabel->setStyleSheet("color: #666; font-size: 11px;");
  m_materialLayout->addWidget(m_rawLengthLabel);



  partTabLayout->addWidget(m_materialGroup);

  partTabLayout->addStretch();
}

void SetupConfigurationPanel::setupMachiningTab() {
  // --- Contouring Tab ---
  QVBoxLayout *contourLayout = new QVBoxLayout(m_contouringTab);
  contourLayout->setContentsMargins(12, 12, 12, 12);
  contourLayout->setSpacing(16);

  QHBoxLayout *contourEnableLayout = new QHBoxLayout();
  m_contouringEnabledCheck = new QCheckBox("Enable Contouring");
  contourEnableLayout->addWidget(m_contouringEnabledCheck);
  contourEnableLayout->addStretch();
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

  // Flood coolant simple toggle
  QHBoxLayout *coolLayout = new QHBoxLayout();
  m_contourFloodCheck = new QCheckBox("Flood Coolant");
  m_contourFloodCheck->setChecked(true);
  coolLayout->addWidget(m_contourFloodCheck);
  coolLayout->addStretch();
  m_machiningParamsLayout->addLayout(coolLayout);

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
  QStringList SURFACE_FINISH_NAMES = {"Rough (32 μm Ra)",   "Medium (16 μm Ra)",
                                      "Fine (8 μm Ra)",     "Smooth (4 μm Ra)",
                                      "Polished (2 μm Ra)", "Mirror (1 μm Ra)"};

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

  QGroupBox *contourToolsGroup = new QGroupBox("Recommended Tools");
  QVBoxLayout *contourToolsLayout = new QVBoxLayout(contourToolsGroup);
  QListWidget *contourToolsList = new QListWidget();
  contourToolsLayout->addWidget(contourToolsList);
  contourLayout->addWidget(contourToolsGroup);
  m_operationToolLists.insert("contouring", contourToolsList);

  // Advanced cutting parameters
  m_contourAdvancedGroup = new QGroupBox("Advanced Cutting");
  QVBoxLayout *contourAdvLayout = new QVBoxLayout(m_contourAdvancedGroup);

  auto createSection = [this](const QString &title, QGroupBox **group,
                              QDoubleSpinBox **depth, QDoubleSpinBox **feed,
                              QDoubleSpinBox **speed, QCheckBox **css) {
    *group = new QGroupBox(title);
    QFormLayout *form = new QFormLayout(*group);
    *depth = new QDoubleSpinBox();
    (*depth)->setRange(0.01, 10.0);
    (*depth)->setSuffix(" mm");
    *feed = new QDoubleSpinBox();
    (*feed)->setRange(0.1, 1000.0);
    (*feed)->setSuffix(" mm/rev");
    *speed = new QDoubleSpinBox();
    (*speed)->setRange(10.0, 10000.0);
    (*speed)->setSuffix(" RPM");
    *css = new QCheckBox("Constant Surface Speed");
    (*css)->setChecked(true);
    form->addRow("Cut Depth:", *depth);
    form->addRow("Feed Rate:", *feed);
    form->addRow("Spindle Speed:", *speed);
    form->addRow(QString(), *css);
    return form;
  };

  createSection("Facing", &m_contourFacingGroup, &m_contourFacingDepthSpin,
                &m_contourFacingFeedSpin, &m_contourFacingSpeedSpin,
                &m_contourFacingCssCheck);
  createSection("Roughing", &m_contourRoughGroup, &m_contourRoughDepthSpin,
                &m_contourRoughFeedSpin, &m_contourRoughSpeedSpin,
                &m_contourRoughCssCheck);
  createSection("Finishing", &m_contourFinishGroup, &m_contourFinishDepthSpin,
                &m_contourFinishFeedSpin, &m_contourFinishSpeedSpin,
                &m_contourFinishCssCheck);

  contourAdvLayout->addWidget(m_contourFacingGroup);
  contourAdvLayout->addWidget(m_contourRoughGroup);
  contourAdvLayout->addWidget(m_contourFinishGroup);
  contourLayout->addWidget(m_contourAdvancedGroup);

  contourLayout->addStretch();

  // --- Threading Tab ---
  QVBoxLayout *threadingLayout = new QVBoxLayout(m_threadingTab);
  threadingLayout->setContentsMargins(12, 12, 12, 12);
  threadingLayout->setSpacing(16);
  QHBoxLayout *threadEnableLayout = new QHBoxLayout();
  m_threadingEnabledCheck = new QCheckBox("Enable Threading");
  threadEnableLayout->addWidget(m_threadingEnabledCheck);
  threadEnableLayout->addStretch();
  threadingLayout->addLayout(threadEnableLayout);
  QHBoxLayout *threadCoolLayout = new QHBoxLayout();
  m_threadFloodCheck = new QCheckBox("Flood Coolant");
  m_threadFloodCheck->setChecked(true);
  threadCoolLayout->addWidget(m_threadFloodCheck);
  threadCoolLayout->addStretch();
  threadingLayout->addLayout(threadCoolLayout);

  QLabel *threadingInfo = new QLabel("Select faces to thread in the 3D view.");
  threadingInfo->setWordWrap(true);
  threadingLayout->addWidget(threadingInfo);

  m_threadFacesTable = new QTableWidget(0, 3);
  QStringList threadHeaders{"Preset", "Pitch", "Depth"};
  m_threadFacesTable->setHorizontalHeaderLabels(threadHeaders);
  m_threadFacesTable->horizontalHeader()->setStretchLastSection(true);
  threadingLayout->addWidget(m_threadFacesTable);

  QHBoxLayout *threadFaceBtnLayout = new QHBoxLayout();
  m_addThreadFaceButton = new QPushButton("Add Face");
  m_removeThreadFaceButton = new QPushButton("Remove Face");
  threadFaceBtnLayout->addWidget(m_addThreadFaceButton);
  threadFaceBtnLayout->addWidget(m_removeThreadFaceButton);
  threadFaceBtnLayout->addStretch();
  threadingLayout->addLayout(threadFaceBtnLayout);

  QGroupBox *threadingToolsGroup = new QGroupBox("Recommended Tools");
  QVBoxLayout *threadingToolsLayout = new QVBoxLayout(threadingToolsGroup);
  QListWidget *threadingToolsList = new QListWidget();
  threadingToolsLayout->addWidget(threadingToolsList);
  threadingLayout->addWidget(threadingToolsGroup);
  m_operationToolLists.insert("threading", threadingToolsList);

  threadingLayout->addStretch();

  // --- Chamfering Tab ---
  QVBoxLayout *chamferLayout = new QVBoxLayout(m_chamferingTab);
  chamferLayout->setContentsMargins(12, 12, 12, 12);
  chamferLayout->setSpacing(16);
  QHBoxLayout *chamferEnableLayout = new QHBoxLayout();
  m_chamferingEnabledCheck = new QCheckBox("Enable Chamfering");
  chamferEnableLayout->addWidget(m_chamferingEnabledCheck);
  chamferEnableLayout->addStretch();
  chamferLayout->addLayout(chamferEnableLayout);
  QLabel *chamferInfo = new QLabel("Select edges to chamfer in the 3D view.");
  chamferInfo->setWordWrap(true);
  chamferLayout->addWidget(chamferInfo);

  QHBoxLayout *chamferSizeLayout = new QHBoxLayout();
  QLabel *chamferSizeLabel = new QLabel("Chamfer Size:");
  chamferSizeLabel->setMinimumWidth(140);
  m_chamferSizeSpin = new QDoubleSpinBox();
  m_chamferSizeSpin->setRange(0.1, 5.0);
  m_chamferSizeSpin->setValue(0.5);
  m_chamferSizeSpin->setDecimals(2);
  m_chamferSizeSpin->setSuffix(" mm");
  chamferSizeLayout->addWidget(chamferSizeLabel);
  chamferSizeLayout->addWidget(m_chamferSizeSpin);
  chamferSizeLayout->addStretch();
  chamferLayout->addLayout(chamferSizeLayout);

  QHBoxLayout *chamferCoolLayout = new QHBoxLayout();
  m_chamferFloodCheck = new QCheckBox("Flood Coolant");
  m_chamferFloodCheck->setChecked(true);
  chamferCoolLayout->addWidget(m_chamferFloodCheck);
  chamferCoolLayout->addStretch();
  chamferLayout->addLayout(chamferCoolLayout);

  m_chamferFacesTable = new QTableWidget(0, 4);
  QStringList chamferHeaders{"Face", "Chamfer Type", "Value A", "Value B/Angle"};
  m_chamferFacesTable->setHorizontalHeaderLabels(chamferHeaders);
  m_chamferFacesTable->horizontalHeader()->setStretchLastSection(true);
  chamferLayout->addWidget(m_chamferFacesTable);

  QHBoxLayout *chamferBtnLayout = new QHBoxLayout();
  m_addChamferFaceButton = new QPushButton("Add Face");
  m_removeChamferFaceButton = new QPushButton("Remove Face");
  chamferBtnLayout->addWidget(m_addChamferFaceButton);
  chamferBtnLayout->addWidget(m_removeChamferFaceButton);
  chamferBtnLayout->addStretch();
  chamferLayout->addLayout(chamferBtnLayout);

  QHBoxLayout *extraStockLayout = new QHBoxLayout();
  QLabel *extraStockLabel = new QLabel("Extra Cleanup Stock:");
  extraStockLabel->setMinimumWidth(140);
  m_extraChamferStockSpin = new QDoubleSpinBox();
  m_extraChamferStockSpin->setRange(0.0, 2.0);
  m_extraChamferStockSpin->setValue(0.0);
  m_extraChamferStockSpin->setSuffix(" mm");
  extraStockLayout->addWidget(extraStockLabel);
  extraStockLayout->addWidget(m_extraChamferStockSpin);
  extraStockLayout->addStretch();
  chamferLayout->addLayout(extraStockLayout);

  QHBoxLayout *diamLeaveLayout = new QHBoxLayout();
  QLabel *diamLeaveLabel = new QLabel("Diameter Leave:");
  diamLeaveLabel->setMinimumWidth(140);
  m_chamferDiameterLeaveSpin = new QDoubleSpinBox();
  m_chamferDiameterLeaveSpin->setRange(0.0, 5.0);
  m_chamferDiameterLeaveSpin->setValue(0.0);
  m_chamferDiameterLeaveSpin->setSuffix(" mm");
  diamLeaveLayout->addWidget(diamLeaveLabel);
  diamLeaveLayout->addWidget(m_chamferDiameterLeaveSpin);
  diamLeaveLayout->addStretch();
  chamferLayout->addLayout(diamLeaveLayout);

  QGroupBox *chamferToolsGroup = new QGroupBox("Recommended Tools");
  QVBoxLayout *chamferToolsLayout = new QVBoxLayout(chamferToolsGroup);
  QListWidget *chamferToolsList = new QListWidget();
  chamferToolsLayout->addWidget(chamferToolsList);
  chamferLayout->addWidget(chamferToolsGroup);
  m_operationToolLists.insert("chamfering", chamferToolsList);

  chamferLayout->addStretch();

  // --- Parting Tab ---
  QVBoxLayout *partLayout = new QVBoxLayout(m_partingTab);
  partLayout->setContentsMargins(12, 12, 12, 12);
  partLayout->setSpacing(16);
  QHBoxLayout *partEnableLayout = new QHBoxLayout();
  m_partingEnabledCheck = new QCheckBox("Enable Parting");
  partEnableLayout->addWidget(m_partingEnabledCheck);
  partEnableLayout->addStretch();
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

  QHBoxLayout *partCoolLayout = new QHBoxLayout();
  m_partFloodCheck = new QCheckBox("Flood Coolant");
  m_partFloodCheck->setChecked(true);
  partCoolLayout->addWidget(m_partFloodCheck);
  partCoolLayout->addStretch();
  partLayout->addLayout(partCoolLayout);

  QGroupBox *partingToolsGroup = new QGroupBox("Recommended Tools");
  QVBoxLayout *partingToolsLayout = new QVBoxLayout(partingToolsGroup);
  QListWidget *partingToolsList = new QListWidget();
  partingToolsLayout->addWidget(partingToolsList);
  partLayout->addWidget(partingToolsGroup);
  m_operationToolLists.insert("parting", partingToolsList);

  // Parting advanced parameters
  m_partingAdvancedGroup = new QGroupBox("Advanced Cutting");
  QFormLayout *partAdvLayout = new QFormLayout(m_partingAdvancedGroup);
  m_partingDepthSpin = new QDoubleSpinBox();
  m_partingDepthSpin->setRange(0.01, 5.0);
  m_partingDepthSpin->setSuffix(" mm");
  m_partingFeedSpin = new QDoubleSpinBox();
  m_partingFeedSpin->setRange(0.1, 500.0);
  m_partingFeedSpin->setSuffix(" mm/rev");
  m_partingSpeedSpin = new QDoubleSpinBox();
  m_partingSpeedSpin->setRange(10.0, 10000.0);
  m_partingSpeedSpin->setSuffix(" RPM");
  m_partingCssCheck = new QCheckBox("Constant Surface Speed");
  m_partingCssCheck->setChecked(true);
  m_partingRetractCombo = new QComboBox();
  m_partingRetractCombo->addItems({"Direct", "Diagonal", "Custom"});
  partAdvLayout->addRow("Cut Depth:", m_partingDepthSpin);
  partAdvLayout->addRow("Feed Rate:", m_partingFeedSpin);
  partAdvLayout->addRow("Spindle Speed:", m_partingSpeedSpin);
  partAdvLayout->addRow("Retract Mode:", m_partingRetractCombo);
  partAdvLayout->addRow(QString(), m_partingCssCheck);
  partLayout->addWidget(m_partingAdvancedGroup);

  partLayout->addStretch();
}

void SetupConfigurationPanel::setupConnections() {
  // Part tab connections
  connect(m_browseButton, &QPushButton::clicked, this,
          &SetupConfigurationPanel::onBrowseStepFile);
  connect(m_manualAxisButton, &QPushButton::clicked, this,
          &SetupConfigurationPanel::onManualAxisSelectionClicked);
  connect(m_materialTypeCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SetupConfigurationPanel::onMaterialChanged);

  // Material and tool management connections
  connect(m_rawDiameterSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_autoRawDiameterButton, &QPushButton::clicked, this,
          &SetupConfigurationPanel::onAutoRawDiameterClicked);

  // Part positioning connections
  connect(m_distanceSlider, &QSlider::valueChanged, this, [this](int value) {
    m_distanceSpinBox->setValue(static_cast<double>(value));
  });
  connect(m_distanceSpinBox,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          [this](double value) {
            m_distanceSlider->setValue(static_cast<int>(value));
            emit distanceToChuckChanged(value);
            emit configurationChanged();
          });
  connect(m_flipOrientationCheckBox, &QCheckBox::toggled, this,
          [this](bool checked) {
            emit orientationFlipped(checked);
            emit configurationChanged();
          });

  // Machining tab connections
  connect(m_facingAllowanceSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_roughingAllowanceSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_finishingAllowanceSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_partingWidthSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_chamferSizeSpin,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);

  connect(m_surfaceFinishCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SetupConfigurationPanel::onConfigurationChanged);
  connect(m_toleranceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &SetupConfigurationPanel::onConfigurationChanged);

  // Operation controls - only connect if widgets exist
  if (m_contouringEnabledCheck) {
    connect(m_contouringEnabledCheck, &QCheckBox::toggled, this,
            &SetupConfigurationPanel::onOperationToggled);
  }
  if (m_threadingEnabledCheck) {
    connect(m_threadingEnabledCheck, &QCheckBox::toggled, this,
            &SetupConfigurationPanel::onOperationToggled);
    connect(m_addThreadFaceButton, &QPushButton::clicked, this,
            &SetupConfigurationPanel::onAddThreadFace);
    connect(m_removeThreadFaceButton, &QPushButton::clicked, this,
            &SetupConfigurationPanel::onRemoveThreadFace);
    connect(m_threadFacesTable, &QTableWidget::itemSelectionChanged, this,
            &SetupConfigurationPanel::onThreadFaceRowSelected);
    connect(m_threadFacesTable, &QTableWidget::cellChanged, this,
            &SetupConfigurationPanel::onThreadFaceCellChanged);
  }
  if (m_chamferingEnabledCheck) {
    connect(m_chamferingEnabledCheck, &QCheckBox::toggled, this,
            &SetupConfigurationPanel::onOperationToggled);
    connect(m_addChamferFaceButton, &QPushButton::clicked, this,
            &SetupConfigurationPanel::onAddChamferFace);
    connect(m_removeChamferFaceButton, &QPushButton::clicked, this,
            &SetupConfigurationPanel::onRemoveChamferFace);
    connect(m_chamferFacesTable, &QTableWidget::itemSelectionChanged, this,
            &SetupConfigurationPanel::onChamferFaceRowSelected);
  }
  if (m_partingEnabledCheck) {
    connect(m_partingEnabledCheck, &QCheckBox::toggled, this,
            &SetupConfigurationPanel::onOperationToggled);
  }
}

void SetupConfigurationPanel::applyTabStyling() {
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
  m_machiningParamsGroup->setStyleSheet(
      groupBoxStyle.arg("#f44336", "244, 67, 54"));
  m_qualityGroup->setStyleSheet(groupBoxStyle.arg("#607D8B", "96, 125, 139"));
}

// Getter implementations
QString SetupConfigurationPanel::getStepFilePath() const {
  return m_stepFileEdit->text();
}

MaterialType SetupConfigurationPanel::getMaterialType() const {
  return static_cast<MaterialType>(m_materialTypeCombo->currentIndex());
}

double SetupConfigurationPanel::getRawDiameter() const {
  return m_rawDiameterSpin->value();
}

double SetupConfigurationPanel::getDistanceToChuck() const {
  return m_distanceSpinBox->value();
}

bool SetupConfigurationPanel::isOrientationFlipped() const {
  return m_flipOrientationCheckBox->isChecked();
}

double SetupConfigurationPanel::getFacingAllowance() const {
  return m_facingAllowanceSpin->value();
}

double SetupConfigurationPanel::getRoughingAllowance() const {
  return m_roughingAllowanceSpin->value();
}

double SetupConfigurationPanel::getFinishingAllowance() const {
  return m_finishingAllowanceSpin->value();
}

double SetupConfigurationPanel::getPartingWidth() const {
  return m_partingWidthSpin->value();
}

SurfaceFinish SetupConfigurationPanel::getSurfaceFinish() const {
  return static_cast<SurfaceFinish>(m_surfaceFinishCombo->currentIndex());
}

double SetupConfigurationPanel::getTolerance() const {
  return m_toleranceSpin->value();
}

bool SetupConfigurationPanel::isOperationEnabled(
    const QString &operationName) const {
  if (operationName == "Contouring")
    return m_contouringEnabledCheck ? m_contouringEnabledCheck->isChecked()
                                    : false;
  if (operationName == "Threading")
    return m_threadingEnabledCheck ? m_threadingEnabledCheck->isChecked()
                                   : false;
  if (operationName == "Chamfering")
    return m_chamferingEnabledCheck ? m_chamferingEnabledCheck->isChecked()
                                    : false;
  if (operationName == "Parting")
    return m_partingEnabledCheck ? m_partingEnabledCheck->isChecked() : false;
  return false;
}

OperationConfig SetupConfigurationPanel::getOperationConfig(
    const QString &operationName) const {
  OperationConfig config;
  config.enabled = isOperationEnabled(operationName);
  config.name = operationName;
  config.description = QString("%1 operation").arg(operationName);
  return config;
}

// Setter implementations
void SetupConfigurationPanel::setStepFilePath(const QString &path) {
  m_stepFileEdit->setText(path);
}

void SetupConfigurationPanel::setMaterialType(MaterialType type) {
  m_materialTypeCombo->setCurrentIndex(static_cast<int>(type));
}

void SetupConfigurationPanel::setRawDiameter(double diameter) {
  m_rawDiameterSpin->setValue(diameter);
}

void SetupConfigurationPanel::setDistanceToChuck(double distance) {
  m_distanceSlider->setValue(static_cast<int>(distance));
  m_distanceSpinBox->setValue(distance);
}

void SetupConfigurationPanel::setOrientationFlipped(bool flipped) {
  m_flipOrientationCheckBox->setChecked(flipped);
}

void SetupConfigurationPanel::updateRawMaterialLength(double length) {
  m_rawLengthLabel->setText(
      QString("Raw material length required: %1 mm").arg(length, 0, 'f', 1));
}

void SetupConfigurationPanel::setFacingAllowance(double allowance) {
  m_facingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setRoughingAllowance(double allowance) {
  m_roughingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setFinishingAllowance(double allowance) {
  m_finishingAllowanceSpin->setValue(allowance);
}

void SetupConfigurationPanel::setPartingWidth(double width) {
  m_partingWidthSpin->setValue(width);
}

void SetupConfigurationPanel::setSurfaceFinish(SurfaceFinish finish) {
  m_surfaceFinishCombo->setCurrentIndex(static_cast<int>(finish));
}

void SetupConfigurationPanel::setTolerance(double tolerance) {
  m_toleranceSpin->setValue(tolerance);
}

void SetupConfigurationPanel::setOperationEnabled(const QString &operationName,
                                                  bool enabled) {
  if (operationName == "Contouring" && m_contouringEnabledCheck) {
    QSignalBlocker blocker(m_contouringEnabledCheck);
    m_contouringEnabledCheck->setChecked(enabled);
  } else if (operationName == "Threading" && m_threadingEnabledCheck) {
    QSignalBlocker blocker(m_threadingEnabledCheck);
    m_threadingEnabledCheck->setChecked(enabled);
  } else if (operationName == "Chamfering" && m_chamferingEnabledCheck) {
    QSignalBlocker blocker(m_chamferingEnabledCheck);
    m_chamferingEnabledCheck->setChecked(enabled);
  } else if (operationName == "Parting" && m_partingEnabledCheck) {
    QSignalBlocker blocker(m_partingEnabledCheck);
    m_partingEnabledCheck->setChecked(enabled);
  }

  updateOperationControls();
}

void SetupConfigurationPanel::updateAxisInfo(const QString &info) {
  m_axisInfoLabel->setText(info);
}

// Slot implementations
void SetupConfigurationPanel::onBrowseStepFile() {
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Select STEP File"), QString(),
      tr("STEP Files (*.step *.stp);;All Files (*)"));

  if (!fileName.isEmpty()) {
    m_stepFileEdit->setText(fileName);
    emit stepFileSelected(fileName);
    emit configurationChanged();
  }
}

void SetupConfigurationPanel::onManualAxisSelectionClicked() {
  emit manualAxisSelectionRequested();
  m_axisInfoLabel->setText("Selection mode enabled - click on a cylindrical "
                           "surface or circular edge in the 3D view");
}

void SetupConfigurationPanel::onAutoRawDiameterClicked() {
  emit autoRawDiameterRequested();
}

void SetupConfigurationPanel::onConfigurationChanged() {
  emit configurationChanged();

  // Emit specific signals for certain changes
  QObject *sender = this->sender();
  if (sender == m_materialTypeCombo) {
    emit materialTypeChanged(getMaterialType());
  }
  if (sender == m_rawDiameterSpin) {
    emit rawMaterialDiameterChanged(getRawDiameter());
  }
}

void SetupConfigurationPanel::onOperationToggled() {
  QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());
  if (checkBox) {
    QString operationName;
    if (checkBox == m_contouringEnabledCheck)
      operationName = "Contouring";
    else if (checkBox == m_threadingEnabledCheck)
      operationName = "Threading";
    else if (checkBox == m_chamferingEnabledCheck)
      operationName = "Chamfering";
    else if (checkBox == m_partingEnabledCheck)
      operationName = "Parting";

    bool enabled = checkBox->isChecked();
    emit operationToggled(operationName, enabled);
    emit configurationChanged();
    updateOperationControls();
    // Triggering automatic generation here caused crashes when the
    // ToolpathGenerationController was not fully initialized.  Until the
    // generation pipeline is more robust we simply avoid starting it
    // automatically when toggling an operation from the setup tab.
    // if (enabled) {
    //     emit automaticToolpathGenerationRequested();
    // }
  }
}

void SetupConfigurationPanel::setMaterialManager(
    MaterialManager *materialManager) {
  m_materialManager = materialManager;
  if (m_materialManager) {
    // Update material combo box with materials from database
    m_materialTypeCombo->clear();
    QStringList materialNames = m_materialManager->getAllMaterialNames();
    for (const QString &name : materialNames) {
      MaterialProperties props = m_materialManager->getMaterialProperties(name);
      m_materialTypeCombo->addItem(props.displayName, name);
    }

    // Connect to material database changes
    connect(m_materialManager, &MaterialManager::materialAdded, this,
            &SetupConfigurationPanel::updateMaterialProperties);
    connect(m_materialManager, &MaterialManager::materialUpdated, this,
            &SetupConfigurationPanel::updateMaterialProperties);

    updateMaterialProperties();
  }
}

void SetupConfigurationPanel::setToolManager(ToolManager *toolManager) {
  m_toolManager = toolManager;
  if (m_toolManager) {
    // Connect to tool database changes
    connect(m_toolManager, &ToolManager::toolAdded, this,
            &SetupConfigurationPanel::updateToolRecommendations);
    connect(m_toolManager, &ToolManager::toolUpdated, this,
            &SetupConfigurationPanel::updateToolRecommendations);

    updateToolRecommendations();
  }
}

QString SetupConfigurationPanel::getSelectedMaterialName() const {
  if (m_materialTypeCombo->currentIndex() >= 0) {
    return m_materialTypeCombo->currentData().toString();
  }
  return QString();
}

QStringList SetupConfigurationPanel::getRecommendedTools() const {
  QStringList toolIds;
  for (auto list : m_operationToolLists) {
    if (!list)
      continue;
    for (int i = 0; i < list->count(); ++i) {
      QListWidgetItem *item = list->item(i);
      if (item && item->data(Qt::UserRole).isValid()) {
        toolIds.append(item->data(Qt::UserRole).toString());
      }
    }
  }
  return toolIds;
}

void SetupConfigurationPanel::updateMaterialProperties() {
  if (!m_materialManager) {
    return;
  }

  QString materialName = getSelectedMaterialName();
  if (materialName.isEmpty()) {
    return;
  }

  MaterialProperties props =
      m_materialManager->getMaterialProperties(materialName);
  if (props.name.isEmpty()) {
    return;
  }

  // Update tool recommendations when material changes
  updateToolRecommendations();

  emit materialSelectionChanged(materialName);
}

void SetupConfigurationPanel::updateToolRecommendations() {
  if (!m_toolManager) {
    return;
  }

  for (auto list : m_operationToolLists) {
    if (list)
      list->clear();
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
  case SurfaceFinish::Mirror_1Ra:
    surfaceFinish = 1.0;
    break;
  case SurfaceFinish::Polish_2Ra:
    surfaceFinish = 2.0;
    break;
  case SurfaceFinish::Smooth_4Ra:
    surfaceFinish = 4.0;
    break;
  case SurfaceFinish::Fine_8Ra:
    surfaceFinish = 8.0;
    break;
  case SurfaceFinish::Medium_16Ra:
    surfaceFinish = 16.0;
    break;
  case SurfaceFinish::Rough_32Ra:
    surfaceFinish = 32.0;
    break;
  }

  // Get tool recommendations for each enabled operation
  QStringList operations;
  if (isOperationEnabled("Contouring"))
    operations.append("contouring");
  if (isOperationEnabled("Threading"))
    operations.append("threading");
  if (isOperationEnabled("Chamfering"))
    operations.append("chamfering");
  if (isOperationEnabled("Parting"))
    operations.append("parting");

  QSet<QString> recommendedToolIds;

  for (const QString &operation : operations) {
    auto recommendations = m_toolManager->recommendTools(
        operation, materialName, workpieceDiameter, surfaceFinish);

    // Add top 2 recommendations for each operation
    int count = 0;
    for (const auto &rec : recommendations) {
      if (count >= 2)
        break;
      if (!recommendedToolIds.contains(rec.toolId)) {
        recommendedToolIds.insert(rec.toolId);

        CuttingTool tool = m_toolManager->getTool(rec.toolId);
        QString itemText = QString("%1 - %2 (%3)")
                               .arg(tool.name)
                               .arg(operation)
                               .arg(rec.suitabilityScore, 0, 'f', 2);

        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, QVariant(rec.toolId));
        item->setToolTip(
            QString("Tool: %1\nOperation: %2\nSuitability: %3\nReason: %4")
                .arg(tool.name)
                .arg(operation)
                .arg(rec.suitabilityScore, 0, 'f', 2)
                .arg(rec.reason));

        QListWidget *targetList = m_operationToolLists.value(operation);
        if (targetList) {
          targetList->addItem(item);
        }
        count++;
      }
    }
  }

  emit toolRecommendationsUpdated(
      QStringList(recommendedToolIds.begin(), recommendedToolIds.end()));
}

void SetupConfigurationPanel::onMaterialChanged() {
  updateMaterialProperties();
  emit configurationChanged();
}

void SetupConfigurationPanel::onToolSelectionRequested() {
  // This could open a tool selection dialog or provide more detailed tool
  // information
  QStringList selectedTools = getRecommendedTools();
  if (!selectedTools.isEmpty()) {
    qDebug() << "Selected tools:" << selectedTools;
    // Here we could emit a signal to show detailed tool information
  }
}

void SetupConfigurationPanel::updateOperationControls() {
  // Placeholder for any additional per-operation controls
}

void SetupConfigurationPanel::updateAdvancedMode() {
  bool adv = m_advancedModeCheck && m_advancedModeCheck->isChecked();

  if (m_roughingAllowanceLabel)
    m_roughingAllowanceLabel->setVisible(adv);
  if (m_roughingAllowanceSpin)
    m_roughingAllowanceSpin->setVisible(adv);
  if (m_finishingAllowanceLabel)
    m_finishingAllowanceLabel->setVisible(adv);
  if (m_finishingAllowanceSpin)
    m_finishingAllowanceSpin->setVisible(adv);
  if (m_partingWidthLabel)
    m_partingWidthLabel->setVisible(adv);
  if (m_partingWidthSpin)
    m_partingWidthSpin->setVisible(adv);
  if (m_partingAdvancedGroup)
    m_partingAdvancedGroup->setVisible(adv);
  if (m_toleranceLabel)
    m_toleranceLabel->setVisible(adv);
  if (m_toleranceSpin)
    m_toleranceSpin->setVisible(adv);
  if (m_contourAdvancedGroup)
    m_contourAdvancedGroup->setVisible(adv);

  if (m_contourFloodCheck)
    m_contourFloodCheck->setVisible(!adv);
  if (m_threadFloodCheck)
    m_threadFloodCheck->setVisible(!adv);
  if (m_chamferFloodCheck)
    m_chamferFloodCheck->setVisible(!adv);
  if (m_partFloodCheck)
    m_partFloodCheck->setVisible(!adv);

  if (adv && m_materialManager) {
    QString materialName = getSelectedMaterialName();
    double finishVal = 16.0;
    switch (getSurfaceFinish()) {
    case SurfaceFinish::Mirror_1Ra:
      finishVal = 1.0;
      break;
    case SurfaceFinish::Polish_2Ra:
      finishVal = 2.0;
      break;
    case SurfaceFinish::Smooth_4Ra:
      finishVal = 4.0;
      break;
    case SurfaceFinish::Fine_8Ra:
      finishVal = 8.0;
      break;
    case SurfaceFinish::Medium_16Ra:
      finishVal = 16.0;
      break;
    case SurfaceFinish::Rough_32Ra:
      finishVal = 32.0;
      break;
    }
    CuttingParameters cp = m_materialManager->calculateCuttingParameters(
        materialName, 10.0, "facing", finishVal);
    if (m_contourFacingDepthSpin)
      m_contourFacingDepthSpin->setValue(cp.depthOfCut);
    if (m_contourFacingFeedSpin)
      m_contourFacingFeedSpin->setValue(cp.feedRate);
    if (m_contourFacingSpeedSpin)
      m_contourFacingSpeedSpin->setValue(cp.spindleSpeed);
    if (m_contourRoughDepthSpin)
      m_contourRoughDepthSpin->setValue(cp.depthOfCut);
    if (m_contourRoughFeedSpin)
      m_contourRoughFeedSpin->setValue(cp.feedRate);
    if (m_contourRoughSpeedSpin)
      m_contourRoughSpeedSpin->setValue(cp.spindleSpeed);
    if (m_contourFinishDepthSpin)
      m_contourFinishDepthSpin->setValue(cp.depthOfCut);
    if (m_contourFinishFeedSpin)
      m_contourFinishFeedSpin->setValue(cp.feedRate);
    if (m_contourFinishSpeedSpin)
      m_contourFinishSpeedSpin->setValue(cp.spindleSpeed);
  }
}

void SetupConfigurationPanel::focusOperationTab(const QString &operationName) {
  int index = 0;
  if (operationName.compare("Contouring", Qt::CaseInsensitive) == 0)
    index = 0;
  else if (operationName.compare("Threading", Qt::CaseInsensitive) == 0)
    index = 1;
  else if (operationName.compare("Chamfering", Qt::CaseInsensitive) == 0)
    index = 2;
  else if (operationName.compare("Parting", Qt::CaseInsensitive) == 0)
    index = 3;
  if (m_operationsTabWidget)
    m_operationsTabWidget->setCurrentIndex(index);
}

void SetupConfigurationPanel::onAddThreadFace() {
  emit requestThreadFaceSelection();
}

void SetupConfigurationPanel::onRemoveThreadFace() {
  QList<QTableWidgetSelectionRange> ranges = m_threadFacesTable->selectedRanges();
  if (ranges.isEmpty())
    return;
  int row = ranges.first().topRow();
  m_threadFacesTable->removeRow(row);
  if (row >= 0 && row < m_threadFaces.size())
    m_threadFaces.remove(row);
  onThreadFaceRowSelected();
}

void SetupConfigurationPanel::addSelectedThreadFace(const TopoDS_Shape &face) {
  int row = m_threadFacesTable->rowCount();
  m_threadFacesTable->insertRow(row);

  QComboBox *presetCombo = new QComboBox();
  presetCombo->addItems({"None", "M6x1", "M8x1.25", "M10x1.5"});
  m_threadFacesTable->setCellWidget(row, 0, presetCombo);

  auto *pitchItem = new QTableWidgetItem(QString::number(1.0, 'f', 2));
  m_threadFacesTable->setItem(row, 1, pitchItem);

  auto *depthItem = new QTableWidgetItem(QString::number(5.0, 'f', 2));
  m_threadFacesTable->setItem(row, 2, depthItem);

  ThreadFaceConfig cfg;
  cfg.face = face;
  cfg.preset = "None";
  cfg.pitch = 1.0;
  cfg.depth = 5.0;
  m_threadFaces.append(cfg);

  connect(presetCombo, &QComboBox::currentTextChanged, this,
          [this, row, pitchItem](const QString &text) {
            if (m_updatingThreadTable)
              return;
            m_updatingThreadTable = true;
            if (text == "M6x1") {
              pitchItem->setText(QString::number(1.0, 'f', 2));
              m_threadFaces[row].pitch = 1.0;
            } else if (text == "M8x1.25") {
              pitchItem->setText(QString::number(1.25, 'f', 2));
              m_threadFaces[row].pitch = 1.25;
            } else if (text == "M10x1.5") {
              pitchItem->setText(QString::number(1.5, 'f', 2));
              m_threadFaces[row].pitch = 1.5;
            }
            m_threadFaces[row].preset = text;
            m_updatingThreadTable = false;
          });
}

void SetupConfigurationPanel::onAddChamferFace() {
  int row = m_chamferFacesTable->rowCount();
  m_chamferFacesTable->insertRow(row);
  m_chamferFacesTable->setItem(row, 0, new QTableWidgetItem(tr("Face %1").arg(row + 1)));
  m_chamferFacesTable->setItem(row, 1, new QTableWidgetItem("Yes"));
  m_chamferFacesTable->setItem(row, 2, new QTableWidgetItem(QString::number(m_chamferSizeSpin->value(), 'f', 2)));
  m_chamferFacesTable->setItem(row, 3, new QTableWidgetItem(QString::number(m_chamferSizeSpin->value(), 'f', 2)));

  ChamferFaceConfig cfg;
  cfg.faceId = QString::number(row);
  cfg.symmetric = true;
  cfg.valueA = m_chamferSizeSpin->value();
  cfg.valueB = m_chamferSizeSpin->value();
  m_chamferFaces.append(cfg);
}

void SetupConfigurationPanel::onRemoveChamferFace() {
  QList<QTableWidgetSelectionRange> ranges = m_chamferFacesTable->selectedRanges();
  if (ranges.isEmpty())
    return;
  int row = ranges.first().topRow();
  m_chamferFacesTable->removeRow(row);
  if (row >= 0 && row < m_chamferFaces.size())
    m_chamferFaces.remove(row);
}

void SetupConfigurationPanel::onThreadFaceRowSelected() {
  QList<QTableWidgetSelectionRange> ranges = m_threadFacesTable->selectedRanges();
  if (ranges.isEmpty()) {
    emit threadFaceDeselected();
    return;
  }
  int row = ranges.first().topRow();
  if (row >= 0 && row < m_threadFaces.size())
    emit threadFaceSelected(m_threadFaces[row].face);
}

void SetupConfigurationPanel::onThreadFaceCellChanged(int row, int column) {
  if (m_updatingThreadTable)
    return;
  if (row < 0 || row >= m_threadFaces.size())
    return;

  if (column == 1) {
    m_updatingThreadTable = true;
    QWidget *w = m_threadFacesTable->cellWidget(row, 0);
    if (auto combo = qobject_cast<QComboBox *>(w)) {
      combo->setCurrentIndex(0); // None
    }
    bool ok = false;
    double val = m_threadFacesTable->item(row, column)->text().toDouble(&ok);
    if (ok) m_threadFaces[row].pitch = val;
    m_threadFaces[row].preset = "None";
    m_updatingThreadTable = false;
  } else if (column == 2) {
    bool ok = false;
    double val = m_threadFacesTable->item(row, column)->text().toDouble(&ok);
    if (ok) m_threadFaces[row].depth = val;
  }
}

void SetupConfigurationPanel::onChamferFaceRowSelected() {
  QList<QTableWidgetSelectionRange> ranges = m_chamferFacesTable->selectedRanges();
  if (ranges.isEmpty())
    return;
  int row = ranges.first().topRow();
  if (row >= 0 && row < m_chamferFaces.size())
    emit chamferFaceSelected(m_chamferFaces[row].faceId);
}

// Utility method implementations
QString SetupConfigurationPanel::materialTypeToString(MaterialType type) {
  if (static_cast<int>(type) < MATERIAL_NAMES.size()) {
    return MATERIAL_NAMES[static_cast<int>(type)];
  }
  return "Unknown";
}

MaterialType
SetupConfigurationPanel::stringToMaterialType(const QString &typeStr) {
  int index = MATERIAL_NAMES.indexOf(typeStr);
  return (index >= 0) ? static_cast<MaterialType>(index) : MaterialType::Custom;
}

QString SetupConfigurationPanel::surfaceFinishToString(SurfaceFinish finish) {
  if (static_cast<int>(finish) < SURFACE_FINISH_NAMES.size()) {
    return SURFACE_FINISH_NAMES[static_cast<int>(finish)];
  }
  return "Unknown";
}

SurfaceFinish
SetupConfigurationPanel::stringToSurfaceFinish(const QString &finishStr) {
  int index = SURFACE_FINISH_NAMES.indexOf(finishStr);
  return (index >= 0) ? static_cast<SurfaceFinish>(index)
                      : SurfaceFinish::Medium_16Ra;
}

} // namespace GUI
} // namespace IntuiCAM
