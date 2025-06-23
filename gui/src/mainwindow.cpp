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
    , m_defaultChuckFilePath("C:/Users/nikla/Downloads/three_jaw_chuck.step")
{
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
        connect(m_setupConfigPanel, &IntuiCAM::GUI::SetupConfigurationPanel::automaticToolpathGenerationRequested,
                this, &MainWindow::handleGenerateToolpaths);
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
    QHBoxLayout* controlTabLayout = new QHBoxLayout(machineWidget);
    controlTabLayout->setContentsMargins(0, 0, 0, 0);
    
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
    
    // Make emergency button bold and red to indicate importance
    QFont emergencyFont = emergencyBtn->font();
    emergencyFont.setBold(true);
    emergencyBtn->setFont(emergencyFont);
    emergencyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #dc3545;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #c82333;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #bd2130;"
        "}"
    );
    
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
    
    controlTabLayout->addWidget(m_machineControlPanel);
    controlTabLayout->addWidget(m_machineFeedWidget, 1);
    
    return machineWidget;
}

void MainWindow::createViewModeOverlayButton(QWidget *parent)
{
    // Create the buttons as regular widgets instead of overlays
    m_viewModeOverlayButton = new QPushButton("Switch to Lathe View", parent);
    m_viewModeOverlayButton->setMaximumWidth(150);
    m_viewModeOverlayButton->setMaximumHeight(30);

    // Create visibility tool button with menu
    m_visibilityButton = new QToolButton(parent);
    m_visibilityButton->setText("Visibility");
    m_visibilityButton->setPopupMode(QToolButton::InstantPopup);
    m_visibilityButton->setMaximumHeight(30);

    m_visibilityMenu = new QMenu(m_visibilityButton);
    m_showChuckAction = m_visibilityMenu->addAction("Show Chuck");
    m_showChuckAction->setCheckable(true);
    m_showChuckAction->setChecked(true);
    m_showRawMaterialAction = m_visibilityMenu->addAction("Show Raw Material");
    m_showRawMaterialAction->setCheckable(true);
    m_showRawMaterialAction->setChecked(true);
    m_showToolpathsAction = m_visibilityMenu->addAction("Show Toolpaths");
    m_showToolpathsAction->setCheckable(true);
    m_showToolpathsAction->setChecked(true);
    m_showPartAction = m_visibilityMenu->addAction("Show Part");
    m_showPartAction->setCheckable(true);
    m_showPartAction->setChecked(true);
    m_visibilityButton->setMenu(m_visibilityMenu);

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

    // Style the visibility button similarly
    m_visibilityButton->setStyleSheet(
        "QToolButton {"
        "  background-color: rgba(240, 240, 240, 220);"
        "  border: 2px solid #666666;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  color: #333333;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(255, 255, 255, 240);"
        "  border: 2px solid #444444;"
        "  color: #222222;"
        "}"
    );

    // Connect the button to the toggle function
    connect(m_viewModeOverlayButton, &QPushButton::clicked, this, &MainWindow::toggleViewMode);

    // Connect visibility actions
    connect(m_showChuckAction, &QAction::toggled, this, &MainWindow::handleShowChuckToggled);
    connect(m_showRawMaterialAction, &QAction::toggled, this, &MainWindow::handleShowRawMaterialToggled);
    connect(m_showToolpathsAction, &QAction::toggled, this, &MainWindow::handleShowToolpathsToggled);
    connect(m_showPartAction, &QAction::toggled, this, &MainWindow::handleShowPartToggled);

    // Connect to view mode changes to update button text
    connect(m_3dViewer, &OpenGL3DWidget::viewModeChanged, this, &MainWindow::updateViewModeOverlayButton);

    // These buttons are now part of the regular layout
    qDebug() << "View mode controls created in MainWindow";
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
    
    // No further positioning needed when part of the layout
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
    if (m_outputWindow) {
        m_outputWindow->append(QString("Operation %1 %2").arg(operationName)
                               .arg(enabled ? "enabled" : "disabled"));
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
    
    // Step 2: Get enabled operations from setup panel
    QStringList enabledOperations;
    if (m_setupConfigPanel) {
        if (m_setupConfigPanel->isOperationEnabled("Contouring")) {
            enabledOperations << "Contouring";
        }
        if (m_setupConfigPanel->isOperationEnabled("Threading")) {
            enabledOperations << "Threading";
        }
        if (m_setupConfigPanel->isOperationEnabled("Chamfering")) {
            enabledOperations << "Chamfering";
        }
        if (m_setupConfigPanel->isOperationEnabled("Parting")) {
            enabledOperations << "Parting";
        }
    }
    
    if (enabledOperations.isEmpty()) {
        statusBar()->showMessage("No operations enabled. Please enable at least one operation.", 5000);
        if (m_outputWindow) {
            m_outputWindow->append("ERROR: No operations enabled - please enable at least one operation in the setup panel");
        }
        return;
    }
    
    if (m_outputWindow) {
        m_outputWindow->append(QString("Enabled operations: %1").arg(enabledOperations.join(", ")));
    }
    
    try {
        // Step 3: Get part geometry and workspace data
        TopoDS_Shape partGeometry = m_workspaceController->getPartShape();
        
        // Step 4: Create enabled operations list for pipeline
        std::vector<IntuiCAM::Toolpath::ToolpathGenerationPipeline::EnabledOperation> pipelineOps;
        
        for (const QString& opName : enabledOperations) {
            IntuiCAM::Toolpath::ToolpathGenerationPipeline::EnabledOperation op;
            op.operationType = opName.toStdString();
            op.enabled = true;
            
            // Fill operation parameters based on setup panel values
            if (opName == "Contouring") {
                op.numericParams["facingAllowance"] = m_setupConfigPanel ? m_setupConfigPanel->getFacingAllowance() : 0.5;
                op.numericParams["roughingAllowance"] = m_setupConfigPanel ? m_setupConfigPanel->getRoughingAllowance() : 0.2;
                op.numericParams["finishingAllowance"] = m_setupConfigPanel ? m_setupConfigPanel->getFinishingAllowance() : 0.05;
                op.stringParams["material"] = m_setupConfigPanel ? m_setupConfigPanel->getSelectedMaterialName().toStdString() : "steel";
            }
            else if (opName == "Threading") {
                op.numericParams["pitch"] = 1.5; // Default metric thread pitch
                op.numericParams["depth"] = 2.0; // Default thread depth
                op.stringParams["threadForm"] = "metric";
            }
            else if (opName == "Chamfering") {
                op.numericParams["chamferSize"] = 0.5; // Default chamfer size
                op.numericParams["chamferAngle"] = 45.0; // Default chamfer angle
            }
            else if (opName == "Parting") {
                op.numericParams["partingWidth"] = m_setupConfigPanel ? m_setupConfigPanel->getPartingWidth() : 3.0;
                op.numericParams["retractDistance"] = 5.0; // Default retract distance
            }
            
            pipelineOps.push_back(std::move(op));
        }
        
        // Step 5: Get global parameters from workspace and setup panel
        IntuiCAM::Toolpath::ToolpathGenerationPipeline::ToolpathGenerationParameters globalParams;
        
        // Get turning axis from workspace (chuck centerline)
        if (m_workspaceController->hasChuckCenterline()) {
            globalParams.turningAxis = m_workspaceController->getChuckCenterlineAxis();
        } else {
            // Default Z-axis if no chuck centerline
            globalParams.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        }
        
        // Get material and other parameters from setup panel
        if (m_setupConfigPanel) {
            globalParams.materialType = m_setupConfigPanel->getSelectedMaterialName().toStdString();
            globalParams.profileTolerance = m_setupConfigPanel->getTolerance();
        }
        
        // Estimate part dimensions (basic bounding box approach)
        Bnd_Box bbox;
        BRepBndLib::Add(partGeometry, bbox);
        if (!bbox.IsVoid()) {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            globalParams.partDiameter = std::max(xmax - xmin, ymax - ymin) * 2.0;  // Rough estimate
            globalParams.partLength = zmax - zmin;
        }
        
        // Step 6: Get primary tool (use first recommended tool or create default)
        std::shared_ptr<IntuiCAM::Toolpath::Tool> primaryTool;
        if (m_toolManager) {
            // Try to get best tool for the first operation
            auto bestTool = m_toolManager->getBestTool(
                enabledOperations.first().toLower(),
                QString::fromStdString(globalParams.materialType),
                globalParams.partDiameter
            );
            
            if (!bestTool.id.isEmpty()) {
                // Convert CuttingTool to Toolpath::Tool (simplified)
                primaryTool = std::make_shared<IntuiCAM::Toolpath::Tool>(
                    IntuiCAM::Toolpath::Tool::Type::Turning,
                    bestTool.name.toStdString()
                );
                primaryTool->setDiameter(bestTool.geometry.diameter);
                primaryTool->setLength(bestTool.geometry.length);
            }
        }
        
        // Create default tool if none found
        if (!primaryTool) {
            primaryTool = std::make_shared<IntuiCAM::Toolpath::Tool>(IntuiCAM::Toolpath::Tool::Type::Turning, "Default Turning Tool");
            primaryTool->setDiameter(12.0); // 12mm default tool diameter
            primaryTool->setLength(100.0);  // 100mm default tool length
        }
        
        if (m_outputWindow) {
            m_outputWindow->append(QString("Using tool: %1 (diameter: %2mm)")
                                   .arg(QString::fromStdString(primaryTool->getName()))
                                   .arg(primaryTool->getDiameter(), 0, 'f', 1));
        }
        
        // Step 7: Create generation request
        IntuiCAM::Toolpath::ToolpathGenerationPipeline::GenerationRequest request;
        request.partGeometry = partGeometry;
        request.enabledOps = pipelineOps;
        request.globalParams = globalParams;
        request.primaryTool = primaryTool;
        
        // Add progress callback
        request.progressCallback = [this](double progress, const std::string& message) {
            statusBar()->showMessage(QString::fromStdString(message), 1000);
            if (m_outputWindow) {
                m_outputWindow->append(QString("Progress: %1% - %2")
                                       .arg(static_cast<int>(progress * 100))
                                       .arg(QString::fromStdString(message)));
            }
        };
        
        // Step 8: Execute pipeline
        if (m_outputWindow) {
            m_outputWindow->append("Executing toolpath generation pipeline...");
        }
        
        IntuiCAM::Toolpath::ToolpathGenerationPipeline pipeline;
        auto result = pipeline.generateToolpaths(request);
        
        // Step 9: Process results
        if (result.success) {
            statusBar()->showMessage(QString("Toolpath generation completed successfully! Generated %1 toolpaths.")
                                     .arg(result.generatedToolpaths.size()), 5000);
            
            if (m_outputWindow) {
                m_outputWindow->append("=== Toolpath Generation Results ===");
                m_outputWindow->append(QString("✓ Generated %1 toolpaths successfully")
                                       .arg(result.generatedToolpaths.size()));
                m_outputWindow->append(QString("✓ Extracted profile with %1 points")
                                       .arg(result.extractedProfile.size()));
                m_outputWindow->append(QString("✓ Total machining time: %1 minutes")
                                       .arg(result.statistics.totalMachiningTime, 0, 'f', 1));
                m_outputWindow->append(QString("✓ Total material removal: %1 mm³")
                                       .arg(result.statistics.materialRemovalVolume, 0, 'f', 2));
                m_outputWindow->append(QString("✓ Total movements: %1")
                                       .arg(result.statistics.totalMovements));
                
                // Log warnings if any
                for (const auto& warning : result.warnings) {
                    m_outputWindow->append(QString("⚠ Warning: %1").arg(QString::fromStdString(warning)));
                }
            }
            
            // Step 10: Display toolpaths in 3D viewer
            if (m_3dViewer && !result.toolpathDisplayObjects.empty()) {
                // Add toolpath display objects to the 3D viewer
                for (const auto& displayObj : result.toolpathDisplayObjects) {
                    m_3dViewer->getContext()->Display(displayObj, Standard_False);
                }
                
                // Add profile display object if available
                if (!result.profileDisplayObject.IsNull()) {
                    m_3dViewer->getContext()->Display(result.profileDisplayObject, Standard_False);
                }
                
                // Update display
                m_3dViewer->getContext()->UpdateCurrentViewer();
                m_3dViewer->update();
                
                if (m_outputWindow) {
                    m_outputWindow->append("✓ Toolpaths displayed in 3D viewer");
                }
            }
            
            // Step 11: Inform user that simulation preview is available
            if (m_outputWindow) {
                m_outputWindow->append("Toolpath generation complete - switch to the Simulation tab to review");
            }
            
        } else {
            // Handle errors
            statusBar()->showMessage(QString("Toolpath generation failed: %1")
                                     .arg(QString::fromStdString(result.errorMessage)), 5000);
            
            if (m_outputWindow) {
                m_outputWindow->append("=== Toolpath Generation Failed ===");
                m_outputWindow->append(QString("✗ Error: %1").arg(QString::fromStdString(result.errorMessage)));
                
                // Log warnings as well
                for (const auto& warning : result.warnings) {
                    m_outputWindow->append(QString("⚠ Warning: %1").arg(QString::fromStdString(warning)));
                }
            }
        }
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception during toolpath generation: %1").arg(e.what());
        statusBar()->showMessage(errorMsg, 5000);
        
        if (m_outputWindow) {
            m_outputWindow->append("=== Toolpath Generation Exception ===");
            m_outputWindow->append(QString("✗ Exception: %1").arg(e.what()));
        }
    } catch (...) {
        QString errorMsg = "Unknown exception during toolpath generation";
        statusBar()->showMessage(errorMsg, 5000);
        
        if (m_outputWindow) {
            m_outputWindow->append("=== Toolpath Generation Exception ===");
            m_outputWindow->append("✗ Unknown exception occurred");
        }
    }
    
    if (m_outputWindow) {
        m_outputWindow->append("=== Toolpath Generation Complete ===");
    }
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
  if (m_setupConfigPanel) {
    m_setupConfigPanel->setRawDiameter(rawMaterialDiameter);
  }

  statusBar()->showMessage(tr("Detected raw material diameter: %1 mm")
                               .arg(rawMaterialDiameter, 0, 'f', 1),
                           3000);

  if (m_outputWindow) {
    m_outputWindow->append(QString("Workpiece workflow completed - detected "
                                   "diameter: %1 mm, raw material: %2 mm")
                               .arg(diameter, 0, 'f', 1)
                               .arg(rawMaterialDiameter, 0, 'f', 1));
  }
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
    if (!m_workspaceController || !m_setupConfigPanel) {
        return;
    }

    // Reapply part setup parameters so that manual axis selection behaves the
    // same as initial part loading
    double dist = m_setupConfigPanel->getDistanceToChuck();
    double rawDia = m_setupConfigPanel->getRawDiameter();
    bool flip = m_setupConfigPanel->isOrientationFlipped();

    m_workspaceController->applyPartLoadingSettings(dist, rawDia, flip);

    if (m_outputWindow) {
        m_outputWindow->append("Manual axis selected - reapplied part setup parameters");
    }
}

void MainWindow::handleRawMaterialCreated(double diameter, double length)
{
    if (m_setupConfigPanel) {
        m_setupConfigPanel->updateRawMaterialLength(length);
    }

    statusBar()->showMessage(tr("Raw material length: %1 mm").arg(length), 2000);

    if (m_outputWindow) {
        m_outputWindow->append(QString("Raw material created - diameter: %1 mm, length: %2 mm")
                                   .arg(diameter)
                                   .arg(length));
    }
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