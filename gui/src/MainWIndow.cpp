#include "MainWindow.h"
#include "Core.h"
#include "IntuiCAMViewerWidget.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_core(nullptr)
    , m_viewerWidget(nullptr)
{
    setWindowTitle("IntuiCAM");
    resize(1024, 768);
    
    // Initialize core
    m_core = new IntuiCAM::Core();
    if (!m_core->initialize()) {
        QMessageBox::critical(this, "Error", "Failed to initialize IntuiCAM core engine!");
        throw std::runtime_error("Failed to initialize core engine");
    }
    
    // Create viewer widget
    m_viewerWidget = new IntuiCAMViewerWidget(this);
    setCentralWidget(m_viewerWidget);
    
    // Set up UI
    createActions();
    createMenus();
    createToolbars();
    createStatusBar();
    
    // Load settings
    QSettings settings("IntuiCAM", "IntuiCAM");
    m_recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFilesMenu();
    
    // Show version info
    statusBar()->showMessage(QString("IntuiCAM v%1 | OpenCASCADE v%2")
                            .arg(QString::fromStdString(m_core->getVersion()))
                            .arg(QString::fromStdString(m_core->getOCCTVersion())));
}

MainWindow::~MainWindow() {
    if (m_core) {
        m_core->shutdown();
        delete m_core;
    }
}

void MainWindow::openFile(const QString &filePath) {
    qDebug() << "Opening file:" << filePath;
    
    // Load file (implement this)
    bool success = true;
    
    if (success) {
        m_currentFilePath = filePath;
        
        // Add to recent files
        m_recentFiles.removeAll(filePath);
        m_recentFiles.prepend(filePath);
        while (m_recentFiles.size() > 10) {
            m_recentFiles.removeLast();
        }
        
        // Save settings
        QSettings settings("IntuiCAM", "IntuiCAM");
        settings.setValue("recentFiles", m_recentFiles);
        
        updateRecentFilesMenu();
        
        statusBar()->showMessage(QString("File loaded: %1").arg(filePath), 5000);
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to open file: %1").arg(filePath));
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Check if there are unsaved changes
    // If so, prompt the user to save
    event->accept();
}

void MainWindow::onNew() {
    // Implement new file
    qDebug() << "New file";
}

void MainWindow::onOpen() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", QString(),
                                                "CAD Files (*.step *.stp);;All Files (*)");
    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
}

void MainWindow::onSave() {
    // Implement save
    qDebug() << "Save file";
}

void MainWindow::onSaveAs() {
    // Implement save as
    qDebug() << "Save as";
}

void MainWindow::onExit() {
    close();
}

void MainWindow::onAbout() {
    QString version = QString::fromStdString(m_core->getVersion());
    QString occtVersion = QString::fromStdString(m_core->getOCCTVersion());
    
    QMessageBox::about(this, "About IntuiCAM",
                      QString("<h3>IntuiCAM v%1</h3>"
                              "<p>CAD/CAM Software</p>"
                              "<p>Built with OpenCASCADE v%2</p>"
                              "<p>Copyright © 2023</p>")
                      .arg(version)
                      .arg(occtVersion));
}

void MainWindow::createActions() {
    // File actions
    m_newAction = new QAction("&New", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNew);
    
    m_openAction = new QAction("&Open...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpen);
    
    m_saveAction = new QAction("&Save", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSave);
    
    m_saveAsAction = new QAction("Save &As...", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);
    
    // Help actions
    m_aboutAction = new QAction("&About", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createMenus() {
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");
    m_fileMenu->addAction(m_newAction);
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addAction(m_saveAsAction);
    m_fileMenu->addSeparator();
    
    // Recent files submenu
    m_recentFilesMenu = m_fileMenu->addMenu("Recent Files");
    updateRecentFilesMenu();
    
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    // Edit menu
    m_editMenu = menuBar()->addMenu("&Edit");
    
    // View menu
    m_viewMenu = menuBar()->addMenu("&View");
    
    // Help menu
    m_helpMenu = menuBar()->addMenu("&Help");
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolbars() {
    // File toolbar
    m_fileToolbar = addToolBar("File");
    m_fileToolbar->addAction(m_newAction);
    m_fileToolbar->addAction(m_openAction);
    m_fileToolbar->addAction(m_saveAction);
    
    // Edit toolbar
    m_editToolbar = addToolBar("Edit");
    
    // View toolbar
    m_viewToolbar = addToolBar("View");
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage("Ready");
}

void MainWindow::updateRecentFilesMenu() {
    m_recentFilesMenu->clear();
    
    for (const QString &file : m_recentFiles) {
        QAction *action = m_recentFilesMenu->addAction(file);
        connect(action, &QAction::triggered, [this, file]() {
            openFile(file);
        });
    }
    
    if (m_recentFiles.isEmpty()) {
        m_recentFilesMenu->addAction("No recent files")->setEnabled(false);
    }
} 