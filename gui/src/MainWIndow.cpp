#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "workspacecontroller.h"
#include "toolpathmanager.h"
#include "workpiecemanager.h"
#include "toolpathtimelinewidget.h"
#include "partloadingpanel.h"
#include "setupconfigurationpanel.h"
#include "operationparameterdialog.h"
#include "materialmanager.h"
#include "toolmanager.h"
#include "toolpathgenerationcontroller.h"
#include "rawmaterialmanager.h"  // For RawMaterialManager signals
#include "chuckmanager.h"

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
#include <QCheckBox>

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
    , m_toolpathTimeline(nullptr)
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
    , m_toolpathManager(nullptr)
    , m_workpieceManager(nullptr)
    , m_materialManager(nullptr)
    , m_toolManager(nullptr)
    , m_toolpathGenerationController(nullptr)
    , m_fileMenu(nullptr)
    , m_editMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_toolsMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_newAction(nullptr)
    , m_openAction(nullptr)
    , m_openStepAction(nullptr)
    , m_saveAction(nullptr)
    , m_exitAction(nullptr)
    , m_aboutAction(nullptr)
    , m_preferencesAction(nullptr)
    , m_toggleViewModeAction(nullptr)
    , m_viewModeOverlayButton(nullptr)
    , m_showChuckCheckBox(nullptr)
    , m_defaultChuckFilePath("C:/Users/nikla/Downloads/three_jaw_chuck.step")
{
    // Create business logic components
    m_workspaceController = new WorkspaceController(this);
    m_stepLoader = new StepLoader();
    m_toolpathManager = new ToolpathManager(this);
    m_workpieceManager = new WorkpieceManager(this);
    
    // Create material and tool managers
    m_materialManager = new IntuiCAM::GUI::MaterialManager(this);
    m_toolManager = new IntuiCAM::GUI::ToolManager(this);
    
    // Create toolpath generation controller
    m_toolpathGenerationController = new IntuiCAM::GUI::ToolpathGenerationController(this);
    
    // Create UI
    createMenus();
    createStatusBar();
    createCentralWidget();
    setupConnections();
    
    // Create the view mode overlay button
    createViewModeOverlayButton();
    
    // Update status
    statusBar()->showMessage(tr("Ready"));
    
    // Set window title and size
    setWindowTitle(tr("IntuiCAM - Computer Aided Manufacturing"));
    resize(1280, 800);
    
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
    
    // Debug menu
    QMenu* debugMenu = menuBar()->addMenu(tr("Debug"));
    
    QAction* actionUpdateToolpaths = debugMenu->addAction(tr("Force Update Toolpaths"));
    connect(actionUpdateToolpaths, &QAction::triggered, this, [this]() {
        if (m_toolpathManager) {
            // Force toolpath update with current workpiece transformation
            m_toolpathManager->applyWorkpieceTransformationToToolpaths();
            
            // Log to output window
            logToOutput("Forced update of all toolpaths with current workpiece transformation");
        }
    });
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
    
    // Setup all connections after UI components are created
    setupUiConnections();
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupConnections()
{
    // Connect menu actions
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(m_openStepAction, &QAction::triggered, this, &MainWindow::openStepFile);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::aboutApplication);
    connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::showPreferences);
    connect(m_toggleViewModeAction, &QAction::triggered, this, &MainWindow::toggleViewMode);
    
    // Connect 3D viewer initialization
    connect(m_3dViewer, &OpenGL3DWidget::viewerInitialized,
            this, &MainWindow::setupWorkspaceConnections);
    
    // Connect 3D viewer signals
    connect(m_3dViewer, &OpenGL3DWidget::shapeSelected, this, &MainWindow::handleShapeSelected);
    connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged, this, &MainWindow::handleViewModeChanged);
    
    // Setup part loading panel connections
    setupPartLoadingConnections();
    
    // Connect tab changed signals
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Connect setup configuration panel signals
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::stepFileSelected,
            this, &MainWindow::handleStepFileSelected);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::configurationChanged,
            this, &MainWindow::handleSetupConfigurationChanged);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::materialTypeChanged,
            this, &MainWindow::handleMaterialTypeChanged);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::rawMaterialDiameterChanged,
            this, &MainWindow::handleRawMaterialDiameterChanged);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::distanceToChuckChanged,
            this, &MainWindow::handlePartLoadingDistanceChanged);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::orientationFlipped,
            this, &MainWindow::handlePartLoadingOrientationFlipped);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::manualAxisSelectionRequested,
            this, &MainWindow::handleManualAxisSelectionRequested);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::automaticToolpathGenerationRequested,
            this, &MainWindow::handleAutomaticToolpathGeneration);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::operationToggled,
            this, &MainWindow::handleOperationToggled);
    connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::operationParametersRequested,
            this, &MainWindow::handleOperationParametersRequested);
    
    // Connect simulate button
    connect(m_simulateButton, &QPushButton::clicked, this, &MainWindow::simulateToolpaths);
    
    // Connect toolpath timeline with toolpath generation controller
    if (m_toolpathTimeline && m_toolpathGenerationController) {
        m_toolpathGenerationController->connectTimelineWidget(m_toolpathTimeline);
        
        // Connect toolpath generation controller signals to main window
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::toolpathAdded,
                this, [this](const QString& name, const QString& type, const QString& toolName) {
                    // Add the toolpath to the timeline
                    m_toolpathTimeline->addToolpath(name, type, toolName, QString());
                    
                    // Log to output window
                    logToOutput(QString("Added %1 toolpath: %2 with tool: %3").arg(type, name, toolName));
                });
        
        // Handle existing connection slots from MainWindow for timeline widget
        // These will now be handled by the ToolpathGenerationController
    }
    
    // Connect WorkpieceManager to ToolpathManager
    if (m_toolpathManager && m_workpieceManager) {
        m_toolpathManager->setWorkpieceManager(m_workpieceManager);
        
        // Connect workpiece transformation signals to update toolpaths
        connect(m_workpieceManager, &WorkpieceManager::workpieceTransformed, 
                m_toolpathManager, &ToolpathManager::applyWorkpieceTransformationToToolpaths);
    }
    
    // Connect workpiece position changes to toolpath updates
    if (m_workspaceController && m_toolpathManager) {
        // Connect using a direct connection to ensure immediate update
        connect(m_workspaceController, &WorkspaceController::workpiecePositionChanged,
                this, [this](double distance) {
                    // Ensure toolpaths update when workpiece position changes
                    if (m_toolpathManager) {
                        qDebug() << "MainWindow: Workpiece position changed to" << distance << "mm, updating toolpaths";
                        
                        // Force toolpath update with current workpiece transformation
                        QTimer::singleShot(100, [this]() {
                            if (m_toolpathManager) {
                                m_toolpathManager->applyWorkpieceTransformationToToolpaths();
                                qDebug() << "MainWindow: Applied toolpath transformations after position change";
                            }
                        });
                    }
                }, Qt::DirectConnection);
    }
    
    // Connect to the toolpath generation controller
    if (m_toolpathGenerationController) {
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationStarted,
                this, &MainWindow::handleToolpathGenerationStarted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::progressUpdated,
                this, &MainWindow::handleToolpathProgressUpdated);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::operationCompleted,
                this, &MainWindow::handleToolpathOperationCompleted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationCompleted,
                this, &MainWindow::handleToolpathGenerationCompleted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::errorOccurred,
                this, &MainWindow::handleToolpathGenerationError);
    }
    
    if (m_workspaceController && m_toolpathGenerationController) {
        connect(m_workspaceController->getWorkpieceManager(), &WorkpieceManager::workpieceTransformed,
                m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::regenerateAllToolpaths);
    }
}

void MainWindow::setupWorkspaceConnections()
{
    if (m_workspaceController && m_3dViewer) {
        // Get the context from the viewer and initialize workspace controller
        Handle(AIS_InteractiveContext) context = m_3dViewer->getContext();
        
        if (!context.IsNull()) {
            m_workspaceController->initialize(context, m_stepLoader);
            if (m_outputWindow) {
                m_outputWindow->append("Workspace controller initialized successfully");
            }
            
            // Initialize the toolpath generation controller with the same context
            if (m_toolpathGenerationController) {
                m_toolpathGenerationController->initialize(context);
                
                // Connect toolpath generation signals
                connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationStarted,
                        this, &MainWindow::handleToolpathGenerationStarted);
                
                connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::progressUpdated,
                        this, &MainWindow::handleToolpathProgressUpdated);
                
                connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::operationCompleted,
                        this, &MainWindow::handleToolpathOperationCompleted);
                
                connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationCompleted,
                        this, [this](const IntuiCAM::GUI::ToolpathGenerationController::GenerationResult& result) {
                            if (result.success) {
                                this->handleToolpathGenerationCompleted();
                                if (m_outputWindow) {
                                    m_outputWindow->append(QString("Toolpath generation completed successfully - %1 operations").arg(result.totalToolpaths));
                                }
                            } else {
                                this->handleToolpathGenerationError(result.errorMessage);
                                if (m_outputWindow) {
                                    m_outputWindow->append(QString("Toolpath generation failed: %1").arg(result.errorMessage));
                                }
                            }
                        });
                
                connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::errorOccurred,
                        this, &MainWindow::handleToolpathGenerationError);
                
                if (m_outputWindow) {
                    m_outputWindow->append("Toolpath generation controller initialized successfully");
                }
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
            connect(m_workspaceController, &WorkspaceController::manualAxisSelected,
                    this, &MainWindow::handleManualAxisSelected);
            connect(m_workspaceController, &WorkspaceController::errorOccurred,
                    this, &MainWindow::handleWorkspaceError);
            
            // Connect raw material creation for length updates
            if (m_workspaceController->getRawMaterialManager()) {
                connect(m_workspaceController->getRawMaterialManager(), &RawMaterialManager::rawMaterialCreated,
                        this, &MainWindow::handleRawMaterialCreated);
            }
            
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

void MainWindow::setupPartLoadingConnections()
{
    if (m_partLoadingPanel) {
        // Connect part loading panel signals to their handlers
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
    // Ask user if they want to save before exiting
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Exit"), tr("Do you want to save your project before exiting?"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    
    if (reply == QMessageBox::Yes) {
        saveProject();
        qApp->quit();
    } else if (reply == QMessageBox::No) {
        qApp->quit();
    }
    // If cancel, do nothing
}

void MainWindow::aboutApplication()
{
    QMessageBox::about(this, tr("About IntuiCAM"),
                       tr("<h2>IntuiCAM</h2>"
                          "<p>Version 1.0</p>"
                          "<p>IntuiCAM is a professional Computer-Aided Manufacturing (CAM) application "
                          "for CNC turning operations. It provides intuitive toolpath generation "
                          "with powerful visualization capabilities.</p>"
                          "<p>Copyright &copy; 2023 IntuiCAM Team</p>"));
}

void MainWindow::showPreferences()
{
    statusBar()->showMessage(tr("Opening preferences..."), 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Opening application preferences...");
    }
    // TODO: Implement preferences dialog
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
    if (!m_toolpathGenerationController) {
        if (m_outputWindow) {
            m_outputWindow->append("Error: Toolpath generation controller not initialized");
        }
        statusBar()->showMessage("Error: Toolpath generation controller not initialized", 3000);
        return;
    }
    
    // Switch to the simulation tab
    m_tabWidget->setCurrentIndex(2); // Simulation tab
    
    // For now, just generate the roughing toolpath if it's not already generated
    if (m_outputWindow) {
        m_outputWindow->append("Simulating toolpaths...");
        m_outputWindow->append("Checking for existing toolpaths...");
    }
    
    // Check if we have any existing toolpaths in the timeline
    bool hasRoughing = false;
    if (m_toolpathTimeline) {
        for (int i = 0; i < m_toolpathTimeline->getToolpathCount(); ++i) {
            if (m_toolpathTimeline->getToolpathType(i).contains("Roughing", Qt::CaseInsensitive)) {
                hasRoughing = true;
                break;
            }
        }
    }
    
    // If we don't have a roughing toolpath, generate one
    if (!hasRoughing) {
        if (m_outputWindow) {
            m_outputWindow->append("No roughing toolpath found. Generating one now...");
        }
        
        // Call the automated toolpath generation
        handleAutomaticToolpathGeneration();
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Using existing roughing toolpath for simulation");
        }
        
        // TODO: Implement actual simulation based on existing toolpaths
        statusBar()->showMessage("Simulating existing toolpaths...", 3000);
    }
    
    // Placeholder for future simulation implementation
    if (m_outputWindow) {
        m_outputWindow->append("Simulation completed - View displayed in simulation tab");
    }
    
    statusBar()->showMessage("Toolpath simulation completed", 3000);
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
    // Create material and tool managers
    m_materialManager = new IntuiCAM::GUI::MaterialManager(this);
    m_toolManager = new IntuiCAM::GUI::ToolManager(this);
    
    m_setupConfigPanel = new IntuiCAM::GUI::SetupConfigurationPanel();
    
    // Set material and tool managers in setup configuration panel
    m_setupConfigPanel->setMaterialManager(m_materialManager);
    m_setupConfigPanel->setToolManager(m_toolManager);
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
    
    // Create toolpath timeline
    m_toolpathTimeline = new ToolpathTimelineWidget();
    m_toolpathTimeline->setMinimumHeight(90);
    m_toolpathTimeline->setMaximumHeight(110);
    
    rightLayout->addWidget(m_3dViewer, 1);
    rightLayout->addWidget(m_toolpathTimeline);
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
    
    // Create the checkbox to toggle chuck visibility
    m_showChuckCheckBox = new QCheckBox("Show Chuck", this);
    m_showChuckCheckBox->setChecked(true);
    m_showChuckCheckBox->setMaximumHeight(30);
    
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
    
    // Style the checkbox similarly (background only, keep default indicator)
    m_showChuckCheckBox->setStyleSheet(
        "QCheckBox {"
        "  background-color: rgba(240, 240, 240, 220);"
        "  border: 2px solid #666666;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  color: #333333;"
        "}"
        "QCheckBox:hover {"
        "  background-color: rgba(255, 255, 255, 240);"
        "  border: 2px solid #444444;"
        "  color: #222222;"
        "}"
    );
    
    // Connect the button to the toggle function
    connect(m_viewModeOverlayButton, &QPushButton::clicked, this, &MainWindow::toggleViewMode);
    
    // Connect checkbox toggled signal
    connect(m_showChuckCheckBox, &QCheckBox::toggled, this, &MainWindow::handleShowChuckToggled);
    
    // Connect to view mode changes to update button text
    connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged, this, &MainWindow::updateViewModeOverlayButton);
    
    // Initially position the button
    positionViewModeOverlayButton();
    
    // Show the controls - they will always be visible on the setup tab
    m_viewModeOverlayButton->show();
    m_viewModeOverlayButton->raise();
    m_showChuckCheckBox->show();
    m_showChuckCheckBox->raise();
    
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
    
    // Only show the controls when we're on the Setup tab (where the 3D viewer is)
    bool onSetupTab = (m_tabWidget && m_tabWidget->currentIndex() == 1);
    if (!onSetupTab) {
        m_viewModeOverlayButton->hide();
        if (m_showChuckCheckBox) m_showChuckCheckBox->hide();
        return;
    }

    m_viewModeOverlayButton->show();
    if (m_showChuckCheckBox) m_showChuckCheckBox->show();

    // Calculate position relative to the 3D viewer widget
    QPoint viewerGlobalPos = m_3dViewer->mapToGlobal(QPoint(0, 0));
    QPoint mainWindowPos = this->mapFromGlobal(viewerGlobalPos);

    // Position controls in top-right corner of the 3D viewer with margin
    const int margin = 15;
    const int spacing = 10;
    const int buttonWidth = m_viewModeOverlayButton->sizeHint().width();
    const int buttonHeight = m_viewModeOverlayButton->sizeHint().height();

    int x = mainWindowPos.x() + m_3dViewer->width() - buttonWidth - margin;
    int y = mainWindowPos.y() + margin;

    // Ensure the button stays within bounds
    x = qMax(margin, qMin(x, width() - buttonWidth - margin));
    y = qMax(margin, qMin(y, height() - buttonHeight - margin));

    // Move the button
    m_viewModeOverlayButton->move(x, y);
    m_viewModeOverlayButton->raise();

    // Position checkbox to the left of the button
    if (m_showChuckCheckBox) {
        int checkWidth = m_showChuckCheckBox->sizeHint().width();
        int checkHeight = m_showChuckCheckBox->sizeHint().height();

        int xCheck = x - checkWidth - spacing;
        int yCheck = y + (buttonHeight - checkHeight) / 2;

        xCheck = qMax(margin, xCheck);
        m_showChuckCheckBox->move(xCheck, yCheck);
        m_showChuckCheckBox->raise();
    }
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
    if (!m_setupConfigPanel || !m_workspaceController) {
        return;
    }

    // Update facing allowance parameter in raw material manager
    double facingAllowance = m_setupConfigPanel->getFacingAllowance();
    RawMaterialManager* rm = m_workspaceController->getRawMaterialManager();
    if (rm) {
        rm->setFacingAllowance(facingAllowance);
    }

    // Recalculate raw material length using current diameter to reflect new allowance
    if (rm && rm->isRawMaterialDisplayed()) {
        double currentDiam = rm->getCurrentDiameter();
        if (currentDiam > 0.0) {
            m_workspaceController->updateRawMaterialDiameter(currentDiam);
        }
    }
}

void MainWindow::handleMaterialTypeChanged(IntuiCAM::GUI::MaterialType material)
{
    // Placeholder
}

void MainWindow::handleRawMaterialDiameterChanged(double diameter)
{
    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        return;
    }
    
    statusBar()->showMessage(tr("Updating raw material diameter: %1 mm").arg(diameter), 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Updating raw material diameter: %1 mm").arg(diameter));
    }
    
    bool success = m_workspaceController->updateRawMaterialDiameter(diameter);
    if (!success) {
        statusBar()->showMessage(tr("Failed to update raw material diameter"), 3000);
        if (m_outputWindow) {
            m_outputWindow->append("Failed to update raw material diameter");
        }
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Raw material diameter updated successfully");
        }
    }
}

void MainWindow::handleManualAxisSelectionRequested()
{
    // Placeholder
}

void MainWindow::handleOperationToggled(const QString& operationName, bool enabled)
{
    // Placeholder
    if (m_outputWindow) {
        m_outputWindow->append(QString("Operation %1 %2").arg(operationName).arg(enabled ? "enabled" : "disabled"));
    }
}

void MainWindow::handleOperationParametersRequested(const QString& operationName)
{
    // Placeholder
    if (m_outputWindow) {
        m_outputWindow->append(QString("Parameters requested for operation: %1").arg(operationName));
    }
}

void MainWindow::handleAutomaticToolpathGeneration()
{
    if (!m_toolpathGenerationController) {
        if (m_outputWindow) {
            m_outputWindow->append("Error: Toolpath generation controller not initialized");
        }
        statusBar()->showMessage("Error: Toolpath generation controller not initialized", 3000);
        return;
    }
    
    // First, connect the toolpath controller to the UI for progress updates
    if (m_outputWindow) {
        m_toolpathGenerationController->connectStatusText(m_outputWindow);
    }
    
    // Get required parameters from workspace components
    double rawDiameter = 0.0;
    double distanceToChuck = 0.0;
    bool orientationFlipped = false;
    
    // Get raw material diameter if available
    if (m_workspaceController && m_workspaceController->getRawMaterialManager()) {
        rawDiameter = m_workspaceController->getRawMaterialManager()->getCurrentDiameter();
    }
    
    // Get part shape if available
    TopoDS_Shape partShape;
    QString stepFilePath;
    
    // Check if we have a valid raw material diameter
    if (rawDiameter <= 0.0) {
        if (m_outputWindow) {
            m_outputWindow->append("Warning: No valid raw material diameter detected. Using default value of 50mm.");
        }
        rawDiameter = 50.0; // Default value
    }
    
    // Create request with appropriate parameters
    IntuiCAM::GUI::ToolpathGenerationController::GenerationRequest request;
    request.rawDiameter = rawDiameter;
    request.distanceToChuck = distanceToChuck;
    request.orientationFlipped = orientationFlipped;
    request.partShape = partShape;
    request.stepFilePath = stepFilePath;
    
    // Set enabled operations (for demo, enable roughing)
    request.enabledOperations = QStringList() << "Roughing";
    
    // Set operation allowances
    request.facingAllowance = 0.5;
    request.roughingAllowance = 0.5;
    request.finishingAllowance = 0.2;
    request.partingWidth = 3.0;
    
    // Set quality parameters
    request.tolerance = 0.01;
    
    // Generate the toolpaths
    m_toolpathGenerationController->generateToolpaths(request);
    
    if (m_outputWindow) {
        m_outputWindow->append("Initiated toolpath generation for roughing operation");
    }
    statusBar()->showMessage("Generating roughing toolpath...", 3000);
}

// Toolpath generation handler implementations
void MainWindow::handleToolpathGenerationStarted()
{
    statusBar()->showMessage("Toolpath generation started...", 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append("Toolpath generation started");
    }
}

void MainWindow::handleToolpathProgressUpdated(int percentage, const QString& statusMessage)
{
    statusBar()->showMessage(QString("Toolpath generation: %1% - %2").arg(percentage).arg(statusMessage), 1000);
}

void MainWindow::handleToolpathOperationCompleted(const QString& operationName, bool success, const QString& message)
{
    QString statusMsg = QString("%1 operation %2: %3")
                        .arg(operationName)
                        .arg(success ? "completed" : "failed")
                        .arg(message);
    
    statusBar()->showMessage(statusMsg, 2000);
    
    // If the operation was successful, update the toolpath timeline
    if (success && m_toolpathTimeline) {
        // Add the toolpath to the timeline if it doesn't exist
        int existingIndex = -1;
        for (int i = 0; i < m_toolpathTimeline->getToolpathCount(); ++i) {
            if (m_toolpathTimeline->getToolpathName(i).contains(operationName, Qt::CaseInsensitive)) {
                existingIndex = i;
                break;
            }
        }
        
        if (existingIndex == -1) {
            // Add new toolpath entry
            QString toolName = "Default Tool"; // In a real implementation, get this from the tool manager
            m_toolpathTimeline->addToolpath(operationName, operationName, toolName);
        }
    }
}

void MainWindow::handleToolpathGenerationCompleted()
{
    statusBar()->showMessage("Toolpath generation completed successfully", 3000);
    
    // Switch to 3D view if we're not already there
    m_tabWidget->setCurrentIndex(1); // Setup tab
    
    // Fit all to make sure the toolpaths are visible
    if (m_3dViewer) {
        m_3dViewer->fitAll();
    }
}

void MainWindow::handleToolpathGenerationError(const QString& errorMessage)
{
    statusBar()->showMessage(QString("Error: %1").arg(errorMessage), 5000);
    
    QMessageBox::warning(this, tr("Toolpath Generation Error"), errorMessage);
}

// Empty implementations for workspace event handlers
void MainWindow::handleWorkspaceError(const QString& source, const QString& message)
{
    // Placeholder
    if (m_outputWindow) {
        m_outputWindow->append(QString("Error from %1: %2").arg(source, message));
    }
    statusBar()->showMessage(tr("Error: ") + message, 3000);
}

void MainWindow::handleChuckInitialized()
{
    // Placeholder
}

void MainWindow::handleWorkpieceWorkflowCompleted(double diameter, double rawMaterialDiameter)
{
    // Placeholder
}

void MainWindow::handleChuckCenterlineDetected(const gp_Ax1& axis)
{
    // Placeholder
}

void MainWindow::handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders)
{
    // Placeholder
}

void MainWindow::handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo)
{
    // Placeholder
}

void MainWindow::handleManualAxisSelected(double diameter, const gp_Ax1& axis)
{
    // Placeholder
}

void MainWindow::handleRawMaterialCreated(double diameter, double length)
{
    // Placeholder
}

// Part loading panel handlers
void MainWindow::handlePartLoadingDistanceChanged(double distance)
{
    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        return;
    }
    
    statusBar()->showMessage(tr("Updating distance to chuck: %1 mm").arg(distance), 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Updating distance to chuck: %1 mm").arg(distance));
    }
    
    bool success = m_workspaceController->updateDistanceToChuck(distance);
    if (!success) {
        statusBar()->showMessage(tr("Failed to update distance to chuck"), 3000);
        if (m_outputWindow) {
            m_outputWindow->append("Failed to update distance to chuck");
        }
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Distance to chuck updated successfully");
        }
    }
}

void MainWindow::handlePartLoadingDiameterChanged(double diameter)
{
    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        return;
    }
    
    statusBar()->showMessage(tr("Updating raw material diameter: %1 mm").arg(diameter), 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Updating raw material diameter: %1 mm").arg(diameter));
    }
    
    bool success = m_workspaceController->updateRawMaterialDiameter(diameter);
    if (!success) {
        statusBar()->showMessage(tr("Failed to update raw material diameter"), 3000);
        if (m_outputWindow) {
            m_outputWindow->append("Failed to update raw material diameter");
        }
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Raw material diameter updated successfully");
        }
    }
}

void MainWindow::handlePartLoadingOrientationFlipped(bool flipped)
{
    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        return;
    }
    
    statusBar()->showMessage(flipped ? tr("Flipping part orientation") : tr("Restoring part orientation"), 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append(flipped ? "Flipping part orientation" : "Restoring part orientation");
    }
    
    bool success = m_workspaceController->flipWorkpieceOrientation(flipped);
    if (!success) {
        statusBar()->showMessage(tr("Failed to change part orientation"), 3000);
        if (m_outputWindow) {
            m_outputWindow->append("Failed to change part orientation");
        }
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Part orientation changed successfully");
        }
    }
}

void MainWindow::handlePartLoadingCylinderChanged(int index)
{
    // Placeholder
}

void MainWindow::handlePartLoadingManualSelection()
{
    // Placeholder
}

void MainWindow::handlePartLoadingReprocess()
{
    // Placeholder
}

// 3D viewer handlers
void MainWindow::handleShapeSelected(const TopoDS_Shape& shape, const gp_Pnt& clickPoint)
{
    // Placeholder
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
            connect(m_workspaceController, &WorkspaceController::manualAxisSelected,
                    this, &MainWindow::handleManualAxisSelected);
            
            // Connect raw material creation for length updates
            if (m_workspaceController->getRawMaterialManager()) {
                connect(m_workspaceController->getRawMaterialManager(), &RawMaterialManager::rawMaterialCreated,
                        this, &MainWindow::handleRawMaterialCreated);
            }
            
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

void MainWindow::handleToolpathSelected(int index)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Selected toolpath at index %1").arg(index));
    }
    
    // Here you would typically highlight the selected toolpath in the 3D viewer
    // This depends on how toolpaths are stored and visualized in your application
    statusBar()->showMessage(tr("Selected toolpath %1").arg(index + 1), 2000);
}

void MainWindow::handleToolpathParametersRequested(int index, const QString& operationType)
{
    // Map operation type string to enum
    IntuiCAM::GUI::OperationParameterDialog::OperationType type;
    
    if (operationType == "Facing") {
        type = IntuiCAM::GUI::OperationParameterDialog::OperationType::Facing;
    } else if (operationType == "Roughing") {
        type = IntuiCAM::GUI::OperationParameterDialog::OperationType::Roughing;
    } else if (operationType == "Finishing") {
        type = IntuiCAM::GUI::OperationParameterDialog::OperationType::Finishing;
    } else if (operationType == "Parting") {
        type = IntuiCAM::GUI::OperationParameterDialog::OperationType::Parting;
    } else {
        // Default to roughing for unknown types
        type = IntuiCAM::GUI::OperationParameterDialog::OperationType::Roughing;
    }
    
    // Create and show parameter dialog
    IntuiCAM::GUI::OperationParameterDialog dialog(type, this);
    
    // Set context information if available
    if (m_materialManager && m_workspaceController) {
        // For now, just use a default material type since getCurrentMaterialType doesn't exist
        dialog.setMaterialType(IntuiCAM::GUI::MaterialType::Aluminum6061);
    }
    
    // Set part dimensions if available from workspace controller
    if (m_workspaceController) {
        // Use the detected diameter instead of a non-existent method
        double diameter = 0.0;
        if (m_workspaceController->getWorkpieceManager()) {
            diameter = m_workspaceController->getWorkpieceManager()->getDetectedDiameter();
        }
        
        // Use raw material diameter as fallback
        double length = 100.0; // Default length
        if (m_workspaceController->getRawMaterialManager() && 
            m_workspaceController->getRawMaterialManager()->isRawMaterialDisplayed()) {
            // We don't have a direct method to get length, but we have diameter
            double rawDiameter = m_workspaceController->getRawMaterialManager()->getCurrentDiameter();
            // Use 3x diameter as a reasonable default length
            length = rawDiameter * 3.0;
        }
        
        dialog.setPartDiameter(diameter);
        dialog.setPartLength(length);
    }
    
    // Show the dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Handle parameter changes based on operation type
        switch (type) {
            case IntuiCAM::GUI::OperationParameterDialog::OperationType::Facing:
                // Process facing parameters
                {
                    auto params = dialog.getFacingParameters();
                    // Update toolpath with new parameters
                    // Example: m_workspaceController->updateToolpathParameters(index, params);
                    
                    // For now just log to output window
                    if (m_outputWindow) {
                        m_outputWindow->append(QString("Updated facing parameters for toolpath %1")
                                              .arg(index + 1));
                        m_outputWindow->append(QString("  Stepover: %1 mm").arg(params.stepover));
                        m_outputWindow->append(QString("  Feed Rate: %1 mm/min").arg(params.feedRate));
                        m_outputWindow->append(QString("  Stock Allowance: %1 mm").arg(params.stockAllowance));
                    }
                }
                break;
                
            case IntuiCAM::GUI::OperationParameterDialog::OperationType::Roughing:
                // Process roughing parameters
                {
                    auto params = dialog.getRoughingParameters();
                    
                    if (m_outputWindow) {
                        m_outputWindow->append(QString("Updated roughing parameters for toolpath %1")
                                              .arg(index + 1));
                        m_outputWindow->append(QString("  Depth of Cut: %1 mm").arg(params.depthOfCut));
                        m_outputWindow->append(QString("  Stock Allowance: %1 mm").arg(params.stockAllowance));
                        m_outputWindow->append(QString("  Adaptive Clearing: %1").arg(params.adaptiveClearing ? "Yes" : "No"));
                    }
                }
                break;
                
            case IntuiCAM::GUI::OperationParameterDialog::OperationType::Finishing:
                // Process finishing parameters
                {
                    auto params = dialog.getFinishingParameters();
                    
                    if (m_outputWindow) {
                        m_outputWindow->append(QString("Updated finishing parameters for toolpath %1")
                                              .arg(index + 1));
                        m_outputWindow->append(QString("  Surface Finish: %1 μm Ra").arg(params.targetSurfaceFinish));
                        m_outputWindow->append(QString("  Radial Stepover: %1 mm").arg(params.radialStepover));
                        m_outputWindow->append(QString("  Spring Passes: %1").arg(params.multipleSpringPasses ? QString::number(params.springPassCount) : "None"));
                    }
                }
                break;
                
            case IntuiCAM::GUI::OperationParameterDialog::OperationType::Parting:
                // Process parting parameters
                {
                    auto params = dialog.getPartingParameters();
                    
                    if (m_outputWindow) {
                        m_outputWindow->append(QString("Updated parting parameters for toolpath %1")
                                              .arg(index + 1));
                        m_outputWindow->append(QString("  Feed Rate: %1 mm/min").arg(params.feedRate));
                        m_outputWindow->append(QString("  Pecking Cycle: %1").arg(params.usePeckingCycle ? "Yes" : "No"));
                        m_outputWindow->append(QString("  Safety Margin: %1 mm").arg(params.safetyMargin));
                    }
                }
                break;
        }
        
        // Update the toolpath visualization if needed
        // This would typically involve regenerating the toolpath with new parameters
        
        // Update the timeline entry with potentially modified operation name
        QString toolName = "Default Tool"; // In a real implementation, get this from the tool manager
        m_toolpathTimeline->updateToolpath(index, 
                                          tr("%1 #%2").arg(operationType).arg(index + 1),
                                          operationType, 
                                          toolName);
    }
}

void MainWindow::handleAddToolpathRequested(const QString& operationType)
{
    // In a real implementation, this would create a new toolpath operation
    // and add it to the workspace controller
    
    // For now, just add it to the timeline
    QString operationName = tr("%1 #%2").arg(operationType).arg(m_toolpathTimeline->getToolpathCount() + 1);
    QString toolName = "Default Tool"; // In a real implementation, get this from the tool manager
    
    int newIndex = m_toolpathTimeline->addToolpath(operationName, operationType, toolName);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Added new %1 toolpath at index %2")
                              .arg(operationType)
                              .arg(newIndex));
    }
    
    // Automatically open the parameter dialog for the new toolpath
    handleToolpathParametersRequested(newIndex, operationType);
}

void MainWindow::handleRemoveToolpathRequested(int index)
{
    // In a real implementation, this would remove the toolpath from the workspace controller
    
    // For now, just remove it from the timeline
    m_toolpathTimeline->removeToolpath(index);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Removed toolpath at index %1").arg(index));
    }
}

void MainWindow::handleToolpathReordered(int fromIndex, int toIndex)
{
    // In a real implementation, this would reorder the toolpaths in the workspace controller
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Reordered toolpath from index %1 to %2")
                              .arg(fromIndex)
                              .arg(toIndex));
    }
    
    // For now, just log the request - in a real implementation we would
    // need to move the actual toolpath data and update the timeline
    statusBar()->showMessage(tr("Reordering toolpaths is not yet implemented"), 2000);
}

// Add this method to log messages to the output window
void MainWindow::logToOutput(const QString& message)
{
    if (m_outputWindow) {
        m_outputWindow->append(message);
    }
}

void MainWindow::setupUiConnections()
{
    // Connect WorkpieceManager to ToolpathManager
    if (m_toolpathManager && m_workpieceManager) {
        m_toolpathManager->setWorkpieceManager(m_workpieceManager);
        
        // Connect workpiece transformation signals to update toolpaths
        connect(m_workpieceManager, &WorkpieceManager::workpieceTransformed, 
                m_toolpathManager, &ToolpathManager::applyWorkpieceTransformationToToolpaths);
    }
    
    // Connect workpiece position changes to toolpath updates
    if (m_workspaceController && m_toolpathManager) {
        // Connect using a direct connection to ensure immediate update
        connect(m_workspaceController, &WorkspaceController::workpiecePositionChanged,
                this, [this](double distance) {
                    // Ensure toolpaths update when workpiece position changes
                    if (m_toolpathManager) {
                        qDebug() << "MainWindow: Workpiece position changed to" << distance << "mm, updating toolpaths";
                        
                        // Force toolpath update with current workpiece transformation
                        QTimer::singleShot(100, [this]() {
                            if (m_toolpathManager) {
                                m_toolpathManager->applyWorkpieceTransformationToToolpaths();
                                qDebug() << "MainWindow: Applied toolpath transformations after position change";
                            }
                        });
                    }
                }, Qt::DirectConnection);
    }
    
    // Connect to the toolpath generation controller
    if (m_toolpathGenerationController) {
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationStarted,
                this, &MainWindow::handleToolpathGenerationStarted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::progressUpdated,
                this, &MainWindow::handleToolpathProgressUpdated);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::operationCompleted,
                this, &MainWindow::handleToolpathOperationCompleted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::generationCompleted,
                this, &MainWindow::handleToolpathGenerationCompleted);
        
        connect(m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::errorOccurred,
                this, &MainWindow::handleToolpathGenerationError);
    }
}

void MainWindow::handleShowChuckToggled(bool checked)
{
    if (!m_workspaceController) {
        return;
    }

    ChuckManager* chuckMgr = m_workspaceController->getChuckManager();
    if (!chuckMgr) {
        return;
    }

    if (checked) {
        // If chuck is already loaded, just redisplay it; otherwise, initialize
        if (chuckMgr->isChuckLoaded()) {
            chuckMgr->redisplayChuck();
            statusBar()->showMessage(tr("Chuck displayed"), 2000);
        } else {
            bool success = m_workspaceController->initializeChuck(m_defaultChuckFilePath);
            statusBar()->showMessage(success ? tr("Chuck loaded and displayed") : tr("Failed to load chuck"), 3000);
        }
    } else {
        // Hide chuck from view
        chuckMgr->clearChuck();
        statusBar()->showMessage(tr("Chuck hidden"), 2000);
    }

    // Log action
    if (m_outputWindow) {
        m_outputWindow->append(QString("Chuck visibility toggled: %1").arg(checked ? "Visible" : "Hidden"));
    }
}