#include "materialadditiondialog.h"
#include <QDebug>

namespace IntuiCAM {
namespace GUI {

MaterialAdditionDialog::MaterialAdditionDialog(MaterialManager* materialManager, QWidget *parent)
    : QDialog(parent)
    , m_materialManager(materialManager)
    , m_materialCreated(false)
{
    setWindowTitle("Add New Material");
    setModal(true);
    resize(500, 600);
    
    setupUI();
    setupPresets();
    setupConnections();
    
    // Set initial state
    validateInputs();
}

MaterialAdditionDialog::~MaterialAdditionDialog()
{
}

MaterialProperties MaterialAdditionDialog::getMaterialProperties() const
{
    return collectMaterialProperties();
}

void MaterialAdditionDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    createPresetGroup();
    createBasicPropertiesGroup();
    createPhysicalPropertiesGroup();
    createMechanicalPropertiesGroup();
    createMachiningPropertiesGroup();
    
    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText("Add Material");
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    
    m_mainLayout->addWidget(m_presetGroup);
    m_mainLayout->addWidget(m_basicPropertiesGroup);
    m_mainLayout->addWidget(m_physicalPropertiesGroup);
    m_mainLayout->addWidget(m_mechanicalPropertiesGroup);
    m_mainLayout->addWidget(m_machiningPropertiesGroup);
    m_mainLayout->addWidget(m_buttonBox);
}

void MaterialAdditionDialog::createPresetGroup()
{
    m_presetGroup = new QGroupBox("Material Presets", this);
    QHBoxLayout* layout = new QHBoxLayout(m_presetGroup);
    
    m_presetCombo = new QComboBox(this);
    m_presetCombo->addItem("Select a preset...", "");
    
    m_loadPresetButton = new QPushButton("Load Preset", this);
    m_loadPresetButton->setEnabled(false);
    
    layout->addWidget(new QLabel("Preset:"));
    layout->addWidget(m_presetCombo);
    layout->addWidget(m_loadPresetButton);
    layout->addStretch();
}

void MaterialAdditionDialog::createBasicPropertiesGroup()
{
    m_basicPropertiesGroup = new QGroupBox("Basic Properties", this);
    QFormLayout* layout = new QFormLayout(m_basicPropertiesGroup);
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Enter unique material name...");
    
    m_displayNameEdit = new QLineEdit(this);
    m_displayNameEdit->setPlaceholderText("Enter display name...");
    
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem("Aluminum", "Aluminum");
    m_categoryCombo->addItem("Steel", "Steel");
    m_categoryCombo->addItem("Stainless Steel", "Stainless Steel");
    m_categoryCombo->addItem("Brass", "Brass");
    m_categoryCombo->addItem("Bronze", "Bronze");
    m_categoryCombo->addItem("Titanium", "Titanium");
    m_categoryCombo->addItem("Plastic", "Plastic");
    m_categoryCombo->addItem("Composite", "Composite");
    m_categoryCombo->addItem("Custom", "Custom");
    
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setPlaceholderText("Enter material description...");
    
    layout->addRow("Material Name *:", m_nameEdit);
    layout->addRow("Display Name:", m_displayNameEdit);
    layout->addRow("Category:", m_categoryCombo);
    layout->addRow("Description:", m_descriptionEdit);
}

void MaterialAdditionDialog::createPhysicalPropertiesGroup()
{
    m_physicalPropertiesGroup = new QGroupBox("Physical Properties", this);
    QFormLayout* layout = new QFormLayout(m_physicalPropertiesGroup);
    
    m_densitySpin = new QDoubleSpinBox(this);
    m_densitySpin->setRange(0.1, 50000.0);
    m_densitySpin->setSuffix(" kg/m³");
    m_densitySpin->setDecimals(2);
    m_densitySpin->setValue(7850.0); // Steel default
    
    m_thermalConductivitySpin = new QDoubleSpinBox(this);
    m_thermalConductivitySpin->setRange(0.1, 1000.0);
    m_thermalConductivitySpin->setSuffix(" W/m·K");
    m_thermalConductivitySpin->setDecimals(2);
    m_thermalConductivitySpin->setValue(50.0);
    
    m_specificHeatSpin = new QDoubleSpinBox(this);
    m_specificHeatSpin->setRange(100.0, 10000.0);
    m_specificHeatSpin->setSuffix(" J/kg·K");
    m_specificHeatSpin->setDecimals(0);
    m_specificHeatSpin->setValue(500.0);
    
    layout->addRow("Density:", m_densitySpin);
    layout->addRow("Thermal Conductivity:", m_thermalConductivitySpin);
    layout->addRow("Specific Heat:", m_specificHeatSpin);
}

void MaterialAdditionDialog::createMechanicalPropertiesGroup()
{
    m_mechanicalPropertiesGroup = new QGroupBox("Mechanical Properties", this);
    QFormLayout* layout = new QFormLayout(m_mechanicalPropertiesGroup);
    
    m_yieldStrengthSpin = new QDoubleSpinBox(this);
    m_yieldStrengthSpin->setRange(10.0, 5000.0);
    m_yieldStrengthSpin->setSuffix(" MPa");
    m_yieldStrengthSpin->setDecimals(0);
    m_yieldStrengthSpin->setValue(250.0);
    
    m_ultimateStrengthSpin = new QDoubleSpinBox(this);
    m_ultimateStrengthSpin->setRange(10.0, 5000.0);
    m_ultimateStrengthSpin->setSuffix(" MPa");
    m_ultimateStrengthSpin->setDecimals(0);
    m_ultimateStrengthSpin->setValue(400.0);
    
    m_hardnessBHNSpin = new QDoubleSpinBox(this);
    m_hardnessBHNSpin->setRange(10.0, 800.0);
    m_hardnessBHNSpin->setSuffix(" BHN");
    m_hardnessBHNSpin->setDecimals(0);
    m_hardnessBHNSpin->setValue(150.0);
    
    layout->addRow("Yield Strength:", m_yieldStrengthSpin);
    layout->addRow("Ultimate Strength:", m_ultimateStrengthSpin);
    layout->addRow("Hardness (Brinell):", m_hardnessBHNSpin);
}

void MaterialAdditionDialog::createMachiningPropertiesGroup()
{
    m_machiningPropertiesGroup = new QGroupBox("Machining Properties", this);
    QFormLayout* layout = new QFormLayout(m_machiningPropertiesGroup);
    
    m_recommendedSurfaceSpeedSpin = new QDoubleSpinBox(this);
    m_recommendedSurfaceSpeedSpin->setRange(10.0, 1000.0);
    m_recommendedSurfaceSpeedSpin->setSuffix(" m/min");
    m_recommendedSurfaceSpeedSpin->setDecimals(0);
    m_recommendedSurfaceSpeedSpin->setValue(120.0);
    
    m_recommendedFeedRateSpin = new QDoubleSpinBox(this);
    m_recommendedFeedRateSpin->setRange(0.01, 5.0);
    m_recommendedFeedRateSpin->setSuffix(" mm/rev");
    m_recommendedFeedRateSpin->setDecimals(3);
    m_recommendedFeedRateSpin->setValue(0.200);
    
    m_maxDepthOfCutSpin = new QDoubleSpinBox(this);
    m_maxDepthOfCutSpin->setRange(0.1, 20.0);
    m_maxDepthOfCutSpin->setSuffix(" mm");
    m_maxDepthOfCutSpin->setDecimals(2);
    m_maxDepthOfCutSpin->setValue(2.0);
    
    m_machinabilityRatingSpin = new QDoubleSpinBox(this);
    m_machinabilityRatingSpin->setRange(0.1, 5.0);
    m_machinabilityRatingSpin->setDecimals(2);
    m_machinabilityRatingSpin->setValue(1.0);
    m_machinabilityRatingSpin->setToolTip("1.0 = Reference material (1018 steel)");
    
    layout->addRow("Surface Speed:", m_recommendedSurfaceSpeedSpin);
    layout->addRow("Feed Rate:", m_recommendedFeedRateSpin);
    layout->addRow("Max Depth of Cut:", m_maxDepthOfCutSpin);
    layout->addRow("Machinability Rating:", m_machinabilityRatingSpin);
}

void MaterialAdditionDialog::setupPresets()
{
    m_presets = createCommonMaterialPresets();
    
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it) {
        m_presetCombo->addItem(it.key(), it.key());
    }
}

void MaterialAdditionDialog::setupConnections()
{
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &MaterialAdditionDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &MaterialAdditionDialog::onRejected);
    
    connect(m_nameEdit, &QLineEdit::textChanged, this, &MaterialAdditionDialog::onNameChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaterialAdditionDialog::onCategoryChanged);
    
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaterialAdditionDialog::onPresetSelected);
    connect(m_loadPresetButton, &QPushButton::clicked, this, [this]() {
        QString presetName = m_presetCombo->currentData().toString();
        if (!presetName.isEmpty()) {
            loadPresetValues(presetName);
        }
    });
    
    // Connect all input fields to validation
    connect(m_nameEdit, &QLineEdit::textChanged, this, &MaterialAdditionDialog::validateInputs);
    connect(m_displayNameEdit, &QLineEdit::textChanged, this, &MaterialAdditionDialog::validateInputs);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaterialAdditionDialog::validateInputs);
}

void MaterialAdditionDialog::onAccepted()
{
    if (!validateRequiredFields()) {
        QMessageBox::warning(this, "Invalid Input", "Please fill in all required fields with valid values.");
        return;
    }
    
    MaterialProperties properties = collectMaterialProperties();
    
    if (m_materialManager->addCustomMaterial(properties)) {
        m_materialCreated = true;
        emit materialCreated(properties.name);
        accept();
    } else {
        emit errorOccurred("Failed to add material. Material name may already exist.");
        QMessageBox::critical(this, "Error", "Failed to add material. Material name may already exist.");
    }
}

void MaterialAdditionDialog::onRejected()
{
    reject();
}

void MaterialAdditionDialog::onCategoryChanged(int index)
{
    Q_UNUSED(index)
    validateInputs();
}

void MaterialAdditionDialog::onNameChanged(const QString& text)
{
    // Auto-fill display name if empty
    if (m_displayNameEdit->text().isEmpty()) {
        m_displayNameEdit->setText(text);
    }
    validateInputs();
}

void MaterialAdditionDialog::onPresetSelected(int index)
{
    QString presetName = m_presetCombo->itemData(index).toString();
    m_loadPresetButton->setEnabled(!presetName.isEmpty());
}

void MaterialAdditionDialog::loadPresetValues(const QString& presetName)
{
    if (!m_presets.contains(presetName)) {
        return;
    }
    
    const MaterialProperties& preset = m_presets[presetName];
    setFieldsFromProperties(preset);
    
    QMessageBox::information(this, "Preset Loaded", 
        QString("Preset '%1' has been loaded. You can now modify the values as needed.").arg(presetName));
}

bool MaterialAdditionDialog::validateMaterialName(const QString& name) const
{
    if (name.isEmpty()) {
        return false;
    }
    
    if (m_materialManager && m_materialManager->hasMaterial(name)) {
        return false;
    }
    
    return true;
}

bool MaterialAdditionDialog::validateRequiredFields() const
{
    return validateMaterialName(m_nameEdit->text()) && 
           !m_categoryCombo->currentText().isEmpty();
}

void MaterialAdditionDialog::validateInputs()
{
    updateAcceptButtonState();
}

void MaterialAdditionDialog::updateAcceptButtonState()
{
    bool valid = validateRequiredFields();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    
    // Update name field style to indicate validity
    QString nameStyle = validateMaterialName(m_nameEdit->text()) ? 
        "QLineEdit { border: 1px solid green; }" : 
        "QLineEdit { border: 1px solid red; }";
    m_nameEdit->setStyleSheet(nameStyle);
}

QMap<QString, MaterialProperties> MaterialAdditionDialog::createCommonMaterialPresets() const
{
    QMap<QString, MaterialProperties> presets;
    
    // Aluminum 6061
    MaterialProperties al6061;
    al6061.name = "6061-T6";
    al6061.displayName = "Aluminum 6061-T6";
    al6061.category = "Aluminum";
    al6061.density = 2700.0;
    al6061.thermalConductivity = 167.0;
    al6061.specificHeat = 896.0;
    al6061.yieldStrength = 276.0;
    al6061.ultimateStrength = 310.0;
    al6061.hardnessBHN = 95.0;
    al6061.recommendedSurfaceSpeed = 300.0;
    al6061.recommendedFeedRate = 0.25;
    al6061.maxDepthOfCut = 3.0;
    al6061.machinabilityRating = 3.0;
    al6061.description = "General purpose aluminum alloy with good strength and corrosion resistance";
    presets["Aluminum 6061-T6"] = al6061;
    
    // Steel 1018
    MaterialProperties steel1018;
    steel1018.name = "1018";
    steel1018.displayName = "Steel 1018";
    steel1018.category = "Steel";
    steel1018.density = 7850.0;
    steel1018.thermalConductivity = 51.9;
    steel1018.specificHeat = 486.0;
    steel1018.yieldStrength = 370.0;
    steel1018.ultimateStrength = 440.0;
    steel1018.hardnessBHN = 126.0;
    steel1018.recommendedSurfaceSpeed = 120.0;
    steel1018.recommendedFeedRate = 0.20;
    steel1018.maxDepthOfCut = 2.0;
    steel1018.machinabilityRating = 1.0;
    steel1018.description = "Low carbon steel, reference material for machinability";
    presets["Steel 1018"] = steel1018;
    
    // Stainless Steel 304
    MaterialProperties ss304;
    ss304.name = "304";
    ss304.displayName = "Stainless Steel 304";
    ss304.category = "Stainless Steel";
    ss304.density = 8000.0;
    ss304.thermalConductivity = 16.2;
    ss304.specificHeat = 500.0;
    ss304.yieldStrength = 205.0;
    ss304.ultimateStrength = 515.0;
    ss304.hardnessBHN = 201.0;
    ss304.recommendedSurfaceSpeed = 80.0;
    ss304.recommendedFeedRate = 0.15;
    ss304.maxDepthOfCut = 1.5;
    ss304.machinabilityRating = 0.45;
    ss304.description = "Austenitic stainless steel with excellent corrosion resistance";
    presets["Stainless Steel 304"] = ss304;
    
    // Brass C360
    MaterialProperties brassC360;
    brassC360.name = "C360";
    brassC360.displayName = "Brass C360";
    brassC360.category = "Brass";
    brassC360.density = 8500.0;
    brassC360.thermalConductivity = 115.0;
    brassC360.specificHeat = 380.0;
    brassC360.yieldStrength = 124.0;
    brassC360.ultimateStrength = 338.0;
    brassC360.hardnessBHN = 100.0;
    brassC360.recommendedSurfaceSpeed = 400.0;
    brassC360.recommendedFeedRate = 0.30;
    brassC360.maxDepthOfCut = 4.0;
    brassC360.machinabilityRating = 3.5;
    brassC360.description = "Free-machining brass with excellent machinability";
    presets["Brass C360"] = brassC360;
    
    return presets;
}

MaterialProperties MaterialAdditionDialog::collectMaterialProperties() const
{
    MaterialProperties props;
    
    props.name = m_nameEdit->text().trimmed();
    props.displayName = m_displayNameEdit->text().trimmed();
    if (props.displayName.isEmpty()) {
        props.displayName = props.name;
    }
    props.category = m_categoryCombo->currentData().toString();
    props.description = m_descriptionEdit->toPlainText().trimmed();
    props.isCustom = true;
    
    props.density = m_densitySpin->value();
    props.thermalConductivity = m_thermalConductivitySpin->value();
    props.specificHeat = m_specificHeatSpin->value();
    
    props.yieldStrength = m_yieldStrengthSpin->value();
    props.ultimateStrength = m_ultimateStrengthSpin->value();
    props.hardnessBHN = m_hardnessBHNSpin->value();
    
    props.recommendedSurfaceSpeed = m_recommendedSurfaceSpeedSpin->value();
    props.recommendedFeedRate = m_recommendedFeedRateSpin->value();
    props.maxDepthOfCut = m_maxDepthOfCutSpin->value();
    props.machinabilityRating = m_machinabilityRatingSpin->value();
    
    return props;
}

void MaterialAdditionDialog::setFieldsFromProperties(const MaterialProperties& props)
{
    m_nameEdit->setText(props.name);
    m_displayNameEdit->setText(props.displayName);
    
    // Set category
    int categoryIndex = m_categoryCombo->findData(props.category);
    if (categoryIndex >= 0) {
        m_categoryCombo->setCurrentIndex(categoryIndex);
    }
    
    m_descriptionEdit->setText(props.description);
    
    m_densitySpin->setValue(props.density);
    m_thermalConductivitySpin->setValue(props.thermalConductivity);
    m_specificHeatSpin->setValue(props.specificHeat);
    
    m_yieldStrengthSpin->setValue(props.yieldStrength);
    m_ultimateStrengthSpin->setValue(props.ultimateStrength);
    m_hardnessBHNSpin->setValue(props.hardnessBHN);
    
    m_recommendedSurfaceSpeedSpin->setValue(props.recommendedSurfaceSpeed);
    m_recommendedFeedRateSpin->setValue(props.recommendedFeedRate);
    m_maxDepthOfCutSpin->setValue(props.maxDepthOfCut);
    m_machinabilityRatingSpin->setValue(props.machinabilityRating);
}

} // namespace GUI
} // namespace IntuiCAM 