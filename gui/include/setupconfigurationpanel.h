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
#include <QStackedWidget>

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

struct WorkpieceGeometry {
    // Raw material geometry (from RawMaterialManager)
    double rawMaterialDiameter = 50.0;          // mm - Current raw material diameter
    double rawMaterialLength = 100.0;           // mm - Total raw material length
    double chuckExtension = 50.0;               // mm - How far raw material extends into chuck (negative Z)
    double facingAllowanceStock = 10.0;         // mm - Extra stock for facing operations (positive Z)
    
    // Work coordinate system (from WorkspaceController)
    double chuckFaceZ = 0.0;                    // mm - Chuck face position (typically Z=0)
    double workOriginZ = 0.0;                   // mm - Work coordinate origin position
    double rawMaterialStartZ = -50.0;           // mm - Raw material start position (into chuck)
    double rawMaterialEndZ = 50.0;              // mm - Raw material end position (workpiece end + allowance)
    
    // Part geometry (from WorkpieceManager analysis)
    double partLength = 40.0;                   // mm - Actual part length along Z axis
    double partMaxDiameter = 20.0;              // mm - Maximum part diameter
    double partMinDiameter = 0.0;               // mm - Minimum part diameter (center hole)
    double partStartZ = 0.0;                    // mm - Part start position
    double partEndZ = 40.0;                     // mm - Part end position
    
    // Workpiece positioning
    double distanceToChuck = 25.0;              // mm - Distance from chuck face to part
    bool orientationFlipped = false;            // Part orientation flip status
    
    // Validation flags
    bool hasValidRawMaterial = false;           // Raw material has been calculated
    bool hasValidWorkpiece = false;             // Workpiece has been loaded and analyzed
    bool hasValidCoordinateSystem = false;     // Work coordinate system initialized
};

class SetupConfigurationPanel : public QWidget {
  Q_OBJECT

public:
  explicit SetupConfigurationPanel(QWidget *parent = nullptr);
  ~SetupConfigurationPanel();

  // Parameter structures for each operation type
  struct FacingParameters {
      // Basic geometry and positioning (from raw material and part analysis)
      double startZ = 0.0;                        // Z position to start facing (mm)
      double endZ = -2.0;                         // Z position to end facing (mm) 
      double maxRadius = 25.0;                    // Maximum radius to face (mm)
      double minRadius = 0.0;                     // Minimum radius (center) (mm)
      double stockAllowance = 0.1;                // Stock allowance for roughing (mm)
      double finalStockAllowance = 0.02;          // Final stock allowance for finishing (mm)
      
      // Cutting strategy and parameters
      int facingStrategy = 0;                     // 0=InsideOut, 1=OutsideIn, 2=Conventional, 3=Climb
      int surfaceQuality = 1;                     // 0=Rough, 1=Medium, 2=Fine, 3=Mirror
      int chipControl = 0;                        // 0=None, 1=ChipBreaking, 2=HighPressureCoolant
      
      // Cutting parameters
      double depthOfCut = 0.5;                    // Depth of cut per pass (mm)
      double radialStepover = 0.8;                // Radial stepover (mm)
      double axialStepover = 0.3;                 // Axial stepover for multi-pass (mm)
      double feedRate = 0.15;                     // Primary feed rate (mm/rev)
      double finishingFeedRate = 0.08;            // Finishing pass feed rate (mm/rev)
      double roughingFeedRate = 0.25;             // Roughing pass feed rate (mm/rev)
      
      // Speed and feed optimization
      double surfaceSpeed = 200.0;                // Surface speed (m/min)
      double minSpindleSpeed = 200.0;             // Minimum spindle speed (RPM)
      double maxSpindleSpeed = 3000.0;            // Maximum spindle speed (RPM)
      bool constantSurfaceSpeed = true;           // Enable constant surface speed
      
      // Pass management
      int numberOfRoughingPasses = 2;            // Number of roughing passes
      bool enableFinishingPass = true;           // Enable finishing pass
      bool enableSpringPass = true;              // Enable final spring pass
      
      // Safety parameters
      double safetyHeight = 2.0;                  // Safety height above part (mm)
      double rapidClearance = 1.0;                // Rapid move clearance (mm)
      double approachDistance = 0.5;              // Tool approach distance (mm)
      double retractDistance = 0.5;               // Tool retract distance (mm)
      
      // Quality control
      double toleranceZ = 0.02;                   // Z-axis tolerance (mm)
      double toleranceX = 0.02;                   // X-axis tolerance (mm)
      bool enableQualityChecks = true;           // Enable quality verification
      
      // Advanced options
      bool enableBackCut = false;                // Enable back-cutting for cleanup
      double dwellTime = 0.0;                     // Dwell time at finish (seconds)
      bool enableCoolant = true;                 // Enable coolant
      int coolantMode = 1;                       // 0=Flood, 1=Mist, 2=HighPressure
  };

  struct RoughingParameters {
      // Geometry parameters (calculated from part and raw material)
      double startDiameter = 50.0;               // Starting diameter (raw material) (mm)
      double endDiameter = 20.0;                 // Final diameter (part) (mm)
      double startZ = 0.0;                       // Z position to start (mm)
      double endZ = -40.0;                       // Z position to end (mm)
      bool isInternal = false;                   // true for internal roughing, false for external
      
      // Cutting strategy
      double depthOfCut = 2.0;                   // Axial depth per pass (mm)
      double stepover = 1.5;                     // Radial stepover (mm)
      double stockAllowance = 0.2;               // Stock left for finishing (mm)
      int roughingStrategy = 0;                  // 0=ParallelPasses, 1=ProfileFollowing, 2=Adaptive
      
      // Cutting parameters
      double feedRate = 0.25;                    // Feed rate (mm/rev)
      double surfaceSpeed = 180.0;               // Surface speed (m/min)
      double minSpindleSpeed = 200.0;            // Minimum spindle speed (RPM)
      double maxSpindleSpeed = 2500.0;           // Maximum spindle speed (RPM)
      bool constantSurfaceSpeed = true;          // Enable constant surface speed
      
      // Pass management
      int numberOfPasses = 4;                    // Total number of passes
      bool enableReversePass = false;            // Enable reverse cutting direction
      
      // Safety parameters
      double safetyHeight = 3.0;                 // Safety height above part (mm)
      double rapidClearance = 2.0;               // Rapid move clearance (mm)
      
      // Profile following options (for complex profiles)
      bool enableProfileFollowing = false;      // Follow complex part profiles
      double profileTolerance = 0.05;           // Profile following tolerance (mm)
      
      // Chip breaking
      bool enableChipBreaking = false;          // Enable chip breaking cycles
      double chipBreakingDistance = 0.1;        // Chip breaking retract distance (mm)
      int chipBreakingFrequency = 5;            // Passes between chip breaks
      
      // Quality control
      double toleranceZ = 0.05;                 // Z-axis tolerance (mm)
      double toleranceX = 0.05;                 // X-axis tolerance (mm)
  };

  struct FinishingParameters {
      // Profile and geometry parameters
      double startZ = 0.0;                       // Z position to start finishing (mm)
      double endZ = -50.0;                       // Z position to end finishing (mm)
      double stockAllowance = 0.05;              // Material left by roughing operation (mm)
      double finalStockAllowance = 0.0;          // Final material allowance (mm)
      
      // Finishing strategy
      int finishingStrategy = 1;                 // 0=SinglePass, 1=MultiPass, 2=ProfileFollowing, 3=AdaptiveFinishing
      int targetQuality = 1;                     // 0=Rough, 1=Medium, 2=Fine, 3=Mirror
      bool enableSpringPass = true;              // Enable final spring pass
      int numberOfPasses = 2;                    // Number of finishing passes
      
      // Cutting parameters
      double depthOfCut = 0.1;                   // Depth of cut per pass (mm)
      double feedRate = 0.08;                    // Primary feed rate (mm/rev)
      double finishingFeedRate = 0.05;           // Final pass feed rate (mm/rev)
      double surfaceSpeed = 250.0;               // Surface speed (m/min)
      double minSpindleSpeed = 300.0;            // Minimum spindle speed (RPM)
      double maxSpindleSpeed = 4000.0;           // Maximum spindle speed (RPM)
      bool constantSurfaceSpeed = true;          // Enable constant surface speed
      
      // Surface quality control
      double surfaceFinishTarget = 1.6;          // Target surface finish (Ra, μm)
      int surfaceFinishMethod = 0;               // 0=Conventional, 1=Climb, 2=Oscillating
      
      // Tool compensation
      bool enableToolRadiusCompensation = true; // Enable G41/G42 compensation
      double toolRadiusOffset = 0.0;            // Additional tool radius offset (mm)
      
      // Safety parameters
      double safetyHeight = 1.0;                 // Safety height above part (mm)
      double rapidClearance = 0.5;               // Rapid move clearance (mm)
      
      // Quality control
      double toleranceZ = 0.01;                 // Z-axis tolerance (mm)
      double toleranceX = 0.01;                 // X-axis tolerance (mm)
      bool enableQualityChecks = true;          // Enable dimensional verification
      
      // Dwell and advanced options
      double dwellTime = 0.2;                   // Dwell time at corners (seconds)
      bool enableVibrationDamping = false;     // Enable vibration damping
      bool enableAdaptiveSpeed = false;        // Enable adaptive speed control
  };

  struct PartingParameters {
      // Basic parting geometry (calculated from part analysis and user input)
      double partingDiameter = 20.0;             // Diameter to part at (mm)
      double partingZ = -40.0;                   // Z position for parting (mm)
      double centerHoleDiameter = 0.0;           // Center hole diameter (0 for solid) (mm)
      double partingWidth = 3.0;                 // Width of parting cut (mm)
      
      // Parting strategy
      int partingStrategy = 0;                   // 0=Straight, 1=Stepped, 2=Groove, 3=Undercut, 4=Trepanning
      int approachDirection = 0;                 // 0=Radial, 1=Axial, 2=Angular
      
      // Cutting parameters
      double feedRate = 30.0;                    // Parting feed rate (mm/min)
      double surfaceSpeed = 120.0;               // Surface speed (m/min)
      double spindleSpeed = 1500.0;              // Spindle speed (RPM)
      
      // Advanced parting options
      double peckDepth = 0.5;                    // Peck drilling depth (mm)
      double retractDistance = 0.2;              // Retract distance between pecks (mm)
      bool enablePecking = true;                 // Enable peck parting
      int numberOfSteps = 3;                     // Number of parting steps (for stepped)
      
      // Safety and finishing
      double safetyDistance = 1.0;               // Safety distance from part (mm)
      bool enableFinishingPass = true;          // Enable finishing pass
      double finishingFeedRate = 15.0;          // Finishing pass feed rate (mm/min)
      
      // Quality control
      double toleranceZ = 0.02;                 // Z-axis tolerance (mm)
      double partingTolerance = 0.05;           // Parting diameter tolerance (mm)
  };

  struct ThreadingParameters {
      // Thread specifications
      int threadForm = 0;                        // 0=Metric, 1=UNC, 2=UNF, 3=BSW, 4=ACME, 5=Trapezoidal, 6=Custom
      int threadType = 0;                        // 0=External, 1=Internal
      int cuttingMethod = 0;                     // 0=SinglePoint, 1=MultiPoint, 2=ChaseThreading
      
      double majorDiameter = 20.0;               // Major diameter (mm)
      double pitch = 1.5;                        // Thread pitch (mm)
      double threadDepth = 0.9;                  // Thread depth (mm)
      double threadLength = 15.0;                // Length of threaded section (mm)
      double startZ = 0.0;                       // Z position to start threading (mm)
      double endZ = 15.0;                        // Z position to end threading (mm)
      
      // Threading strategy
      int numberOfPasses = 6;                    // Number of threading passes
      double firstPassDepth = 0.2;               // Depth of first pass (mm)
      double finalPassDepth = 0.05;              // Depth of final pass (mm)
      bool enableSpringPasses = true;           // Enable spring passes
      int springPassCount = 2;                   // Number of spring passes
      
      // Cutting parameters
      double feedRate = 1.5;                     // Feed rate = pitch (mm/rev)
      double spindleSpeed = 800.0;               // Spindle speed (RPM)
      double surfaceSpeed = 80.0;                // Surface speed (m/min)
      
      // Lead in/out
      double leadInDistance = 2.0;               // Lead-in distance (mm)
      double leadOutDistance = 2.0;              // Lead-out distance (mm)
      double leadInAngle = 45.0;                 // Lead-in angle (degrees)
      double leadOutAngle = 45.0;                // Lead-out angle (degrees)
      
      // Quality and safety
      double threadTolerance = 0.05;             // Thread tolerance (mm)
      bool enableCoolant = true;                // Enable coolant for threading
      bool synchronizedSpindle = true;          // Use spindle synchronization
  };

  struct ChamferingParameters {
      // Chamfer geometry
      double targetDiameter = 20.0;              // Target diameter for chamfering (mm)
      double chamferLength = 1.0;                // Chamfer length/distance (mm)
      double chamferAngle = 45.0;                // Chamfer angle (degrees)
      double startZ = 0.0;                       // Z position to start chamfering (mm)
      double endZ = 0.0;                         // Z position to end chamfering (mm)
      
      // Chamfer type and strategy
      int chamferType = 0;                       // 0=External, 1=Internal, 2=Face
      int chamferStrategy = 0;                   // 0=SinglePass, 1=MultiPass, 2=ProfileFollowing
      
      // Cutting parameters
      double feedRate = 0.1;                     // Feed rate (mm/rev)
      double surfaceSpeed = 200.0;               // Surface speed (m/min)
      double spindleSpeed = 2000.0;              // Spindle speed (RPM)
      
      // Quality parameters
      double toleranceChamfer = 0.02;            // Chamfer tolerance (mm)
      int surfaceFinish = 1;                     // 0=Rough, 1=Medium, 2=Fine
  };

  struct GroovingParameters {
      // Groove geometry
      double grooveDiameter = 18.0;              // Diameter at groove center (mm)
      double grooveWidth = 2.0;                  // Groove width (mm)
      double grooveDepth = 1.0;                  // Groove depth (mm)
      double grooveZ = -20.0;                    // Z position of groove center (mm)
      
      // Groove type and strategy
      int grooveType = 0;                        // 0=External, 1=Internal, 2=Face
      int grooveProfile = 0;                     // 0=Rectangular, 1=Rounded, 2=VGroove, 3=Custom
      
      // Cutting parameters
      double feedRate = 0.05;                    // Feed rate (mm/rev)
      double surfaceSpeed = 150.0;               // Surface speed (m/min)
      double spindleSpeed = 1500.0;              // Spindle speed (RPM)
      
      // Advanced grooving
      bool enablePecking = true;                 // Enable peck grooving
      double peckDepth = 0.2;                    // Peck depth per pass (mm)
      double retractDistance = 0.1;              // Retract distance (mm)
      
      // Quality parameters
      double toleranceGroove = 0.02;             // Groove tolerance (mm)
      int surfaceFinish = 1;                     // 0=Rough, 1=Medium, 2=Fine
  };

  struct DrillingParameters {
      // Drilling geometry
      double drillDiameter = 6.0;                // Drill diameter (mm)
      double drillDepth = 20.0;                  // Drilling depth (mm)
      double startZ = 0.0;                       // Z position to start drilling (mm)
      double endZ = -20.0;                       // Z position to end drilling (mm)
      bool hasCenterHole = false;                // Part already has center hole
      
      // Drilling strategy
      int drillingStrategy = 0;                  // 0=Conventional, 1=Peck, 2=DeepHole, 3=HighSpeed
      
      // Cutting parameters
      double feedRate = 0.15;                    // Feed rate (mm/rev)
      double spindleSpeed = 1200.0;              // Spindle speed (RPM)
      double surfaceSpeed = 60.0;                // Surface speed (m/min)
      
      // Peck drilling parameters
      bool enablePeckDrilling = true;           // Enable peck drilling
      double peckDepth = 2.0;                   // Peck depth per cycle (mm)
      double retractDistance = 1.0;             // Retract distance (mm)
      double dwellTime = 0.1;                   // Dwell time at bottom (seconds)
      
      // Quality and safety
      double toleranceDrilling = 0.05;          // Drilling tolerance (mm)
      bool enableCoolant = true;                // Enable coolant
  };

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

  // Additional pipeline parameter getters
  double getRawMaterialLength() const;
  double getPartLength() const;

  // Operation parameter getters (return complete, UI-configured parameters)
  FacingParameters getFacingParameters() const;
  RoughingParameters getRoughingParameters() const;
  FinishingParameters getFinishingParameters() const;
  PartingParameters getPartingParameters() const;
  ThreadingParameters getThreadingParameters() const;
  ChamferingParameters getChamferingParameters() const;
  GroovingParameters getGroovingParameters() const;
  DrillingParameters getDrillingParameters() const;

  // Workpiece geometry integration
  WorkpieceGeometry getWorkpieceGeometry() const;
  void setWorkpieceGeometry(const WorkpieceGeometry& geometry);
  void updateOperationParametersWithGeometry(const WorkpieceGeometry& geometry);
  
  // Parameter calculation with geometry
  void calculateFacingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateRoughingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateFinishingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculatePartingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateThreadingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateChamferingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateGroovingParametersFromGeometry(const WorkpieceGeometry& geometry);
  void calculateDrillingParametersFromGeometry(const WorkpieceGeometry& geometry);
  
  // UI-based parameter collection (reads actual UI values)
  FacingParameters collectFacingParametersFromUI() const;
  RoughingParameters collectRoughingParametersFromUI() const;
  FinishingParameters collectFinishingParametersFromUI() const;
  PartingParameters collectPartingParametersFromUI() const;
  ThreadingParameters collectThreadingParametersFromUI() const;
  ChamferingParameters collectChamferingParametersFromUI() const;
  GroovingParameters collectGroovingParametersFromUI() const;
  DrillingParameters collectDrillingParametersFromUI() const;

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
  void showOperationWidget(const QString &operationName);

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
  QStackedWidget *m_operationsStackedWidget;
  QWidget *m_facingTab;
  QWidget *m_roughingTab;
  QWidget *m_finishingTab;
  QWidget *m_leftCleanupTab;
  QWidget *m_neutralCleanupTab;
  QWidget *m_threadingTab;
  QWidget *m_chamferingTab;
  QWidget *m_partingTab;
  
  // Operation selection state
  QString m_currentSelectedOperation;

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

  // Workpiece geometry and coordinate system information
  WorkpieceGeometry m_workpieceGeometry;
  
  // Operation parameter structures
  FacingParameters m_facingParams;
  RoughingParameters m_roughingParams;
  FinishingParameters m_finishingParams;
  PartingParameters m_partingParams;
  ThreadingParameters m_threadingParams;
  ChamferingParameters m_chamferingParams;
  GroovingParameters m_groovingParams;
  DrillingParameters m_drillingParams;

};

} // namespace GUI
} // namespace IntuiCAM

#endif // SETUPCONFIGURATIONPANEL_H
