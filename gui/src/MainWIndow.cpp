#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "workspacecontroller.h"
#include "partloadingpanel.h"
#include "setupconfigurationpanel.h"
#include "workpiecemanager.h"  // For CylinderInfo definition
#include "rawmaterialmanager.h"  // For RawMaterialManager signals

// Namespace usage
using namespace IntuiCAM::GUI;

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
#include <QPushButton>
#include <QGroupBox>
#include <QFrame>
#include <QFileInfo>
#include <QMessageBox>

// OpenCASCADE includes for gp_Ax1
#include <gp_Ax1.hxx>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_tabWidget(nullptr)
    , m_homeTab(nullptr)
    , m_setupTab(nullptr)
    , m_mainSplitter(nullptr)
    , m_setupConfigPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_simulateButton(nullptr)
    , m_leftSplitter(nullptr)
    , m_projectTree(nullptr)
    , m_propertiesPanel(nullptr)
    , m_partLoadingPanel(nullptr)
    , m_simulationTab(nullptr)
    , m_simulationViewport(nullptr)
    , m_simulationControls(nullptr)
    , m_uploadToMachineButton(nullptr)
    , m_exportGCodeButton(nullptr)
    , m_machineTab(nullptr)
    , m_machineFeedWidget(nullptr)
    , m_machineControlPanel(nullptr)
    , m_outputWindow(nullptr)
    , m_workspaceController(nullptr)
    , m_stepLoader(nullptr)
    , m_viewModeOverlayButton(nullptr)
{
    setWindowTitle("IntuiCAM - Computer Aided Manufacturing");
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    // Initialize components following modular architecture
    m_stepLoader = new StepLoader();
    m_workspaceController = new WorkspaceController(this);
    
    createMenus();
    createCentralWidget();
    createStatusBar();
    setupConnections();
    
    // Create the view mode overlay button
    createViewModeOverlayButton();
    
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

// Toolbar removed per user request - project actions are available in File menu
// and view toggle button moved to Setup tab

void MainWindow::createCentralWidget()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Create main tab widget
    m_tabWidget = new QTabWidget;
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setMovable(false);
    m_tabWidget->setUsesScrollButtons(false);
    
    // Use default Qt styling
    
    // Create tabs
    m_homeTab = createHomeTab();
    m_setupTab = createSetupTab();
    m_simulationTab = createSimulationTab();
    m_machineTab = createMachineTab();
    
    // Add tabs to tab widget
    m_tabWidget->addTab(m_homeTab, "Home");
    m_tabWidget->addTab(m_setupTab, "Setup");
    m_tabWidget->addTab(m_simulationTab, "Simulation");
    m_tabWidget->addTab(m_machineTab, "Machine");
    
    // Output window (shared across all tabs)
    m_outputWindow = new QTextEdit;
    m_outputWindow->setMaximumHeight(150);
    m_outputWindow->setPlainText("Output Log:\nWelcome to IntuiCAM - Computer Aided Manufacturing\nApplication started successfully.\n");
    m_outputWindow->setReadOnly(true);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_outputWindow);
    
    m_centralWidget->setLayout(mainLayout);
    
    // Connect tab change signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Start on Setup tab (index 1) since that's where the action is
    m_tabWidget->setCurrentIndex(1);
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
        
        // Set workspace controller reference in 3D viewer for selection filtering
        m_3dViewer->setWorkspaceController(m_workspaceController);
        
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
        
        // Connect manual axis selection from 3D view
        connect(m_workspaceController, &WorkspaceController::manualAxisSelected,
                this, &MainWindow::handleManualAxisSelected);
        
        // Connect raw material creation for length updates
        if (m_workspaceController->getRawMaterialManager()) {
            connect(m_workspaceController->getRawMaterialManager(), &RawMaterialManager::rawMaterialCreated,
                    this, &MainWindow::handleRawMaterialCreated);
        }
        
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
    
    // Update the setup panel with the raw material diameter
    if (m_setupConfigPanel) {
        m_setupConfigPanel->setRawDiameter(rawMaterialDiameter);
    }
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

void MainWindow::handleManualAxisSelected(double diameter, const gp_Ax1& axis)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Manual rotational axis selected - Diameter: %1mm")
                              .arg(diameter, 0, 'f', 1));
        m_outputWindow->append(QString("Axis location: (%1, %2, %3), Direction: (%4, %5, %6)")
                              .arg(axis.Location().X(), 0, 'f', 2)
                              .arg(axis.Location().Y(), 0, 'f', 2)
                              .arg(axis.Location().Z(), 0, 'f', 2)
                              .arg(axis.Direction().X(), 0, 'f', 3)
                              .arg(axis.Direction().Y(), 0, 'f', 3)
                              .arg(axis.Direction().Z(), 0, 'f', 3));
    }
    statusBar()->showMessage("Manual rotational axis selected and workpiece aligned", 3000);
    
    // Update the setup panel axis info
    if (m_setupConfigPanel) {
        QString axisInfo = QString("✓ Rotational axis selected - Diameter: %1mm").arg(diameter, 0, 'f', 1);
        m_setupConfigPanel->updateAxisInfo(axisInfo);
    }
}

void MainWindow::handleRawMaterialCreated(double diameter, double length)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material created - Diameter: %1mm, Length: %2mm (auto-calculated)")
                              .arg(diameter, 0, 'f', 1)
                              .arg(length, 0, 'f', 1));
    }
    
    // Update the setup panel length display
    if (m_setupConfigPanel) {
        m_setupConfigPanel->updateRawMaterialLength(length);
    }
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
        statusBar()->showMessage("Analyzing selected geometry for cylindrical features...", 3000);
        
        if (m_outputWindow) {
            m_outputWindow->append("Analyzing selected shape for cylindrical features...");
        }
        
        // Use the new manual axis selection functionality
        bool success = m_workspaceController->processManualAxisSelection(shape, clickPoint);
        
        if (success) {
            if (m_outputWindow) {
                m_outputWindow->append("✓ Successfully extracted cylindrical axis and aligned workpiece with Z-axis");
            }
            statusBar()->showMessage("Rotational axis selected and workpiece aligned", 3000);
            
            // Update the viewer to show the transformation
            if (m_3dViewer && m_3dViewer->isViewerInitialized()) {
                m_3dViewer->update();
            }
        } else {
            if (m_outputWindow) {
                m_outputWindow->append("✗ Failed to extract cylindrical axis from selected geometry");
                m_outputWindow->append("Please select a cylindrical face or circular edge from the workpiece");
            }
            statusBar()->showMessage("Invalid selection - please select cylindrical geometry", 5000);
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

void MainWindow::onTabChanged(int index)
{
    QString tabName;
    switch (index) {
        case 0: tabName = "Home"; break;
        case 1: tabName = "Setup"; break;
        case 2: tabName = "Simulation"; break;
        case 3: tabName = "Machine"; break;
        default: tabName = "Unknown"; break;
    }
    
    // Update overlay button visibility and position based on current tab
    positionViewModeOverlayButton();
    
    statusBar()->showMessage(QString("Switched to %1 tab").arg(tabName), 2000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Switched to %1 tab").arg(tabName));
    }
}

void MainWindow::simulateToolpaths()
{
    if (m_outputWindow) {
        m_outputWindow->append("Generating toolpaths and G-code...");
    }
    statusBar()->showMessage("Generating toolpaths...", 3000);
    
    // TODO: Implement actual toolpath generation
    // For now, just switch to simulation tab
    m_tabWidget->setCurrentIndex(2); // Switch to Simulation tab
    
    if (m_outputWindow) {
        m_outputWindow->append("✓ Toolpaths generated successfully - Switching to Simulation view");
    }
    statusBar()->showMessage("Ready for simulation", 2000);
}

QWidget* MainWindow::createHomeTab()
{
    QWidget* homeWidget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(homeWidget);
    layout->setSpacing(20);
    layout->setContentsMargins(40, 40, 40, 40);
    
    // Welcome section
    QLabel* welcomeLabel = new QLabel("Welcome to IntuiCAM");
    QFont titleFont = welcomeLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    welcomeLabel->setFont(titleFont);
    
    QLabel* subtitleLabel = new QLabel("Professional CAM software for CNC turning operations");
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(12);
    subtitleLabel->setFont(subtitleFont);
    
    // Quick actions section
    QGroupBox* quickActionsGroup = new QGroupBox("Quick Actions");
    
    QHBoxLayout* actionsLayout = new QHBoxLayout(quickActionsGroup);
    actionsLayout->setSpacing(15);
    
    QPushButton* newProjectBtn = new QPushButton("New Project");
    QPushButton* openProjectBtn = new QPushButton("Open Project");
    QPushButton* importStepBtn = new QPushButton("Import STEP File");
    
    // Use default button styling
    newProjectBtn->setMinimumWidth(140);
    openProjectBtn->setMinimumWidth(140);
    importStepBtn->setMinimumWidth(140);
    
    actionsLayout->addWidget(newProjectBtn);
    actionsLayout->addWidget(openProjectBtn);
    actionsLayout->addWidget(importStepBtn);
    actionsLayout->addStretch();
    
    // Recent projects section
    QGroupBox* recentGroup = new QGroupBox("Recent Projects");
    
    QVBoxLayout* recentLayout = new QVBoxLayout(recentGroup);
    
    QLabel* noRecentLabel = new QLabel("No recent projects");
    QFont italicFont = noRecentLabel->font();
    italicFont.setItalic(true);
    noRecentLabel->setFont(italicFont);
    recentLayout->addWidget(noRecentLabel);
    
    // Get started section
    QGroupBox* getStartedGroup = new QGroupBox("Getting Started");
    
    QVBoxLayout* startedLayout = new QVBoxLayout(getStartedGroup);
    
    QLabel* stepLabel = new QLabel(
        "1. Import your STEP file in the Setup tab\n"
        "2. Configure part positioning and raw material\n"
        "3. Generate toolpaths and G-code\n"
        "4. Simulate the machining process\n"
        "5. Export or upload to your CNC machine"
    );
    startedLayout->addWidget(stepLabel);
    
    // Layout
    layout->addWidget(welcomeLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(quickActionsGroup);
    layout->addWidget(recentGroup);
    layout->addWidget(getStartedGroup);
    layout->addStretch();
    
    // Connect buttons
    connect(newProjectBtn, &QPushButton::clicked, this, &MainWindow::newProject);
    connect(openProjectBtn, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(importStepBtn, &QPushButton::clicked, [this]() {
        m_tabWidget->setCurrentIndex(1); // Switch to Setup tab
        openStepFile();
    });
    
    return homeWidget;
}

QWidget* MainWindow::createSetupTab()
{
    QWidget* setupWidget = new QWidget;
    QVBoxLayout* setupLayout = new QVBoxLayout(setupWidget);
    setupLayout->setContentsMargins(0, 0, 0, 0);
    setupLayout->setSpacing(0);
    
    // New Orca Slicer-inspired layout: Left panel + Right 3D viewer
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    
    // Left side: Setup Configuration Panel (Orca Slicer-inspired)
    m_setupConfigPanel = new SetupConfigurationPanel();
    m_setupConfigPanel->setMinimumWidth(350);
    m_setupConfigPanel->setMaximumWidth(450);
    
    // Right side: 3D viewer and operation controls
    QWidget* rightWidget = new QWidget;
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(8);
    
    // 3D Viewport - Pure visualization component
    m_3dViewer = new OpenGL3DWidget();
    m_3dViewer->setMinimumSize(600, 400);
    
    // Operation controls frame
    QFrame* operationFrame = new QFrame;
    operationFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
    operationFrame->setStyleSheet(
        "QFrame {"
        "  background-color: #F8F9FA;"
        "  border: 2px solid #DEE2E6;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "}"
    );
    
    QHBoxLayout* operationLayout = new QHBoxLayout(operationFrame);
    operationLayout->setSpacing(12);
    
    // Simulate button (now integrated with other controls)
    m_simulateButton = new QPushButton("▶ Simulate Toolpaths");
    m_simulateButton->setMinimumHeight(35);
    m_simulateButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #007BFF;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0056B3;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #004085;"
        "}"
    );
    
    // Additional operation controls
    QPushButton* exportButton = new QPushButton("💾 Export G-Code");
    exportButton->setMinimumHeight(35);
    exportButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #28A745;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1E7E34;"
        "}"
    );
    
    // Status indicator
    QLabel* statusIndicator = new QLabel("⚡ Ready for CAM operations");
    statusIndicator->setStyleSheet(
        "QLabel {"
        "  color: #495057;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "}"
    );
    
    operationLayout->addWidget(statusIndicator);
    operationLayout->addStretch();
    operationLayout->addWidget(m_simulateButton);
    operationLayout->addWidget(exportButton);
    
    rightLayout->addWidget(m_3dViewer, 1);
    rightLayout->addWidget(operationFrame);
    
    // Add to main splitter
    m_mainSplitter->addWidget(m_setupConfigPanel);
    m_mainSplitter->addWidget(rightWidget);
    
    // Set splitter sizes (left panel 30%, viewport 70%)
    m_mainSplitter->setSizes({350, 850});
    
    setupLayout->addWidget(m_mainSplitter);
    
    // Create legacy components for backward compatibility (initially hidden)
    m_leftSplitter = new QSplitter(Qt::Vertical);
    m_leftSplitter->hide(); // Hide initially
    
    m_projectTree = new QTreeWidget;
    m_projectTree->setHeaderLabel("Project");
    m_projectTree->hide();
    
    m_partLoadingPanel = new PartLoadingPanel();
    m_partLoadingPanel->hide();
    
    m_propertiesPanel = new QTextEdit;
    m_propertiesPanel->hide();
    
    // Connect signals from new setup configuration panel
    connect(m_setupConfigPanel, &SetupConfigurationPanel::stepFileSelected,
            this, &MainWindow::handleStepFileSelected);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::configurationChanged,
            this, &MainWindow::handleSetupConfigurationChanged);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::materialTypeChanged,
            this, &MainWindow::handleMaterialTypeChanged);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::rawMaterialDiameterChanged,
            this, &MainWindow::handleRawMaterialDiameterChanged);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::distanceToChuckChanged,
            this, &MainWindow::handlePartLoadingDistanceChanged);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::orientationFlipped,
            this, &MainWindow::handlePartLoadingOrientationFlipped);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::manualAxisSelectionRequested,
            this, &MainWindow::handleManualAxisSelectionRequested);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::automaticToolpathGenerationRequested,
            this, &MainWindow::handleAutomaticToolpathGeneration);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::operationToggled,
            this, &MainWindow::handleOperationToggled);
    connect(m_setupConfigPanel, &SetupConfigurationPanel::operationParametersRequested,
            this, &MainWindow::handleOperationParametersRequested);
    
    // Connect existing simulate button
    connect(m_simulateButton, &QPushButton::clicked, this, &MainWindow::simulateToolpaths);
    
    // Connect export button
    connect(exportButton, &QPushButton::clicked, [this]() {
        if (m_outputWindow) {
            m_outputWindow->append("Exporting G-Code... (integrated with toolpath generation)");
        }
        statusBar()->showMessage("G-Code export initiated", 2000);
        // Switch to simulation tab for G-code preview
        m_tabWidget->setCurrentIndex(2);
    });
    
    return setupWidget;
}

QWidget* MainWindow::createSimulationTab()
{
    QWidget* simulationWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(simulationWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Left panel - simulation controls
    m_simulationControls = new QWidget;
    m_simulationControls->setMinimumWidth(300);
    m_simulationControls->setMaximumWidth(400);
    
    QVBoxLayout* controlsLayout = new QVBoxLayout(m_simulationControls);
    controlsLayout->setContentsMargins(15, 15, 15, 15);
    controlsLayout->setSpacing(15);
    
    // Simulation controls title
    QLabel* titleLabel = new QLabel("Simulation Controls");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    // Playback controls
    QGroupBox* playbackGroup = new QGroupBox("Playback");
    QVBoxLayout* playbackLayout = new QVBoxLayout(playbackGroup);
    
    QPushButton* playBtn = new QPushButton("▶ Play");
    QPushButton* pauseBtn = new QPushButton("⏸ Pause");
    QPushButton* stopBtn = new QPushButton("⏹ Stop");
    QPushButton* resetBtn = new QPushButton("⏮ Reset");
    
    playbackLayout->addWidget(playBtn);
    playbackLayout->addWidget(pauseBtn);
    playbackLayout->addWidget(stopBtn);
    playbackLayout->addWidget(resetBtn);
    
    // Export controls
    QGroupBox* exportGroup = new QGroupBox("Export & Upload");
    QVBoxLayout* exportLayout = new QVBoxLayout(exportGroup);
    
    m_exportGCodeButton = new QPushButton("Export G-Code");
    m_uploadToMachineButton = new QPushButton("Upload to Machine");
    
    // Use default button styling
    QFont buttonFont = m_exportGCodeButton->font();
    buttonFont.setWeight(QFont::Medium);
    m_exportGCodeButton->setFont(buttonFont);
    m_uploadToMachineButton->setFont(buttonFont);
    
    exportLayout->addWidget(m_exportGCodeButton);
    exportLayout->addWidget(m_uploadToMachineButton);
    
    // Simulation info
    QGroupBox* infoGroup = new QGroupBox("Simulation Info");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    QLabel* infoLabel = new QLabel(
        "• Toolpath visualization\n"
        "• Material removal simulation\n"
        "• Collision detection\n"
        "• Machining time estimation"
    );
    infoLayout->addWidget(infoLabel);
    
    // Layout controls
    controlsLayout->addWidget(titleLabel);
    controlsLayout->addWidget(playbackGroup);
    controlsLayout->addWidget(exportGroup);
    controlsLayout->addWidget(infoGroup);
    controlsLayout->addStretch();
    
    // Right side - simulation viewport
    m_simulationViewport = new QWidget;
    
    QVBoxLayout* viewportLayout = new QVBoxLayout(m_simulationViewport);
    QLabel* placeholderLabel = new QLabel("Simulation Viewport\n\n[Toolpath visualization will be displayed here]");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    QFont placeholderFont = placeholderLabel->font();
    placeholderFont.setPointSize(12);
    placeholderLabel->setFont(placeholderFont);
    placeholderLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
    placeholderLabel->setMargin(40);
    viewportLayout->addWidget(placeholderLabel);
    
    // Connect buttons (skeleton functionality)
    connect(m_exportGCodeButton, &QPushButton::clicked, [this]() {
        if (m_outputWindow) {
            m_outputWindow->append("Exporting G-Code... (not yet implemented)");
        }
        statusBar()->showMessage("G-Code export functionality coming soon", 3000);
    });
    
    connect(m_uploadToMachineButton, &QPushButton::clicked, [this]() {
        if (m_outputWindow) {
            m_outputWindow->append("Uploading to machine... (not yet implemented)");
        }
        statusBar()->showMessage("Machine upload functionality coming soon", 3000);
        // Switch to Machine tab
        m_tabWidget->setCurrentIndex(3);
    });
    
    layout->addWidget(m_simulationControls);
    layout->addWidget(m_simulationViewport, 1);
    
    return simulationWidget;
}

QWidget* MainWindow::createMachineTab()
{
    QWidget* machineWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(machineWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Left panel - machine controls
    m_machineControlPanel = new QWidget;
    m_machineControlPanel->setMinimumWidth(300);
    m_machineControlPanel->setMaximumWidth(400);
    
    QVBoxLayout* controlLayout = new QVBoxLayout(m_machineControlPanel);
    controlLayout->setContentsMargins(15, 15, 15, 15);
    controlLayout->setSpacing(15);
    
    // Machine controls title
    QLabel* titleLabel = new QLabel("Machine Control");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    // Connection status
    QGroupBox* connectionGroup = new QGroupBox("Connection Status");
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);
    
    QLabel* statusLabel = new QLabel("Status: Disconnected");
    QFont statusFont = statusLabel->font();
    statusFont.setBold(true);
    statusLabel->setFont(statusFont);
    
    QPushButton* connectBtn = new QPushButton("Connect to Machine");
    
    connectionLayout->addWidget(statusLabel);
    connectionLayout->addWidget(connectBtn);
    
    // Machine control buttons
    QGroupBox* controlGroup = new QGroupBox("Manual Control");
    QVBoxLayout* manualLayout = new QVBoxLayout(controlGroup);
    
    QPushButton* homeBtn = new QPushButton("Home Machine");
    QPushButton* jogBtn = new QPushButton("Jog Mode");
    QPushButton* emergencyBtn = new QPushButton("Emergency Stop");
    
    // Make emergency button bold to indicate importance
    QFont emergencyFont = emergencyBtn->font();
    emergencyFont.setBold(true);
    emergencyBtn->setFont(emergencyFont);
    
    manualLayout->addWidget(homeBtn);
    manualLayout->addWidget(jogBtn);
    manualLayout->addWidget(emergencyBtn);
    
    // Machine info
    QGroupBox* infoGroup = new QGroupBox("Machine Information");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    QLabel* infoLabel = new QLabel(
        "Model: Not Connected\n"
        "Position: X: 0.00  Z: 0.00\n"
        "Spindle: Stopped\n"
        "Feed Rate: 0 mm/min"
    );
    QFont monoFont("Courier", infoLabel->font().pointSize());
    infoLabel->setFont(monoFont);
    infoLayout->addWidget(infoLabel);
    
    // Layout controls
    controlLayout->addWidget(titleLabel);
    controlLayout->addWidget(connectionGroup);
    controlLayout->addWidget(controlGroup);
    controlLayout->addWidget(infoGroup);
    controlLayout->addStretch();
    
    // Right side - machine feed/camera view
    m_machineFeedWidget = new QWidget;
    
    QVBoxLayout* feedLayout = new QVBoxLayout(m_machineFeedWidget);
    QLabel* feedLabel = new QLabel("Machine Camera Feed\n\n[Live feed from CNC machine will be displayed here]");
    feedLabel->setAlignment(Qt::AlignCenter);
    QFont feedFont = feedLabel->font();
    feedFont.setPointSize(12);
    feedLabel->setFont(feedFont);
    feedLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
    feedLabel->setMargin(40);
    feedLayout->addWidget(feedLabel);
    
    // Connect buttons (skeleton functionality)
    connect(connectBtn, &QPushButton::clicked, [this, statusLabel]() {
        if (m_outputWindow) {
            m_outputWindow->append("Attempting to connect to machine... (not yet implemented)");
        }
        statusLabel->setText("Status: Connecting...");
        statusBar()->showMessage("Machine connection functionality coming soon", 3000);
    });
    
    layout->addWidget(m_machineControlPanel);
    layout->addWidget(m_machineFeedWidget, 1);
    
    return machineWidget;
}

void MainWindow::createViewModeOverlayButton()
{
    // Create the overlay button as a direct child of the main window
    m_viewModeOverlayButton = new QPushButton("Switch to Lathe View", this);
    m_viewModeOverlayButton->setMaximumWidth(150);
    m_viewModeOverlayButton->setMaximumHeight(30);
    
    // Style the button to be semi-transparent and visually appealing
    m_viewModeOverlayButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(240, 240, 240, 220);"
        "  border: 2px solid #666666;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  color: #333333;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 240);"
        "  border: 2px solid #444444;"
        "  color: #222222;"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(200, 200, 200, 220);"
        "  border: 2px solid #555555;"
        "}"
    );
    
    // Connect the button to the toggle function
    connect(m_viewModeOverlayButton, &QPushButton::clicked, this, &MainWindow::toggleViewMode);
    
    // Connect to view mode changes to update button text
    connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged, this, &MainWindow::updateViewModeOverlayButton);
    
    // Initially position the button
    positionViewModeOverlayButton();
    
    // Show the button - it will always be visible now
    m_viewModeOverlayButton->show();
    m_viewModeOverlayButton->raise();
    
    qDebug() << "View mode overlay button created in MainWindow";
}

void MainWindow::updateViewModeOverlayButton()
{
    if (!m_viewModeOverlayButton || !m_3dViewer) {
        return;
    }
    
    // Update button text based on current view mode
    ViewMode currentMode = m_3dViewer->getViewMode();
    if (currentMode == ViewMode::Mode3D) {
        m_viewModeOverlayButton->setText("Switch to Lathe View");
    } else {
        m_viewModeOverlayButton->setText("Switch to 3D View");
    }
    
    // Ensure button stays positioned correctly and on top
    positionViewModeOverlayButton();
    m_viewModeOverlayButton->raise();
}

void MainWindow::positionViewModeOverlayButton()
{
    if (!m_viewModeOverlayButton || !m_3dViewer) {
        return;
    }
    
    // Only show the button when we're on the Setup tab (where the 3D viewer is)
    bool onSetupTab = (m_tabWidget && m_tabWidget->currentIndex() == 1);
    if (!onSetupTab) {
        m_viewModeOverlayButton->hide();
        return;
    }
    
    m_viewModeOverlayButton->show();
    
    // Calculate position relative to the 3D viewer widget
    QPoint viewerGlobalPos = m_3dViewer->mapToGlobal(QPoint(0, 0));
    QPoint mainWindowPos = this->mapFromGlobal(viewerGlobalPos);
    
    // Position in top-right corner of the 3D viewer with margin
    const int margin = 15;
    const int buttonWidth = m_viewModeOverlayButton->sizeHint().width();
    const int buttonHeight = m_viewModeOverlayButton->sizeHint().height();
    
    int x = mainWindowPos.x() + m_3dViewer->width() - buttonWidth - margin;
    int y = mainWindowPos.y() + margin;
    
    // Ensure the button stays within the main window bounds
    x = qMax(margin, qMin(x, width() - buttonWidth - margin));
    y = qMax(margin, qMin(y, height() - buttonHeight - margin));
    
    m_viewModeOverlayButton->move(x, y);
    m_viewModeOverlayButton->raise();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Reposition the overlay button when the window is resized
    QTimer::singleShot(0, this, &MainWindow::positionViewModeOverlayButton);
}

// New slot implementations for Setup Configuration Panel
void MainWindow::handleStepFileSelected(const QString& filePath)
{
    if (filePath.isEmpty()) {
        statusBar()->showMessage(tr("No file selected"), 2000);
        return;
    }
    
    statusBar()->showMessage(tr("Loading STEP file..."), 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Loading STEP file: %1").arg(filePath));
    }
    
    // Load the STEP file using workspace controller - same logic as original openStepFile()
    if (m_stepLoader && m_workspaceController && m_workspaceController->isInitialized()) {
        TopoDS_Shape shape = m_stepLoader->loadStepFile(filePath);
        
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
                if (m_3dViewer) {
                    m_3dViewer->fitAll();
                }
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

void MainWindow::handleSetupConfigurationChanged()
{
    if (m_outputWindow) {
        m_outputWindow->append("Setup configuration changed - updating CAM parameters");
    }
    
    // Update status
    statusBar()->showMessage("Configuration updated", 1500);
}

void MainWindow::handleMaterialTypeChanged(IntuiCAM::GUI::MaterialType material)
{
    if (m_outputWindow) {
        // Convert enum back to material name for display using our utility function
        QString materialName = SetupConfigurationPanel::materialTypeToString(material);
        m_outputWindow->append(QString("Material changed to: %1").arg(materialName));
    }
    
    statusBar()->showMessage("Material type updated - cutting parameters adjusted", 2000);
}

void MainWindow::handleRawMaterialDiameterChanged(double diameter)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material diameter changed: %1 mm").arg(diameter, 0, 'f', 1));
    }
    
    // Auto-apply diameter change through workspace controller (same logic as legacy system)
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

void MainWindow::handleManualAxisSelectionRequested()
{
    if (m_outputWindow) {
        m_outputWindow->append("Manual axis selection requested - Click on a cylindrical face or edge in the 3D view");
    }
    statusBar()->showMessage("Click on a cylindrical face or edge to select rotational axis", 5000);
    
    // Enable selection mode in the 3D viewer
    if (m_3dViewer) {
        m_3dViewer->setSelectionMode(true);
        if (m_outputWindow) {
            m_outputWindow->append("Selection mode enabled - click on the workpiece to select an axis");
        }
    }
}

void MainWindow::handleOperationToggled(const QString& operationName, bool enabled)
{
    if (m_outputWindow) {
        QString status = enabled ? "enabled" : "disabled";
        m_outputWindow->append(QString("Operation '%1' %2").arg(operationName, status));
    }
    
    statusBar()->showMessage(QString("%1 operation %2").arg(operationName, enabled ? "enabled" : "disabled"), 1500);
}

void MainWindow::handleOperationParametersRequested(const QString& operationName)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Opening parameter dialog for '%1' operation").arg(operationName));
    }
    
    // TODO: Open operation-specific parameter dialog
    statusBar()->showMessage(QString("Parameter dialog for %1 (coming soon)").arg(operationName), 2000);
    
    // For now, show a message box as placeholder
    QMessageBox::information(this, 
        QString("%1 Parameters").arg(operationName),
        QString("Parameter configuration for %1 operation will be available soon.\n\n"
                "This will include settings like:\n"
                "• Feed rates and speeds\n"
                "• Cutting depths\n"
                "• Tool selection\n"
                "• Safety margins").arg(operationName));
}

void MainWindow::handleAutomaticToolpathGeneration()
{
    if (m_outputWindow) {
        m_outputWindow->append("Starting automatic toolpath generation...");
    }
    
    statusBar()->showMessage("Generating toolpaths automatically...", 0);
    
    // Get enabled operations from setup panel
    if (m_setupConfigPanel) {
        QStringList enabledOps;
        if (m_setupConfigPanel->isOperationEnabled("Facing")) enabledOps << "Facing";
        if (m_setupConfigPanel->isOperationEnabled("Roughing")) enabledOps << "Roughing";
        if (m_setupConfigPanel->isOperationEnabled("Finishing")) enabledOps << "Finishing";
        if (m_setupConfigPanel->isOperationEnabled("Parting")) enabledOps << "Parting";
        
        if (m_outputWindow) {
            m_outputWindow->append(QString("Generating %1 operations:").arg(enabledOps.size()));
            for (const QString& op : enabledOps) {
                m_outputWindow->append(QString("  • %1").arg(op));
            }
        }
        
        // TODO: Integrate with actual toolpath generation
        // This would connect to the core toolpath generation system
        
        // For now, simulate the process with a delay
        QTimer::singleShot(2000, this, [this]() {
            if (m_outputWindow) {
                m_outputWindow->append("Toolpath generation completed successfully!");
                m_outputWindow->append("Ready for simulation or G-code export.");
            }
            statusBar()->showMessage("Toolpaths generated successfully", 3000);
            
            // Switch to simulation tab to show results
            if (m_tabWidget) {
                m_tabWidget->setCurrentIndex(2);
            }
        });
    }
} 