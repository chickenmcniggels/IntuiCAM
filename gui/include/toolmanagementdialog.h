#ifndef TOOLMANAGEMENTDIALOG_H
#define TOOLMANAGEMENTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QFormLayout>
#include <QButtonGroup>
#include <QRadioButton>
#include <QProgressBar>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QSlider>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>

// Project includes
#include "opengl3dwidget.h"
#include "IntuiCAM/Toolpath/ToolTypes.h"

// Forward declarations
class QJsonObject;
class QJsonDocument;

namespace IntuiCAM {
namespace Toolpath {
    // Forward declarations from ToolTypes.h
    struct GeneralTurningInsert;
    struct ThreadingInsert;
    struct GroovingInsert;
    struct ToolHolder;
    struct ToolAssembly;
    struct CuttingData;
    enum class InsertShape;
    enum class InsertReliefAngle;
    enum class InsertTolerance;
    enum class InsertMaterial;
    enum class HandOrientation;
    enum class ClampingStyle;
    enum class ToolType;
    enum class ThreadProfile;
    enum class ThreadTipType;
    enum class CoolantType;
}
}

class ToolManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolManagementDialog(QWidget *parent = nullptr);
    ~ToolManagementDialog();

    // Tool management operations
    void addNewTool(IntuiCAM::Toolpath::ToolType toolType = IntuiCAM::Toolpath::ToolType::GENERAL_TURNING);
    void editTool(const QString& toolId);
    void deleteTool(const QString& toolId);
    void duplicateTool(const QString& toolId);
    
    // Tool library operations
    void loadToolLibrary(const QString& filePath);
    void saveToolLibrary(const QString& filePath);
    void importToolsFromCatalog(const QString& catalogPath);
    void exportSelectedTools(const QString& filePath);
    
    // Tool filtering and search
    void filterByToolType(IntuiCAM::Toolpath::ToolType toolType);
    void filterByMaterial(IntuiCAM::Toolpath::InsertMaterial material);
    void filterByManufacturer(const QString& manufacturer);
    void searchTools(const QString& searchTerm);
    void clearFilters();

signals:
    void toolAdded(const QString& toolId);
    void toolModified(const QString& toolId);
    void toolDeleted(const QString& toolId);
    void toolSelected(const QString& toolId);
    void toolLibraryChanged();
    void errorOccurred(const QString& message);
    
    // Phase 4: Advanced Feature Signals
    void tool3DVisualizationChanged(const QString& toolId);
    void toolLifeWarning(const QString& toolId, double remainingPercentage);
    void toolLifeCritical(const QString& toolId, double remainingPercentage);
    void toolMaintenanceScheduled(const QString& toolId, const QString& date);
    void cuttingParametersOptimized(const QString& toolId);
    void toolDeflectionCalculated(const QString& toolId, double deflection);
    void surfaceFinishAnalyzed(const QString& toolId, double predictedFinish);

private slots:
    // Main interface slots
    void onToolListSelectionChanged();
    void onAddToolClicked();
    void onEditToolClicked();
    void onDeleteToolClicked();
    void onDuplicateToolClicked();
    void onToolTypeChanged();
    void onSearchTextChanged(const QString& text);
    
    // Tool editing slots
    void onInsertParameterChanged();
    void onHolderParameterChanged();
    void onCuttingDataChanged();
    void onISOCodeChanged();
    void onManualParametersChanged();
    
    // 3D visualization slots
    void onVisualizationModeChanged(int mode);
    void onViewModeChanged(int mode);
    void onToolGeometryChanged();
    void updateToolVisualization();
    
    // Database and validation slots
    void onValidateISO();
    void onLoadFromDatabase();
    void onSaveToDatabase();
    
    // Import/Export slots
    void onImportLibrary();
    void onExportLibrary();
    void onImportCatalog();
    void onExportCatalog();

    // Phase 4: Advanced Feature Slots
    void on3DViewModeChanged(const QString& mode);
    void on3DViewPlaneChanged(const QString& plane);
    void onViewPlaneLockChanged(bool locked);
    void onShowDimensionsChanged(bool show);
    void onShowAnnotationsChanged(bool show);
    void onZoomChanged(int value);
    void onFitViewClicked();
    void onResetViewClicked();
    
    // Tool Life Management Slots
    void onToolLifeParameterChanged();
    void onScheduleMaintenanceClicked();
    void onResetToolLifeClicked();
    void onGenerateReportClicked();
    void onEnableAlertsChanged(bool enabled);
    void onToolLifeWarning(const QString& toolId);
    void onToolLifeCritical(const QString& toolId);
    
    // Advanced Cutting Data Slots
    void onOptimizeParametersClicked();
    void onCalculateDeflectionClicked();
    void onAnalyzeSurfaceFinishClicked();
    void onWorkpieceMaterialChanged(const QString& material);
    void onOperationTypeChanged(const QString& operation);
    void onSurfaceFinishRequirementChanged(double value);
    void onDeflectionLimitChanged(double value);
    
    // Real-time Update Slots
    void onParameterUpdateTimeout();
    void onVisualizationUpdateTimeout();
    void onToolLifeUpdateTimeout();
    void onRealTimeParameterChanged();

    // Filter slots
    void onFilterChanged();

private:
    // UI Creation methods
    void setupUI();
    void createMainLayout();
    void createToolListPanel();
    void createToolEditPanel();
    void create3DVisualizationPanel();
    void createToolbar();
    void createStatusBar();
    
    // Tool editing panels
    QWidget* createInsertPropertiesTab();
    QWidget* createHolderPropertiesTab();
    QWidget* createCuttingDataTab();
    QWidget* createToolInfoTab();
    void createGeneralTurningPanel();
    void createThreadingPanel();
    void createGroovingPanel();
    void createHolderPanel();
    void createCuttingDataPanel();
    void createToolInfoPanel();
    
    // Tool visualization
    void setup3DViewer();
    void generate3DToolGeometry();
    void generateInsertGeometry(const IntuiCAM::Toolpath::GeneralTurningInsert& insert);
    void generateHolderGeometry(const IntuiCAM::Toolpath::ToolHolder& holder);
    void generateAssemblyGeometry(const IntuiCAM::Toolpath::ToolAssembly& assembly);
    void updateVisualizationModes();
    
    // Phase 4: Advanced 3D Visualization
    void generate3DInsertGeometry(const IntuiCAM::Toolpath::GeneralTurningInsert& insert);
    void generate3DThreadingInsertGeometry(const IntuiCAM::Toolpath::ThreadingInsert& insert);
    void generate3DGroovingInsertGeometry(const IntuiCAM::Toolpath::GroovingInsert& insert);
    void generate3DHolderGeometry(const IntuiCAM::Toolpath::ToolHolder& holder);
    void generate3DAssemblyGeometry(const IntuiCAM::Toolpath::ToolAssembly& assembly);
    void updateRealTime3DVisualization();
    void enable3DViewPlaneLocking(bool locked);
    void set3DViewPlane(const QString& plane); // "XZ", "XY", "YZ"
    void create3DToolCrossSections();
    void highlight3DToolFeatures(const QStringList& features);
    
    // Phase 4: Tool Life Management
    void initializeToolLifeTracking();
    void updateToolLifeDisplay();
    void calculateRemainingToolLife();
    void scheduleToolMaintenance();
    void trackToolUsage(const QString& toolId, double minutes, int cycles);
    void generateToolLifeReport();
    void predictToolReplacement();
    void setToolLifeAlerts(bool enabled);
    void checkToolLifeWarnings();
    
    // Phase 4: Advanced Cutting Data
    void calculateOptimalCuttingParameters();
    void optimizeCuttingDataForMaterial(const QString& material);
    void validateCuttingParameterLimits();
    void generateCuttingDataRecommendations();
    void updateMaterialDatabase();
    void calculateToolDeflection();
    void analyzeSurfaceFinishRequirements();
    void optimizeToolpathParameters();
    
    // Phase 4: Real-time Parameter Updates
    void enableRealTimeUpdates(bool enabled);
    void connectParameterSignals();
    void throttledParameterUpdate();
    void validateParametersInRealTime();
    
    // Data management
    void loadToolsFromDatabase();
    void saveToolsToDatabase();
    void populateToolList();
    void updateToolDetails();
    void clearToolDetails();
    
    // Validation and helpers
    bool validateCurrentTool();
    bool validateISOCode(const QString& isoCode);
    void updateCompatibleHolders();
    void updateRecommendedCuttingData();
    void loadDefaultParameters();
    
    // ISO database integration
    void populateISOInsertSizes();
    void populateISOHolderTypes();
    void populateMaterialGrades();
    void populateCoatingTypes();
    
    // Tool data conversion
    IntuiCAM::Toolpath::ToolAssembly getCurrentToolAssembly();
    void setCurrentToolAssembly(const IntuiCAM::Toolpath::ToolAssembly& assembly);
    
    // Utility functions
    QString formatToolType(IntuiCAM::Toolpath::ToolType toolType);
    
    // Parameter synchronization methods
    void loadToolParametersIntoFields(const IntuiCAM::Toolpath::ToolAssembly& assembly);
    void loadGeneralTurningInsertParameters(const IntuiCAM::Toolpath::GeneralTurningInsert& insert);
    void loadThreadingInsertParameters(const IntuiCAM::Toolpath::ThreadingInsert& insert);
    void loadGroovingInsertParameters(const IntuiCAM::Toolpath::GroovingInsert& insert);
    void loadHolderParameters(const IntuiCAM::Toolpath::ToolHolder& holder);
    void loadCuttingDataParameters(const IntuiCAM::Toolpath::CuttingData& cuttingData);
    
    void updateToolAssemblyFromFields();
    void updateCuttingDataFromFields();
    void updateInsertDataFromFields();
    void updateGeneralTurningInsertFromFields();
    void updateThreadingInsertFromFields();
    void updateGroovingInsertFromFields();
    void updateHolderDataFromFields();
    
    void setComboBoxByValue(QComboBox* comboBox, int value);
    void clearAllParameterFields();
    void updateToolTypeSpecificUI();
    IntuiCAM::Toolpath::ToolAssembly createSampleToolFromId(const QString& toolId);
    
    // File operations
    bool loadToolsFromFile(const QString& filePath);
    bool saveToolsToFile(const QString& filePath);
    
    // UI Components - Main Layout
    QSplitter* m_mainSplitter;
    QVBoxLayout* m_mainLayout;
    
    // Tool List Panel
    QWidget* m_toolListPanel;
    QVBoxLayout* m_toolListLayout;
    QLineEdit* m_searchBox;
    QComboBox* m_toolTypeFilter;
    QComboBox* m_materialFilter;
    QComboBox* m_manufacturerFilter;
    QPushButton* m_clearFiltersButton;
    QTreeWidget* m_toolTreeWidget;
    QListWidget* m_toolListWidget;
    
    // Toolbar
    QHBoxLayout* m_toolbarLayout;
    QPushButton* m_addToolButton;
    QPushButton* m_editToolButton;
    QPushButton* m_deleteToolButton;
    QPushButton* m_duplicateToolButton;
    QPushButton* m_importLibraryButton;
    QPushButton* m_exportLibraryButton;
    QPushButton* m_importCatalogButton;
    
    // Tool Edit Panel
    QWidget* m_toolEditPanel;
    QVBoxLayout* m_toolEditLayout;
    QTabWidget* m_toolEditTabs;
    
    // Insert Tabs
    QWidget* m_insertTab;
    QWidget* m_holderTab;
    QWidget* m_cuttingDataTab;
    QWidget* m_toolInfoTab;
    
    // General Turning Insert Tab
    QWidget* m_turningInsertTab;
    QFormLayout* m_turningInsertLayout;
    QLineEdit* m_isoCodeEdit;
    QComboBox* m_insertShapeCombo;
    QComboBox* m_reliefAngleCombo;
    QComboBox* m_toleranceCombo;
    QComboBox* m_materialCombo;
    QComboBox* m_substrateCombo;
    QComboBox* m_coatingCombo;
    QDoubleSpinBox* m_inscribedCircleSpin;
    QDoubleSpinBox* m_thicknessSpin;
    QDoubleSpinBox* m_cornerRadiusSpin;
    QDoubleSpinBox* m_cuttingEdgeLengthSpin;
    QDoubleSpinBox* m_widthSpin;
    QDoubleSpinBox* m_rakeAngleSpin;
    QDoubleSpinBox* m_inclinationAngleSpin;
    QPushButton* m_loadFromISOButton;
    QPushButton* m_validateISOButton;
    
    // Threading Insert Tab
    QWidget* m_threadingInsertTab;
    QFormLayout* m_threadingInsertLayout;
    QLineEdit* m_threadingISOCodeEdit;
    QComboBox* m_threadingShapeCombo;
    QComboBox* m_threadingToleranceCombo;
    QLineEdit* m_crossSectionEdit;
    QComboBox* m_threadingMaterialCombo;
    QDoubleSpinBox* m_threadingThicknessSpin;
    QDoubleSpinBox* m_threadingWidthSpin;
    QDoubleSpinBox* m_minThreadPitchSpin;
    QDoubleSpinBox* m_maxThreadPitchSpin;
    QCheckBox* m_internalThreadsCheck;
    QCheckBox* m_externalThreadsCheck;
    QComboBox* m_threadProfileCombo;
    QDoubleSpinBox* m_threadProfileAngleSpin;
    QComboBox* m_threadTipTypeCombo;
    QDoubleSpinBox* m_threadTipRadiusSpin;
    
    // Grooving Insert Tab
    QWidget* m_groovingInsertTab;
    QFormLayout* m_groovingInsertLayout;
    QLineEdit* m_groovingISOCodeEdit;
    QComboBox* m_groovingShapeCombo;
    QComboBox* m_groovingToleranceCombo;
    QLineEdit* m_groovingCrossSectionEdit;
    QComboBox* m_groovingMaterialCombo;
    QDoubleSpinBox* m_groovingThicknessSpin;
    QDoubleSpinBox* m_groovingOverallLengthSpin;
    QDoubleSpinBox* m_groovingWidthSpin;
    QDoubleSpinBox* m_groovingCornerRadiusSpin;
    QDoubleSpinBox* m_groovingHeadLengthSpin;
    QDoubleSpinBox* m_grooveWidthSpin;
    
    // Tool Holder Tab
    QFormLayout* m_holderLayout;
    QLineEdit* m_holderISOCodeEdit;
    QComboBox* m_handOrientationCombo;
    QComboBox* m_clampingStyleCombo;
    QDoubleSpinBox* m_cuttingWidthSpin;
    QDoubleSpinBox* m_headLengthSpin;
    QDoubleSpinBox* m_overallLengthSpin;
    QDoubleSpinBox* m_shankWidthSpin;
    QDoubleSpinBox* m_shankHeightSpin;
    QCheckBox* m_roundShankCheck;
    QDoubleSpinBox* m_shankDiameterSpin;
    QDoubleSpinBox* m_insertSeatAngleSpin;
    QDoubleSpinBox* m_insertSetbackSpin;
    QDoubleSpinBox* m_sideAngleSpin;
    QDoubleSpinBox* m_backAngleSpin;
    QListWidget* m_compatibleInsertsListWidget;
    QCheckBox* m_isInternalCheck;
    QCheckBox* m_isGroovingCheck;
    QCheckBox* m_isThreadingCheck;
    QPushButton* m_generateHolderCodeButton;
    
    // Cutting Data Tab
    QFormLayout* m_cuttingDataLayout;
    QCheckBox* m_constantSurfaceSpeedCheck;
    QDoubleSpinBox* m_surfaceSpeedSpin;
    QDoubleSpinBox* m_spindleRPMSpin;
    QCheckBox* m_feedPerRevolutionCheck;
    QDoubleSpinBox* m_cuttingFeedrateSpin;
    QDoubleSpinBox* m_leadInFeedrateSpin;
    QDoubleSpinBox* m_leadOutFeedrateSpin;
    QDoubleSpinBox* m_maxDepthOfCutSpin;
    QDoubleSpinBox* m_maxFeedrateSpin;
    QDoubleSpinBox* m_minSurfaceSpeedSpin;
    QDoubleSpinBox* m_maxSurfaceSpeedSpin;
    QCheckBox* m_floodCoolantCheck;
    QCheckBox* m_mistCoolantCheck;
    QComboBox* m_preferredCoolantCombo;
    QDoubleSpinBox* m_coolantPressureSpin;
    QDoubleSpinBox* m_coolantFlowSpin;
    QPushButton* m_loadRecommendedDataButton;
    
    // Tool Info Tab
    QFormLayout* m_toolInfoLayout;
    QLineEdit* m_toolNameEdit;
    QLineEdit* m_vendorEdit;
    QLineEdit* m_productIdEdit;
    QLineEdit* m_productLinkEdit;
    QLineEdit* m_manufacturerEdit;
    QLineEdit* m_partNumberEdit;
    QTextEdit* m_notesEdit;
    QCheckBox* m_isActiveCheck;
    QLineEdit* m_toolNumberEdit;
    QSpinBox* m_turretPositionSpin;
    QDoubleSpinBox* m_toolOffsetXSpin;
    QDoubleSpinBox* m_toolOffsetZSpin;
    QDoubleSpinBox* m_toolLengthOffsetSpin;
    QDoubleSpinBox* m_toolRadiusOffsetSpin;
    QDoubleSpinBox* m_expectedLifeMinutesSpin;
    QDoubleSpinBox* m_usageMinutesSpin;
    QSpinBox* m_cycleCountSpin;
    QLineEdit* m_lastMaintenanceDateEdit;
    QLineEdit* m_nextMaintenanceDateEdit;
    
    // 3D Visualization Panel
    QWidget* m_visualizationPanel;
    QVBoxLayout* m_visualizationLayout;
    QHBoxLayout* m_visualizationControlsLayout;
    QComboBox* m_visualizationModeCombo;
    QCheckBox* m_showInsertCheck;
    QCheckBox* m_showHolderCheck;
    QCheckBox* m_showAssemblyCheck;
    QPushButton* m_exportImageButton;
    OpenGL3DWidget* m_3dViewer;
    
    // Additional 3D Visualization Components
    QWidget* m_visualization3DPanel;
    OpenGL3DWidget* m_opengl3DWidget;
    OpenGL3DWidget* m_3dToolViewer;
    QTabWidget* m_visualizationTabs;
    QWidget* m_3dViewTab;
    QWidget* m_2dViewTab;
    QWidget* m_sectionsTab;
    
    // Status and progress
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    
    // Data members
    std::vector<IntuiCAM::Toolpath::ToolAssembly> m_toolLibrary;
    QVector<IntuiCAM::Toolpath::ToolAssembly> m_toolDatabase;
    IntuiCAM::Toolpath::ToolAssembly m_currentToolAssembly;
    QString m_currentToolId;
    IntuiCAM::Toolpath::ToolType m_currentToolType;
    bool m_isEditing;
    bool m_dataChanged;
    
    // 3D Visualization
    Handle(AIS_InteractiveContext) m_aisContext;
    
    // Timers
    QTimer* m_updateTimer;
    QTimer* m_validationTimer;
    
    // Constants
    static const int UPDATE_DELAY_MS = 500;
    static const int VALIDATION_DELAY_MS = 1000;
    static const QString TOOL_DATABASE_FILE;
    static const QString TOOL_CATALOG_FILTER;
    static const QString TOOL_LIBRARY_FILTER;

    // 3D Visualization Controls
    QGroupBox* m_viewControlsGroup;
    QComboBox* m_viewModeCombo;
    QComboBox* m_viewPlaneCombo;
    QPushButton* m_fitViewButton;
    QPushButton* m_resetViewButton;
    QCheckBox* m_lockViewPlaneCheck;
    QCheckBox* m_showDimensionsCheck;
    QCheckBox* m_showAnnotationsCheck;
    QSlider* m_zoomSlider;
    
    // Tool Life Management Panel
    QWidget* m_toolLifePanel;
    QGroupBox* m_toolLifeGroup;
    QProgressBar* m_toolLifeProgress;
    QLabel* m_remainingLifeLabel;
    QLabel* m_usageTimeLabel;
    QLabel* m_cycleCountLabel;
    QLabel* m_lastMaintenanceLabel;
    QLabel* m_nextMaintenanceLabel;
    QPushButton* m_scheduleMaintenanceButton;
    QPushButton* m_resetToolLifeButton;
    QPushButton* m_generateReportButton;
    QCheckBox* m_enableAlertsCheck;
    
    // Advanced Cutting Data Panel
    QWidget* m_advancedCuttingPanel;
    QGroupBox* m_cuttingOptimizationGroup;
    QPushButton* m_optimizeParametersButton;
    QPushButton* m_calculateDeflectionButton;
    QPushButton* m_analyzeSurfaceFinishButton;
    QComboBox* m_workpieceMaterialCombo;
    QComboBox* m_operationTypeCombo;
    QDoubleSpinBox* m_requiredSurfaceFinishSpin;
    QDoubleSpinBox* m_allowableDeflectionSpin;
    
    // Real-time Update System
    QTimer* m_parameterUpdateTimer;
    QTimer* m_visualizationUpdateTimer;
    QTimer* m_toolLifeUpdateTimer;
    bool m_realTimeUpdatesEnabled;
    bool m_3dVisualizationEnabled;
    QString m_current3DViewPlane;
    
    // Tool Life Tracking Data
    struct ToolLifeData {
        double expectedLifeMinutes;
        double usageMinutes;
        int cycleCount;
        QString lastMaintenanceDate;
        QString nextMaintenanceDate;
        bool alertsEnabled;
        double warningThreshold; // percentage
        double criticalThreshold; // percentage
        
        ToolLifeData() : expectedLifeMinutes(480), usageMinutes(0), cycleCount(0),
                        alertsEnabled(true), warningThreshold(80.0), criticalThreshold(95.0) {}
    };
    QMap<QString, ToolLifeData> m_toolLifeTracking;
    
    // Advanced Cutting Data Cache
    struct CuttingDataCache {
        QString material;
        QString operation;
        IntuiCAM::Toolpath::CuttingData optimizedData;
        double calculatedDeflection;
        double predictedSurfaceFinish;
        QString lastCalculated;
        
        CuttingDataCache() : calculatedDeflection(0), predictedSurfaceFinish(0) {}
    };
    QMap<QString, CuttingDataCache> m_cuttingDataCache;
    
    // 3D Visualization State
    TopoDS_Shape m_currentInsertShape;
    TopoDS_Shape m_currentHolderShape;
    TopoDS_Shape m_currentAssemblyShape;
    Handle(AIS_InteractiveObject) m_insertAIS;
    Handle(AIS_InteractiveObject) m_holderAIS;
    Handle(AIS_InteractiveObject) m_assemblyAIS;
};

#endif // TOOLMANAGEMENTDIALOG_H 