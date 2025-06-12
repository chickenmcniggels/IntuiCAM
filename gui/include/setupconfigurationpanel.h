#ifndef SETUPCONFIGURATIONPANEL_H
#define SETUPCONFIGURATIONPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QFrame>
#include <QTabWidget>

// Forward declarations
class QListWidget;

// Include manager headers
#include "materialmanager.h"
#include "toolmanager.h"

namespace IntuiCAM {
namespace GUI {

enum class MaterialType {
    Aluminum6061,
    Aluminum7075,
    Steel1018,
    Steel4140,
    StainlessSteel316,
    StainlessSteel304,
    Brass360,
    Bronze,
    Titanium,
    Plastic_ABS,
    Plastic_Delrin,
    Custom
};

enum class SurfaceFinish {
    Rough_32Ra,     // 32 μm Ra (rough machining)
    Medium_16Ra,    // 16 μm Ra (standard machining)
    Fine_8Ra,       // 8 μm Ra (finish machining)
    Smooth_4Ra,     // 4 μm Ra (precision machining)
    Polish_2Ra,     // 2 μm Ra (polished)
    Mirror_1Ra      // 1 μm Ra (mirror finish)
};

struct OperationConfig {
    bool enabled;
    QString name;
    QString description;
    QStringList parameters;
};

class SetupConfigurationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SetupConfigurationPanel(QWidget *parent = nullptr);
    ~SetupConfigurationPanel();

    // Getters
    QString getStepFilePath() const;
    MaterialType getMaterialType() const;
    double getRawDiameter() const;
    double getDistanceToChuck() const;
    bool isOrientationFlipped() const;

    // Setters
    void setStepFilePath(const QString& path);
    void setMaterialType(MaterialType type);
    void setRawDiameter(double diameter);
    void setDistanceToChuck(double distance);
    void setOrientationFlipped(bool flipped);
    void updateRawMaterialLength(double length);
    void updateAxisInfo(const QString& info);

    // Material and Tool Management
    void setMaterialManager(MaterialManager* materialManager);
    void setToolManager(ToolManager* toolManager);
    QString getSelectedMaterialName() const;
    QStringList getRecommendedTools() const;
    void updateMaterialProperties();
    void updateToolRecommendations();

    // Utility methods
    static QString materialTypeToString(MaterialType type);
    static MaterialType stringToMaterialType(const QString& typeStr);
    static QString surfaceFinishToString(SurfaceFinish finish);
    static SurfaceFinish stringToSurfaceFinish(const QString& finishStr);

signals:
    void configurationChanged();
    void stepFileSelected(const QString& filePath);
    void materialTypeChanged(MaterialType type);
    void rawMaterialDiameterChanged(double diameter);
    void distanceToChuckChanged(double distance);
    void orientationFlipped(bool flipped);
    void manualAxisSelectionRequested();
    void automaticToolpathGenerationRequested();
    void materialSelectionChanged(const QString& materialName);
    void toolRecommendationsUpdated(const QStringList& toolIds);

public slots:
    void onBrowseStepFile();
    void onConfigurationChanged();
    void onManualAxisSelectionClicked();
    void onGenerateToolpaths();
    void onMaterialChanged();
    void onToolSelectionRequested();

private:
    void setupUI();
    void setupPartSection();
    void setupOperationTabs();
    void setupConnections();
    void applyTabStyling();

    // Main layout and tab widgets
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_operationTabs;
    QWidget* m_contouringTab;
    QWidget* m_threadingTab;
    QWidget* m_chamferingTab;
    QWidget* m_partingTab;

    // Part Tab Components (Part Setup + Material Settings)
    QGroupBox* m_partSetupGroup;
    QVBoxLayout* m_partSetupLayout;
    QHBoxLayout* m_stepFileLayout;
    QLineEdit* m_stepFileEdit;
    QPushButton* m_browseButton;
    QPushButton* m_manualAxisButton;
    QLabel* m_axisInfoLabel;
    
    // Part positioning controls
    QHBoxLayout* m_distanceLayout;
    QLabel* m_distanceLabel;
    QSlider* m_distanceSlider;
    QDoubleSpinBox* m_distanceSpinBox;
    QCheckBox* m_flipOrientationCheckBox;

    QGroupBox* m_materialGroup;
    QVBoxLayout* m_materialLayout;
    QHBoxLayout* m_materialTypeLayout;
    QLabel* m_materialTypeLabel;
    QComboBox* m_materialTypeCombo;
    QHBoxLayout* m_rawDiameterLayout;
    QLabel* m_rawDiameterLabel;
    QDoubleSpinBox* m_rawDiameterSpin;
    QLabel* m_rawLengthLabel; // Now just a label showing auto-calculated length

    // Generation Controls
    QFrame* m_generationFrame;
    QVBoxLayout* m_generationLayout;
    QPushButton* m_generateButton;

    // Material and Tool Management Integration
    MaterialManager* m_materialManager;
    ToolManager* m_toolManager;
    QGroupBox* m_toolSelectionGroup;
    QVBoxLayout* m_toolLayout;
    QLabel* m_recommendedToolsLabel;
    QListWidget* m_recommendedToolsList;
    QPushButton* m_toolDetailsButton;
    QPushButton* m_customizeToolsButton;

    // Enhanced material display
    QLabel* m_materialPropertiesLabel;
    QPushButton* m_materialDetailsButton;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // SETUPCONFIGURATIONPANEL_H 
