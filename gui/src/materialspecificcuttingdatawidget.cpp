#include "materialspecificcuttingdatawidget.h"
#include <QDebug>

namespace IntuiCAM {
namespace GUI {

// ============================================================================
// MaterialSpecificCuttingDataWidget Implementation
// ============================================================================

MaterialSpecificCuttingDataWidget::MaterialSpecificCuttingDataWidget(QWidget *parent)
    : QWidget(parent)
    , m_materialManager(nullptr)
{
    setupUI();
    setupConnections();
}

MaterialSpecificCuttingDataWidget::~MaterialSpecificCuttingDataWidget()
{
}

void MaterialSpecificCuttingDataWidget::setMaterialManager(MaterialManager* materialManager)
{
    m_materialManager = materialManager;
    if (m_materialManager) {
        refreshMaterialTabs();
        
        // Connect to material manager signals
        connect(m_materialManager, &MaterialManager::materialAdded,
                this, &MaterialSpecificCuttingDataWidget::addMaterialTab);
        connect(m_materialManager, &MaterialManager::materialRemoved,
                this, &MaterialSpecificCuttingDataWidget::removeMaterialTab);
    }
}

void MaterialSpecificCuttingDataWidget::loadCuttingData(const IntuiCAM::Toolpath::CuttingData& cuttingData)
{
    // Load material-specific cutting data for each material tab
    for (auto it = m_materialTabs.begin(); it != m_materialTabs.end(); ++it) {
        const QString& materialName = it.key();
        MaterialSpecificCuttingDataTab* tab = it.value();
        
        if (cuttingData.hasMaterialSettings(materialName.toStdString())) {
            auto materialSettings = cuttingData.getMaterialSettings(materialName.toStdString());
            tab->setCuttingData(materialSettings);
        } else {
            // Create default settings for this material
            IntuiCAM::Toolpath::MaterialSpecificCuttingData defaultSettings(materialName.toStdString());
            defaultSettings.enabled = true; // Enable materials by default
            tab->setCuttingData(defaultSettings);
        }
    }
}

IntuiCAM::Toolpath::CuttingData MaterialSpecificCuttingDataWidget::getCuttingData() const
{
    IntuiCAM::Toolpath::CuttingData cuttingData;
    
    // Collect cutting data from all material tabs
    for (auto it = m_materialTabs.begin(); it != m_materialTabs.end(); ++it) {
        const QString& materialName = it.key();
        MaterialSpecificCuttingDataTab* tab = it.value();
        
        auto materialSettings = tab->getCuttingData();
        cuttingData.setMaterialSettings(materialName.toStdString(), materialSettings);
    }
    
    return cuttingData;
}

void MaterialSpecificCuttingDataWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    createMaterialManagementHeader();
    createTabWidget();
}

void MaterialSpecificCuttingDataWidget::createMaterialManagementHeader()
{
    m_headerLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel("Material-Specific Cutting Data", this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    m_addMaterialButton = new QPushButton("Add Material", this);
    m_addMaterialButton->setIcon(QIcon(":/icons/add.png")); // Assuming you have an add icon
    m_addMaterialButton->setToolTip("Add a new material to the database");
    
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_addMaterialButton);
    
    m_mainLayout->addLayout(m_headerLayout);
}

void MaterialSpecificCuttingDataWidget::createTabWidget()
{
    m_materialTabWidget = new QTabWidget(this);
    m_materialTabWidget->setTabsClosable(false);
    m_materialTabWidget->setMovable(false);
    
    // Create scroll area for the tab widget
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_materialTabWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_mainLayout->addWidget(m_scrollArea);
}

void MaterialSpecificCuttingDataWidget::setupConnections()
{
    connect(m_addMaterialButton, &QPushButton::clicked,
            this, &MaterialSpecificCuttingDataWidget::onAddMaterialClicked);
    connect(m_materialTabWidget, &QTabWidget::currentChanged,
            this, &MaterialSpecificCuttingDataWidget::onMaterialTabChanged);
}

void MaterialSpecificCuttingDataWidget::refreshMaterialTabs()
{
    if (!m_materialManager) {
        return;
    }
    
    // Clear existing tabs
    m_materialTabWidget->clear();
    qDeleteAll(m_materialTabs);
    m_materialTabs.clear();
    
    // Create tabs for all materials
    QStringList materials = m_materialManager->getAllMaterialNames();
    for (const QString& materialName : materials) {
        addMaterialTab(materialName);
    }
}

void MaterialSpecificCuttingDataWidget::addMaterialTab(const QString& materialName)
{
    if (m_materialTabs.contains(materialName)) {
        return; // Tab already exists
    }
    
    MaterialSpecificCuttingDataTab* tab = createMaterialTab(materialName);
    if (tab) {
        m_materialTabs[materialName] = tab;
        int index = m_materialTabWidget->addTab(tab, materialName);
        updateTabTitle(materialName, tab->isEnabled());
        
        // Connect tab signals
        connect(tab, &MaterialSpecificCuttingDataTab::enabledChanged,
                this, &MaterialSpecificCuttingDataWidget::onMaterialEnabledChanged);
        connect(tab, &MaterialSpecificCuttingDataTab::cuttingDataChanged,
                this, &MaterialSpecificCuttingDataWidget::onMaterialTabCuttingDataChanged);
    }
}

void MaterialSpecificCuttingDataWidget::removeMaterialTab(const QString& materialName)
{
    MaterialSpecificCuttingDataTab* tab = getMaterialTab(materialName);
    if (!tab) {
        return;
    }
    
    int index = findMaterialTabIndex(materialName);
    if (index >= 0) {
        m_materialTabWidget->removeTab(index);
    }
    
    m_materialTabs.remove(materialName);
    delete tab;
}

MaterialSpecificCuttingDataTab* MaterialSpecificCuttingDataWidget::createMaterialTab(const QString& materialName)
{
    MaterialSpecificCuttingDataTab* tab = new MaterialSpecificCuttingDataTab(materialName, this);
    return tab;
}

MaterialSpecificCuttingDataTab* MaterialSpecificCuttingDataWidget::getMaterialTab(const QString& materialName)
{
    return m_materialTabs.value(materialName, nullptr);
}

int MaterialSpecificCuttingDataWidget::findMaterialTabIndex(const QString& materialName)
{
    for (int i = 0; i < m_materialTabWidget->count(); ++i) {
        if (m_materialTabWidget->tabText(i).startsWith(materialName)) {
            return i;
        }
    }
    return -1;
}

void MaterialSpecificCuttingDataWidget::updateTabTitle(const QString& materialName, bool enabled)
{
    int index = findMaterialTabIndex(materialName);
    if (index >= 0) {
        QString title = materialName;
        if (enabled) {
            title += " ✓";
            m_materialTabWidget->setTabIcon(index, QIcon(":/icons/enabled.png"));
        } else {
            m_materialTabWidget->setTabIcon(index, QIcon(":/icons/disabled.png"));
        }
        m_materialTabWidget->setTabText(index, title);
        
        // Update tab color to indicate enabled/disabled state
        if (enabled) {
            m_materialTabWidget->tabBar()->setTabTextColor(index, QColor(0, 128, 0)); // Green
        } else {
            m_materialTabWidget->tabBar()->setTabTextColor(index, QColor(128, 128, 128)); // Gray
        }
    }
}

QStringList MaterialSpecificCuttingDataWidget::getEnabledMaterials() const
{
    QStringList enabledMaterials;
    for (auto it = m_materialTabs.begin(); it != m_materialTabs.end(); ++it) {
        if (it.value()->isEnabled()) {
            enabledMaterials.append(it.key());
        }
    }
    return enabledMaterials;
}

void MaterialSpecificCuttingDataWidget::onAddMaterialClicked()
{
    if (!m_materialManager) {
        QMessageBox::warning(this, "Error", "Material manager is not available.");
        return;
    }
    
    MaterialAdditionDialog* dialog = new MaterialAdditionDialog(m_materialManager, this);
    connect(dialog, &MaterialAdditionDialog::materialCreated,
            this, &MaterialSpecificCuttingDataWidget::onMaterialAddedFromDialog);
    
    dialog->exec();
    dialog->deleteLater();
}

void MaterialSpecificCuttingDataWidget::onMaterialTabChanged(int index)
{
    Q_UNUSED(index)
    // Handle tab change if needed
}

void MaterialSpecificCuttingDataWidget::onMaterialEnabledChanged(const QString& materialName, bool enabled)
{
    updateTabTitle(materialName, enabled);
    emit materialEnabledChanged(materialName, enabled);
    emit cuttingDataChanged();
}

void MaterialSpecificCuttingDataWidget::onMaterialAddedFromDialog(const QString& materialName)
{
    addMaterialTab(materialName);
    emit materialAdded(materialName);
}

void MaterialSpecificCuttingDataWidget::onMaterialTabCuttingDataChanged()
{
    emit cuttingDataChanged();
}

// ============================================================================
// MaterialSpecificCuttingDataTab Implementation
// ============================================================================

MaterialSpecificCuttingDataTab::MaterialSpecificCuttingDataTab(const QString& materialName, QWidget *parent)
    : QWidget(parent)
    , m_materialName(materialName)
    , m_isEnabled(false)
{
    setupUI();
    setupConnections();
    updateUIState();
}

MaterialSpecificCuttingDataTab::~MaterialSpecificCuttingDataTab()
{
}

void MaterialSpecificCuttingDataTab::setCuttingData(const IntuiCAM::Toolpath::MaterialSpecificCuttingData& data)
{
    // Block signals to prevent triggering change events during loading
    blockSignals(true);
    
    m_isEnabled = data.enabled;
    m_enabledCheckBox->setChecked(data.enabled);
    
    // Speed control
    m_constantSurfaceSpeedCheckBox->setChecked(data.constantSurfaceSpeed);
    m_surfaceSpeedSpin->setValue(data.surfaceSpeed);
    m_spindleRPMSpin->setValue(static_cast<int>(data.spindleRPM));
    
    // Feed control
    m_feedPerRevolutionCheckBox->setChecked(data.feedPerRevolution);
    m_cuttingFeedrateSpin->setValue(data.cuttingFeedrate);
    m_leadInFeedrateSpin->setValue(data.leadInFeedrate);
    m_leadOutFeedrateSpin->setValue(data.leadOutFeedrate);
    
    // Cutting limits
    m_maxDepthOfCutSpin->setValue(data.maxDepthOfCut);
    m_maxFeedrateSpin->setValue(data.maxFeedrate);
    m_minSurfaceSpeedSpin->setValue(data.minSurfaceSpeed);
    m_maxSurfaceSpeedSpin->setValue(data.maxSurfaceSpeed);
    
    // Coolant
    m_floodCoolantCheckBox->setChecked(data.floodCoolant);
    m_mistCoolantCheckBox->setChecked(data.mistCoolant);
    m_coolantPressureSpin->setValue(data.coolantPressure);
    m_coolantFlowSpin->setValue(data.coolantFlow);
    
    // Set preferred coolant
    int coolantIndex = static_cast<int>(data.preferredCoolant);
    if (coolantIndex >= 0 && coolantIndex < m_preferredCoolantCombo->count()) {
        m_preferredCoolantCombo->setCurrentIndex(coolantIndex);
    }
    
    blockSignals(false);
    updateUIState();
}

IntuiCAM::Toolpath::MaterialSpecificCuttingData MaterialSpecificCuttingDataTab::getCuttingData() const
{
    IntuiCAM::Toolpath::MaterialSpecificCuttingData data(m_materialName.toStdString());
    
    data.enabled = m_isEnabled;
    
    // Speed control
    data.constantSurfaceSpeed = m_constantSurfaceSpeedCheckBox->isChecked();
    data.surfaceSpeed = m_surfaceSpeedSpin->value();
    data.spindleRPM = m_spindleRPMSpin->value();
    
    // Feed control
    data.feedPerRevolution = m_feedPerRevolutionCheckBox->isChecked();
    data.cuttingFeedrate = m_cuttingFeedrateSpin->value();
    data.leadInFeedrate = m_leadInFeedrateSpin->value();
    data.leadOutFeedrate = m_leadOutFeedrateSpin->value();
    
    // Cutting limits
    data.maxDepthOfCut = m_maxDepthOfCutSpin->value();
    data.maxFeedrate = m_maxFeedrateSpin->value();
    data.minSurfaceSpeed = m_minSurfaceSpeedSpin->value();
    data.maxSurfaceSpeed = m_maxSurfaceSpeedSpin->value();
    
    // Coolant
    data.floodCoolant = m_floodCoolantCheckBox->isChecked();
    data.mistCoolant = m_mistCoolantCheckBox->isChecked();
    data.preferredCoolant = static_cast<IntuiCAM::Toolpath::CoolantType>(m_preferredCoolantCombo->currentIndex());
    data.coolantPressure = m_coolantPressureSpin->value();
    data.coolantFlow = m_coolantFlowSpin->value();
    
    return data;
}

void MaterialSpecificCuttingDataTab::setEnabled(bool enabled)
{
    m_isEnabled = enabled;
    m_enabledCheckBox->setChecked(enabled);
    updateUIState();
}

bool MaterialSpecificCuttingDataTab::isEnabled() const
{
    return m_isEnabled;
}

void MaterialSpecificCuttingDataTab::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Create scroll area for content
    m_scrollArea = new QScrollArea(this);
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    
    createEnabledGroup();
    createSpeedControlGroup();
    createFeedControlGroup();
    createCuttingLimitsGroup();
    createCoolantGroup();
    
    m_contentLayout->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_mainLayout->addWidget(m_scrollArea);
}

void MaterialSpecificCuttingDataTab::createEnabledGroup()
{
    m_enabledGroup = new QGroupBox("Material Configuration", m_contentWidget);
    QVBoxLayout* layout = new QVBoxLayout(m_enabledGroup);
    
    m_enabledCheckBox = new QCheckBox(QString("Enable cutting data for %1").arg(m_materialName), m_enabledGroup);
    m_statusLabel = new QLabel("", m_enabledGroup);
    m_statusLabel->setWordWrap(true);
    
    layout->addWidget(m_enabledCheckBox);
    layout->addWidget(m_statusLabel);
    
    m_contentLayout->addWidget(m_enabledGroup);
}

void MaterialSpecificCuttingDataTab::createSpeedControlGroup()
{
    m_speedControlGroup = new QGroupBox("Speed Control", m_contentWidget);
    QFormLayout* layout = new QFormLayout(m_speedControlGroup);
    
    m_constantSurfaceSpeedCheckBox = new QCheckBox("Constant Surface Speed (CSS)", m_speedControlGroup);
    
    m_surfaceSpeedSpin = new QDoubleSpinBox(m_speedControlGroup);
    m_surfaceSpeedSpin->setRange(10, 1000);
    m_surfaceSpeedSpin->setSuffix(" m/min");
    m_surfaceSpeedSpin->setDecimals(0);
    m_surfaceSpeedSpin->setValue(200);
    
    m_spindleRPMSpin = new QSpinBox(m_speedControlGroup);
    m_spindleRPMSpin->setRange(50, 6000);
    m_spindleRPMSpin->setSuffix(" RPM");
    m_spindleRPMSpin->setValue(1000);
    
    m_surfaceSpeedLabel = new QLabel("Surface Speed:", m_speedControlGroup);
    m_spindleRPMLabel = new QLabel("Spindle RPM:", m_speedControlGroup);
    
    layout->addRow(m_constantSurfaceSpeedCheckBox);
    layout->addRow(m_surfaceSpeedLabel, m_surfaceSpeedSpin);
    layout->addRow(m_spindleRPMLabel, m_spindleRPMSpin);
    
    m_contentLayout->addWidget(m_speedControlGroup);
}

void MaterialSpecificCuttingDataTab::createFeedControlGroup()
{
    m_feedControlGroup = new QGroupBox("Feed Control", m_contentWidget);
    QFormLayout* layout = new QFormLayout(m_feedControlGroup);
    
    m_feedPerRevolutionCheckBox = new QCheckBox("Feed per Revolution", m_feedControlGroup);
    
    m_cuttingFeedrateSpin = new QDoubleSpinBox(m_feedControlGroup);
    m_cuttingFeedrateSpin->setRange(0.01, 10.0);
    m_cuttingFeedrateSpin->setDecimals(3);
    m_cuttingFeedrateSpin->setValue(0.200);
    
    m_leadInFeedrateSpin = new QDoubleSpinBox(m_feedControlGroup);
    m_leadInFeedrateSpin->setRange(0.01, 5.0);
    m_leadInFeedrateSpin->setDecimals(3);
    m_leadInFeedrateSpin->setValue(0.100);
    
    m_leadOutFeedrateSpin = new QDoubleSpinBox(m_feedControlGroup);
    m_leadOutFeedrateSpin->setRange(0.01, 5.0);
    m_leadOutFeedrateSpin->setDecimals(3);
    m_leadOutFeedrateSpin->setValue(0.100);
    
    m_cuttingFeedrateLabel = new QLabel("Cutting Feed Rate:", m_feedControlGroup);
    m_leadInFeedrateLabel = new QLabel("Lead-In Feed Rate:", m_feedControlGroup);
    m_leadOutFeedrateLabel = new QLabel("Lead-Out Feed Rate:", m_feedControlGroup);
    
    layout->addRow(m_feedPerRevolutionCheckBox);
    layout->addRow(m_cuttingFeedrateLabel, m_cuttingFeedrateSpin);
    layout->addRow(m_leadInFeedrateLabel, m_leadInFeedrateSpin);
    layout->addRow(m_leadOutFeedrateLabel, m_leadOutFeedrateSpin);
    
    m_contentLayout->addWidget(m_feedControlGroup);
}

void MaterialSpecificCuttingDataTab::createCuttingLimitsGroup()
{
    m_cuttingLimitsGroup = new QGroupBox("Cutting Limits", m_contentWidget);
    QFormLayout* layout = new QFormLayout(m_cuttingLimitsGroup);
    
    m_maxDepthOfCutSpin = new QDoubleSpinBox(m_cuttingLimitsGroup);
    m_maxDepthOfCutSpin->setRange(0.1, 20.0);
    m_maxDepthOfCutSpin->setSuffix(" mm");
    m_maxDepthOfCutSpin->setDecimals(2);
    m_maxDepthOfCutSpin->setValue(2.0);
    
    m_maxFeedrateSpin = new QDoubleSpinBox(m_cuttingLimitsGroup);
    m_maxFeedrateSpin->setRange(10, 5000);
    m_maxFeedrateSpin->setSuffix(" mm/min");
    m_maxFeedrateSpin->setDecimals(0);
    m_maxFeedrateSpin->setValue(1000);
    
    m_minSurfaceSpeedSpin = new QDoubleSpinBox(m_cuttingLimitsGroup);
    m_minSurfaceSpeedSpin->setRange(10, 500);
    m_minSurfaceSpeedSpin->setSuffix(" m/min");
    m_minSurfaceSpeedSpin->setDecimals(0);
    m_minSurfaceSpeedSpin->setValue(50);
    
    m_maxSurfaceSpeedSpin = new QDoubleSpinBox(m_cuttingLimitsGroup);
    m_maxSurfaceSpeedSpin->setRange(50, 1000);
    m_maxSurfaceSpeedSpin->setSuffix(" m/min");
    m_maxSurfaceSpeedSpin->setDecimals(0);
    m_maxSurfaceSpeedSpin->setValue(500);
    
    layout->addRow("Max Depth of Cut:", m_maxDepthOfCutSpin);
    layout->addRow("Max Feed Rate:", m_maxFeedrateSpin);
    layout->addRow("Min Surface Speed:", m_minSurfaceSpeedSpin);
    layout->addRow("Max Surface Speed:", m_maxSurfaceSpeedSpin);
    
    m_contentLayout->addWidget(m_cuttingLimitsGroup);
}

void MaterialSpecificCuttingDataTab::createCoolantGroup()
{
    m_coolantGroup = new QGroupBox("Coolant Settings", m_contentWidget);
    QFormLayout* layout = new QFormLayout(m_coolantGroup);
    
    m_floodCoolantCheckBox = new QCheckBox("Flood Coolant", m_coolantGroup);
    m_mistCoolantCheckBox = new QCheckBox("Mist Coolant", m_coolantGroup);
    
    m_preferredCoolantCombo = new QComboBox(m_coolantGroup);
    m_preferredCoolantCombo->addItem("None", static_cast<int>(IntuiCAM::Toolpath::CoolantType::NONE));
    m_preferredCoolantCombo->addItem("Flood", static_cast<int>(IntuiCAM::Toolpath::CoolantType::FLOOD));
    m_preferredCoolantCombo->addItem("Mist", static_cast<int>(IntuiCAM::Toolpath::CoolantType::MIST));
    m_preferredCoolantCombo->addItem("Air Blast", static_cast<int>(IntuiCAM::Toolpath::CoolantType::AIR_BLAST));
    m_preferredCoolantCombo->addItem("High Pressure", static_cast<int>(IntuiCAM::Toolpath::CoolantType::HIGH_PRESSURE));
    
    m_coolantPressureSpin = new QDoubleSpinBox(m_coolantGroup);
    m_coolantPressureSpin->setRange(0, 100);
    m_coolantPressureSpin->setSuffix(" bar");
    m_coolantPressureSpin->setDecimals(1);
    m_coolantPressureSpin->setValue(0);
    
    m_coolantFlowSpin = new QDoubleSpinBox(m_coolantGroup);
    m_coolantFlowSpin->setRange(0, 100);
    m_coolantFlowSpin->setSuffix(" L/min");
    m_coolantFlowSpin->setDecimals(1);
    m_coolantFlowSpin->setValue(0);
    
    layout->addRow(m_floodCoolantCheckBox);
    layout->addRow(m_mistCoolantCheckBox);
    layout->addRow("Preferred Coolant:", m_preferredCoolantCombo);
    layout->addRow("Coolant Pressure:", m_coolantPressureSpin);
    layout->addRow("Coolant Flow:", m_coolantFlowSpin);
    
    m_contentLayout->addWidget(m_coolantGroup);
}

void MaterialSpecificCuttingDataTab::setupConnections()
{
    connect(m_enabledCheckBox, &QCheckBox::toggled,
            this, &MaterialSpecificCuttingDataTab::onEnabledChanged);
    connect(m_constantSurfaceSpeedCheckBox, &QCheckBox::toggled,
            this, &MaterialSpecificCuttingDataTab::onConstantSurfaceSpeedToggled);
    connect(m_feedPerRevolutionCheckBox, &QCheckBox::toggled,
            this, &MaterialSpecificCuttingDataTab::onFeedPerRevolutionToggled);
    
    connectParameterSignals();
}

void MaterialSpecificCuttingDataTab::connectParameterSignals()
{
    // Speed control
    connect(m_surfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_spindleRPMSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    
    // Feed control
    connect(m_cuttingFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_leadInFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_leadOutFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    
    // Cutting limits
    connect(m_maxDepthOfCutSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_maxFeedrateSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_minSurfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_maxSurfaceSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    
    // Coolant
    connect(m_floodCoolantCheckBox, &QCheckBox::toggled,
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_mistCoolantCheckBox, &QCheckBox::toggled,
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_preferredCoolantCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_coolantPressureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
    connect(m_coolantFlowSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MaterialSpecificCuttingDataTab::onParameterChanged);
}

void MaterialSpecificCuttingDataTab::updateUIState()
{
    // Update enabled state of all controls
    bool enabled = m_isEnabled;
    
    m_speedControlGroup->setEnabled(enabled);
    m_feedControlGroup->setEnabled(enabled);
    m_cuttingLimitsGroup->setEnabled(enabled);
    m_coolantGroup->setEnabled(enabled);
    
    // Update status label
    if (enabled) {
        m_statusLabel->setText(QString("Cutting data for %1 is <b>enabled</b>. This tool will appear in recommended tools for %1.")
                              .arg(m_materialName));
        m_statusLabel->setStyleSheet("color: green;");
    } else {
        m_statusLabel->setText(QString("Cutting data for %1 is <b>disabled</b>. This tool will not be recommended for %1.")
                              .arg(m_materialName));
        m_statusLabel->setStyleSheet("color: #888888;");
    }
    
    // Update speed control visibility
    bool constantSurfaceSpeed = m_constantSurfaceSpeedCheckBox->isChecked();
    m_surfaceSpeedLabel->setVisible(constantSurfaceSpeed);
    m_surfaceSpeedSpin->setVisible(constantSurfaceSpeed);
    m_spindleRPMLabel->setVisible(!constantSurfaceSpeed);
    m_spindleRPMSpin->setVisible(!constantSurfaceSpeed);
    
    // Update feed rate units
    bool feedPerRevolution = m_feedPerRevolutionCheckBox->isChecked();
    updateFeedRateUnits(feedPerRevolution);
}

void MaterialSpecificCuttingDataTab::updateFeedRateUnits(bool feedPerRevolution)
{
    QString units = feedPerRevolution ? " mm/rev" : " mm/min";
    
    m_cuttingFeedrateSpin->setSuffix(units);
    m_leadInFeedrateSpin->setSuffix(units);
    m_leadOutFeedrateSpin->setSuffix(units);
    
    // Update labels
    QString cuttingLabel = feedPerRevolution ? "Cutting Feed Rate:" : "Cutting Feed Rate:";
    QString leadInLabel = feedPerRevolution ? "Lead-In Feed Rate:" : "Lead-In Feed Rate:";
    QString leadOutLabel = feedPerRevolution ? "Lead-Out Feed Rate:" : "Lead-Out Feed Rate:";
    
    m_cuttingFeedrateLabel->setText(cuttingLabel);
    m_leadInFeedrateLabel->setText(leadInLabel);
    m_leadOutFeedrateLabel->setText(leadOutLabel);
}

void MaterialSpecificCuttingDataTab::onEnabledChanged(bool enabled)
{
    m_isEnabled = enabled;
    updateUIState();
    emit enabledChanged(m_materialName, enabled);
    emit cuttingDataChanged();
}

void MaterialSpecificCuttingDataTab::onConstantSurfaceSpeedToggled(bool enabled)
{
    Q_UNUSED(enabled)
    updateUIState();
    emit cuttingDataChanged();
}

void MaterialSpecificCuttingDataTab::onFeedPerRevolutionToggled(bool enabled)
{
    updateFeedRateUnits(enabled);
    emit cuttingDataChanged();
}

void MaterialSpecificCuttingDataTab::onParameterChanged()
{
    emit cuttingDataChanged();
}

} // namespace GUI
} // namespace IntuiCAM 