#ifndef MATERIALADDITIONDIALOG_H
#define MATERIALADDITIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "materialmanager.h"

namespace IntuiCAM {
namespace GUI {

class MaterialAdditionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialAdditionDialog(MaterialManager* materialManager, QWidget *parent = nullptr);
    ~MaterialAdditionDialog();

    // Get the created material properties
    MaterialProperties getMaterialProperties() const;
    
    // Check if material was successfully created
    bool wasMaterialCreated() const { return m_materialCreated; }

signals:
    void materialCreated(const QString& materialName);
    void errorOccurred(const QString& message);

private slots:
    void onAccepted();
    void onRejected();
    void onCategoryChanged(int index);
    void onNameChanged(const QString& text);
    void validateInputs();
    void onPresetSelected(int index);
    void loadPresetValues(const QString& presetName);

private:
    void setupUI();
    void createBasicPropertiesGroup();
    void createPhysicalPropertiesGroup();
    void createMechanicalPropertiesGroup();
    void createMachiningPropertiesGroup();
    void createPresetGroup();
    void setupConnections();
    
    // UI validation
    bool validateMaterialName(const QString& name) const;
    bool validateRequiredFields() const;
    void updateAcceptButtonState();
    
    // Preset management
    void setupPresets();
    QMap<QString, MaterialProperties> createCommonMaterialPresets() const;
    
    // Helper methods
    MaterialProperties collectMaterialProperties() const;
    void resetAllFields();
    void setFieldsFromProperties(const MaterialProperties& props);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QDialogButtonBox* m_buttonBox;
    
    // Basic Properties Group
    QGroupBox* m_basicPropertiesGroup;
    QLineEdit* m_nameEdit;
    QLineEdit* m_displayNameEdit;
    QComboBox* m_categoryCombo;
    QTextEdit* m_descriptionEdit;
    
    // Physical Properties Group
    QGroupBox* m_physicalPropertiesGroup;
    QDoubleSpinBox* m_densitySpin;
    QDoubleSpinBox* m_thermalConductivitySpin;
    QDoubleSpinBox* m_specificHeatSpin;
    
    // Mechanical Properties Group
    QGroupBox* m_mechanicalPropertiesGroup;
    QDoubleSpinBox* m_yieldStrengthSpin;
    QDoubleSpinBox* m_ultimateStrengthSpin;
    QDoubleSpinBox* m_hardnessBHNSpin;
    
    // Machining Properties Group
    QGroupBox* m_machiningPropertiesGroup;
    QDoubleSpinBox* m_recommendedSurfaceSpeedSpin;
    QDoubleSpinBox* m_recommendedFeedRateSpin;
    QDoubleSpinBox* m_maxDepthOfCutSpin;
    QDoubleSpinBox* m_machinabilityRatingSpin;
    
    // Preset Group
    QGroupBox* m_presetGroup;
    QComboBox* m_presetCombo;
    QPushButton* m_loadPresetButton;
    
    // Data
    MaterialManager* m_materialManager;
    bool m_materialCreated;
    QMap<QString, MaterialProperties> m_presets;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // MATERIALADDITIONDIALOG_H 