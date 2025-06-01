#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "workspacecontroller.h"
#include "cylinderselectiondialog.h"
#include "workpiecemanager.h"  // For CylinderInfo definition

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QFont>
#include <QTreeWidgetItem>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>  // Add this include for qDebug()

// OpenCASCADE includes for gp_Ax1
#include <gp_Ax1.hxx>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftSplitter(nullptr)
    , m_projectTree(nullptr)
    , m_propertiesPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_outputWindow(nullptr)
    , m_workspaceController(nullptr)
    , m_stepLoader(nullptr)
    , m_cylinderSelectionDialog(nullptr)
{
    setWindowTitle("IntuiCAM - Computer Aided Manufacturing");
    setMinimumSize(1024, 768);
    resize(1400, 900);
    
    // Initialize components following modular architecture
    m_stepLoader = new StepLoader();
    m_workspaceController = new WorkspaceController(this);
    
    createMenus();
    createToolBars();
    createCentralWidget();
    createStatusBar();
    setupConnections();
    
    // Set initial status
    statusBar()->showMessage("Ready - Welcome to IntuiCAM", 2000);
    
    // Initialize workspace automatically after a short delay to ensure OpenGL is ready
    QTimer::singleShot(100, this, &MainWindow::initializeWorkspace);
}

MainWindow::~MainWindow()
{
    // Clean up our custom objects
    delete m_stepLoader;
    // Qt handles cleanup of other widgets and WorkspaceController automatically
}

void MainWindow::createMenus()
{
    // Create File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_newAction = new QAction(tr("&New Project"), this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("Create a new CAM project"));
    m_fileMenu->addAction(m_newAction);
    
    m_openAction = new QAction(tr("&Open Project"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open an existing CAM project"));
    m_fileMenu->addAction(m_openAction);
    
    m_fileMenu->addSeparator();
    
    m_openStepAction = new QAction(tr("Open &STEP File"), this);
    m_openStepAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    m_openStepAction->setStatusTip(tr("Import a STEP file as workpiece"));
    m_fileMenu->addAction(m_openStepAction);
    
    m_fileMenu->addSeparator();
    
    m_saveAction = new QAction(tr("&Save Project"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Save the current CAM project"));
    m_fileMenu->addAction(m_saveAction);
    
    m_fileMenu->addSeparator();
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    m_fileMenu->addAction(m_exitAction);
    
    // Create Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    
    m_preferencesAction = new QAction(tr("&Preferences"), this);
    m_preferencesAction->setStatusTip(tr("Configure application settings"));
    m_editMenu->addAction(m_preferencesAction);
    
    // Create View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    
    // Create Tools menu
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    
    // Create Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    
    m_aboutAction = new QAction(tr("&About IntuiCAM"), this);
    m_aboutAction->setStatusTip(tr("Show information about the application"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBars()
{
    QToolBar* m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setMovable(false);
    
    if (m_newAction) m_mainToolBar->addAction(m_newAction);
    if (m_openAction) m_mainToolBar->addAction(m_openAction);
    if (m_openStepAction) m_mainToolBar->addAction(m_openStepAction);
    if (m_saveAction) m_mainToolBar->addAction(m_saveAction);
    
    m_mainToolBar->addSeparator();
}

void MainWindow::createCentralWidget()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Main horizontal splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left vertical splitter for project tree and properties
    m_leftSplitter = new QSplitter(Qt::Vertical);
    
    // Project tree
    m_projectTree = new QTreeWidget;
    m_projectTree->setHeaderLabel("Project");
    m_projectTree->setMinimumWidth(250);
    m_projectTree->setMaximumWidth(400);
    
    // Add some example project structure
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(m_projectTree);
    rootItem->setText(0, "CAM Project");
    
    QTreeWidgetItem *partsItem = new QTreeWidgetItem(rootItem);
    partsItem->setText(0, "Parts");
    
    QTreeWidgetItem *toolsItem = new QTreeWidgetItem(rootItem);
    toolsItem->setText(0, "Tools");
    
    QTreeWidgetItem *operationsItem = new QTreeWidgetItem(rootItem);
    operationsItem->setText(0, "Operations");
    
    m_projectTree->expandAll();
    
    // Properties panel
    m_propertiesPanel = new QTextEdit;
    m_propertiesPanel->setMaximumHeight(200);
    m_propertiesPanel->setPlainText("Properties panel - Select an item to view details");
    m_propertiesPanel->setReadOnly(true);
    
    // Add to left splitter
    m_leftSplitter->addWidget(m_projectTree);
    m_leftSplitter->addWidget(m_propertiesPanel);
    m_leftSplitter->setSizes({400, 150});
    
    // 3D Viewport - Pure visualization component
    m_3dViewer = new OpenGL3DWidget();
    m_3dViewer->setMinimumSize(600, 400);
    
    // Output window
    m_outputWindow = new QTextEdit;
    m_outputWindow->setMaximumHeight(150);
    m_outputWindow->setPlainText("Output Log:\nWelcome to IntuiCAM - Computer Aided Manufacturing\nApplication started successfully.\n");
    m_outputWindow->setReadOnly(true);
    
    // Add to main splitter
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_3dViewer);
    
    // Set splitter sizes (left panel 25%, viewport 75%)
    m_mainSplitter->setSizes({300, 900});
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->addWidget(m_outputWindow);
    
    m_centralWidget->setLayout(mainLayout);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupConnections()
{
    // Connect UI actions
    if (m_newAction) connect(m_newAction, &QAction::triggered, this, &MainWindow::newProject);
    if (m_openAction) connect(m_openAction, &QAction::triggered, this, &MainWindow::openProject);
    if (m_openStepAction) connect(m_openStepAction, &QAction::triggered, this, &MainWindow::openStepFile);
    if (m_saveAction) connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    if (m_exitAction) connect(m_exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
    if (m_aboutAction) connect(m_aboutAction, &QAction::triggered, this, &MainWindow::aboutApplication);
    if (m_preferencesAction) connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::showPreferences);
    
    // Connect 3D viewer initialization
    connect(m_3dViewer, &OpenGL3DWidget::viewerInitialized,
            this, &MainWindow::setupWorkspaceConnections);
}

void MainWindow::setupWorkspaceConnections()
{
    // Initialize workspace controller when 3D viewer is ready
    if (m_workspaceController && m_3dViewer->isViewerInitialized()) {
        m_workspaceController->initialize(m_3dViewer->getContext(), m_stepLoader);
        
        // Connect workspace controller signals
        connect(m_workspaceController, &WorkspaceController::errorOccurred,
                this, &MainWindow::handleWorkspaceError);
        
        connect(m_workspaceController, &WorkspaceController::chuckInitialized,
                this, &MainWindow::handleChuckInitialized);
        
        connect(m_workspaceController, &WorkspaceController::workpieceWorkflowCompleted,
                this, &MainWindow::handleWorkpieceWorkflowCompleted);
        
        // Connect chuck centerline detection
        connect(m_workspaceController, &WorkspaceController::chuckCenterlineDetected,
                this, &MainWindow::handleChuckCenterlineDetected);
        
        // Connect multiple cylinders detection for manual selection
        connect(m_workspaceController, &WorkspaceController::multipleCylindersDetected,
                this, &MainWindow::handleMultipleCylindersDetected);
        
        // Connect cylinder axis selection
        connect(m_workspaceController, &WorkspaceController::cylinderAxisSelected,
                this, &MainWindow::handleCylinderAxisSelected);
        
        qDebug() << "Workspace controller connections established";
    }
}

void MainWindow::newProject()
{
    statusBar()->showMessage(tr("Creating new project..."), 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Creating new CAM project...");
    }
    // TODO: Implement new project functionality
}

void MainWindow::openProject()
{
    statusBar()->showMessage(tr("Opening project..."), 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Opening CAM project...");
    }
    // TODO: Implement open project functionality
}

void MainWindow::openStepFile()
{
    statusBar()->showMessage(tr("Opening STEP file..."), 2000);
    
    // Get the user's documents directory as default
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    
    // Open file dialog
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open STEP File"),
        defaultDir,
        tr("STEP Files (*.step *.stp *.STEP *.STP);;All Files (*)")
    );
    
    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No file selected"), 2000);
        return;
    }
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Loading STEP file: %1").arg(fileName));
    }
    
    // Load the STEP file using workspace controller
    if (m_stepLoader && m_workspaceController && m_workspaceController->isInitialized()) {
        TopoDS_Shape shape = m_stepLoader->loadStepFile(fileName);
        
        if (m_stepLoader->isValid() && !shape.IsNull()) {
            // Clear previous workpieces (workspace controller handles this cleanly)
            m_workspaceController->clearWorkpieces();
            
            // Add workpiece through workspace controller (handles full workflow)
            bool success = m_workspaceController->addWorkpiece(shape);
            
            if (success) {
                statusBar()->showMessage(tr("STEP file loaded and processed successfully"), 3000);
                if (m_outputWindow) {
                    m_outputWindow->append("STEP file loaded as workpiece and processed by workspace controller.");
                }
                
                // Fit view to show all content
                m_3dViewer->fitAll();
            } else {
                QString errorMsg = "Failed to process workpiece through workspace controller";
                statusBar()->showMessage(errorMsg, 5000);
                if (m_outputWindow) {
                    m_outputWindow->append(errorMsg);
                }
            }
        } else {
            QString errorMsg = QString("Failed to load STEP file: %1").arg(m_stepLoader->getLastError());
            statusBar()->showMessage(errorMsg, 5000);
            if (m_outputWindow) {
                m_outputWindow->append(errorMsg);
            }
            QMessageBox::warning(this, tr("Error Loading STEP File"), errorMsg);
        }
    } else {
        QString errorMsg = "Workspace controller not initialized";
        statusBar()->showMessage(errorMsg, 5000);
        if (m_outputWindow) {
            m_outputWindow->append(errorMsg);
        }
    }
}

void MainWindow::saveProject()
{
    statusBar()->showMessage(tr("Saving project..."), 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Saving CAM project...");
    }
    // TODO: Implement save project functionality
}

void MainWindow::exitApplication()
{
    QApplication::quit();
}

void MainWindow::aboutApplication()
{
    QMessageBox::about(this, tr("About IntuiCAM"),
                       tr("<h2>IntuiCAM</h2>"
                          "<p>Computer Aided Manufacturing for CNC Turning</p>"
                          "<p><b>Version:</b> 1.0.0 (Development)</p>"
                          "<p><b>Built with:</b></p>"
                          "<ul>"
                          "<li>Qt 6.9.0</li>"
                          "<li>OpenCASCADE 7.6.0</li>"
                          "<li>VTK 9.4.1</li>"
                          "</ul>"
                          "<p>An open-source CAM application for CNC turning operations.</p>"));
}

void MainWindow::showPreferences()
{
    statusBar()->showMessage(tr("Opening preferences..."), 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Opening application preferences...");
    }
    // TODO: Implement preferences dialog
}

void MainWindow::initializeWorkspace()
{
    // This method is called after UI initialization
    if (m_workspaceController && m_3dViewer->isViewerInitialized()) {
        // Initialize chuck automatically
        QString chuckFilePath = "C:/Users/nikla/Downloads/three_jaw_chuck.step";
        
        if (m_outputWindow) {
            m_outputWindow->append("Initializing workspace with 3-jaw chuck...");
        }
        
        bool success = m_workspaceController->initializeChuck(chuckFilePath);
        if (success) {
            statusBar()->showMessage("Workspace initialization completed", 3000);
            m_3dViewer->fitAll();
        } else {
            if (m_outputWindow) {
                m_outputWindow->append("Warning: Chuck initialization failed - workspace available without chuck");
            }
        }
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Error: Workspace controller or 3D viewer not ready");
        }
    }
}

void MainWindow::handleWorkspaceError(const QString& source, const QString& message)
{
    QString fullMessage = QString("[%1] %2").arg(source, message);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Error: %1").arg(fullMessage));
    }
    
    statusBar()->showMessage(QString("Error in %1").arg(source), 5000);
    qDebug() << "Workspace error:" << fullMessage;
}

void MainWindow::handleChuckInitialized()
{
    if (m_outputWindow) {
        m_outputWindow->append("Chuck initialized successfully in workspace");
    }
    statusBar()->showMessage("Chuck ready", 2000);
}

void MainWindow::handleWorkpieceWorkflowCompleted(double diameter, double rawMaterialDiameter)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Workpiece workflow completed - Detected: %1mm, Raw material: %2mm")
                              .arg(diameter, 0, 'f', 1)
                              .arg(rawMaterialDiameter, 0, 'f', 1));
    }
    statusBar()->showMessage("Workpiece processing completed", 3000);
}

void MainWindow::handleChuckCenterlineDetected(const gp_Ax1& axis)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Chuck centerline detected - Location: (%1, %2, %3), Direction: (%4, %5, %6)")
                              .arg(axis.Location().X(), 0, 'f', 2)
                              .arg(axis.Location().Y(), 0, 'f', 2)
                              .arg(axis.Location().Z(), 0, 'f', 2)
                              .arg(axis.Direction().X(), 0, 'f', 3)
                              .arg(axis.Direction().Y(), 0, 'f', 3)
                              .arg(axis.Direction().Z(), 0, 'f', 3));
    }
    statusBar()->showMessage("Chuck centerline detected and aligned", 3000);
}

void MainWindow::handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Multiple cylinders detected (%1 total) - Manual selection required").arg(cylinders.size()));
        for (int i = 0; i < cylinders.size(); ++i) {
            m_outputWindow->append(QString("  %1. %2").arg(i + 1).arg(cylinders[i].description));
        }
    }
    statusBar()->showMessage("Multiple cylinders detected - Select turning axis", 5000);
    
    // Show cylinder selection dialog
    showCylinderSelectionDialog(cylinders);
}

void MainWindow::handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Cylinder axis selected: %1 (Index: %2)")
                              .arg(cylinderInfo.description)
                              .arg(index));
    }
    statusBar()->showMessage("Turning axis selected and applied", 3000);
}

void MainWindow::showCylinderSelectionDialog(const QVector<CylinderInfo>& cylinders)
{
    // Clean up previous dialog if it exists
    if (m_cylinderSelectionDialog) {
        m_cylinderSelectionDialog->deleteLater();
        m_cylinderSelectionDialog = nullptr;
    }
    
    // Create and show cylinder selection dialog
    m_cylinderSelectionDialog = new CylinderSelectionDialog(cylinders, 0, this);
    
    // Connect dialog result
    connect(m_cylinderSelectionDialog, &QDialog::accepted, [this]() {
        int selectedIndex = m_cylinderSelectionDialog->getSelectedCylinderIndex();
        if (selectedIndex >= 0 && m_workspaceController) {
            bool success = m_workspaceController->selectWorkpieceCylinderAxis(selectedIndex);
            if (success) {
                if (m_outputWindow) {
                    m_outputWindow->append(QString("User selected cylinder %1 for turning axis").arg(selectedIndex + 1));
                }
                // Fit view to show updated workspace
                m_3dViewer->fitAll();
            } else {
                QMessageBox::warning(this, "Selection Error", 
                                   QString("Failed to apply selected cylinder axis %1").arg(selectedIndex + 1));
            }
        }
        m_cylinderSelectionDialog->deleteLater();
        m_cylinderSelectionDialog = nullptr;
    });
    
    connect(m_cylinderSelectionDialog, &QDialog::rejected, [this]() {
        if (m_outputWindow) {
            m_outputWindow->append("Cylinder selection cancelled - Using automatic selection");
        }
        statusBar()->showMessage("Cylinder selection cancelled", 3000);
        m_cylinderSelectionDialog->deleteLater();
        m_cylinderSelectionDialog = nullptr;
    });
    
    // Show dialog
    m_cylinderSelectionDialog->show();
    m_cylinderSelectionDialog->raise();
    m_cylinderSelectionDialog->activateWindow();
} 