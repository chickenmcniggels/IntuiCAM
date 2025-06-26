#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "workspacecontroller.h"
#include "workpiecemanager.h"
#include "partloadingpanel.h"
#include "setupconfigurationpanel.h"
#include "materialmanager.h"
#include "toolmanager.h"
#include "toolmanagementtab.h"
#include "toolmanagementdialog.h"

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
#include <QFile>
#include <QDir>
#include <QPushButton>
#include <QGroupBox>
#include <QFrame>
#include <QFileInfo>
#include <QMessageBox>
#include <QToolButton>

// OpenCASCADE includes for geometry handling
#include <gp_Ax1.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_Drawer.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

// IntuiCAM Toolpath Pipeline includes
#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/Types.h>

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
    , m_workpieceManager(nullptr)
    , m_materialManager(nullptr)
    , m_toolManager(nullptr)
    , m_operationTileContainer(nullptr)
    , m_toolManagementTab(nullptr)
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
    , m_visibilityButton(nullptr)
    , m_visibilityMenu(nullptr)
    , m_showChuckAction(nullptr)
    , m_showRawMaterialAction(nullptr)
    , m_showToolpathsAction(nullptr)
    , m_showPartAction(nullptr)
    , m_showProfilesAction(nullptr)  // Initialize profile visibility action
    , m_defaultChuckFilePath("assets/models/three_jaw_chuck.step")
{
    // Initialize GUI state
    m_defaultChuckFilePath = "assets/models/three_jaw_chuck.step";
    
    // Initialize timer
    m_toolpathRegenerationTimer = nullptr;
    
    // Create business logic components
    m_workspaceController = new WorkspaceController(this);
    m_stepLoader = new StepLoader();
    m_workpieceManager = nullptr;  // will be obtained from WorkspaceController
    
    // Create material and tool managers
    m_materialManager = new IntuiCAM::GUI::MaterialManager(this);
    m_toolManager = new IntuiCAM::GUI::ToolManager(this);
    
    // Create tool management components
    m_toolManagementTab = new ToolManagementTab(this);
    // ToolManagementDialog is now created on-demand when needed
    
    // Ensure default tools are created immediately on first run
    // This will create the tool database if it doesn't exist
    m_toolManagementTab->ensureDefaultToolsExist();
    
    // Create UI
    createMenus();
    createStatusBar();
    createCentralWidget();
    setupConnections();
    
    
    // Update status
    statusBar()->showMessage(tr("Ready"));
    
    // Set window title and size
    setWindowTitle(tr("IntuiCAM - Computer Aided Manufacturing"));
    resize(1280, 800);
    
    // Initialize the workspace once the OpenGL viewer is ready
    connect(m_3dViewer, &OpenGL3DWidget::viewerInitialized,
            this, &MainWindow::initializeWorkspace);

    // Fallback initialization in case the signal was emitted before the
    // connection or the viewer initializes instantly
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
    
    // Debug menu (toolpath functionality removed)
    QMenu* debugMenu = menuBar()->addMenu(tr("Debug"));
    
    QAction* actionShowStatus = debugMenu->addAction(tr("Show System Status"));
    connect(actionShowStatus, &QAction::triggered, this, [this]() {
        logToOutput("System Status: GUI components loaded, toolpath generation pipeline removed");
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

    // Tool management tab uses the previously created m_toolManagementTab
    
    // Add tabs to tab widget
    m_tabWidget->addTab(m_homeTab, "Home");
    m_tabWidget->addTab(m_setupTab, "Setup");
    m_tabWidget->addTab(m_simulationTab, "Simulation");
    m_tabWidget->addTab(m_machineTab, "Machine");
    m_tabWidget->addTab(m_toolManagementTab, "Tool Management");
    
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
    
    // Connections will be configured once the workspace is initialized
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
    
    // Connect tab widget signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Connect 3D viewer signals
    if (m_3dViewer) {
        connect(m_3dViewer, &OpenGL3DWidget::shapeSelected, 
                this, &MainWindow::handleShapeSelected);
        connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged,
                this, &MainWindow::handleViewModeChanged);
    }
    
    // Connect setup configuration panel signals
    if (m_setupConfigPanel) {
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
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::autoRawDiameterRequested,
                this, &MainWindow::handleAutoRawDiameterRequested);
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::requestThreadFaceSelection,
                this, &MainWindow::handleThreadFaceSelectionRequested);
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::threadFaceSelected,
                this, &MainWindow::handleThreadFaceSelected);
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::threadFaceDeselected,
                this, &MainWindow::clearHighlightedThreadFace);
        if (m_workspaceController) {
            connect(m_workspaceController->getWorkpieceManager(), &WorkpieceManager::workpieceTransformed,
                    this, &MainWindow::handleWorkpieceTransformed);
        }
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::operationToggled,
                this, &MainWindow::handleOperationToggled);

        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::recommendedToolActivated,
                this, [this](const QString& toolId) {
                    if (m_toolManagementTab && m_tabWidget) {
                        m_toolManagementTab->selectTool(toolId);
                        m_tabWidget->setCurrentWidget(m_toolManagementTab);
                    }
                });
    }
    
    // Operation tile connections
    if (m_operationTileContainer) {
        connect(m_operationTileContainer, &IntuiCAM::GUI::OperationTileContainer::operationEnabledChanged,
                this, &MainWindow::handleOperationTileEnabledChanged);
        connect(m_operationTileContainer, &IntuiCAM::GUI::OperationTileContainer::operationClicked,
                this, &MainWindow::handleOperationTileClicked);
        connect(m_operationTileContainer, &IntuiCAM::GUI::OperationTileContainer::operationToolSelectionRequested,
                this, &MainWindow::handleOperationTileToolSelectionRequested);
        connect(m_operationTileContainer, &IntuiCAM::GUI::OperationTileContainer::operationExpandedChanged,
                this, &MainWindow::handleOperationTileExpandedChanged);
        
        // Initialize default operation selection - select "Facing" if it's enabled
        QTimer::singleShot(100, this, [this]() {
            if (m_operationTileContainer && m_setupConfigPanel) {
                if (m_operationTileContainer->isTileEnabled("Facing")) {
                    m_operationTileContainer->setSelectedOperation("Facing");
                    m_setupConfigPanel->showOperationWidget("Facing");
                }
            }
        });
    }
    
    // Tool management connections
    if (m_toolManagementTab) {
        // Connect tool tab signals to open individual editing dialogs
        connect(m_toolManagementTab, &ToolManagementTab::toolDoubleClicked,
                this, [this](const QString& toolId) {
                    // Create a new dialog for editing this specific tool
                    auto dialog = new ToolManagementDialog(toolId, this);
                    
                    // Set the material manager for the dialog
                    if (m_materialManager) {
                        dialog->setMaterialManager(m_materialManager);
                    }
                    
                    // Connect to handle tool saves
                    connect(dialog, &ToolManagementDialog::toolSaved,
                            m_toolManagementTab, &ToolManagementTab::onToolModified);
                    
                    // Show the dialog modally
                    dialog->exec();
                    dialog->deleteLater();
                });
    }
    
    // Toolpath pipeline connections removed
    // Operation parameter dialog connections can be added here if needed
    // to ensure proper initialization order
    
    // REMOVED DUPLICATE: This connection is handled by ToolpathGenerationController to avoid duplication
    // if (m_workspaceController && m_toolpathGenerationController) {
    //     connect(m_workspaceController->getWorkpieceManager(), &WorkpieceManager::workpieceTransformed,
    //             m_toolpathGenerationController, &IntuiCAM::GUI::ToolpathGenerationController::regenerateAllToolpaths);
    // }
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

            // Use the controller's workpiece manager for further interactions
            m_workpieceManager = m_workspaceController->getWorkpieceManager();

            // Set up UI connections that depend on a valid workpiece manager
            setupUiConnections();
            
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
            
            // Connect workpiece position changes to regenerate toolpaths
            connect(m_workspaceController, &WorkspaceController::workpiecePositionChanged,
                    this, &MainWindow::handleWorkpiecePositionChanged);
            
            // Connect raw material creation for length updates
            if (m_workspaceController->getRawMaterialManager()) {
                connect(m_workspaceController->getRawMaterialManager(), &RawMaterialManager::rawMaterialCreated,
                        this, &MainWindow::handleRawMaterialCreated);
            }
            
            // Enable auto-fit for initial file loading, but disable for parameter updates
            m_3dViewer->setAutoFitEnabled(true);
            
            // Try to automatically load the chuck from various possible locations
            QStringList possibleChuckPaths = {
                "C:/Users/nikla/Downloads/three_jaw_chuck.step",
                "assets/models/three_jaw_chuck.step",
                QDir::currentPath() + "/assets/models/three_jaw_chuck.step"
            };
            
            bool chuckSuccess = false;
            QString usedChuckPath;
            
            for (const QString& chuckPath : possibleChuckPaths) {
                if (QFile::exists(chuckPath)) {
                    chuckSuccess = m_workspaceController->initializeChuck(chuckPath);
                    if (chuckSuccess) {
                        usedChuckPath = chuckPath;
                        break;
                    }
                }
            }
            
            if (chuckSuccess) {
                if (m_outputWindow) {
                    m_outputWindow->append("Chuck loaded successfully from: " + usedChuckPath);
                }
                statusBar()->showMessage("Workspace and chuck ready", 2000);
            } else {
                if (m_outputWindow) {
                    m_outputWindow->append("Warning: No chuck file found in standard locations - chuck not loaded");
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
        TopoDS_Shape shape = m_stepLoader->loadStepFile(fileName.toStdString());
        
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

                // Immediately apply part and material setup parameters
                if (m_setupConfigPanel) {
                    double dist = m_setupConfigPanel->getDistanceToChuck();
                    double rawDia = m_setupConfigPanel->getRawDiameter();
                    bool flip = m_setupConfigPanel->isOrientationFlipped();
                    m_workspaceController->applyPartLoadingSettings(dist, rawDia, flip);
                }
            } else {
                QString errorMsg = "Failed to process workpiece through workspace controller";
                statusBar()->showMessage(errorMsg, 5000);
                if (m_outputWindow) {
                    m_outputWindow->append(errorMsg);
                }
            }
        } else {
            QString errorMsg = QString("Failed to load STEP file: %1")
                                     .arg(QString::fromStdString(m_stepLoader->getLastError()));
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
        if (m_viewModeOverlayButton) {
            m_viewModeOverlayButton->setText("Lathe View");
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
        if (m_viewModeOverlayButton) {
            m_viewModeOverlayButton->setText("3D View");
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
        case 4: tabName = "Tool Management"; break;
        default: tabName = "Unknown"; break;
    }
    
    // Buttons are part of the setup tab layout, no special handling needed
    
    statusBar()->showMessage(QString("Switched to %1 tab").arg(tabName), 2000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Switched to %1 tab").arg(tabName));
    }

    // Keep the 3D viewer's OpenGL context active while the Setup tab is open
    // to prevent it from unloading and turning black when other widgets gain
    // focus. Continuous updates ensure the framebuffer stays valid.
    if (m_3dViewer) {
        if (index == 1) {
            m_3dViewer->setContinuousUpdate(true);
            m_3dViewer->update();
        } else {
            m_3dViewer->setContinuousUpdate(false);
        }
    }
}

void MainWindow::simulateToolpaths() {
  // Temporarily disable simulation functionality.
  if (m_outputWindow) {
    m_outputWindow->append("Toolpath simulation is currently disabled.");
  }
  statusBar()->showMessage("Toolpath simulation not implemented yet", 3000);
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
    // Use already created material and tool managers to avoid duplication
    // m_materialManager = new IntuiCAM::GUI::MaterialManager(this);
    // m_toolManager = new IntuiCAM::GUI::ToolManager(this);
    
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

    // Create view mode and visibility controls to place above the viewer
    createViewModeOverlayButton(rightWidget);
    QHBoxLayout* viewerControlsLayout = new QHBoxLayout;
    viewerControlsLayout->setContentsMargins(0, 0, 0, 0);
    viewerControlsLayout->setSpacing(8);
    viewerControlsLayout->addWidget(m_visibilityButton);
    viewerControlsLayout->addWidget(m_viewModeOverlayButton);
    viewerControlsLayout->addStretch();

    rightLayout->addLayout(viewerControlsLayout);

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
    
    // Generate button
    m_generateButton = new QPushButton("⚙ Generate Toolpaths");
    m_generateButton->setMinimumHeight(35);
    m_generateButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #6c757d;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #5a6268;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #545b62;"
        "}"
    );

    // Simulate button
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
    operationLayout->addWidget(m_generateButton);
    operationLayout->addWidget(m_simulateButton);
    operationLayout->addWidget(exportButton);
    
    rightLayout->addWidget(m_3dViewer, 1);
    
    // Add operation tiles under the viewer
    m_operationTileContainer = new IntuiCAM::GUI::OperationTileContainer();
    m_operationTileContainer->setMaximumHeight(150);
    rightLayout->addWidget(m_operationTileContainer);
    
    // Set default selected operation (Facing is enabled by default)
    m_operationTileContainer->setSelectedOperation("Facing");
    if (m_setupConfigPanel) {
        m_setupConfigPanel->showOperationWidget("Facing");
    }
    
    rightLayout->addWidget(operationFrame);
    
    // Add setup panel and viewer to main splitter
    m_mainSplitter->addWidget(m_setupConfigPanel);
    m_mainSplitter->addWidget(rightWidget);

    // Set splitter sizes (left panel 30%, viewport 70%)
    m_mainSplitter->setSizes({350, 800});
    
    setupLayout->addWidget(m_mainSplitter);
    
    // Connect export button
    connect(exportButton, &QPushButton::clicked, [this]() {
        if (m_outputWindow) {
            m_outputWindow->append("Exporting G-Code... (integrated with toolpath generation)");
        }
        statusBar()->showMessage("G-Code export initiated", 2000);
        // Switch to simulation tab for G-code preview
        m_tabWidget->setCurrentIndex(2);
    });
    
    // Connect generate and simulate buttons (restored functionality)
    connect(m_generateButton, &QPushButton::clicked, this, &MainWindow::handleGenerateToolpaths);
    connect(m_simulateButton, &QPushButton::clicked, this, &MainWindow::simulateToolpaths);
    
    return setupWidget;
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
    if (!m_workspaceController || !m_3dViewer) {
        statusBar()->showMessage(tr("Error: Cannot start axis selection mode"), 3000);
        return;
    }
    
    // Enable selection mode in the 3D viewer
    m_3dViewer->setSelectionMode(true);
    
    // Inform the user
    statusBar()->showMessage(tr("Select a cylindrical surface or circular edge in the 3D view"), 5000);
    
    if (m_outputWindow) {
        m_outputWindow->append("Axis selection mode enabled. Please select a cylindrical surface or circular edge in the 3D view.");
    }
}

void MainWindow::handleAutoRawDiameterRequested()
{
    if (!m_workspaceController || !m_workspaceController->isInitialized()) {
        return;
    }

    double diameter = m_workspaceController->getAutoRawMaterialDiameter();
    if (diameter <= 0.0) {
        statusBar()->showMessage(tr("Failed to determine raw material diameter"), 3000);
        return;
    }

    m_setupConfigPanel->setRawDiameter(diameter);
    m_workspaceController->updateRawMaterialDiameter(diameter);
    statusBar()->showMessage(tr("Auto raw diameter set to %1 mm").arg(diameter, 0, 'f', 1), 3000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Auto raw diameter calculated: %1 mm").arg(diameter, 0, 'f', 1));
    }
}

void MainWindow::handleThreadFaceSelectionRequested()
{
    if (!m_3dViewer || !m_workspaceController) {
        return;
    }
    
    m_selectingThreadFace = true;
    m_3dViewer->setSelectionMode(true);
    statusBar()->showMessage(tr("Select a cylindrical face for threading"), 5000);
    highlightThreadCandidateFaces();
    if (m_outputWindow)
        m_outputWindow->append("Thread face selection mode enabled");
}

void MainWindow::handleThreadFaceSelected(const TopoDS_Shape& face)
{
    clearThreadCandidateHighlights();
    if (!m_3dViewer || !m_workspaceController) {
        return;
    }

    m_currentThreadFaceLocal = face;
    updateHighlightedThreadFace();
}

void MainWindow::handleOperationToggled(const QString& operationName, bool enabled)
{
    statusBar()->showMessage(QString("%1 operation %2").arg(operationName).arg(enabled ? "enabled" : "disabled"), 2000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Operation %1: %2").arg(operationName).arg(enabled ? "ENABLED" : "DISABLED"));
    }
}

void MainWindow::handleGenerateToolpaths()
{
    statusBar()->showMessage("Starting toolpath generation...", 2000);
    
    if (m_outputWindow) {
        m_outputWindow->append("=== Toolpath Generation Started ===");
    }
    
    // Step 1: Check if we have a part loaded
    if (!m_workspaceController || !m_workspaceController->hasPartShape()) {
        statusBar()->showMessage("Error: No part loaded. Please load a STEP file first.", 5000);
        if (m_outputWindow) {
            m_outputWindow->append("ERROR: No part shape loaded - please load a STEP file first");
        }
        return;
    }
    
    try {
        // Step 2: Get part geometry and workspace data
        TopoDS_Shape partGeometry = m_workspaceController->getPartShape();
        
        // Step 3: Get turning axis from workspace (chuck centerline)
        gp_Ax1 turningAxis;
        if (m_workspaceController->hasChuckCenterline()) {
            turningAxis = m_workspaceController->getChuckCenterlineAxis();
        } else {
            // Default Z-axis if no chuck centerline
            turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        }
        
        // Step 4: Create pipeline and extract inputs from part
        IntuiCAM::Toolpath::ToolpathGenerationPipeline pipeline;
        auto inputs = pipeline.extractInputsFromPart(partGeometry, turningAxis);
        
        // Step 5: Update pipeline inputs with GUI parameters
        if (m_setupConfigPanel) {
            // Basic parameters
            inputs.rawMaterialDiameter = m_setupConfigPanel->getRawDiameter();
            inputs.facingAllowance = m_setupConfigPanel->getFacingAllowance();
            
            // Calculate part dimensions from geometry
            Bnd_Box bbox;
            BRepBndLib::Add(partGeometry, bbox);
            if (!bbox.IsVoid()) {
                double xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                
                // Calculate part length and update GUI
                double calculatedPartLength = zmax - zmin;
                inputs.partLength = calculatedPartLength;
                m_setupConfigPanel->setPartLength(calculatedPartLength);
                
                // Calculate raw material length (part length + facing allowance + some extra stock)
                double calculatedRawLength = calculatedPartLength + inputs.facingAllowance + 5.0; // 5mm extra
                inputs.rawMaterialLength = calculatedRawLength;
                inputs.z0 = calculatedRawLength; // Set provisional datum to raw material length
                m_setupConfigPanel->setRawMaterialLength(calculatedRawLength);
            } else {
                // Fallback values if geometry analysis fails
                inputs.partLength = m_setupConfigPanel->getPartLength();
                inputs.rawMaterialLength = m_setupConfigPanel->getRawMaterialLength();
                inputs.z0 = inputs.rawMaterialLength;
            }
            
            // New pipeline-specific parameters
            inputs.largestDrillSize = m_setupConfigPanel->getLargestDrillSize();
            inputs.internalFinishingPasses = m_setupConfigPanel->getInternalFinishingPasses();
            inputs.externalFinishingPasses = m_setupConfigPanel->getExternalFinishingPasses();
            inputs.partingAllowance = m_setupConfigPanel->getPartingAllowance();
            
            // Operation enablement flags
            inputs.facing = m_setupConfigPanel->isOperationEnabled("Facing");
            inputs.parting = m_setupConfigPanel->isOperationEnabled("Parting");
            inputs.chamfering = m_setupConfigPanel->isOperationEnabled("Chamfering");
            inputs.threading = m_setupConfigPanel->isOperationEnabled("Threading");
            
            // New detailed operation flags
            inputs.machineInternalFeatures = m_setupConfigPanel->isMachineInternalFeaturesEnabled();
            inputs.drilling = m_setupConfigPanel->isDrillingEnabled();
            inputs.internalRoughing = m_setupConfigPanel->isInternalRoughingEnabled();
            inputs.externalRoughing = m_setupConfigPanel->isExternalRoughingEnabled();
            inputs.internalFinishing = m_setupConfigPanel->isInternalFinishingEnabled();
            inputs.externalFinishing = m_setupConfigPanel->isExternalFinishingEnabled();
            inputs.internalGrooving = m_setupConfigPanel->isInternalGroovingEnabled();
            inputs.externalGrooving = m_setupConfigPanel->isExternalGroovingEnabled();
            
            if (m_outputWindow) {
                m_outputWindow->append("Pipeline parameters:");
                m_outputWindow->append(QString("  Raw material diameter: %1 mm").arg(inputs.rawMaterialDiameter));
                m_outputWindow->append(QString("  Raw material length: %1 mm").arg(inputs.rawMaterialLength));
                m_outputWindow->append(QString("  Part length: %1 mm").arg(inputs.partLength));
                m_outputWindow->append(QString("  Z0 datum: %1 mm").arg(inputs.z0));
                m_outputWindow->append(QString("  Facing allowance: %1 mm").arg(inputs.facingAllowance));
                m_outputWindow->append(QString("  Largest drill size: %1 mm").arg(inputs.largestDrillSize));
                m_outputWindow->append(QString("  Internal finishing passes: %1").arg(inputs.internalFinishingPasses));
                m_outputWindow->append(QString("  External finishing passes: %1").arg(inputs.externalFinishingPasses));
                m_outputWindow->append(QString("  Parting allowance: %1 mm").arg(inputs.partingAllowance));
                
                m_outputWindow->append("Enabled operations:");
                if (inputs.facing) m_outputWindow->append("  ✓ Facing");
                if (inputs.machineInternalFeatures) {
                    m_outputWindow->append("  ✓ Internal Features:");
                    if (inputs.drilling) m_outputWindow->append("    ✓ Drilling");
                    if (inputs.internalRoughing) m_outputWindow->append("    ✓ Internal Roughing");
                    if (inputs.internalFinishing) m_outputWindow->append("    ✓ Internal Finishing");
                    if (inputs.internalGrooving) m_outputWindow->append("    ✓ Internal Grooving");
                }
                if (inputs.externalRoughing) m_outputWindow->append("  ✓ External Roughing");
                if (inputs.externalFinishing) m_outputWindow->append("  ✓ External Finishing");
                if (inputs.externalGrooving) m_outputWindow->append("  ✓ External Grooving");
                if (inputs.chamfering) m_outputWindow->append("  ✓ Chamfering");
                if (inputs.threading) m_outputWindow->append("  ✓ Threading");
                if (inputs.parting) m_outputWindow->append("  ✓ Parting");
            }
        }
        
        // Check if any operations are enabled
        bool hasEnabledOps = inputs.facing || inputs.drilling || inputs.internalRoughing || 
                           inputs.externalRoughing || inputs.internalFinishing || 
                           inputs.externalFinishing || inputs.internalGrooving || 
                           inputs.externalGrooving || inputs.chamfering || 
                           inputs.threading || inputs.parting;
        
        if (!hasEnabledOps) {
            statusBar()->showMessage("No operations enabled. Please enable at least one operation.", 5000);
            if (m_outputWindow) {
                m_outputWindow->append("ERROR: No operations enabled - please enable at least one operation");
            }
            return;
        }
        
        // Step 6: Execute pipeline
        if (m_outputWindow) {
            m_outputWindow->append("Executing toolpath generation pipeline...");
        }
        
        auto result = pipeline.executePipeline(inputs);
        
        if (result.success) {
            statusBar()->showMessage(QString("Toolpath generation completed successfully - %1 toolpaths generated")
                                   .arg(result.timeline.size()), 5000);
            if (m_outputWindow) {
                m_outputWindow->append(QString("SUCCESS: Generated %1 toolpaths in %2 ms")
                                     .arg(result.timeline.size())
                                     .arg(result.processingTime.count()));
                m_outputWindow->append("=== Toolpath Generation Completed ===");
            }
            
            // Get workpiece transformation to match toolpath positions with part position
            gp_Trsf workpieceTransform;
            if (m_workspaceController && m_workspaceController->getWorkpieceManager()) {
                workpieceTransform = m_workspaceController->getWorkpieceManager()->getCurrentTransformation();
                if (m_outputWindow) {
                    gp_XYZ translation = workpieceTransform.TranslationPart();
                    m_outputWindow->append(QString("Applying workpiece transformation - Translation: (%1, %2, %3)")
                                         .arg(translation.X(), 0, 'f', 2)
                                         .arg(translation.Y(), 0, 'f', 2)
                                         .arg(translation.Z(), 0, 'f', 2));
                    
                    // Add rotation matrix logging to debug rotation issues
                    gp_Mat rotationMatrix = workpieceTransform.HVectorialPart();
                    m_outputWindow->append(QString("Rotation matrix: [%1 %2 %3] [%4 %5 %6] [%7 %8 %9]")
                                         .arg(rotationMatrix.Value(1,1), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(1,2), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(1,3), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(2,1), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(2,2), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(2,3), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(3,1), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(3,2), 0, 'f', 3)
                                         .arg(rotationMatrix.Value(3,3), 0, 'f', 3));
                    
                    m_outputWindow->append(QString("Transformation form: %1").arg(static_cast<int>(workpieceTransform.Form())));
                }
            }
            
            // Create display objects with workpiece transformation
            auto transformedDisplayObjects = pipeline.createToolpathDisplayObjects(result.timeline, workpieceTransform);
            
            // Display toolpaths in 3D viewer
            if (m_3dViewer) {
                for (const auto& displayObj : transformedDisplayObjects) {
                    if (!displayObj.IsNull()) {
                        m_3dViewer->getContext()->Display(displayObj, Standard_False);
                    }
                }
                m_3dViewer->update();
                
            }
        } else {
            statusBar()->showMessage(QString("Toolpath generation failed: %1").arg(QString::fromStdString(result.errorMessage)), 5000);
            if (m_outputWindow) {
                m_outputWindow->append(QString("ERROR: %1").arg(QString::fromStdString(result.errorMessage)));
                for (const auto& warning : result.warnings) {
                    m_outputWindow->append(QString("WARNING: %1").arg(QString::fromStdString(warning)));
                }
                m_outputWindow->append("=== Toolpath Generation Failed ===");
            }
        }
        
    } catch (const std::exception& e) {
        statusBar()->showMessage(QString("Toolpath generation error: %1").arg(e.what()), 5000);
        if (m_outputWindow) {
            m_outputWindow->append(QString("EXCEPTION: %1").arg(e.what()));
            m_outputWindow->append("=== Toolpath Generation Failed ===");
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
    // Only process selection if the 3D viewer is in selection mode
    if (m_3dViewer && m_3dViewer->isSelectionModeActive()) {
        if (m_selectingThreadFace) {
            if (!shape.IsNull() && shape.ShapeType() == TopAbs_FACE) {
                TopoDS_Face face = TopoDS::Face(shape);
                BRepAdaptor_Surface surf(face);
                if (surf.GetType() == GeomAbs_Cylinder) {
                    if (m_setupConfigPanel) {
                        // Convert the selected face to local coordinates
                        gp_Trsf invTrsf = m_workspaceController->getWorkpieceManager()->getCurrentTransformation().Inverted();
                        TopoDS_Shape localFace = BRepBuilderAPI_Transform(face, invTrsf).Shape();
                        m_setupConfigPanel->addSelectedThreadFace(localFace);
                        handleThreadFaceSelected(localFace);
                    }
                } else {
                    statusBar()->showMessage(tr("Selected face is not cylindrical"), 3000);
                    clearThreadCandidateHighlights();
                }
            } else {
                statusBar()->showMessage(tr("Invalid selection for thread face"), 3000);
                clearThreadCandidateHighlights();
            }

            m_3dViewer->setSelectionMode(false);
            m_selectingThreadFace = false;
            return;
        }
        
        if (!m_workspaceController) {
            statusBar()->showMessage(tr("Error: Workspace controller not initialized"), 3000);
            m_3dViewer->setSelectionMode(false); // Disable selection mode
            return;
        }
        
        // Process the selection with the workspace controller
        bool success = m_workspaceController->processManualAxisSelection(shape, clickPoint);
        
        if (success) {
            statusBar()->showMessage(tr("Axis selected successfully"), 3000);
            if (m_outputWindow) {
                m_outputWindow->append("Rotational axis selected successfully");
            }
        } else {
            statusBar()->showMessage(tr("Failed to extract axis from selection. Try selecting a cylindrical face or circular edge."), 5000);
            if (m_outputWindow) {
                m_outputWindow->append("Failed to extract axis. Please try selecting a different cylindrical face or circular edge.");
            }
        }
        
        // Disable selection mode after processing
        m_3dViewer->setSelectionMode(false);
    }
}

void MainWindow::initializeWorkspace()
{
    // Prevent multiple initializations
    static bool initialized = false;
    if (initialized) {
        qDebug() << "Workspace already initialized, skipping duplicate initialization";
        return;
    }
    
    // Initialize workspace using the proper setupWorkspaceConnections method
    setupWorkspaceConnections();
    initialized = true;
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
    // UI connections are now simplified without toolpath generation pipeline
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
        if (chuckMgr->isChuckLoaded()) {
            chuckMgr->setChuckVisible(true);
            statusBar()->showMessage(tr("Chuck displayed"), 2000);
        } else {
            bool success = m_workspaceController->initializeChuck(m_defaultChuckFilePath);
            statusBar()->showMessage(success ? tr("Chuck loaded and displayed") : tr("Failed to load chuck"), 3000);
            if (!success) {
                m_showChuckAction->setChecked(false);
            }
        }
    } else {
        chuckMgr->setChuckVisible(false);
        statusBar()->showMessage(tr("Chuck hidden"), 2000);
    }

    // Log action
    if (m_outputWindow) {
        m_outputWindow->append(QString("Chuck visibility toggled: %1").arg(checked ? "Visible" : "Hidden"));
    }
}

void MainWindow::handleShowRawMaterialToggled(bool checked)
{
    if (!m_workspaceController) {
        return;
    }

    RawMaterialManager* rm = m_workspaceController->getRawMaterialManager();
    if (!rm) {
        return;
    }

    if (checked) {
        rm->setRawMaterialVisible(true);
        statusBar()->showMessage(tr("Raw material displayed"), 2000);
    } else {
        rm->setRawMaterialVisible(false);
        statusBar()->showMessage(tr("Raw material hidden"), 2000);
    }

    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material visibility toggled: %1").arg(checked ? "Visible" : "Hidden"));
    }
}

void MainWindow::handleShowToolpathsToggled(bool checked)
{
    Q_UNUSED(checked)
    statusBar()->showMessage("Toolpaths feature not available", 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Toolpaths feature not available");
    }
}

void MainWindow::handleShowPartToggled(bool checked)
{
    if (m_workspaceController) {
        WorkpieceManager* workpieceManager = m_workspaceController->getWorkpieceManager();
        if (workpieceManager) {
            workpieceManager->setWorkpiecesVisible(checked);
            
            // Check if m_outputWindow is valid before using it
            if (m_outputWindow) {
                m_outputWindow->append(QString("Part visibility toggled: %1").arg(checked ? "Visible" : "Hidden"));
            }
        }
    }
}

void MainWindow::handleShowProfilesToggled(bool checked)
{
    if (m_workspaceController) {
        m_workspaceController->setProfileVisible(checked);
        statusBar()->showMessage(QString("Profiles %1").arg(checked ? "shown" : "hidden"), 2000);
        
        if (m_outputWindow) {
            m_outputWindow->append(QString("Profile visibility toggled: %1").arg(checked ? "Visible" : "Hidden"));
        }
    }
}

void MainWindow::highlightThreadCandidateFaces()
{
    clearThreadCandidateHighlights();
    if (!m_workspaceController || !m_3dViewer)
        return;

    Handle(AIS_InteractiveContext) ctx = m_3dViewer->getContext();

    // Deactivate base workpiece shapes so only cylindrical faces are selectable
    QVector<Handle(AIS_Shape)> workpieces =
            m_workspaceController->getWorkpieceManager()->getWorkpieces();
    for (const Handle(AIS_Shape)& wp : workpieces) {
        if (!wp.IsNull()) {
            ctx->Deactivate(wp);
        }
    }

    TopoDS_Shape part = m_workspaceController->getPartShape();
    gp_Trsf trsf = m_workspaceController->getWorkpieceManager()->getCurrentTransformation();

    for (TopExp_Explorer exp(part, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face f = TopoDS::Face(exp.Current());
        BRepAdaptor_Surface surf(f);
        if (surf.GetType() == GeomAbs_Cylinder) {
            TopoDS_Shape global = f.Moved(trsf);
            Handle(AIS_Shape) ais = new AIS_Shape(global);
            // Keep shape invisible but selectable
            ais->SetTransparency(1.0);
            ctx->Display(ais, AIS_Shaded, 0, false);
            Handle(Prs3d_Drawer) dr = new Prs3d_Drawer();
            dr->SetColor(Quantity_NOC_LIGHTBLUE);
            dr->SetTransparency(Standard_ShortReal(0.6));
            ctx->HilightWithColor(ais, dr, Standard_False);
            ctx->Activate(ais, 0, Standard_False);
            ctx->Activate(ais, 4, Standard_False);
            m_candidateThreadFaces.append(ais);
        }
    }
    m_3dViewer->update();
}

void MainWindow::clearThreadCandidateHighlights()
{
    if (!m_3dViewer)
        return;
    Handle(AIS_InteractiveContext) ctx = m_3dViewer->getContext();
    for (const Handle(AIS_Shape)& ais : m_candidateThreadFaces) {
        if (!ais.IsNull()) {
            ctx->Unhilight(ais, Standard_False);
            ctx->Remove(ais, Standard_False);
        }
    }
    // Reactivate normal workpiece selection
    if (m_workspaceController) {
        QVector<Handle(AIS_Shape)> workpieces =
                m_workspaceController->getWorkpieceManager()->getWorkpieces();
        for (const Handle(AIS_Shape)& wp : workpieces) {
            if (!wp.IsNull()) {
                ctx->Activate(wp, 0, Standard_False);
                ctx->Activate(wp, 4, Standard_False);
            }
        }
    }
    m_candidateThreadFaces.clear();
    m_3dViewer->update();
}

void MainWindow::updateHighlightedThreadFace()
{
    if (!m_3dViewer || m_currentThreadFaceLocal.IsNull() || !m_workspaceController)
        return;

    Handle(AIS_InteractiveContext) ctx = m_3dViewer->getContext();
    if (!m_currentThreadFaceAIS.IsNull()) {
        ctx->Remove(m_currentThreadFaceAIS, Standard_False);
    }
    gp_Trsf trsf = m_workspaceController->getWorkpieceManager()->getCurrentTransformation();
    TopoDS_Shape global = m_currentThreadFaceLocal.Moved(trsf);
    m_currentThreadFaceAIS = new AIS_Shape(global);
    ctx->Display(m_currentThreadFaceAIS, AIS_Shaded, 0, false);
    Handle(Prs3d_Drawer) dr = new Prs3d_Drawer();
    dr->SetColor(Quantity_NOC_GREEN);
    dr->SetTransparency(Standard_ShortReal(0.3));
    ctx->HilightWithColor(m_currentThreadFaceAIS, dr, Standard_False);
    m_3dViewer->update();
}

void MainWindow::clearHighlightedThreadFace()
{
    if (!m_3dViewer || m_currentThreadFaceAIS.IsNull())
        return;

    Handle(AIS_InteractiveContext) ctx = m_3dViewer->getContext();
    ctx->Remove(m_currentThreadFaceAIS, Standard_False);
    m_currentThreadFaceAIS.Nullify();
    m_currentThreadFaceLocal.Nullify();
    m_3dViewer->update();
}

void MainWindow::handleWorkpieceTransformed()
{
    if (m_selectingThreadFace) {
        highlightThreadCandidateFaces();
    }
    updateHighlightedThreadFace();
}

// Parameter synchronization handlers removed with toolpath generation pipeline

void MainWindow::handleRawMaterialCreated(double diameter, double length)
{
    // Update the setup configuration panel with calculated raw material dimensions
    if (m_setupConfigPanel) {
        m_setupConfigPanel->setRawDiameter(diameter);
        m_setupConfigPanel->setRawMaterialLength(length);
    }
    
    statusBar()->showMessage(QString("Raw material created: %1mm diameter x %2mm length")
                           .arg(diameter, 0, 'f', 1)
                           .arg(length, 0, 'f', 1), 3000);
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material dimensions: %1mm diameter x %2mm length")
                             .arg(diameter, 0, 'f', 1)
                             .arg(length, 0, 'f', 1));
    }
}

// Missing handler implementations
void MainWindow::handleWorkspaceError(const QString& source, const QString& message)
{
    statusBar()->showMessage(QString("Error from %1: %2").arg(source).arg(message), 5000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("ERROR [%1]: %2").arg(source).arg(message));
    }
}

void MainWindow::handleChuckInitialized()
{
    statusBar()->showMessage("Chuck initialized successfully", 2000);
    if (m_outputWindow) {
        m_outputWindow->append("Chuck initialization completed");
    }
}

void MainWindow::handleWorkpieceWorkflowCompleted(double detectedDiameter, double rawMaterialDiameter)
{
    statusBar()->showMessage(QString("Workpiece workflow completed - Detected: %1mm, Raw: %2mm")
                           .arg(detectedDiameter, 0, 'f', 1)
                           .arg(rawMaterialDiameter, 0, 'f', 1), 3000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Workpiece workflow completed:")
                             .append(QString("\n  Detected diameter: %1mm").arg(detectedDiameter, 0, 'f', 1))
                             .append(QString("\n  Raw material diameter: %1mm").arg(rawMaterialDiameter, 0, 'f', 1)));
    }
}

void MainWindow::handleChuckCenterlineDetected(const gp_Ax1& axis)
{
    statusBar()->showMessage("Chuck centerline detected", 2000);
    if (m_outputWindow) {
        auto origin = axis.Location();
        auto direction = axis.Direction();
        m_outputWindow->append(QString("Chuck centerline detected at (%1, %2, %3) with direction (%4, %5, %6)")
                             .arg(origin.X(), 0, 'f', 2)
                             .arg(origin.Y(), 0, 'f', 2)
                             .arg(origin.Z(), 0, 'f', 2)
                             .arg(direction.X(), 0, 'f', 3)
                             .arg(direction.Y(), 0, 'f', 3)
                             .arg(direction.Z(), 0, 'f', 3));
    }
}

void MainWindow::handleMultipleCylindersDetected(const QList<CylinderInfo>& cylinders)
{
    statusBar()->showMessage(QString("Multiple cylinders detected: %1 candidates").arg(cylinders.size()), 3000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Multiple cylinders detected: %1 candidates").arg(cylinders.size()));
        for (int i = 0; i < cylinders.size(); ++i) {
            const auto& cyl = cylinders[i];
            m_outputWindow->append(QString("  Cylinder %1: diameter=%2mm, length=%3mm")
                                 .arg(i + 1)
                                 .arg(cyl.diameter, 0, 'f', 1)
                                 .arg(cyl.estimatedLength, 0, 'f', 1));
        }
    }
}

void MainWindow::handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo)
{
    statusBar()->showMessage(QString("Cylinder %1 selected - diameter: %2mm")
                           .arg(index + 1)
                           .arg(cylinderInfo.diameter, 0, 'f', 1), 3000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Selected cylinder %1: diameter=%2mm, length=%3mm")
                             .arg(index + 1)
                             .arg(cylinderInfo.diameter, 0, 'f', 1)
                             .arg(cylinderInfo.estimatedLength, 0, 'f', 1));
    }
}

void MainWindow::handleManualAxisSelected(double diameter, const gp_Ax1& axis)
{
    statusBar()->showMessage(QString("Manual axis selected - diameter: %1mm").arg(diameter, 0, 'f', 1), 3000);
    if (m_outputWindow) {
        auto origin = axis.Location();
        auto direction = axis.Direction();
        m_outputWindow->append(QString("Manual axis selected: diameter=%1mm")
                             .arg(diameter, 0, 'f', 1));
        m_outputWindow->append(QString("  Axis origin: (%1, %2, %3)")
                             .arg(origin.X(), 0, 'f', 2)
                             .arg(origin.Y(), 0, 'f', 2)
                             .arg(origin.Z(), 0, 'f', 2));
        m_outputWindow->append(QString("  Axis direction: (%1, %2, %3)")
                             .arg(direction.X(), 0, 'f', 3)
                             .arg(direction.Y(), 0, 'f', 3)
                             .arg(direction.Z(), 0, 'f', 3));
    }
}

void MainWindow::handlePartLoadingDistanceChanged(double distance)
{
    statusBar()->showMessage(QString("Distance to chuck changed: %1mm").arg(distance, 0, 'f', 1), 2000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Part distance to chuck updated: %1mm").arg(distance, 0, 'f', 1));
    }
    
    // Actually update the workpiece position in the workspace controller
    if (m_workspaceController && m_workspaceController->isInitialized()) {
        bool success = m_workspaceController->updateDistanceToChuck(distance);
        if (!success) {
            statusBar()->showMessage(QString("Failed to update workpiece position"), 3000);
            if (m_outputWindow) {
                m_outputWindow->append("Failed to update workpiece position");
            }
        }
    }
}

void MainWindow::handlePartLoadingDiameterChanged(double diameter)
{
    statusBar()->showMessage(QString("Part loading diameter changed: %1mm").arg(diameter, 0, 'f', 1), 2000);
    if (m_outputWindow) {
        m_outputWindow->append(QString("Part loading diameter updated: %1mm").arg(diameter, 0, 'f', 1));
    }
}

void MainWindow::handleWorkpiecePositionChanged(double distance)
{
    if (m_outputWindow) {
        m_outputWindow->append(QString("Workpiece position changed to %1mm").arg(distance, 0, 'f', 1));
    }
    
    // Don't regenerate toolpaths immediately - this causes slider lag
    // Instead, use a timer to debounce the regeneration
    if (m_toolpathRegenerationTimer) {
        m_toolpathRegenerationTimer->stop();
    } else {
        m_toolpathRegenerationTimer = new QTimer(this);
        m_toolpathRegenerationTimer->setSingleShot(true);
        connect(m_toolpathRegenerationTimer, &QTimer::timeout, this, [this]() {
            if (m_outputWindow) {
                m_outputWindow->append("Regenerating toolpaths after position change...");
            }
            
            // Clear existing toolpaths from display
            if (m_3dViewer && m_3dViewer->getContext()) {
                // Find and remove toolpath display objects
                AIS_ListOfInteractive allObjects;
                m_3dViewer->getContext()->DisplayedObjects(allObjects);
                
                for (AIS_ListOfInteractive::Iterator it(allObjects); it.More(); it.Next()) {
                    Handle(AIS_InteractiveObject) obj = it.Value();
                    // Check if this is a toolpath object
                    if (!obj.IsNull() && obj->DynamicType()->Name() == std::string("AIS_Shape")) {
                        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
                        if (!shapeObj.IsNull()) {
                            TopoDS_Shape shape = shapeObj->Shape();
                            if (!shape.IsNull() && (shape.ShapeType() == TopAbs_EDGE || shape.ShapeType() == TopAbs_WIRE)) {
                                m_3dViewer->getContext()->Remove(shapeObj, Standard_False);
                            }
                        }
                    }
                }
                m_3dViewer->update();
            }
            
            // Regenerate toolpaths if we have operations enabled
            if (m_setupConfigPanel) {
                handleGenerateToolpaths();
            }
        });
    }
    
    // Start or restart the timer - regenerate toolpaths 500ms after the last position change
    m_toolpathRegenerationTimer->start(500);
}

// Operation tile handlers
void MainWindow::handleOperationTileEnabledChanged(const QString& operationName, bool enabled)
{
    // Update the setup configuration panel to reflect the change
    if (m_setupConfigPanel) {
        m_setupConfigPanel->setOperationEnabled(operationName, enabled);
    }
    
    // Select and show the operation widget if enabled
    if (enabled && m_setupConfigPanel && m_operationTileContainer) {
        // If no operation is currently selected, or if this is the first enabled operation,
        // automatically select this one
        QString currentSelection = m_operationTileContainer->getSelectedOperation();
        if (currentSelection.isEmpty() || !m_operationTileContainer->isTileEnabled(currentSelection)) {
            m_operationTileContainer->setSelectedOperation(operationName);
            m_setupConfigPanel->showOperationWidget(operationName);
        }
    }
    
    // Auto-select default tool for newly enabled operations
    if (enabled && m_operationTileContainer && m_toolManager) {
        QString defaultTool = getDefaultToolForOperation(operationName);
        if (!defaultTool.isEmpty()) {
            m_operationTileContainer->setTileSelectedTool(operationName, defaultTool);
        }
    }
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Operation %1 %2").arg(operationName, enabled ? "enabled" : "disabled"));
    }
    
    statusBar()->showMessage(QString("Operation %1 %2").arg(operationName, enabled ? "enabled" : "disabled"), 2000);
}

void MainWindow::handleOperationTileClicked(const QString& operationName)
{
    // Set the selected operation in the tile container
    if (m_operationTileContainer) {
        m_operationTileContainer->setSelectedOperation(operationName);
    }
    
    // Show the corresponding operation widget in the setup configuration panel
    if (m_setupConfigPanel) {
        m_setupConfigPanel->showOperationWidget(operationName);
    }
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Selected %1 operation").arg(operationName));
    }
    
    statusBar()->showMessage(QString("Selected %1 operation").arg(operationName), 2000);
}

void MainWindow::handleOperationTileToolSelectionRequested(const QString& operationName)
{
    // Open tool management dialog for this operation
    if (m_toolManagementTab && m_tabWidget) {
        // Switch to tool management tab
        m_tabWidget->setCurrentWidget(m_toolManagementTab);
        
        // Filter tools appropriate for this operation
        m_toolManagementTab->filterToolsByOperation(operationName);
        
        if (m_outputWindow) {
            m_outputWindow->append(QString("Opening tool selection for %1 operation").arg(operationName));
        }
        
        statusBar()->showMessage(QString("Select tool for %1 operation").arg(operationName), 2000);
    }
}

void MainWindow::handleOperationTileExpandedChanged(const QString& operationName, bool expanded)
{
    if (operationName == "Internal Features") {
        if (m_outputWindow) {
            m_outputWindow->append(QString("Internal Features %1").arg(expanded ? "expanded" : "collapsed"));
        }
        
        statusBar()->showMessage(QString("Internal Features %1").arg(expanded ? "expanded" : "collapsed"), 1000);
    }
}

QString MainWindow::getDefaultToolForOperation(const QString& operationName) const
{
    if (!m_toolManager) return QString();
    
    // Get all available tools and find the first suitable one for each operation
    QStringList allTools = m_toolManager->getAllToolIds();
    
    for (const QString& toolId : allTools) {
        IntuiCAM::GUI::CuttingTool tool = m_toolManager->getTool(toolId);
        
        // Match operation to appropriate tool types
        if (operationName == "Facing" && tool.type == IntuiCAM::GUI::ToolType::TurningInsert) {
            return tool.name;
        } else if (operationName == "Roughing" && tool.type == IntuiCAM::GUI::ToolType::TurningInsert) {
            return tool.name;
        } else if (operationName == "Finishing" && tool.type == IntuiCAM::GUI::ToolType::TurningInsert) {
            return tool.name;
        } else if (operationName == "Parting" && tool.type == IntuiCAM::GUI::ToolType::PartingTool) {
            return tool.name;
        } else if (operationName == "Threading" && tool.type == IntuiCAM::GUI::ToolType::ThreadingTool) {
            return tool.name;
        } else if (operationName == "Grooving" && tool.type == IntuiCAM::GUI::ToolType::FormTool) {
            return tool.name;  // Use FormTool for grooving operations
        } else if (operationName == "Drilling" && tool.type == IntuiCAM::GUI::ToolType::BoringBar) {
            return tool.name;  // Use BoringBar for drilling operations
        }
    }
    
    // Fallback to first available tool
    if (!allTools.isEmpty()) {
        IntuiCAM::GUI::CuttingTool tool = m_toolManager->getTool(allTools.first());
        return tool.name;
    }
    
    return QString();
}

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
        TopoDS_Shape shape = m_stepLoader->loadStepFile(filePath.toStdString());
        
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

                // Immediately apply part and material setup parameters
                if (m_setupConfigPanel) {
                    double dist = m_setupConfigPanel->getDistanceToChuck();
                    double rawDia = m_setupConfigPanel->getRawDiameter();
                    bool flip = m_setupConfigPanel->isOrientationFlipped();
                    m_workspaceController->applyPartLoadingSettings(dist, rawDia, flip);
                }
            } else {
                QString errorMsg = "Failed to process workpiece through workspace controller";
                statusBar()->showMessage(errorMsg, 5000);
                if (m_outputWindow) {
                    m_outputWindow->append(errorMsg);
                }
            }
        } else {
            QString errorMsg = QString("Failed to load STEP file: %1")
                                     .arg(QString::fromStdString(m_stepLoader->getLastError()));
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

// Stub implementations for missing methods
QWidget* MainWindow::createSimulationTab()
{
    QWidget* simulationTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(simulationTab);
    
    QLabel* placeholder = new QLabel("Simulation Tab - Coming Soon");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("font-size: 24px; color: #666;");
    
    layout->addWidget(placeholder);
    
    return simulationTab;
}

QWidget* MainWindow::createMachineTab()
{
    QWidget* machineTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(machineTab);
    
    QLabel* placeholder = new QLabel("Machine Tab - Coming Soon");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("font-size: 24px; color: #666;");
    
    layout->addWidget(placeholder);
    
    return machineTab;
}

void MainWindow::createViewModeOverlayButton(QWidget* parent)
{
    // Create view mode toggle button
    m_viewModeOverlayButton = new QPushButton("3D View", parent);
    m_viewModeOverlayButton->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 0, 0, 100);"
        "  color: white;"
        "  border: 1px solid #555;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(0, 0, 0, 150);"
        "}"
    );
    
    // Connect to toggle view mode
    connect(m_viewModeOverlayButton, &QPushButton::clicked, this, &MainWindow::toggleViewMode);
    
    // Create visibility button and menu
    m_visibilityButton = new QToolButton(parent);
    m_visibilityButton->setText("👁 Visibility");
    m_visibilityButton->setPopupMode(QToolButton::InstantPopup);
    m_visibilityButton->setStyleSheet(
        "QToolButton {"
        "  background-color: rgba(0, 0, 0, 100);"
        "  color: white;"
        "  border: 1px solid #555;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(0, 0, 0, 150);"
        "}"
        "QToolButton::menu-indicator {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: center right;"
        "}"
    );
    
    // Create visibility menu
    m_visibilityMenu = new QMenu(m_visibilityButton);
    
    // Create visibility actions
    m_showChuckAction = new QAction("Show Chuck", this);
    m_showChuckAction->setCheckable(true);
    m_showChuckAction->setChecked(true);
    connect(m_showChuckAction, &QAction::triggered, this, &MainWindow::handleShowChuckToggled);
    
    m_showRawMaterialAction = new QAction("Show Raw Material", this);
    m_showRawMaterialAction->setCheckable(true);
    m_showRawMaterialAction->setChecked(true);
    connect(m_showRawMaterialAction, &QAction::triggered, this, &MainWindow::handleShowRawMaterialToggled);
    
    m_showToolpathsAction = new QAction("Show Toolpaths", this);
    m_showToolpathsAction->setCheckable(true);
    m_showToolpathsAction->setChecked(true);
    connect(m_showToolpathsAction, &QAction::triggered, this, &MainWindow::handleShowToolpathsToggled);
    
    m_showPartAction = new QAction("Show Part", this);
    m_showPartAction->setCheckable(true);
    m_showPartAction->setChecked(true);
    connect(m_showPartAction, &QAction::triggered, this, &MainWindow::handleShowPartToggled);
    
    m_showProfilesAction = new QAction("Show Profiles", this);
    m_showProfilesAction->setCheckable(true);
    m_showProfilesAction->setChecked(true);
    connect(m_showProfilesAction, &QAction::triggered, this, &MainWindow::handleShowProfilesToggled);
    
    // Add actions to menu
    m_visibilityMenu->addAction(m_showChuckAction);
    m_visibilityMenu->addAction(m_showRawMaterialAction);
    m_visibilityMenu->addSeparator();
    m_visibilityMenu->addAction(m_showPartAction);
    m_visibilityMenu->addAction(m_showProfilesAction);
    m_visibilityMenu->addSeparator();
    m_visibilityMenu->addAction(m_showToolpathsAction);
    
    // Set menu on button
    m_visibilityButton->setMenu(m_visibilityMenu);
}