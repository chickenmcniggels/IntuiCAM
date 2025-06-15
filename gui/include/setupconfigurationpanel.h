#ifndef SETUPCONFIGURATIONPANEL_H
#define SETUPCONFIGURATIONPANEL_H

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

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
  Rough_32Ra,  // 32 μm Ra (rough machining)
  Medium_16Ra, // 16 μm Ra (standard machining)
  Fine_8Ra,    // 8 μm Ra (finish machining)
  Smooth_4Ra,  // 4 μm Ra (precision machining)
  Polish_2Ra,  // 2 μm Ra (polished)
  Mirror_1Ra   // 1 μm Ra (mirror finish)
};

struct OperationConfig {
  bool enabled;
  QString name;
  QString description;
  QStringList parameters;
};

class SetupConfigurationPanel : public QWidget {
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
  double getFacingAllowance() const;
  double getRoughingAllowance() const;
  double getFinishingAllowance() const;
  double getPartingWidth() const;
  SurfaceFinish getSurfaceFinish() const;
  double getTolerance() const;
  bool isOperationEnabled(const QString &operationName) const;
  OperationConfig getOperationConfig(const QString &operationName) const;

  // Setters
  void setStepFilePath(const QString &path);
  void setMaterialType(MaterialType type);
  void setRawDiameter(double diameter);
  void setDistanceToChuck(double distance);
  void setOrientationFlipped(bool flipped);
  void updateRawMaterialLength(double length);
  void setFacingAllowance(double allowance);
  void setRoughingAllowance(double allowance);
  void setFinishingAllowance(double allowance);
  void setPartingWidth(double width);
  void setSurfaceFinish(SurfaceFinish finish);
  void setTolerance(double tolerance);
  void setOperationEnabled(const QString &operationName, bool enabled);
  void updateAxisInfo(const QString &info);

  // Material and Tool Management
  void setMaterialManager(MaterialManager *materialManager);
  void setToolManager(ToolManager *toolManager);
  QString getSelectedMaterialName() const;
  QStringList getRecommendedTools() const;
  void updateMaterialProperties();
  void updateToolRecommendations();
  void focusOperationTab(const QString &operationName);

  // Utility methods
  static QString materialTypeToString(MaterialType type);
  static MaterialType stringToMaterialType(const QString &typeStr);
  static QString surfaceFinishToString(SurfaceFinish finish);
  static SurfaceFinish stringToSurfaceFinish(const QString &finishStr);

signals:
  void configurationChanged();
  void stepFileSelected(const QString &filePath);
  void materialTypeChanged(MaterialType type);
  void rawMaterialDiameterChanged(double diameter);
  void distanceToChuckChanged(double distance);
  void orientationFlipped(bool flipped);
  void manualAxisSelectionRequested();
  void operationToggled(const QString &operationName, bool enabled);
  void automaticToolpathGenerationRequested();
  void materialSelectionChanged(const QString &materialName);
  void toolRecommendationsUpdated(const QStringList &toolIds);

public slots:
  void onBrowseStepFile();
  void onConfigurationChanged();
  void onManualAxisSelectionClicked();
  void onOperationToggled();
  void onMaterialChanged();
  void onToolSelectionRequested();

private:
  void setupUI();
  void setupPartTab();
  void setupMachiningTab(); // now sets up operation-specific tabs
  void setupConnections();
  void applyTabStyling();
  void updateOperationControls();
  void updateAdvancedMode();

  // Main layout and tabs
  QVBoxLayout *m_mainLayout;
  QWidget *m_partTab;
  QTabWidget *m_operationsTabWidget;
  QWidget *m_contouringTab;
  QWidget *m_threadingTab;
  QWidget *m_chamferingTab;
  QWidget *m_partingTab;

  // Part Tab Components (Part Setup + Material Settings)
  QGroupBox *m_partSetupGroup;
  QVBoxLayout *m_partSetupLayout;
  QHBoxLayout *m_stepFileLayout;
  QLineEdit *m_stepFileEdit;
  QPushButton *m_browseButton;
  QPushButton *m_manualAxisButton;
  QLabel *m_axisInfoLabel;

  // Part positioning controls
  QHBoxLayout *m_distanceLayout;
  QLabel *m_distanceLabel;
  QSlider *m_distanceSlider;
  QDoubleSpinBox *m_distanceSpinBox;
  QCheckBox *m_flipOrientationCheckBox;

  QGroupBox *m_materialGroup;
  QVBoxLayout *m_materialLayout;
  QHBoxLayout *m_materialTypeLayout;
  QLabel *m_materialTypeLabel;
  QComboBox *m_materialTypeCombo;
  QHBoxLayout *m_rawDiameterLayout;
  QLabel *m_rawDiameterLabel;
  QDoubleSpinBox *m_rawDiameterSpin;
  QLabel *m_rawLengthLabel; // Now just a label showing auto-calculated length

  // Machining Tab Components (Machining Parameters + Operations + Quality)
  QGroupBox *m_machiningParamsGroup;
  QVBoxLayout *m_machiningParamsLayout;
  QHBoxLayout *m_facingAllowanceLayout;
  QLabel *m_facingAllowanceLabel;
  QDoubleSpinBox *m_facingAllowanceSpin;
  QHBoxLayout *m_roughingAllowanceLayout;
  QLabel *m_roughingAllowanceLabel;
  QDoubleSpinBox *m_roughingAllowanceSpin;
  QHBoxLayout *m_finishingAllowanceLayout;
  QLabel *m_finishingAllowanceLabel;
  QDoubleSpinBox *m_finishingAllowanceSpin;
  QHBoxLayout *m_partingWidthLayout;
  QLabel *m_partingWidthLabel;
  QDoubleSpinBox *m_partingWidthSpin;

  // Advanced cutting parameter widgets
  QGroupBox *m_contourAdvancedGroup;
  QDoubleSpinBox *m_contourDepthSpin;
  QDoubleSpinBox *m_contourFeedSpin;
  QDoubleSpinBox *m_contourSpeedSpin;

  // Advanced mode toggle
  QCheckBox *m_advancedModeCheck;

  // Simplified parameters
  QSpinBox *m_finishingPassesSpin;

  // Threading face table
  QTableWidget *m_threadFacesTable;
  QPushButton *m_addThreadFaceButton;
  QPushButton *m_removeThreadFaceButton;

  // Chamfering face table
  QTableWidget *m_chamferFacesTable;
  QPushButton *m_addChamferFaceButton;
  QPushButton *m_removeChamferFaceButton;
  QDoubleSpinBox *m_extraChamferStockSpin;
  QDoubleSpinBox *m_chamferDiameterLeaveSpin;

  // Legacy placeholders to preserve binary compatibility
  QGroupBox *m_operationsGroup;
  QVBoxLayout *m_operationsLayout;
  QCheckBox *m_contouringEnabledCheck;
  QCheckBox *m_threadingEnabledCheck;
  QDoubleSpinBox *m_threadPitchSpin;
  QCheckBox *m_chamferingEnabledCheck;
  QDoubleSpinBox *m_chamferSizeSpin;
  QCheckBox *m_partingEnabledCheck;

  QGroupBox *m_qualityGroup;
  QVBoxLayout *m_qualityLayout;
  QHBoxLayout *m_surfaceFinishLayout;
  QLabel *m_surfaceFinishLabel;
  QComboBox *m_surfaceFinishCombo;
  QHBoxLayout *m_toleranceLayout;
  QLabel *m_toleranceLabel;
  QDoubleSpinBox *m_toleranceSpin;

  // Material and Tool Management Integration
  MaterialManager *m_materialManager;
  ToolManager *m_toolManager;
  QMap<QString, QListWidget *> m_operationToolLists;

  // Enhanced material display
  QLabel *m_materialPropertiesLabel;
  QPushButton *m_materialDetailsButton;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // SETUPCONFIGURATIONPANEL_H
