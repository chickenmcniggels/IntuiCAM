#include "mainwindow.h"
#include "opengl3dwidget.h"
#include "steploader.h"
#include "chuckmanager.h"
#include "workpiecemanager.h"
#include "rawmaterialmanager.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_leftSplitter(nullptr)
    , m_projectTree(nullptr)
    , m_propertiesPanel(nullptr)
    , m_3dViewer(nullptr)
    , m_outputWindow(nullptr)
    , m_stepLoader(nullptr)
    , m_mainToolBar(nullptr)
{
    setWindowTitle("IntuiCAM - Computer Aided Manufacturing");
    setMinimumSize(1024, 768);
    resize(1400, 900);
    
    // Initialize STEP loader
    m_stepLoader = new StepLoader();
    
    createMenus();
    createToolBars();
    createCentralWidget();
    createStatusBar();
    setupConnections();
    
    // Set initial status
    statusBar()->showMessage("Ready - Welcome to IntuiCAM", 2000);
    
    // Initialize chuck automatically after a short delay to ensure OpenGL is ready
    QTimer::singleShot(100, this, &MainWindow::initializeChuck);
}

MainWindow::~MainWindow()
{
    // Clean up our custom objects
    delete m_stepLoader;
    // Qt handles cleanup of widgets automatically
}

void MainWindow::createMenus()
{
    // File Menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_newAction = new QAction(tr("&New Project"), this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("Create a new CAM project"));
    m_fileMenu->addAction(m_newAction);
    
    m_openAction = new QAction(tr("&Open Project"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open an existing CAM project"));
    m_fileMenu->addAction(m_openAction);
    
    m_openStepAction = new QAction(tr("Open &STEP File"), this);
    m_openStepAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    m_openStepAction->setStatusTip(tr("Open a STEP file for viewing"));
    m_fileMenu->addAction(m_openStepAction);
    
    m_fileMenu->addSeparator();
    
    m_saveAction = new QAction(tr("&Save Project"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Save the current project"));
    m_fileMenu->addAction(m_saveAction);
    
    m_fileMenu->addSeparator();
    
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    m_fileMenu->addAction(m_exitAction);
    
    // Edit Menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_preferencesAction = new QAction(tr("&Preferences"), this);
    m_preferencesAction->setStatusTip(tr("Configure application preferences"));
    m_editMenu->addAction(m_preferencesAction);
    
    // View Menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    
    // Tools Menu
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    
    // Help Menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_aboutAction = new QAction(tr("&About IntuiCAM"), this);
    m_aboutAction->setStatusTip(tr("Show information about IntuiCAM"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBars()
{
    m_mainToolBar = addToolBar(tr("Main"));
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
    
    // 3D Viewport - OpenCASCADE viewer
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
    if (m_newAction) connect(m_newAction, &QAction::triggered, this, &MainWindow::newProject);
    if (m_openAction) connect(m_openAction, &QAction::triggered, this, &MainWindow::openProject);
    if (m_openStepAction) connect(m_openStepAction, &QAction::triggered, this, &MainWindow::openStepFile);
    if (m_saveAction) connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    if (m_exitAction) connect(m_exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
    if (m_aboutAction) connect(m_aboutAction, &QAction::triggered, this, &MainWindow::aboutApplication);
    if (m_preferencesAction) connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::showPreferences);
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
    
    // Load the STEP file
    if (m_stepLoader && m_3dViewer) {
        TopoDS_Shape shape = m_stepLoader->loadStepFile(fileName);
        
        if (m_stepLoader->isValid() && !shape.IsNull()) {
            // Clear previous workpieces only (keep chuck)
            if (m_3dViewer) {
                if (m_3dViewer->getWorkpieceManager()) {
                    m_3dViewer->getWorkpieceManager()->clearWorkpieces();
                }
                if (m_3dViewer->getRawMaterialManager()) {
                    m_3dViewer->getRawMaterialManager()->clearRawMaterial();
                }
            }
            
            // Add as workpiece (this will auto-align and create raw material)
            m_3dViewer->addWorkpiece(shape);
            
            statusBar()->showMessage(tr("STEP file loaded and aligned successfully"), 3000);
            if (m_outputWindow) {
                m_outputWindow->append("STEP file loaded as workpiece and auto-aligned with chuck.");
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
        QString errorMsg = "STEP loader or 3D viewer not initialized";
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

void MainWindow::initializeChuck()
{
    // Initialize chuck automatically
    QString chuckFilePath = "C:/Users/nikla/Downloads/three_jaw_chuck.step";
    if (m_3dViewer) {
        if (m_outputWindow) {
            m_outputWindow->append("Initializing 3-jaw chuck...");
        }
        m_3dViewer->initializeChuck(chuckFilePath);
        statusBar()->showMessage("Chuck initialization completed", 3000);
    } else {
        if (m_outputWindow) {
            m_outputWindow->append("Error: 3D viewer not available for chuck initialization");
        }
    }
} 