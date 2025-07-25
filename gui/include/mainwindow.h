#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QLabel>
#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QTimer>

// OpenCASCADE includes
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_Drawer.hxx>
#include <BRepAdaptor_Surface.hxx>

// Project includes

// Forward declarations for our custom classes
class OpenGL3DWidget;
class StepLoader;
class WorkspaceController;
class PartLoadingPanel;
class WorkpieceManager;
class ToolManagementTab;
class ToolManagementDialog;

// Forward declarations for namespaced types
namespace IntuiCAM {
namespace GUI {
    class SetupConfigurationPanel;
    class MaterialManager;
    class ToolManager;
    class OperationParameterDialog;
    enum class MaterialType;
    enum class SurfaceFinish;
}
}

// Include CylinderInfo definition
#include "workpiecemanager.h"
#include "operationtilewidget.h"

// Include ViewMode enum
enum class ViewMode;  // Forward declaration

QT_BEGIN_NAMESPACE
class QLabel;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QVBoxLayout;
class QHBoxLayout;
class QSplitter;
class QTextEdit;
class QTreeWidget;
class QAction;
class QTabWidget;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // Getters for managers
    IntuiCAM::GUI::MaterialManager* getMaterialManager() const { return m_materialManager; }

private slots:
    void newProject();
    void openProject();
    void openStepFile();
    void saveProject();
    void exitApplication();
    void aboutApplication();
    void showPreferences();
    void onTabChanged(int index);
    
    // Workspace controller event handlers
    void handleWorkspaceError(const QString& source, const QString& message);
    void handleChuckInitialized();
    void handleWorkpieceWorkflowCompleted(double diameter, double rawMaterialDiameter);
    void handleChuckCenterlineDetected(const gp_Ax1& axis);
    void handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders);
    void handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);
    void handleManualAxisSelected(double diameter, const gp_Ax1& axis);
    void handleRawMaterialCreated(double diameter, double length);
    
    // Part loading panel handlers (legacy)
    void handlePartLoadingDistanceChanged(double distance);
    void handlePartLoadingDiameterChanged(double diameter);
    void handleWorkpiecePositionChanged(double distance);
    void handlePartLoadingOrientationFlipped(bool flipped);
    void handlePartLoadingCylinderChanged(int index);
    void handlePartLoadingManualSelection();
    void handlePartLoadingReprocess();
    
    // Setup configuration panel handlers (new)
    void handleStepFileSelected(const QString& filePath);
    void handleSetupConfigurationChanged();
    void handleMaterialTypeChanged(IntuiCAM::GUI::MaterialType material);
    void handleRawMaterialDiameterChanged(double diameter);
    void handleManualAxisSelectionRequested();
    void handleAutoRawDiameterRequested();
    void handleThreadFaceSelectionRequested();
    void handleThreadFaceSelected(const TopoDS_Shape& face);
    void handleWorkpieceTransformed();
    void handleOperationToggled(const QString& operationName, bool enabled);
    void handleGenerateToolpaths();
    
    // Operation tile handlers
    void handleOperationTileEnabledChanged(const QString& operationName, bool enabled);
    void handleOperationTileClicked(const QString& operationName);
    void handleOperationTileToolSelectionRequested(const QString& operationName);
    void handleOperationTileExpandedChanged(const QString& operationName, bool expanded);
    
    // 3D viewer handlers
    void handleShapeSelected(const TopoDS_Shape& shape, const gp_Pnt& clickPoint);
    void handleViewModeChanged(ViewMode mode);
    void toggleViewMode();
    
    // Setup tab actions
    void simulateToolpaths();
    
    // Operation parameter dialog handler


    // Overlay control for chuck visibility
    void handleShowChuckToggled(bool checked);
    void handleShowRawMaterialToggled(bool checked);
    void handleShowToolpathsToggled(bool checked);
    void handleShowPartToggled(bool checked);
    void handleShowProfilesToggled(bool checked);

private:
    void createMenus();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();
    void setupWorkspaceConnections();
    void setupPartLoadingConnections();
    void setupUiConnections();
    void logToOutput(const QString& message);
    
    // Tab creation methods
    QWidget* createHomeTab();
    QWidget* createSetupTab();
    QWidget* createSimulationTab();
    QWidget* createMachineTab();

    // UI Components
    QWidget *m_centralWidget;
    QTabWidget *m_tabWidget;
    
    // Home tab components
    QWidget *m_homeTab;
    
    // Setup tab components (new Orca Slicer-inspired interface)
    QWidget *m_setupTab;
    QSplitter *m_mainSplitter;
    IntuiCAM::GUI::SetupConfigurationPanel *m_setupConfigPanel;
    OpenGL3DWidget *m_3dViewer;
    QPushButton *m_generateButton;
    QPushButton *m_simulateButton;
    
    // Legacy components (for gradual migration)
    QSplitter *m_leftSplitter;
    QTreeWidget *m_projectTree;
    QTextEdit *m_propertiesPanel;
    PartLoadingPanel *m_partLoadingPanel;
    
    // Simulation tab components
    QWidget *m_simulationTab;
    QWidget *m_simulationViewport;
    QWidget *m_simulationControls;
    QPushButton *m_uploadToMachineButton;
    QPushButton *m_exportGCodeButton;
    
    // Machine tab components
    QWidget *m_machineTab;
    QWidget *m_machineFeedWidget;
    QWidget *m_machineControlPanel;
    
    // Output/Log window (shared)
    QTextEdit *m_outputWindow;
    
    // Business logic controllers
    WorkspaceController *m_workspaceController;
    StepLoader *m_stepLoader;
    WorkpieceManager *m_workpieceManager;
    
    // Material and Tool Management
    IntuiCAM::GUI::MaterialManager *m_materialManager;
    IntuiCAM::GUI::ToolManager *m_toolManager;
    
    // Operation Tile System
    IntuiCAM::GUI::OperationTileContainer *m_operationTileContainer;
    
    
    // Tool Management Components
    class ToolManagementTab *m_toolManagementTab;

    bool m_selectingThreadFace = false;
    
    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;
    
    // Actions
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_openStepAction;
    QAction *m_saveAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    QAction *m_preferencesAction;
    QAction *m_toggleViewModeAction;
    
    // Overlay UI elements
    QPushButton *m_viewModeOverlayButton;
    QToolButton *m_visibilityButton;
    QMenu *m_visibilityMenu;
    QAction *m_showChuckAction;
    QAction *m_showRawMaterialAction;
    QAction *m_showToolpathsAction;
    QAction *m_showPartAction;
    QAction *m_showProfilesAction;
    QString m_defaultChuckFilePath;

    // Timer for debouncing toolpath regeneration
    QTimer* m_toolpathRegenerationTimer;

private:
    void createViewModeOverlayButton(QWidget* parent);
    void updateViewModeOverlayButton();
    void initializeWorkspace();
    
    void highlightThreadCandidateFaces();
    void clearThreadCandidateHighlights();
    void updateHighlightedThreadFace();
    void clearHighlightedThreadFace();
    
    // Helper methods
    QString getDefaultToolForOperation(const QString& operationName) const;

    QVector<Handle(AIS_Shape)> m_candidateThreadFaces;
    Handle(AIS_Shape) m_currentThreadFaceAIS;
    TopoDS_Shape m_currentThreadFaceLocal;
    int m_currentThreadRow = -1;
};

#endif // MAINWINDOW_H