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
#include <QVector>
#include <TopoDS_Shape.hxx>
#include <QVBoxLayout>
#include <QWidget>

// Forward declarations
class QListWidget;
class QListWidgetItem;

// Include manager headers
#include "materialmanager.h"
#include "toolmanager.h"

// Forward declarations for cutting parameters
struct CuttingParameters;
struct CuttingTool;

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

struct ThreadFaceConfig {
  TopoDS_Shape face;
  QString preset;
  double pitch = 1.0;
  double depth = 5.0;
};

struct ChamferFaceConfig {
  QString faceId;
  bool symmetric = true;
  double valueA = 0.5;
  double valueB = 0.5;
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

  // New pipeline-specific getters
  double getLargestDrillSize() const;
  int getInternalFinishingPasses() const;
  int getExternalFinishingPasses() const;
  double getPartingAllowance() const;
  bool isDrillingEnabled() const;
  bool isInternalRoughingEnabled() const;
  bool isExternalRoughingEnabled() const;
  bool isInternalFinishingEnabled() const;
  bool isExternalFinishingEnabled() const;
  bool isInternalGroovingEnabled() const;
  bool isExternalGroovingEnabled() const;
  bool isMachineInternalFeaturesEnabled() const;

  // Additional pipeline parameters
  double getRawMaterialLength() const;
  double getPartLength() const;

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

  // New pipeline-specific setters
  void setLargestDrillSize(double size);
  void setInternalFinishingPasses(int passes);
  void setExternalFinishingPasses(int passes);
  void setPartingAllowance(double allowance);
  void setDrillingEnabled(bool enabled);
  void setInternalRoughingEnabled(bool enabled);
  void setExternalRoughingEnabled(bool enabled);
  void setInternalFinishingEnabled(bool enabled);
  void setExternalFinishingEnabled(bool enabled);
  void setInternalGroovingEnabled(bool enabled);
  void setExternalGroovingEnabled(bool enabled);
  void setMachineInternalFeaturesEnabled(bool enabled);

  // Additional pipeline parameter setters
  void setRawMaterialLength(double length);
  void setPartLength(double length);

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
  void autoRawDiameterRequested();
  void distanceToChuckChanged(double distance);
  void orientationFlipped(bool flipped);
  void manualAxisSelectionRequested();
  void operationToggled(const QString &operationName, bool enabled);
  void materialSelectionChanged(const QString &materialName);
  void toolRecommendationsUpdated(const QStringList &toolIds);
  void recommendedToolActivated(const QString &toolId);
  void requestThreadFaceSelection();
  void threadFaceSelected(const TopoDS_Shape &face);
  void threadFaceDeselected();
  void chamferFaceSelected(const QString &faceId);

public slots:
  void onBrowseStepFile();
  void onConfigurationChanged();
  void onManualAxisSelectionClicked();
  void onAutoRawDiameterClicked();
  void onOperationToggled();
  void onMaterialChanged();
  void onToolSelectionRequested();
  void onRecommendedToolDoubleClicked(QListWidgetItem *item);
  void onAddThreadFace();
  void addSelectedThreadFace(const TopoDS_Shape &face);
  void onRemoveThreadFace();
  void onAddChamferFace();
  void onRemoveChamferFace();
  void onThreadFaceRowSelected();
  void onThreadFaceCellChanged(int row, int column);
  void onChamferFaceRowSelected();

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
  QWidget *m_facingTab;
  QWidget *m_roughingTab;
  QWidget *m_finishingTab;
  QWidget *m_leftCleanupTab;
  QWidget *m_neutralCleanupTab;
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
  QPushButton *m_autoRawDiameterButton;
  QLabel *m_rawLengthLabel; // Displays current raw material length

  // Machining Parameter panels per operation
  QGroupBox *m_facingParamsGroup;
  QVBoxLayout *m_facingParamsLayout;
  QGroupBox *m_internalRoughingParamsGroup;
  QVBoxLayout *m_internalRoughingParamsLayout;
  QGroupBox *m_internalFinishingParamsGroup;
  QVBoxLayout *m_internalFinishingParamsLayout;
  QGroupBox *m_finishingParamsGroup;
  QVBoxLayout *m_finishingParamsLayout;
  QGroupBox *m_partingParamsGroup;
  QVBoxLayout *m_partingParamsLayout;
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
  // Operation advanced groups
  QGroupBox *m_facingAdvancedGroup;
  QGroupBox *m_roughingAdvancedGroup;
  QGroupBox *m_finishingAdvancedGroup;
  QDoubleSpinBox *m_facingDepthSpin;
  QDoubleSpinBox *m_facingFeedSpin;
  QDoubleSpinBox *m_facingSpeedSpin;
  QCheckBox *m_facingCssCheck;
  QDoubleSpinBox *m_roughingDepthSpin;
  QDoubleSpinBox *m_roughingFeedSpin;
  QDoubleSpinBox *m_roughingSpeedSpin;
  QCheckBox *m_roughingCssCheck;
  QDoubleSpinBox *m_finishingDepthSpin;
  QDoubleSpinBox *m_finishingFeedSpin;
  QDoubleSpinBox *m_finishingSpeedSpin;
  QCheckBox *m_finishingCssCheck;

  // Legacy flat advanced members kept for compatibility
  QDoubleSpinBox *m_contourDepthSpin;
  QDoubleSpinBox *m_contourFeedSpin;
  QDoubleSpinBox *m_contourSpeedSpin;

  // Flood coolant (simple mode)
  QCheckBox *m_contourFloodCheck;
  QCheckBox *m_chamferFloodCheck;
  QCheckBox *m_partFloodCheck;
  QCheckBox *m_threadFloodCheck;

  // Advanced mode toggle
  QCheckBox *m_advancedModeCheck;

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


  // Stored face/edge configurations
  QVector<ThreadFaceConfig> m_threadFaces;
  QVector<ChamferFaceConfig> m_chamferFaces;

  bool m_updatingThreadTable = false;
  
  // Parting advanced group
  QGroupBox *m_partingAdvancedGroup;
  QDoubleSpinBox *m_partingDepthSpin;
  QDoubleSpinBox *m_partingFeedSpin;
  QDoubleSpinBox *m_partingSpeedSpin;
  QCheckBox *m_partingCssCheck;
  QComboBox *m_partingRetractCombo;

  // Legacy placeholders to preserve binary compatibility
  QGroupBox *m_operationsGroup;
  QVBoxLayout *m_operationsLayout;
  QCheckBox *m_facingEnabledCheck;
  QCheckBox *m_roughingEnabledCheck;
  QCheckBox *m_finishingEnabledCheck;
  QCheckBox *m_leftCleanupEnabledCheck;
  QCheckBox *m_neutralCleanupEnabledCheck;
  QCheckBox *m_threadingEnabledCheck;
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

  // Tool selection tracking
  QMap<QString, QString> m_selectedToolsPerOperation; // operation -> toolId
  
  // Methods for tool selection and parameter loading
  void onToolSelectionChanged(const QString& operation, const QString& toolId);
  void loadToolParametersToAdvancedSettings(const QString& toolId, const QString& operation);
  void clearAdvancedSettingsForOperation(const QString& operation);
  bool isToolSelectedForOperation(const QString& operation) const;
  void updateOperationAdvancedSettings(const QString& operation, bool advancedMode);
  void loadContouringParameters(const CuttingParameters& params, const CuttingTool& tool);
  void loadPartingParameters(const CuttingParameters& params, const CuttingTool& tool);

  // New pipeline-specific UI controls
  QDoubleSpinBox *m_largestDrillSizeSpin;
  QSpinBox *m_internalFinishingPassesSpin;
  QSpinBox *m_externalFinishingPassesSpin;
  QDoubleSpinBox *m_partingAllowanceSpin;
  QCheckBox *m_drillingEnabledCheck;
  QCheckBox *m_internalRoughingEnabledCheck;
  QCheckBox *m_externalRoughingEnabledCheck;
  QCheckBox *m_internalFinishingEnabledCheck;
  QCheckBox *m_externalFinishingEnabledCheck;
  QCheckBox *m_internalGroovingEnabledCheck;
  QCheckBox *m_externalGroovingEnabledCheck;
  QCheckBox *m_machineInternalFeaturesEnabledCheck;

  // Additional pipeline parameter storage
  double m_rawMaterialLength = 50.0;  // mm
  double m_partLength = 40.0;         // mm

};

} // namespace GUI
} // namespace IntuiCAM

#endif // SETUPCONFIGURATIONPANEL_H
