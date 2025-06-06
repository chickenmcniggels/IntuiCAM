#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "workspacecontroller.h"
#include "partloadingpanel.h"
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
    , m_partLoadingPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_outputWindow(nullptr)
    , m_workspaceController(nullptr)
    , m_stepLoader(nullptr)
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
    
    // Initialize view mode action text (start in 3D mode)
    if (m_toggleViewModeAction) {
        m_toggleViewModeAction->setText(tr("Switch to &Lathe View"));
    }
    
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
    
    m_toggleViewModeAction = new QAction(tr("Toggle &Lathe View"), this);
    m_toggleViewModeAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_toggleViewModeAction->setStatusTip(tr("Toggle between 3D view and XZ plane (lathe) view"));
    m_toggleViewModeAction->setCheckable(false);  // We'll update the text instead
    m_viewMenu->addAction(m_toggleViewModeAction);
    
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
    
    // Add view mode toggle to toolbar
    if (m_toggleViewModeAction) m_mainToolBar->addAction(m_toggleViewModeAction);
}

void MainWindow::createCentralWidget()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Main horizontal splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left vertical splitter for project tree, part loading panel, and properties
    m_leftSplitter = new QSplitter(Qt::Vertical);
    
    // Project tree
    m_projectTree = new QTreeWidget;
    m_projectTree->setHeaderLabel("Project");
    m_projectTree->setMinimumWidth(280);
    m_projectTree->setMaximumWidth(450);
    
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
    
    // Part loading panel
    m_partLoadingPanel = new PartLoadingPanel();
    m_partLoadingPanel->setMinimumHeight(300);
    m_partLoadingPanel->setMaximumHeight(600);
    
    // Properties panel
    m_propertiesPanel = new QTextEdit;
    m_propertiesPanel->setMaximumHeight(150);
    m_propertiesPanel->setPlainText("Properties panel - Select an item to view details");
    m_propertiesPanel->setReadOnly(true);
    
    // Add to left splitter
    m_leftSplitter->addWidget(m_projectTree);
    m_leftSplitter->addWidget(m_partLoadingPanel);
    m_leftSplitter->addWidget(m_propertiesPanel);
    m_leftSplitter->setSizes({200, 350, 100});
    
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
    
    // Set splitter sizes (left panel 30%, viewport 70%)
    m_mainSplitter->setSizes({350, 800});
    
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
    if (m_toggleViewModeAction) connect(m_toggleViewModeAction, &QAction::triggered, this, &MainWindow::toggleViewMode);
    
    // Connect 3D viewer initialization
    connect(m_3dViewer, &OpenGL3DWidget::viewerInitialized,
            this, &MainWindow::setupWorkspaceConnections);
    
    // Setup part loading panel connections
    setupPartLoadingConnections();
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
        
        // Connect 3D viewer selection for manual axis selection
        connect(m_3dViewer, &OpenGL3DWidget::shapeSelected,
                this, &MainWindow::handleShapeSelected);
        
        // Connect view mode changes
        connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged,
                this, &MainWindow::handleViewModeChanged);
        
        qDebug() << "Workspace controller connections established";
    }
}

void MainWindow::setupPartLoadingConnections()
{
    if (!m_partLoadingPanel) return;

    // Connect part loading panel signals
    connect(m_partLoadingPanel, &PartLoadingPanel::distanceToChuckChanged,
            this, &MainWindow::handlePartLoadingDistanceChanged);
    
    connect(m_partLoadingPanel, &PartLoadingPanel::rawMaterialDiameterChanged,
            this, &MainWindow::handlePartLoadingDiameterChanged);
    
    connect(m_partLoadingPanel, &PartLoadingPanel::orientationFlipped,
            this, &MainWindow::handlePartLoadingOrientationFlipped);
    
    connect(m_partLoadingPanel, &PartLoadingPanel::cylinderSelectionChanged,
            this, &MainWindow::handlePartLoadingCylinderChanged);
    
    connect(m_partLoadingPanel, &PartLoadingPanel::manualAxisSelectionRequested,
            this, &MainWindow::handlePartLoadingManualSelection);
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
    if (m_workspaceController && m_3dViewer) {
        // Get the context from the viewer and initialize workspace controller
        Handle(AIS_InteractiveContext) context = m_3dViewer->getContext();
        
        if (!context.IsNull()) {
            m_workspaceController->initialize(context, m_stepLoader);
            if (m_outputWindow) {
                m_outputWindow->append("Workspace controller initialized successfully");
            }
            
            // Setup connections for workspace events
            connect(m_workspaceController, &WorkspaceController::chuckInitialized,
                    this, &MainWindow::handleChuckInitialized);
            connect(m_workspaceController, &WorkspaceController::chuckCenterlineDetected,
                    this, &MainWindow::handleChuckCenterlineDetected);
            connect(m_workspaceController, &WorkspaceController::multipleCylindersDetected,
                    this, &MainWindow::handleMultipleCylindersDetected);
            connect(m_workspaceController, &WorkspaceController::cylinderAxisSelected,
                    this, &MainWindow::handleCylinderAxisSelected);
            connect(m_workspaceController, &WorkspaceController::workpieceWorkflowCompleted,
                    this, &MainWindow::handleWorkpieceWorkflowCompleted);
            
            // Enable auto-fit for initial file loading, but disable for parameter updates
            m_3dViewer->setAutoFitEnabled(true);
            
            // Automatically load the chuck
            QString chuckFilePath = "C:/Users/nikla/Downloads/three_jaw_chuck.step";
            bool chuckSuccess = m_workspaceController->initializeChuck(chuckFilePath);
            if (chuckSuccess) {
                if (m_outputWindow) {
                    m_outputWindow->append("Chuck loaded successfully from: " + chuckFilePath);
                }
                statusBar()->showMessage("Workspace and chuck ready", 2000);
            } else {
                if (m_outputWindow) {
                    m_outputWindow->append("Warning: Failed to load chuck from: " + chuckFilePath);
                }
                statusBar()->showMessage("Workspace ready (chuck not loaded)", 3000);
            }
        } else {
            statusBar()->showMessage("Failed to get viewer context", 3000);
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
        m_outputWindow->append(QString("Multiple cylinders detected (%1 total) - Use part loading panel to select").arg(cylinders.size()));
        for (int i = 0; i < cylinders.size(); ++i) {
            m_outputWindow->append(QString("  %1. %2").arg(i + 1).arg(cylinders[i].description));
        }
    }
    statusBar()->showMessage("Multiple cylinders detected - Use part loading panel", 5000);
    
    // Update part loading panel with cylinder information
    if (m_partLoadingPanel) {
        m_partLoadingPanel->updateCylinderInfo(cylinders);
    }
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

void MainWindow::handlePartLoadingDistanceChanged(double distance)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Distance to chuck changed: %1 mm").arg(distance, 0, 'f', 1));
    }
    
    // Auto-apply distance change through workspace controller
    if (m_workspaceController) {
        bool success = m_workspaceController->updateDistanceToChuck(distance);
        if (success) {
            statusBar()->showMessage(QString("Distance to chuck updated: %1 mm").arg(distance, 0, 'f', 1), 2000);
            // The WorkspaceController already handles the context update, just ensure the viewer refreshes
            if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
                m_3dViewer->update();
            }
        } else {
            statusBar()->showMessage("Failed to update distance to chuck", 3000);
        }
    }
}

void MainWindow::handlePartLoadingDiameterChanged(double diameter)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material diameter changed: %1 mm").arg(diameter, 0, 'f', 1));
    }
    
    // Auto-apply diameter change through workspace controller
    if (m_workspaceController) {
        bool success = m_workspaceController->updateRawMaterialDiameter(diameter);
        if (success) {
            statusBar()->showMessage(QString("Raw material diameter updated: %1 mm").arg(diameter, 0, 'f', 1), 2000);
            // The WorkspaceController already handles the context update, just ensure the viewer refreshes
            if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
                m_3dViewer->update();
            }
        } else {
            statusBar()->showMessage("Failed to update raw material diameter", 3000);
        }
    }
}

void MainWindow::handlePartLoadingOrientationFlipped(bool flipped)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Part orientation %1").arg(flipped ? "flipped" : "restored"));
    }
    
    // Auto-apply orientation flip through workspace controller
    if (m_workspaceController) {
        bool success = m_workspaceController->flipWorkpieceOrientation(flipped);
        if (success) {
            statusBar()->showMessage(QString("Part orientation %1").arg(flipped ? "flipped" : "restored"), 2000);
            // The WorkspaceController already handles the context update, just ensure the viewer refreshes
            if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
                m_3dViewer->update();
            }
        } else {
            statusBar()->showMessage("Failed to flip part orientation", 3000);
        }
    }
}

void MainWindow::handlePartLoadingCylinderChanged(int index)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Selected cylinder axis changed to index: %1").arg(index));
    }
    
    // Apply cylinder selection through workspace controller
    if (m_workspaceController && index >= 0) {
        bool success = m_workspaceController->selectWorkpieceCylinderAxis(index);
        if (success) {
            if (m_outputWindow) {
                m_outputWindow->append(QString("Applied cylinder axis selection: %1").arg(index));
            }
            // The WorkspaceController already handles the context update, just ensure the viewer refreshes
            if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
                m_3dViewer->update();
            }
        } else {
            QMessageBox::warning(this, "Selection Error", 
                               QString("Failed to apply selected cylinder axis %1").arg(index + 1));
        }
    }
}

void MainWindow::handlePartLoadingManualSelection()
{
    if (m_outputWindow) {
        m_outputWindow->append("Manual axis selection requested - Click on a cylindrical face or edge in the 3D view");
    }
    statusBar()->showMessage("Click on a cylindrical face or edge to select axis", 5000);
    
    // Enable selection mode in the 3D viewer
    if (m_3dViewer) {
        m_3dViewer->setSelectionMode(true);
        if (m_outputWindow) {
            m_outputWindow->append("Selection mode enabled - click on the workpiece to select an axis");
        }
    }
}

void MainWindow::handleShapeSelected(const TopoDS_Shape& shape, const gp_Pnt& clickPoint)
{
    // Disable selection mode after selection
    if (m_3dViewer) {
        m_3dViewer->setSelectionMode(false);
    }
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Shape selected at point: (%1, %2, %3)")
                              .arg(clickPoint.X(), 0, 'f', 2)
                              .arg(clickPoint.Y(), 0, 'f', 2)
                              .arg(clickPoint.Z(), 0, 'f', 2));
    }
    
    // Process the selected shape through workspace controller
    if (m_workspaceController) {
        // TODO: Implement shape analysis to extract cylindrical axis
        // For now, just provide feedback
        statusBar()->showMessage("Shape selected - analyzing for cylindrical features...", 3000);
        
        if (m_outputWindow) {
            m_outputWindow->append("Analyzing selected shape for cylindrical features...");
            m_outputWindow->append("Note: Manual axis selection from 3D view is not yet fully implemented");
        }
    }
}

void MainWindow::handlePartLoadingReprocess()
{
    if (m_outputWindow) {
        m_outputWindow->append("Reprocessing part loading workflow...");
    }
    statusBar()->showMessage("Reprocessing part loading workflow...", 3000);
    
    // Trigger reprocessing through workspace controller
    if (m_workspaceController) {
        bool success = m_workspaceController->reprocessCurrentWorkpiece();
        if (success) {
            if (m_outputWindow) {
                m_outputWindow->append("Part loading workflow reprocessed successfully");
            }
            statusBar()->showMessage("Workflow reprocessed successfully", 3000);
            m_3dViewer->fitAll();
        } else {
            if (m_outputWindow) {
                m_outputWindow->append("Failed to reprocess part loading workflow");
            }
            statusBar()->showMessage("Failed to reprocess workflow", 3000);
        }
    }
}

void MainWindow::toggleViewMode()
{
    if (m_3dViewer) {
        m_3dViewer->toggleViewMode();
    }
}

void MainWindow::handleViewModeChanged(ViewMode mode)
{
    if (mode == ViewMode::Mode3D) {
        if (m_toggleViewModeAction) {
            m_toggleViewModeAction->setText(tr("Switch to &Lathe View"));
            m_toggleViewModeAction->setStatusTip(tr("Switch to XZ plane (lathe) view"));
        }
        statusBar()->showMessage("Switched to 3D view mode", 2000);
        if (m_outputWindow) {
            m_outputWindow->append("View mode: 3D - Full rotation and zoom available");
        }
    } else if (mode == ViewMode::LatheXZ) {
        if (m_toggleViewModeAction) {
            m_toggleViewModeAction->setText(tr("Switch to &3D View"));
            m_toggleViewModeAction->setStatusTip(tr("Switch to full 3D view"));
        }
        statusBar()->showMessage("Switched to lathe XZ view mode", 2000);
        if (m_outputWindow) {
            m_outputWindow->append("View mode: Lathe XZ - X increases top to bottom, Z left to right");
            m_outputWindow->append("Use left click to pan, wheel to zoom. Rotation disabled in this mode.");
        }
    }
} 