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

// OpenCASCADE includes
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

// Forward declarations for our custom classes
class OpenGL3DWidget;
class StepLoader;
class WorkspaceController;
class PartLoadingPanel;

// Include CylinderInfo definition
#include "workpiecemanager.h"

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
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newProject();
    void openProject();
    void openStepFile();
    void saveProject();
    void exitApplication();
    void aboutApplication();
    void showPreferences();
    void initializeWorkspace();  // Initialize the workspace after UI is ready
    
    // Workspace controller event handlers
    void handleWorkspaceError(const QString& source, const QString& message);
    void handleChuckInitialized();
    void handleWorkpieceWorkflowCompleted(double diameter, double rawMaterialDiameter);
    void handleChuckCenterlineDetected(const gp_Ax1& axis);
    void handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders);
    void handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);
    
    // Part loading panel handlers
    void handlePartLoadingDistanceChanged(double distance);
    void handlePartLoadingDiameterChanged(double diameter);
    void handlePartLoadingOrientationFlipped(bool flipped);
    void handlePartLoadingCylinderChanged(int index);
    void handlePartLoadingManualSelection();
    void handlePartLoadingReprocess();
    
    // 3D viewer handlers
    void handleShapeSelected(const TopoDS_Shape& shape, const gp_Pnt& clickPoint);

private:
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();
    void setupWorkspaceConnections();
    void setupPartLoadingConnections();

    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter;
    
    // Project tree and properties
    QTreeWidget *m_projectTree;
    QTextEdit *m_propertiesPanel;
    
    // Part loading panel
    PartLoadingPanel *m_partLoadingPanel;
    
    // 3D Viewport - pure visualization component
    OpenGL3DWidget *m_3dViewer;
    
    // Output/Log window
    QTextEdit *m_outputWindow;
    
    // Business logic controllers
    WorkspaceController *m_workspaceController;
    StepLoader *m_stepLoader;
    
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
};

#endif // MAINWINDOW_H