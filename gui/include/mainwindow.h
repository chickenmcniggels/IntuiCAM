#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTextEdit>
#include <QtCore/QTimer>

// Forward declarations
class OpenGL3DWidget;
class PartLoadingPanel;
class WorkspaceController;
class StepLoader;
struct CylinderInfo;

#include <Standard_Handle.hxx>
#include <AIS_InteractiveContext.hxx>
#include <gp_Ax1.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // File menu actions
    void newProject();
    void openProject();
    void openStepFile();
    void saveProject();
    void exitApplication();
    
    // Help menu actions
    void aboutApplication();
    
    // View menu actions
    void showPreferences();
    
    // Workspace controller slots
    void handleWorkspaceError(const QString& source, const QString& message);
    void handleChuckInitialized();
    void handleWorkpieceWorkflowCompleted(double diameter, double rawMaterialDiameter);
    void handleChuckCenterlineDetected(const gp_Ax1& axis);
    void handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders);
    void handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);
    
    // Part loading panel slots
    void handlePartLoadingDistanceChanged(double distance);
    void handlePartLoadingDiameterChanged(double diameter);
    void handlePartLoadingOrientationFlipped(bool flipped);
    void handlePartLoadingCylinderChanged(int index);
    void handlePartLoadingManualSelection();

    // 3D viewer interaction slots
    void handleShapeSelected(const TopoDS_Shape& shape, const gp_Pnt& clickPoint);

    // Initialization and connection setup
    void setupWorkspaceConnections();
    void setupPartLoadingConnections();
    void initializeWorkspace();

private:
    void createMenus();
    void createToolBars();
    void createCentralWidget();
    void createStatusBar();
    void setupConnections();

    // UI Components - Central widget structure
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    
    // Left panel components
    QTreeWidget* m_projectTree;
    PartLoadingPanel* m_partLoadingPanel;
    QTextEdit* m_propertiesPanel;
    
    // Main area
    OpenGL3DWidget* m_3dViewer;
    QTextEdit* m_outputWindow;
    
    // Core components
    WorkspaceController* m_workspaceController;
    StepLoader* m_stepLoader;
    
    // Menu components
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_openStepAction;
    QAction* m_saveAction;
    QAction* m_exitAction;
    QAction* m_aboutAction;
    QAction* m_preferencesAction;
};

#endif // MAINWINDOW_H 