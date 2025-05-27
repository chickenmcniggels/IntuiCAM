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
    void openStepFile();  // New slot for opening STEP files
    void saveProject();
    void exitApplication();
    void aboutApplication();
    void showPreferences();
    void initializeChuck();  // Initialize the chuck after UI is ready

private:
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();

    // UI Components
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter;
    
    // Project tree and properties
    QTreeWidget *m_projectTree;
    QTextEdit *m_propertiesPanel;
    
    // 3D Viewport - replace QLabel with OpenCASCADE viewer
    OpenGL3DWidget *m_3dViewer;
    
    // Output/Log window
    QTextEdit *m_outputWindow;
    
    // STEP file loader
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
    QAction *m_openStepAction;  // New action for opening STEP files
    QAction *m_saveAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
    QAction *m_preferencesAction;
    
    // Toolbars
    QToolBar *m_mainToolBar;
};

#endif // MAINWINDOW_H