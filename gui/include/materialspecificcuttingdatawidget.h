#ifndef MATERIALSPECIFICCUTTINGDATAWIDGET_H
#define MATERIALSPECIFICCUTTINGDATAWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QMenu>
#include <QAction>

#include "materialmanager.h"
#include "materialadditiondialog.h"
#include "IntuiCAM/Toolpath/ToolTypes.h"

namespace IntuiCAM {
namespace GUI {

// Forward declaration
class MaterialSpecificCuttingDataTab;

class MaterialSpecificCuttingDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialSpecificCuttingDataWidget(QWidget *parent = nullptr);
    ~MaterialSpecificCuttingDataWidget();

    // Set the material manager
    void setMaterialManager(MaterialManager* materialManager);
    
    // Load cutting data from ToolAssembly
    void loadCuttingData(const IntuiCAM::Toolpath::CuttingData& cuttingData);
    
    // Get the current cutting data
    IntuiCAM::Toolpath::CuttingData getCuttingData() const;
    
    // Material management
    void refreshMaterialTabs();
    void addMaterialTab(const QString& materialName);
    void removeMaterialTab(const QString& materialName);
    
    // Get enabled materials
    QStringList getEnabledMaterials() const;

signals:
    void cuttingDataChanged();
    void materialEnabledChanged(const QString& materialName, bool enabled);
    void materialAdded(const QString& materialName);

private slots:
    void onAddMaterialClicked();
    void onMaterialTabChanged(int index);
    void onMaterialEnabledChanged(const QString& materialName, bool enabled);
    void onMaterialAddedFromDialog(const QString& materialName);
    void onMaterialTabCuttingDataChanged();

private:
    void setupUI();
    void createMaterialManagementHeader();
    void createTabWidget();
    void setupConnections();
    
    // Material tab management
    MaterialSpecificCuttingDataTab* createMaterialTab(const QString& materialName);
    MaterialSpecificCuttingDataTab* getMaterialTab(const QString& materialName);
    int findMaterialTabIndex(const QString& materialName);
    void updateTabTitle(const QString& materialName, bool enabled);
    
    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QLabel* m_titleLabel;
    QPushButton* m_addMaterialButton;
    QTabWidget* m_materialTabWidget;
    QScrollArea* m_scrollArea;
    
    // Data
    MaterialManager* m_materialManager;
    QMap<QString, MaterialSpecificCuttingDataTab*> m_materialTabs;
};

// Individual material tab for cutting data
class MaterialSpecificCuttingDataTab : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialSpecificCuttingDataTab(const QString& materialName, QWidget *parent = nullptr);
    ~MaterialSpecificCuttingDataTab();

    // Get/Set material cutting data
    void setCuttingData(const IntuiCAM::Toolpath::MaterialSpecificCuttingData& data);
    IntuiCAM::Toolpath::MaterialSpecificCuttingData getCuttingData() const;
    
    // Enable/disable material
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Get material name
    QString getMaterialName() const { return m_materialName; }

signals:
    void enabledChanged(const QString& materialName, bool enabled);
    void cuttingDataChanged();

private slots:
    void onEnabledChanged(bool enabled);
    void onConstantSurfaceSpeedToggled(bool enabled);
    void onFeedPerRevolutionToggled(bool enabled);
    void onParameterChanged();

private:
    void setupUI();
    void createEnabledGroup();
    void createSpeedControlGroup();
    void createFeedControlGroup();
    void createCuttingLimitsGroup();
    void createCoolantGroup();
    void setupConnections();
    void updateUIState();
    void updateFeedRateUnits(bool feedPerRevolution);
    void connectParameterSignals();

    // UI Components
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    
    // Enabled Group
    QGroupBox* m_enabledGroup;
    QCheckBox* m_enabledCheckBox;
    QLabel* m_statusLabel;
    
    // Speed Control Group
    QGroupBox* m_speedControlGroup;
    QCheckBox* m_constantSurfaceSpeedCheckBox;
    QDoubleSpinBox* m_surfaceSpeedSpin;
    QSpinBox* m_spindleRPMSpin;
    QLabel* m_surfaceSpeedLabel;
    QLabel* m_spindleRPMLabel;
    
    // Feed Control Group
    QGroupBox* m_feedControlGroup;
    QCheckBox* m_feedPerRevolutionCheckBox;
    QDoubleSpinBox* m_cuttingFeedrateSpin;
    QDoubleSpinBox* m_leadInFeedrateSpin;
    QDoubleSpinBox* m_leadOutFeedrateSpin;
    QLabel* m_cuttingFeedrateLabel;
    QLabel* m_leadInFeedrateLabel;
    QLabel* m_leadOutFeedrateLabel;
    
    // Cutting Limits Group
    QGroupBox* m_cuttingLimitsGroup;
    QDoubleSpinBox* m_maxDepthOfCutSpin;
    QDoubleSpinBox* m_maxFeedrateSpin;
    QDoubleSpinBox* m_minSurfaceSpeedSpin;
    QDoubleSpinBox* m_maxSurfaceSpeedSpin;
    
    // Coolant Group
    QGroupBox* m_coolantGroup;
    QCheckBox* m_floodCoolantCheckBox;
    QCheckBox* m_mistCoolantCheckBox;
    QComboBox* m_preferredCoolantCombo;
    QDoubleSpinBox* m_coolantPressureSpin;
    QDoubleSpinBox* m_coolantFlowSpin;
    
    // Data
    QString m_materialName;
    bool m_isEnabled;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // MATERIALSPECIFICCUTTINGDATAWIDGET_H 