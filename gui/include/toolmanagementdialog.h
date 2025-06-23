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
#include <QJsonObject>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_MaterialAspect.hxx>

// Project includes
#include "opengl3dwidget.h"
#include "materialspecificcuttingdatawidget.h"
#include "materialmanager.h"
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

namespace IntuiCAM {
    namespace GUI {
        class ToolManager;
    }
}

class OpenGL3DWidget;

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
    
    // Set the tool manager for persistence
    void setToolManager(IntuiCAM::GUI::ToolManager* toolManager);
    
    // Set the material manager for material-specific cutting data
    void setMaterialManager(IntuiCAM::GUI::MaterialManager* materialManager);

signals:
    void toolSaved(const QString& toolId);
    void errorOccurred(const QString& message);
    void toolNameChanged(const QString& toolId, const QString& newName);
    
    // 3D visualization signals
    void tool3DVisualizationChanged(const QString& toolId);
    void toolGeometryUpdated(const TopoDS_Shape& toolShape);

private slots:
    // Tool type selection
    void onToolTypeChanged(int index);
    
    // Parameter change slots (trigger auto-save)
    void onInsertParameterChanged();
    void onHolderParameterChanged();
    void onCuttingDataChanged();
    void onToolInfoChanged();
    void onToolNameEdited(const QString& text);
    void onISOCodeChanged();
    
    // UI logic change slots
    void onConstantSurfaceSpeedToggled(bool enabled);
    void onFeedPerRevolutionToggled(bool enabled);
    
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
    void onWireframeClicked();
    void onShadedClicked();
    void onShadedWithEdgesClicked();
    void onIsometricViewClicked();
    void onFrontViewClicked();
    void onTopViewClicked();
    void onRightViewClicked();

private:
    // UI Creation methods
    void setupUI();
    void createMainLayout();
    void createToolEditPanel();
    void create3DVisualizationPanel();
    void createToolTypeSelector();
    
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
    TopoDS_Shape createInsertGeometry();
    TopoDS_Shape createHolderGeometry();
    TopoDS_Shape createAssembledToolGeometry();
    void updateVisualizationMode(int mode);
    void setupViewControls();
    void applyMaterialToShape(Handle(AIS_Shape) aisShape, const QString& materialType);
    void clearPreviousToolGeometry();
    void updateViewControlsState();
    
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
    
    // Tool ID generation
    QString getToolTypePrefix(IntuiCAM::Toolpath::ToolType toolType) const;
    QString generateUniqueToolId(const QString& prefix) const;
    
    // Validation
    bool validateCurrentTool();
    bool validateISOCode(const QString& isoCode);
    
    // UI helpers
    void updateToolTypeSpecificUI();
    void setComboBoxByValue(QComboBox* comboBox, int value);
    void clearAllParameterFields();
    QString formatToolType(IntuiCAM::Toolpath::ToolType toolType);
    void updateFeedRateUnits(bool feedPerRevolution);
    
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
    void updateToolInfoFromFields();
    
    // Helper method for initializing tool assembly
    void initializeToolAssemblyForType(IntuiCAM::Toolpath::ToolType toolType);
    
    // Tool type specific UI methods  
    void showToolTypeSpecificTabs(IntuiCAM::Toolpath::ToolType toolType);
    void hideAllInsertTabs();
    void setupDefaultToolParameters(IntuiCAM::Toolpath::ToolType toolType);
    
    // Default parameter setup methods for each tool type
    void setupGeneralTurningDefaults();
    void setupBoringDefaults();
    void setupThreadingDefaults();
    void setupGroovingDefaults();
    void setupPartingDefaults();
    void setupFormToolDefaults();
    void setupHolderDefaults();
    void setupCuttingDataDefaults(IntuiCAM::Toolpath::ToolType toolType);
    void setupCapabilitiesForToolType(IntuiCAM::Toolpath::ToolType toolType);
    
    // Tool assembly persistence
    QString getToolAssemblyDatabasePath() const;
    bool saveToolAssemblyToDatabase();
    bool loadToolAssemblyFromDatabase(const QString& toolId);
    QJsonObject toolAssemblyToJson(const IntuiCAM::Toolpath::ToolAssembly& assembly) const;
    IntuiCAM::Toolpath::ToolAssembly toolAssemblyFromJson(const QJsonObject& json) const;
    
    // Conversion helpers for insert types
    QJsonObject generalTurningInsertToJson(const IntuiCAM::Toolpath::GeneralTurningInsert& insert) const;
    IntuiCAM::Toolpath::GeneralTurningInsert generalTurningInsertFromJson(const QJsonObject& json) const;
    QJsonObject threadingInsertToJson(const IntuiCAM::Toolpath::ThreadingInsert& insert) const;
    IntuiCAM::Toolpath::ThreadingInsert threadingInsertFromJson(const QJsonObject& json) const;
    QJsonObject groovingInsertToJson(const IntuiCAM::Toolpath::GroovingInsert& insert) const;
    IntuiCAM::Toolpath::GroovingInsert groovingInsertFromJson(const QJsonObject& json) const;
    QJsonObject toolHolderToJson(const IntuiCAM::Toolpath::ToolHolder& holder) const;
    IntuiCAM::Toolpath::ToolHolder toolHolderFromJson(const QJsonObject& json) const;
    QJsonObject cuttingDataToJson(const IntuiCAM::Toolpath::CuttingData& cuttingData) const;
    IntuiCAM::Toolpath::CuttingData cuttingDataFromJson(const QJsonObject& json) const;

    // Tool geometry generation helpers
    TopoDS_Shape createSquareInsert(double inscribedCircle, double thickness, double cornerRadius);
    TopoDS_Shape createTriangleInsert(double inscribedCircle, double thickness, double cornerRadius);
    TopoDS_Shape createDiamondInsert(double inscribedCircle, double thickness, double cornerRadius);
    TopoDS_Shape createRoundInsert(double inscribedCircle, double thickness);
    TopoDS_Shape createThreadingInsert(double thickness, double width, double length);
    TopoDS_Shape createGroovingInsert(double thickness, double width, double length, double grooveWidth);
    TopoDS_Shape createRectangularHolder(double length, double width, double height);
    TopoDS_Shape createCylindricalHolder(double diameter, double length);
    
    // View control helper methods
    void setStandardView(const gp_Dir& viewDirection, const gp_Dir& upDirection);
    void resetCameraPosition();
    void fitViewToTool();

private:
    // Main layout
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    
    // Tool Edit Panel
    QWidget* m_toolEditPanel;
    QVBoxLayout* m_toolEditLayout;
    QComboBox* m_toolTypeCombo;
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
    QPushButton* m_wireframeButton;
    QPushButton* m_shadedButton;
    QPushButton* m_shadedEdgesButton;
    QPushButton* m_isometricViewButton;
    QPushButton* m_frontViewButton;
    QPushButton* m_topViewButton;
    QPushButton* m_rightViewButton;
    QCheckBox* m_showDimensionsCheck;
    QCheckBox* m_showAnnotationsCheck;
    QSlider* m_zoomSlider;
    QLabel* m_zoomLabel;
    
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
    
    // Material-Specific Cutting Data Tab Components
    IntuiCAM::GUI::MaterialSpecificCuttingDataWidget* m_materialSpecificCuttingDataWidget;
    
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
    
    // Tool Capabilities (moved from holder tab)
    QCheckBox* m_internalThreadingCheck;
    QCheckBox* m_internalBoringCheck;
    QCheckBox* m_partingGroovingCheck;
    QCheckBox* m_externalThreadingCheck;
    QCheckBox* m_longitudinalTurningCheck;
    QCheckBox* m_facingCheck;
    QCheckBox* m_chamferingCheck;
    
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
    
    // Tool manager reference for broader integration
    IntuiCAM::GUI::ToolManager* m_toolManager;
    
    // Material manager reference for material-specific cutting data
    IntuiCAM::GUI::MaterialManager* m_materialManager;
    
    // Tool geometry objects
    Handle(AIS_Shape) m_currentInsertShape;
    Handle(AIS_Shape) m_currentHolderShape;
    Handle(AIS_Shape) m_currentAssembledShape;
    TopoDS_Shape m_currentToolGeometry;
    
    // Visualization state
    int m_currentVisualizationMode;
    bool m_showDimensions;
    bool m_showAnnotations;
    double m_currentZoomLevel;
};

#endif // TOOLMANAGEMENTDIALOG_H 