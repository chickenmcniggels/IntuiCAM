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

// Forward declarations for our custom classes
class OpenGL3DWidget;
class StepLoader;
class WorkspaceController;

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

private:
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();
    void setupWorkspaceConnections();

    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter;
    
    // Project tree and properties
    QTreeWidget *m_projectTree;
    QTextEdit *m_propertiesPanel;
    
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