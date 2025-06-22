#ifndef TOOLMANAGEMENTDIALOG_H
#define TOOLMANAGEMENTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QTimer>
#include <QSlider>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>

// Project includes
#include "opengl3dwidget.h"
#include "IntuiCAM/Toolpath/ToolTypes.h"

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
    // Constructor for editing an existing tool
    explicit ToolManagementDialog(const QString& toolId, QWidget *parent = nullptr);
    
    // Constructor for creating a new tool
    explicit ToolManagementDialog(IntuiCAM::Toolpath::ToolType toolType = IntuiCAM::Toolpath::ToolType::GENERAL_TURNING, 
                                  QWidget *parent = nullptr);
    
    ~ToolManagementDialog();

    // Get the current tool data
    IntuiCAM::Toolpath::ToolAssembly getToolAssembly() const;
    
    // Check if this is a new tool being created
    bool isNewTool() const { return m_isNewTool; }
    
    // Get the tool ID
    QString getToolId() const { return m_currentToolId; }

signals:
    void toolSaved(const QString& toolId);
    void errorOccurred(const QString& message);
    
    // 3D visualization signals
    void tool3DVisualizationChanged(const QString& toolId);

private slots:
    // Parameter change slots (trigger auto-save)
    void onInsertParameterChanged();
    void onHolderParameterChanged();
    void onCuttingDataChanged();
    void onToolInfoChanged();
    void onISOCodeChanged();
    
    // 3D visualization slots
    void onVisualizationModeChanged(int mode);
    void updateToolVisualization();
    
    // Auto-save timer slot
    void onAutoSaveTimeout();
    
    // 3D view control slots
    void onFitViewClicked();
    void onResetViewClicked();
    void onShowDimensionsChanged(bool show);
    void onShowAnnotationsChanged(bool show);
    void onZoomChanged(int value);

private:
    // UI Creation methods
    void setupUI();
    void createMainLayout();
    void createToolEditPanel();
    void create3DVisualizationPanel();
    
    // Tool editing tabs
    QWidget* createInsertPropertiesTab();
    QWidget* createHolderPropertiesTab();
    QWidget* createCuttingDataTab();
    QWidget* createToolInfoTab();
    
    // Specific insert type panels
    void createGeneralTurningPanel();
    void createThreadingPanel();
    void createGroovingPanel();
    void createHolderPanel();
    void createCuttingDataPanel();
    void createToolInfoPanel();
    
    // 3D Visualization methods
    void setup3DViewer();
    void generate3DToolGeometry();
    void updateRealTime3DVisualization();
    
    // Auto-save functionality
    void setupAutoSave();
    void saveCurrentTool();
    void connectParameterSignals();
    void markAsModified();
    
    // Data loading/saving
    void loadToolData(const QString& toolId);
    void initializeNewTool(IntuiCAM::Toolpath::ToolType toolType);
    void loadToolParametersIntoFields(const IntuiCAM::Toolpath::ToolAssembly& assembly);
    void updateToolAssemblyFromFields();
    
    // Validation
    bool validateCurrentTool();
    bool validateISOCode(const QString& isoCode);
    
    // UI helpers
    void updateToolTypeSpecificUI();
    void setComboBoxByValue(QComboBox* comboBox, int value);
    void clearAllParameterFields();
    QString formatToolType(IntuiCAM::Toolpath::ToolType toolType);
    
    // Tool parameter loading methods
    void loadGeneralTurningInsertParameters(const IntuiCAM::Toolpath::GeneralTurningInsert& insert);
    void loadThreadingInsertParameters(const IntuiCAM::Toolpath::ThreadingInsert& insert);
    void loadGroovingInsertParameters(const IntuiCAM::Toolpath::GroovingInsert& insert);
    void loadHolderParameters(const IntuiCAM::Toolpath::ToolHolder& holder);
    void loadCuttingDataParameters(const IntuiCAM::Toolpath::CuttingData& cuttingData);
    
    // Tool parameter updating methods
    void updateGeneralTurningInsertFromFields();
    void updateThreadingInsertFromFields();
    void updateGroovingInsertFromFields();
    void updateHolderDataFromFields();
    void updateCuttingDataFromFields();

private:
    // Main layout
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    
    // Tool Edit Panel
    QWidget* m_toolEditPanel;
    QVBoxLayout* m_toolEditLayout;
    QTabWidget* m_toolEditTabs;
    
    // Tabs
    QWidget* m_insertTab;
    QWidget* m_holderTab;
    QWidget* m_cuttingDataTab;
    QWidget* m_toolInfoTab;
    
    // 3D Visualization Panel
    QWidget* m_visualization3DPanel;
    QVBoxLayout* m_visualizationLayout;
    OpenGL3DWidget* m_3dViewer;
    
    // 3D Controls
    QGroupBox* m_viewControlsGroup;
    QComboBox* m_visualizationModeCombo;
    QPushButton* m_fitViewButton;
    QPushButton* m_resetViewButton;
    QCheckBox* m_showDimensionsCheck;
    QCheckBox* m_showAnnotationsCheck;
    QSlider* m_zoomSlider;
    
    // General Turning Insert Tab Components
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
    
    // Threading Insert Tab Components
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
    
    // Grooving Insert Tab Components
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
    
    // Tool Holder Tab Components
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
    QCheckBox* m_isInternalCheck;
    QCheckBox* m_isGroovingCheck;
    QCheckBox* m_isThreadingCheck;
    
    // Cutting Data Tab Components
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
    
    // Tool Info Tab Components
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
    
    // Data members
    IntuiCAM::Toolpath::ToolAssembly m_currentToolAssembly;
    QString m_currentToolId;
    IntuiCAM::Toolpath::ToolType m_currentToolType;
    bool m_isNewTool;
    bool m_dataModified;
    
    // Auto-save system
    QTimer* m_autoSaveTimer;
    bool m_autoSaveEnabled;
    
    // 3D Visualization
    Handle(AIS_InteractiveContext) m_aisContext;
    
    // Constants
    static const int AUTO_SAVE_DELAY_MS = 1000; // 1 second after last change
};

#endif // TOOLMANAGEMENTDIALOG_H 