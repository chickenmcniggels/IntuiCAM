#include "toolmanagementtab.h"
#include "toolmanagementdialog.h"
#include "mainwindow.h"
#include "IntuiCAM/Toolpath/ToolTypes.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QStyle>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>

using namespace IntuiCAM::Toolpath;

ToolManagementTab::ToolManagementTab(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_toolbarLayout(nullptr)
    , m_toolbarFrame(nullptr)
    , m_addToolButton(nullptr)
    , m_editToolButton(nullptr)
    , m_deleteToolButton(nullptr)
    , m_duplicateToolButton(nullptr)
    , m_moreActionsButton(nullptr)
    , m_moreActionsMenu(nullptr)
    , m_importLibraryAction(nullptr)
    , m_exportLibraryAction(nullptr)
    , m_loadDefaultsAction(nullptr)
    , m_refreshAction(nullptr)
    , m_filterPanel(nullptr)
    , m_filterLayout(nullptr)
    , m_searchBox(nullptr)
    , m_toolTypeFilter(nullptr)
    , m_materialFilter(nullptr)
    , m_statusFilter(nullptr)
    , m_clearFiltersButton(nullptr)
    , m_toolListWidget(nullptr)
    , m_toolListLayout(nullptr)
    , m_toolTreeWidget(nullptr)
    , m_toolDetailsPanel(nullptr)
    , m_toolDetailsLayout(nullptr)
    , m_toolDetailsTitle(nullptr)
    , m_toolSummaryLabel(nullptr)
    , m_toolInfoFrame(nullptr)
    , m_toolInfoLayout(nullptr)
{
    setupUI();
    setupConnections();
    setupContextMenu();
    // Ensure default tools exist before populating the list
    ensureDefaultToolsExist();
    populateToolList();
}

ToolManagementTab::~ToolManagementTab() = default;

void ToolManagementTab::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    createToolbar();
    createFilterPanel();
    
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);
    
    createToolListWidget();
    createToolDetailsPanel();
    createStatusPanel();
    
    // Apply modern styling
    setStyleSheet(R"(
        QWidget {
            background-color: #f5f5f5;
        }
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QPushButton {
            background-color: #0078d4;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #106ebe;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        QLineEdit, QComboBox {
            padding: 6px;
            border: 1px solid #cccccc;
            border-radius: 4px;
            background-color: white;
        }
        QLineEdit:focus, QComboBox:focus {
            border-color: #0078d4;
        }
        QTreeWidget {
            background-color: white;
            border: 1px solid #cccccc;
            border-radius: 4px;
        }
        QTreeWidget::item {
            padding: 4px;
        }
        QTreeWidget::item:selected {
            background-color: #0078d4;
            color: white;
        }
    )");
}

void ToolManagementTab::createToolbar() {
    m_toolbarFrame = new QFrame();
    m_toolbarFrame->setFrameStyle(QFrame::StyledPanel);
    m_toolbarLayout = new QHBoxLayout(m_toolbarFrame);
    
    m_addToolButton = new QPushButton("Add Tool");
    m_addToolButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    
    m_editToolButton = new QPushButton("Edit Tool");
    m_editToolButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_editToolButton->setEnabled(false);
    
    m_deleteToolButton = new QPushButton("Delete Tool");
    m_deleteToolButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    m_deleteToolButton->setEnabled(false);
    
    m_duplicateToolButton = new QPushButton("Duplicate");
    m_duplicateToolButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileLinkIcon));
    m_duplicateToolButton->setEnabled(false);
    
    m_moreActionsButton = new QToolButton();
    m_moreActionsButton->setText("More");
    m_moreActionsButton->setPopupMode(QToolButton::InstantPopup);
    
    m_moreActionsMenu = new QMenu(this);
    m_importLibraryAction = m_moreActionsMenu->addAction("Import Library...");
    m_exportLibraryAction = m_moreActionsMenu->addAction("Export Library...");
    m_moreActionsMenu->addSeparator();
    m_loadDefaultsAction = m_moreActionsMenu->addAction("Load Defaults");
    m_refreshAction = m_moreActionsMenu->addAction("Refresh");
    
    m_moreActionsButton->setMenu(m_moreActionsMenu);
    
    m_toolbarLayout->addWidget(m_addToolButton);
    m_toolbarLayout->addWidget(m_editToolButton);
    m_toolbarLayout->addWidget(m_deleteToolButton);
    m_toolbarLayout->addWidget(m_duplicateToolButton);
    m_toolbarLayout->addWidget(m_moreActionsButton);
    m_toolbarLayout->addStretch();
    
    m_mainLayout->addWidget(m_toolbarFrame);
}

void ToolManagementTab::createFilterPanel() {
    m_filterPanel = new QGroupBox("Filter Tools");
    m_filterLayout = new QHBoxLayout(m_filterPanel);
    
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search tools...");
    
    m_toolTypeFilter = new QComboBox();
    m_toolTypeFilter->addItem("All Types");
    m_toolTypeFilter->addItem("General Turning");
    m_toolTypeFilter->addItem("Threading");
    m_toolTypeFilter->addItem("Grooving");
    m_toolTypeFilter->addItem("Boring");
    m_toolTypeFilter->addItem("Parting");
    
    m_materialFilter = new QComboBox();
    m_materialFilter->addItem("All Materials");
    m_materialFilter->addItem("Uncoated Carbide");
    m_materialFilter->addItem("Coated Carbide");
    m_materialFilter->addItem("Cermet");
    m_materialFilter->addItem("Ceramic");
    m_materialFilter->addItem("CBN");
    m_materialFilter->addItem("PCD");
    
    m_statusFilter = new QComboBox();
    m_statusFilter->addItem("All Status");
    m_statusFilter->addItem("Active");
    m_statusFilter->addItem("Inactive");
    
    m_clearFiltersButton = new QPushButton("Clear");
    
    m_filterLayout->addWidget(new QLabel("Search:"));
    m_filterLayout->addWidget(m_searchBox);
    m_filterLayout->addWidget(new QLabel("Type:"));
    m_filterLayout->addWidget(m_toolTypeFilter);
    m_filterLayout->addWidget(new QLabel("Material:"));
    m_filterLayout->addWidget(m_materialFilter);
    m_filterLayout->addWidget(new QLabel("Status:"));
    m_filterLayout->addWidget(m_statusFilter);
    m_filterLayout->addWidget(m_clearFiltersButton);
    
    m_mainLayout->addWidget(m_filterPanel);
}

void ToolManagementTab::createToolListWidget() {
    m_toolListWidget = new QWidget();
    m_toolListWidget->setMinimumWidth(400);
    
    m_toolListLayout = new QVBoxLayout(m_toolListWidget);
    m_toolListLayout->addWidget(new QLabel("Tool Library"));
    
    m_toolTreeWidget = new QTreeWidget();
    m_toolTreeWidget->setHeaderLabels({
        "Name", "Type", "Tool #", "Position", 
        "Status", "Insert", "Holder", "Usage"
    });
    
    m_toolTreeWidget->header()->setStretchLastSection(false);
    m_toolTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    m_toolListLayout->addWidget(m_toolTreeWidget);
    m_mainSplitter->addWidget(m_toolListWidget);
}

void ToolManagementTab::createToolDetailsPanel() {
    m_toolDetailsPanel = new QWidget();
    m_toolDetailsPanel->setMinimumWidth(300);
    
    m_toolDetailsLayout = new QVBoxLayout(m_toolDetailsPanel);
    
    m_toolDetailsTitle = new QLabel("Tool Details");
    m_toolDetailsTitle->setStyleSheet("font-weight: bold; font-size: 14pt;");
    m_toolDetailsLayout->addWidget(m_toolDetailsTitle);
    
    m_toolSummaryLabel = new QLabel("Select a tool to view details");
    m_toolSummaryLabel->setWordWrap(true);
    m_toolDetailsLayout->addWidget(m_toolSummaryLabel);
    
    m_toolInfoFrame = new QFrame();
    m_toolInfoFrame->setFrameStyle(QFrame::StyledPanel);
    m_toolInfoLayout = new QGridLayout(m_toolInfoFrame);
    
    // Create tool information labels
    m_toolTypeLabel = new QLabel("Type:");
    m_toolTypeValue = new QLabel("-");
    m_toolNameLabel = new QLabel("Name:");
    m_toolNameValue = new QLabel("-");
    m_toolNumberLabel = new QLabel("Tool #:");
    m_toolNumberValue = new QLabel("-");
    m_turretPositionLabel = new QLabel("Position:");
    m_turretPositionValue = new QLabel("-");
    m_toolStatusLabel = new QLabel("Status:");
    m_toolStatusValue = new QLabel("-");
    m_insertInfoLabel = new QLabel("Insert:");
    m_insertInfoValue = new QLabel("-");
    m_holderInfoLabel = new QLabel("Holder:");
    m_holderInfoValue = new QLabel("-");
    m_cuttingDataLabel = new QLabel("Cutting Data:");
    m_cuttingDataValue = new QLabel("-");
    m_toolLifeLabel = new QLabel("Tool Life:");
    m_toolLifeValue = new QLabel("-");
    m_lastUsedLabel = new QLabel("Last Used:");
    m_lastUsedValue = new QLabel("-");
    
    int row = 0;
    m_toolInfoLayout->addWidget(m_toolTypeLabel, row, 0);
    m_toolInfoLayout->addWidget(m_toolTypeValue, row++, 1);
    m_toolInfoLayout->addWidget(m_toolNameLabel, row, 0);
    m_toolInfoLayout->addWidget(m_toolNameValue, row++, 1);
    m_toolInfoLayout->addWidget(m_toolNumberLabel, row, 0);
    m_toolInfoLayout->addWidget(m_toolNumberValue, row++, 1);
    m_toolInfoLayout->addWidget(m_turretPositionLabel, row, 0);
    m_toolInfoLayout->addWidget(m_turretPositionValue, row++, 1);
    m_toolInfoLayout->addWidget(m_toolStatusLabel, row, 0);
    m_toolInfoLayout->addWidget(m_toolStatusValue, row++, 1);
    m_toolInfoLayout->addWidget(m_insertInfoLabel, row, 0);
    m_toolInfoLayout->addWidget(m_insertInfoValue, row++, 1);
    m_toolInfoLayout->addWidget(m_holderInfoLabel, row, 0);
    m_toolInfoLayout->addWidget(m_holderInfoValue, row++, 1);
    m_toolInfoLayout->addWidget(m_cuttingDataLabel, row, 0);
    m_toolInfoLayout->addWidget(m_cuttingDataValue, row++, 1);
    m_toolInfoLayout->addWidget(m_toolLifeLabel, row, 0);
    m_toolInfoLayout->addWidget(m_toolLifeValue, row++, 1);
    m_toolInfoLayout->addWidget(m_lastUsedLabel, row, 0);
    m_toolInfoLayout->addWidget(m_lastUsedValue, row++, 1);
    
    m_toolDetailsLayout->addWidget(m_toolInfoFrame);
    m_toolDetailsLayout->addStretch();
    
    m_mainSplitter->addWidget(m_toolDetailsPanel);
}

void ToolManagementTab::createStatusPanel() {
    // Status panel implementation
    auto statusFrame = new QFrame();
    statusFrame->setFrameStyle(QFrame::StyledPanel);
    statusFrame->setMaximumHeight(30);
    
    auto statusLayout = new QHBoxLayout(statusFrame);
    auto statusLabel = new QLabel("Ready");
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    m_mainLayout->addWidget(statusFrame);
}

void ToolManagementTab::setupConnections() {
    // Toolbar connections
    connect(m_addToolButton, &QPushButton::clicked,
            this, &ToolManagementTab::onAddToolTriggered);
    connect(m_editToolButton, &QPushButton::clicked,
            this, &ToolManagementTab::onEditToolTriggered);
    connect(m_deleteToolButton, &QPushButton::clicked,
            this, &ToolManagementTab::onDeleteToolTriggered);
    connect(m_duplicateToolButton, &QPushButton::clicked,
            this, &ToolManagementTab::onDuplicateToolTriggered);
    
    // Menu connections
    connect(m_importLibraryAction, &QAction::triggered,
            this, &ToolManagementTab::onImportLibraryTriggered);
    connect(m_exportLibraryAction, &QAction::triggered,
            this, &ToolManagementTab::onExportLibraryTriggered);
    connect(m_loadDefaultsAction, &QAction::triggered,
            this, &ToolManagementTab::onLoadDefaultsTriggered);
    connect(m_refreshAction, &QAction::triggered,
            this, &ToolManagementTab::onRefreshRequested);
    
    // Filter connections
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &ToolManagementTab::onSearchTextChanged);
    connect(m_toolTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementTab::onFilterChanged);
    connect(m_materialFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementTab::onFilterChanged);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolManagementTab::onFilterChanged);
    connect(m_clearFiltersButton, &QPushButton::clicked,
            this, [this]() {
                m_searchBox->clear();
                m_toolTypeFilter->setCurrentIndex(0);
                m_materialFilter->setCurrentIndex(0);
                m_statusFilter->setCurrentIndex(0);
                clearFilters();
            });
    
    // Tool list connections
    connect(m_toolTreeWidget, &QTreeWidget::currentItemChanged,
            this, &ToolManagementTab::onToolListSelectionChanged);
    connect(m_toolTreeWidget, &QTreeWidget::itemDoubleClicked,
            this, &ToolManagementTab::onToolListDoubleClicked);
    connect(m_toolTreeWidget, &QTreeWidget::customContextMenuRequested,
            this, &ToolManagementTab::onToolListContextMenuRequested);
    
    m_toolTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void ToolManagementTab::setupContextMenu() {
    // Context menu will be created dynamically in onToolListContextMenuRequested
    m_toolTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void ToolManagementTab::populateToolList() {
    m_toolTreeWidget->clear();
    
    // Load tools from the tool assembly database
    QString databasePath = getToolAssemblyDatabasePath();
    QFile file(databasePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Tool assembly database not found, creating default tools:" << databasePath;
        
        // Automatically create default tools when no database exists
        createDefaultToolDatabase();
        
        // Try to open the newly created database
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to create or open tool assembly database:" << databasePath;
            return;
        }
    }
    
    // Read and parse the tool database
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse tool assembly database:" << error.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "Tool assembly database is not a valid JSON object";
        return;
    }
    
    QJsonObject database = doc.object();
    if (!database.contains("tools")) {
        qDebug() << "No tools found in database";
        return;
    }
    
    QJsonArray toolsArray = database["tools"].toArray();
    
    // Populate tree widget with tools from database
    for (const QJsonValue& toolValue : toolsArray) {
        QJsonObject toolObj = toolValue.toObject();
        
        auto item = new QTreeWidgetItem(m_toolTreeWidget);
        item->setText(COL_NAME, toolObj["name"].toString());
        item->setData(COL_NAME, Qt::UserRole, toolObj.value("id").toString());
        item->setText(COL_TYPE, formatToolType(static_cast<IntuiCAM::Toolpath::ToolType>(toolObj["toolType"].toInt())));
        item->setText(COL_TOOL_NUMBER, toolObj["toolNumber"].toString());
        item->setText(COL_TURRET_POS, QString::number(toolObj["turretPosition"].toInt()));
        item->setText(COL_STATUS, toolObj["isActive"].toBool() ? "Active" : "Inactive");
        
        // Extract insert type from the tool data
        QString insertType = "Unknown";
        if (toolObj.contains("turningInsert")) {
            QJsonObject insertObj = toolObj["turningInsert"].toObject();
            insertType = insertObj["isoCode"].toString();
            if (insertType.isEmpty()) {
                insertType = QString("General Turning Insert");
            }
        } else if (toolObj.contains("threadingInsert")) {
            QJsonObject insertObj = toolObj["threadingInsert"].toObject();
            insertType = insertObj["isoCode"].toString();
            if (insertType.isEmpty()) {
                insertType = QString("Threading Insert");
            }
        } else if (toolObj.contains("groovingInsert")) {
            QJsonObject insertObj = toolObj["groovingInsert"].toObject();
            insertType = insertObj["isoCode"].toString();
            if (insertType.isEmpty()) {
                insertType = QString("Grooving Insert");
            }
        }
        item->setText(COL_INSERT_TYPE, insertType);
        
        // Extract holder type from the tool data
        QString holderType = "Unknown";
        if (toolObj.contains("holder")) {
            QJsonObject holderObj = toolObj["holder"].toObject();
            holderType = holderObj["isoCode"].toString();
            if (holderType.isEmpty()) {
                holderType = QString("Tool Holder");
            }
        }
        item->setText(COL_HOLDER_TYPE, holderType);
        
        // Format usage (for now, showing placeholder)
        double usageMinutes = toolObj["usageMinutes"].toDouble(0.0);
        double expectedLifeMinutes = toolObj["expectedLifeMinutes"].toDouble(480.0);
        QString usage = QString("%1/%2 min").arg(usageMinutes, 0, 'f', 0).arg(expectedLifeMinutes, 0, 'f', 0);
        item->setText(COL_USAGE, usage);
        
        // Set visual appearance based on status
        if (item->text(COL_STATUS) == "Active") {
            item->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
        } else {
            item->setForeground(COL_STATUS, QBrush(getToolStatusColor(false)));
        }
    }
    
    qDebug() << "Loaded" << toolsArray.size() << "tools from database";
    
    // Resize columns to fit content
    for (int i = 0; i < m_toolTreeWidget->columnCount(); ++i) {
        m_toolTreeWidget->resizeColumnToContents(i);
    }
}

// Slot implementations
void ToolManagementTab::onToolListSelectionChanged() {
    auto currentItem = m_toolTreeWidget->currentItem();
    bool hasSelection = (currentItem != nullptr);
    
    m_editToolButton->setEnabled(hasSelection);
    m_deleteToolButton->setEnabled(hasSelection);
    m_duplicateToolButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString toolId = currentItem->data(COL_NAME, Qt::UserRole).toString();
        displayToolInfo(toolId);
        emit toolSelected(toolId);
    } else {
        clearToolInfo();
    }
}

void ToolManagementTab::onToolListDoubleClicked() {
    auto currentItem = m_toolTreeWidget->currentItem();
    if (currentItem) {
        QString toolId = currentItem->data(COL_NAME, Qt::UserRole).toString();
        emit toolDoubleClicked(toolId);
    }
}

void ToolManagementTab::onToolListContextMenuRequested(const QPoint& pos) {
    auto item = m_toolTreeWidget->itemAt(pos);
    if (item) {
        // Ensure the clicked item becomes the current selection so that
        // subsequent actions operate on the correct tool id
        m_toolTreeWidget->setCurrentItem(item);
        QString toolId = item->data(COL_NAME, Qt::UserRole).toString();
        bool isActive = (item->text(COL_STATUS) == "Active");
        
        // Create context menu
        QMenu contextMenu(this);
        
        QAction* editAction = contextMenu.addAction(QIcon(), "Edit Tool...");
        connect(editAction, &QAction::triggered, this, &ToolManagementTab::onEditToolAction);
        
        QAction* duplicateAction = contextMenu.addAction(QIcon(), "Duplicate Tool");
        connect(duplicateAction, &QAction::triggered, this, &ToolManagementTab::onDuplicateToolAction);
        
        contextMenu.addSeparator();
        
        QAction* propertiesAction = contextMenu.addAction(QIcon(), "Properties...");
        connect(propertiesAction, &QAction::triggered, this, &ToolManagementTab::onToolPropertiesAction);
        
        contextMenu.addSeparator();
        
        if (isActive) {
            QAction* setInactiveAction = contextMenu.addAction(QIcon(), "Set Inactive");
            connect(setInactiveAction, &QAction::triggered, this, &ToolManagementTab::onSetInactiveAction);
        } else {
            QAction* setActiveAction = contextMenu.addAction(QIcon(), "Set Active");
            connect(setActiveAction, &QAction::triggered, this, &ToolManagementTab::onSetActiveAction);
        }
        
        contextMenu.addSeparator();
        
        QAction* deleteAction = contextMenu.addAction(QIcon(), "Delete Tool");
        deleteAction->setShortcut(QKeySequence::Delete);
        connect(deleteAction, &QAction::triggered, this, &ToolManagementTab::onDeleteToolAction);
        
        // Show context menu
        contextMenu.exec(m_toolTreeWidget->mapToGlobal(pos));
        
        emit toolContextMenuRequested(toolId, m_toolTreeWidget->mapToGlobal(pos));
    }
}

void ToolManagementTab::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    applyFilters();
}

void ToolManagementTab::onFilterChanged() {
    applyFilters();
}

void ToolManagementTab::onRefreshRequested() {
    refreshToolList();
}

void ToolManagementTab::onAddToolTriggered() {
    addNewTool();
}

void ToolManagementTab::onEditToolTriggered() {
    editSelectedTool();
}

void ToolManagementTab::onDeleteToolTriggered() {
    deleteSelectedTool();
}

void ToolManagementTab::onDuplicateToolTriggered() {
    duplicateSelectedTool();
}

void ToolManagementTab::onImportLibraryTriggered() {
    importToolLibrary();
}

void ToolManagementTab::onExportLibraryTriggered() {
    exportToolLibrary();
}

void ToolManagementTab::onLoadDefaultsTriggered() {
    loadDefaultTools();
}

// Public interface implementations
void ToolManagementTab::refreshToolList() {
    populateToolList();
    updateToolCounts();
    updateStatusBar();
}

void ToolManagementTab::selectTool(const QString& toolId) {
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_toolTreeWidget->topLevelItem(i);
        if (item->data(COL_NAME, Qt::UserRole).toString() == toolId) {
            m_toolTreeWidget->setCurrentItem(item);
            break;
        }
    }
}

void ToolManagementTab::addNewTool() {
    // Clean up any existing tools with empty IDs before adding new tool
    cleanupEmptyIdTools();
    
    auto dialog = new ToolManagementDialog(IntuiCAM::Toolpath::ToolType::GENERAL_TURNING, this);
    
    // Get MaterialManager from parent and set it on the dialog
    auto mainWindow = qobject_cast<MainWindow*>(window());
    if (mainWindow && mainWindow->getMaterialManager()) {
        dialog->setMaterialManager(mainWindow->getMaterialManager());
    }
    
    // Connect signals to handle new tool creation
    connect(dialog, &ToolManagementDialog::toolSaved,
            this, [this](const QString& toolId) {
                qDebug() << "Tool saved signal received for:" << toolId;
                
                // Check for empty ID
                if (toolId.isEmpty()) {
                    qWarning() << "Received empty tool ID from save operation";
                    QMessageBox::warning(this, "Save Error", 
                        "Tool was saved with an empty ID. This tool may not be manageable. Please delete it and create a new one.");
                    refreshToolList();
                    return;
                }
                
                // Verify the tool was actually added to the database
                if (verifyToolInDatabase(toolId)) {
                    refreshToolList();
                    selectTool(toolId); // Select the newly added tool
                    emit toolAdded(toolId);
                } else {
                    qWarning() << "Tool was not properly saved to database:" << toolId;
                    QMessageBox::warning(this, "Save Error", 
                        QString("Tool '%1' was not properly saved to the database. Please try again.").arg(toolId));
                }
            });
    connect(dialog, &ToolManagementDialog::toolNameChanged,
            this, [this](const QString& id, const QString& name) {
                for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
                    auto item = m_toolTreeWidget->topLevelItem(i);
                    if (item->data(COL_NAME, Qt::UserRole).toString() == id) {
                        item->setText(COL_NAME, name);
                        break;
                    }
                }
            });
    
    // Connect error handling
    connect(dialog, &ToolManagementDialog::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Tool Management Error", error);
                emit errorOccurred(error);
            });
    
    // Show dialog and handle result
    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        qDebug() << "Tool dialog accepted, refreshing tool list";
        refreshToolList();
        updateToolCounts();
        emit toolLibraryChanged();
    }
    
    dialog->deleteLater();
}

void ToolManagementTab::cleanupEmptyIdTools() {
    QString databasePath = getToolAssemblyDatabasePath();
    QFile dbFile(databasePath);
    
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open database for cleanup";
        return;
    }

    QByteArray data = dbFile.readAll();
    dbFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "Cannot parse database for cleanup";
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray toolsArray = rootObj["tools"].toArray();
    QJsonArray cleanedArray;
    bool foundEmptyIds = false;
    
    // Remove tools with empty IDs and assign new IDs to any that should be kept
    for (int i = 0; i < toolsArray.size(); ++i) {
        QJsonObject toolObj = toolsArray[i].toObject();
        QString currentId = toolObj["id"].toString();
        
        if (currentId.isEmpty()) {
            foundEmptyIds = true;
            qDebug() << "Found tool with empty ID, removing:" << toolObj["name"].toString();
            // Don't add to cleaned array (effectively deleting it)
        } else {
            cleanedArray.append(toolObj);
        }
    }
    
    if (foundEmptyIds) {
        // Update the database
        rootObj["tools"] = cleanedArray;
        rootObj["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        rootObj["toolCount"] = cleanedArray.size();
        
        if (dbFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonDocument newDoc(rootObj);
            dbFile.write(newDoc.toJson());
            dbFile.close();
            qDebug() << "Cleaned up tools with empty IDs from database";
        }
    }
}

bool ToolManagementTab::verifyToolInDatabase(const QString& toolId) {
    QString databasePath = getToolAssemblyDatabasePath();
    QFile dbFile(databasePath);
    
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open database for verification:" << databasePath;
        return false;
    }

    QByteArray data = dbFile.readAll();
    dbFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse database JSON during verification:" << error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Database is not a valid JSON object during verification";
        return false;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray toolsArray = rootObj["tools"].toArray();
    
    // Check if the tool exists in the database
    for (const QJsonValue& toolValue : toolsArray) {
        QJsonObject toolObj = toolValue.toObject();
        if (toolObj["id"].toString() == toolId) {
            qDebug() << "Verified tool exists in database:" << toolId;
            return true;
        }
    }

    qWarning() << "Tool not found in database during verification:" << toolId;
    return false;
}

void ToolManagementTab::editSelectedTool() {
    QString toolId = getSelectedToolId();
    
    // Handle special case of tools with empty IDs
    if (toolId.isEmpty()) {
        auto selectedItems = m_toolTreeWidget->selectedItems();
        if (!selectedItems.isEmpty()) {
            auto item = selectedItems.first();
            QString toolName = item->text(COL_NAME);
            
            auto reply = QMessageBox::question(this, "Edit Tool with Empty ID",
                QString("Tool '%1' has an empty ID and cannot be edited safely.\n\n"
                       "Would you like to delete this tool instead?\n\n"
                       "You can then create a new tool with proper settings.").arg(toolName),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                deleteSelectedTool();
            }
            return;
        } else {
            QMessageBox::information(this, "No Tool Selected", 
                "Please select a tool to edit from the list.");
            return;
        }
    }
    
    auto dialog = new ToolManagementDialog(toolId, this);
    
    // Get MaterialManager from parent and set it on the dialog
    auto mainWindow = qobject_cast<MainWindow*>(window());
    if (mainWindow && mainWindow->getMaterialManager()) {
        dialog->setMaterialManager(mainWindow->getMaterialManager());
    }
    
    // Connect signals
    connect(dialog, &ToolManagementDialog::toolSaved,
            this, [this](const QString& modifiedToolId) {
                qDebug() << "Tool saved signal received for:" << modifiedToolId;
                refreshToolList();
                selectTool(modifiedToolId); // Re-select the modified tool
                emit toolModified(modifiedToolId);
            });
    connect(dialog, &ToolManagementDialog::toolNameChanged,
            this, [this](const QString& id, const QString& name) {
                for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
                    auto item = m_toolTreeWidget->topLevelItem(i);
                    if (item->data(COL_NAME, Qt::UserRole).toString() == id) {
                        item->setText(COL_NAME, name);
                        break;
                    }
                }
            });
    
    // Connect error handling
    connect(dialog, &ToolManagementDialog::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Tool Management Error", error);
                emit errorOccurred(error);
            });
    
    // Show dialog and handle result
    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        qDebug() << "Tool edit dialog accepted, refreshing tool list";
        refreshToolList();
        updateToolCounts();
        emit toolLibraryChanged();
    }
    
    dialog->deleteLater();
}

void ToolManagementTab::deleteSelectedTool() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        auto reply = QMessageBox::question(this, "Delete Tool",
            QString("Are you sure you want to delete tool '%1'?\n\nThis action cannot be undone.").arg(toolId),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            bool deletionSuccessful = false;
            
            // Remove tool from the persistent database first
            QString databasePath = getToolAssemblyDatabasePath();
            if (deleteToolFromDatabase(toolId, databasePath)) {
                // Only remove from UI if database deletion was successful
                auto selectedItems = m_toolTreeWidget->selectedItems();
                if (!selectedItems.isEmpty()) {
                    auto item = selectedItems.first();
                    int index = m_toolTreeWidget->indexOfTopLevelItem(item);
                    if (index >= 0) {
                        delete m_toolTreeWidget->takeTopLevelItem(index);
                        deletionSuccessful = true;
                    }
                }
            }

            if (deletionSuccessful) {
                // Clear tool details display
                clearToolInfo();
                
                // Update button states
                m_editToolButton->setEnabled(false);
                m_deleteToolButton->setEnabled(false);
                m_duplicateToolButton->setEnabled(false);
                
                emit toolDeleted(toolId);
                updateToolCounts();
                emit toolLibraryChanged();
                
                // Show success message
                QMessageBox::information(this, "Tool Deleted", 
                    QString("Tool '%1' has been successfully deleted from the library.").arg(toolId));
            } else {
                QMessageBox::warning(this, "Deletion Failed", 
                    QString("Failed to delete tool '%1' from the database. Please try again.").arg(toolId));
            }
        }
    }
}

bool ToolManagementTab::deleteToolFromDatabase(const QString& toolId, const QString& databasePath) {
    QFile dbFile(databasePath);
    if (!dbFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open database for reading:" << databasePath;
        return false;
    }

    QByteArray data = dbFile.readAll();
    dbFile.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse database JSON:" << error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Database is not a valid JSON object";
        return false;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray toolsArray = rootObj["tools"].toArray();
    QJsonArray newArray;
    bool toolFound = false;
    int deletedToolIndex = -1;
    
    // Filter out the tool to be deleted
    for (int i = 0; i < toolsArray.size(); ++i) {
        QJsonObject obj = toolsArray[i].toObject();
        QString currentId = obj["id"].toString();
        
        // Handle different deletion cases
        bool shouldDelete = false;
        if (toolId.isEmpty() && currentId.isEmpty()) {
            // Both are empty - delete the first empty ID tool found
            shouldDelete = true;
        } else if (!toolId.isEmpty() && currentId == toolId) {
            // Normal case - exact ID match
            shouldDelete = true;
        }
        
        if (shouldDelete && !toolFound) {
            toolFound = true;
            deletedToolIndex = i;
            qDebug() << "Found tool to delete at index" << i << "with ID:" << (currentId.isEmpty() ? "<empty>" : currentId);
            // Don't add this tool to the new array (effectively deleting it)
        } else {
            newArray.append(obj);
        }
    }

    if (!toolFound) {
        qWarning() << "Tool not found in database:" << (toolId.isEmpty() ? "<empty ID>" : toolId);
        
        // If looking for empty ID tool, try to find and delete any tool with empty ID
        if (toolId.isEmpty()) {
            for (int i = 0; i < toolsArray.size(); ++i) {
                QJsonObject obj = toolsArray[i].toObject();
                if (obj["id"].toString().isEmpty()) {
                    // Found a tool with empty ID, delete it
                    newArray = QJsonArray(); // Reset the array
                    for (int j = 0; j < toolsArray.size(); ++j) {
                        if (j != i) {
                            newArray.append(toolsArray[j]);
                        }
                    }
                    toolFound = true;
                    qDebug() << "Deleted tool with empty ID at index:" << i;
                    break;
                }
            }
        }
        
        if (!toolFound) {
            return false;
        }
    }

    // Update the database with the new array
    rootObj["tools"] = newArray;
    rootObj["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj["toolCount"] = newArray.size();
    
    // Write back to database
    if (!dbFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open database for writing:" << databasePath;
        return false;
    }

    QJsonDocument newDoc(rootObj);
    qint64 bytesWritten = dbFile.write(newDoc.toJson());
    dbFile.close();

    if (bytesWritten == -1) {
        qWarning() << "Failed to write to database";
        return false;
    }

    qDebug() << "Successfully deleted tool from database:" << (toolId.isEmpty() ? "<empty ID>" : toolId);
    qDebug() << "Database now contains" << newArray.size() << "tools";
    return true;
}

void ToolManagementTab::duplicateSelectedTool() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        // Find the original tool item
        QTreeWidgetItem* originalItem = nullptr;
        for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
            auto item = m_toolTreeWidget->topLevelItem(i);
            if (item->data(COL_NAME, Qt::UserRole).toString() == toolId) {
                originalItem = item;
                break;
            }
        }
        
        if (originalItem) {
            // Create duplicate with modified name
            QTreeWidgetItem* duplicateItem = new QTreeWidgetItem(m_toolTreeWidget);
            duplicateItem->setText(COL_NAME, originalItem->text(COL_NAME) + " Copy");
            duplicateItem->setData(COL_NAME, Qt::UserRole, toolId + "_copy");
            duplicateItem->setText(COL_TYPE, originalItem->text(COL_TYPE));
            duplicateItem->setText(COL_TOOL_NUMBER, "T99"); // Temporary tool number
            duplicateItem->setText(COL_TURRET_POS, "99");
            duplicateItem->setText(COL_STATUS, "Inactive"); // Start as inactive
            duplicateItem->setText(COL_INSERT_TYPE, originalItem->text(COL_INSERT_TYPE));
            duplicateItem->setText(COL_HOLDER_TYPE, originalItem->text(COL_HOLDER_TYPE));
            duplicateItem->setText(COL_USAGE, "0/480 min"); // Reset usage
            
            // Select the new item
            m_toolTreeWidget->setCurrentItem(duplicateItem);
            
            emit toolAdded(duplicateItem->data(COL_NAME, Qt::UserRole).toString());
            updateToolCounts();
            emit toolLibraryChanged();
            
            QMessageBox::information(this, "Tool Duplicated", 
                QString("Tool '%1' has been duplicated. Please edit the tool number and turret position.").arg(toolId));
        }
    }
}

void ToolManagementTab::importToolLibrary() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Tool Library", "", "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonArray toolsArray = rootObj["tools"].toArray();
                
                int importedCount = 0;
                for (const QJsonValue& toolValue : toolsArray) {
                    QJsonObject toolObj = toolValue.toObject();
                    
                    // Validate required fields
                    if (!toolObj.contains("name") || !toolObj.contains("type")) {
                        continue; // Skip invalid tools
                    }
                    
                    // Create new tree widget item
                    QTreeWidgetItem* item = new QTreeWidgetItem(m_toolTreeWidget);
                    item->setText(COL_NAME, toolObj["name"].toString());
                    item->setData(COL_NAME, Qt::UserRole, toolObj.value("id").toString());
                    item->setText(COL_TYPE, toolObj["type"].toString());
                    item->setText(COL_TOOL_NUMBER, toolObj["toolNumber"].toString("T00"));
                    item->setText(COL_TURRET_POS, QString::number(toolObj["turretPosition"].toInt(99)));
                    item->setText(COL_STATUS, toolObj["isActive"].toBool() ? "Active" : "Inactive");
                    item->setText(COL_INSERT_TYPE, toolObj["insertType"].toString("Unknown"));
                    item->setText(COL_HOLDER_TYPE, toolObj["holderType"].toString("Unknown"));
                    item->setText(COL_USAGE, toolObj["usage"].toString("0/480 min"));
                    
                    // Set visual appearance based on status
                    if (item->text(COL_STATUS) == "Active") {
                        item->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
                    } else {
                        item->setForeground(COL_STATUS, QBrush(getToolStatusColor(false)));
                    }
                    
                    importedCount++;
                }
                
                // Resize columns to fit content
                for (int i = 0; i < m_toolTreeWidget->columnCount(); ++i) {
                    m_toolTreeWidget->resizeColumnToContents(i);
                }
                
                QMessageBox::information(this, "Import Complete",
                    QString("Successfully imported %1 tools from library.").arg(importedCount));
                
                updateToolCounts();
                emit toolLibraryChanged();
            } else {
                QMessageBox::warning(this, "Import Error", "Invalid JSON format in tool library file.");
            }
        } else {
            QMessageBox::warning(this, "Import Error", "Could not open tool library file for reading.");
        }
    }
}

void ToolManagementTab::exportToolLibrary() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Tool Library", "tool_library.json", "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QJsonObject rootObj;
        QJsonArray toolsArray;
        
        // Export all tools from the tree widget
        for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = m_toolTreeWidget->topLevelItem(i);
            
            QJsonObject toolObj;
            toolObj["id"] = item->data(COL_NAME, Qt::UserRole).toString();
            toolObj["name"] = item->text(COL_NAME);
            toolObj["type"] = item->text(COL_TYPE);
            toolObj["toolNumber"] = item->text(COL_TOOL_NUMBER);
            toolObj["turretPosition"] = item->text(COL_TURRET_POS).toInt();
            toolObj["isActive"] = (item->text(COL_STATUS) == "Active");
            toolObj["insertType"] = item->text(COL_INSERT_TYPE);
            toolObj["holderType"] = item->text(COL_HOLDER_TYPE);
            toolObj["usage"] = item->text(COL_USAGE);
            
            // Add metadata
            toolObj["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            toolObj["version"] = "1.0";
            
            toolsArray.append(toolObj);
        }
        
        rootObj["tools"] = toolsArray;
        rootObj["version"] = "1.0";
        rootObj["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        rootObj["exportedBy"] = "IntuiCAM Tool Management System";
        rootObj["toolCount"] = toolsArray.size();
        
        QJsonDocument doc(rootObj);
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            QMessageBox::information(this, "Export Complete",
                QString("Successfully exported %1 tools to library.").arg(toolsArray.size()));
        } else {
            QMessageBox::warning(this, "Export Error", "Could not open file for writing.");
        }
    }
}

void ToolManagementTab::loadDefaultTools() {
    auto reply = QMessageBox::question(this, "Load Default Tools",
        "This will replace your current tool library with 9 professional lathe tools. Continue?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Create 9 comprehensive default tools with realistic parameters for all materials
        QJsonArray toolsArray;
        
        // Tool 1: Right-hand tool: internal metric threading
        QJsonObject tool1;
        tool1["id"] = "RH_INTERNAL_THREADING_T01";
        tool1["name"] = "Right-Hand Internal Threading";
        tool1["manufacturer"] = "Iscar";
        tool1["toolType"] = static_cast<int>(ToolType::THREADING);
        tool1["toolNumber"] = "T01";
        tool1["turretPosition"] = 1;
        tool1["isActive"] = true;
        tool1["toolOffset_X"] = 0.0;
        tool1["toolOffset_Z"] = 0.0;
        tool1["toolLengthOffset"] = 0.0;
        tool1["toolRadiusOffset"] = 0.0;
        tool1["expectedLifeMinutes"] = 240.0;
        tool1["usageMinutes"] = 0.0;
        tool1["cycleCount"] = 0;
        tool1["notes"] = "Right-hand internal metric threading tool for M8-M20 threads";
        tool1["internalThreading"] = true;
        tool1["externalThreading"] = false;
        tool1["internalBoring"] = false;
        tool1["partingGrooving"] = false;
        tool1["longitudinalTurning"] = false;
        tool1["facing"] = false;
        tool1["chamfering"] = false;
        
        // Threading Insert
        QJsonObject threadingInsert1;
        threadingInsert1["isoCode"] = "16IR1.0ISO";
        threadingInsert1["thickness"] = 3.18;
        threadingInsert1["width"] = 6.0;
        threadingInsert1["minThreadPitch"] = 0.75;
        threadingInsert1["maxThreadPitch"] = 2.5;
        threadingInsert1["internalThreads"] = true;
        threadingInsert1["externalThreads"] = false;
        threadingInsert1["threadProfile"] = static_cast<int>(ThreadProfile::METRIC);
        threadingInsert1["threadProfileAngle"] = 60.0;
        threadingInsert1["threadTipType"] = static_cast<int>(ThreadTipType::SHARP_POINT);
        threadingInsert1["threadTipRadius"] = 0.0;
        threadingInsert1["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        threadingInsert1["manufacturer"] = "Iscar";
        threadingInsert1["name"] = "16IR1.0ISO Internal Threading";
        threadingInsert1["isActive"] = true;
        tool1["threadingInsert"] = threadingInsert1;
        
        // Tool Holder
        QJsonObject holder1;
        holder1["isoCode"] = "SIR2020K16";
        holder1["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder1["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
        holder1["cuttingWidth"] = 16.0;
        holder1["headLength"] = 40.0;
        holder1["overallLength"] = 150.0;
        holder1["shankWidth"] = 20.0;
        holder1["shankHeight"] = 20.0;
        holder1["isRoundShank"] = false;
        holder1["insertSeatAngle"] = 90.0;
        holder1["insertSetback"] = 1.5;
        holder1["manufacturer"] = "Iscar";
        tool1["holder"] = holder1;
        
        // Cutting Data with material-specific settings
        QJsonObject cuttingData1;
        cuttingData1["constantSurfaceSpeed"] = true;
        cuttingData1["surfaceSpeed"] = 80.0;
        cuttingData1["spindleSpeed"] = 300;
        cuttingData1["feedPerRevolution"] = true;
        cuttingData1["cuttingFeedrate"] = 1.25; // pitch dependent
        cuttingData1["plungeFeedrate"] = 0.03;
        cuttingData1["retractFeedrate"] = 1.5;
        cuttingData1["maxDepthOfCut"] = 0.4;
        cuttingData1["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool1["cuttingData"] = cuttingData1;
        
        toolsArray.append(tool1);
        
        // Tool 2: Right-hand tool: internal boring
        QJsonObject tool2;
        tool2["id"] = "RH_INTERNAL_BORING_T02";
        tool2["name"] = "Right-Hand Internal Boring";
        tool2["manufacturer"] = "Kennametal";
        tool2["toolType"] = static_cast<int>(ToolType::BORING);
        tool2["toolNumber"] = "T02";
        tool2["turretPosition"] = 2;
        tool2["isActive"] = true;
        tool2["toolOffset_X"] = 0.0;
        tool2["toolOffset_Z"] = 0.0;
        tool2["toolLengthOffset"] = 0.0;
        tool2["toolRadiusOffset"] = 0.8;
        tool2["expectedLifeMinutes"] = 360.0;
        tool2["usageMinutes"] = 0.0;
        tool2["cycleCount"] = 0;
        tool2["notes"] = "Right-hand internal boring tool, min bore 15mm";
        tool2["internalThreading"] = false;
        tool2["externalThreading"] = false;
        tool2["internalBoring"] = true;
        tool2["partingGrooving"] = false;
        tool2["longitudinalTurning"] = false;
        tool2["facing"] = false;
        tool2["chamfering"] = false;
        
        // General Turning Insert (used for boring)
        QJsonObject turningInsert2;
        turningInsert2["isoCode"] = "CCMT09T304";
        turningInsert2["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert2["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert2["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert2["sizeSpecifier"] = "09";
        turningInsert2["inscribedCircle"] = 9.525;
        turningInsert2["thickness"] = 3.97;
        turningInsert2["cornerRadius"] = 0.4;
        turningInsert2["cuttingEdgeLength"] = 9.525;
        turningInsert2["width"] = 9.525;
        turningInsert2["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert2["substrate"] = "WC-Co";
        turningInsert2["coating"] = "TiAlN";
        turningInsert2["manufacturer"] = "Kennametal";
        turningInsert2["partNumber"] = "CCMT 09 T3 04-HP KC5010";
        turningInsert2["rake_angle"] = 0.0;
        turningInsert2["inclination_angle"] = -6.0;
        turningInsert2["name"] = "CCMT09T304 Boring Insert";
        turningInsert2["isActive"] = true;
        tool2["turningInsert"] = turningInsert2;
        
        // Tool Holder (Boring Bar)
        QJsonObject holder2;
        holder2["isoCode"] = "S16R-SCLCR09";
        holder2["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder2["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
        holder2["cuttingWidth"] = 9.0;
        holder2["headLength"] = 120.0;
        holder2["overallLength"] = 200.0;
        holder2["shankWidth"] = 16.0;
        holder2["shankHeight"] = 16.0;
        holder2["isRoundShank"] = true;
        holder2["shankDiameter"] = 16.0;
        holder2["insertSeatAngle"] = 95.0;
        holder2["insertSetback"] = 1.0;
        holder2["manufacturer"] = "Kennametal";
        tool2["holder"] = holder2;
        
        // Cutting Data
        QJsonObject cuttingData2;
        cuttingData2["constantSurfaceSpeed"] = true;
        cuttingData2["surfaceSpeed"] = 180.0;
        cuttingData2["spindleSpeed"] = 600;
        cuttingData2["feedPerRevolution"] = true;
        cuttingData2["cuttingFeedrate"] = 0.15;
        cuttingData2["plungeFeedrate"] = 0.05;
        cuttingData2["retractFeedrate"] = 3.0;
        cuttingData2["maxDepthOfCut"] = 1.5;
        cuttingData2["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool2["cuttingData"] = cuttingData2;
        
        toolsArray.append(tool2);
        
        // Tool 3: Right-hand tool: parting / grooving
        QJsonObject tool3;
        tool3["id"] = "RH_PARTING_GROOVING_T03";
        tool3["name"] = "Right-Hand Parting / Grooving";
        tool3["manufacturer"] = "Mitsubishi";
        tool3["toolType"] = static_cast<int>(ToolType::GROOVING);
        tool3["toolNumber"] = "T03";
        tool3["turretPosition"] = 3;
        tool3["isActive"] = true;
        tool3["toolOffset_X"] = 0.0;
        tool3["toolOffset_Z"] = 0.0;
        tool3["toolLengthOffset"] = 0.0;
        tool3["toolRadiusOffset"] = 0.0;
        tool3["expectedLifeMinutes"] = 180.0;
        tool3["usageMinutes"] = 0.0;
        tool3["cycleCount"] = 0;
        tool3["notes"] = "Right-hand 3mm parting and grooving tool";
        tool3["internalThreading"] = false;
        tool3["externalThreading"] = false;
        tool3["internalBoring"] = false;
        tool3["partingGrooving"] = true;
        tool3["longitudinalTurning"] = false;
        tool3["facing"] = false;
        tool3["chamfering"] = false;
        
        // Grooving Insert
        QJsonObject groovingInsert3;
        groovingInsert3["isoCode"] = "MGMN300";
        groovingInsert3["thickness"] = 3.0;
        groovingInsert3["overallLength"] = 15.0;
        groovingInsert3["width"] = 3.0;
        groovingInsert3["cornerRadius"] = 0.05;
        groovingInsert3["headLength"] = 10.0;
        groovingInsert3["grooveWidth"] = 3.0;
        groovingInsert3["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        groovingInsert3["manufacturer"] = "Mitsubishi";
        groovingInsert3["name"] = "MGMN300 Parting/Grooving Insert";
        groovingInsert3["isActive"] = true;
        tool3["groovingInsert"] = groovingInsert3;
        
        // Tool Holder
        QJsonObject holder3;
        holder3["isoCode"] = "MGFHR2020-3";
        holder3["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder3["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder3["cuttingWidth"] = 3.0;
        holder3["headLength"] = 30.0;
        holder3["overallLength"] = 150.0;
        holder3["shankWidth"] = 20.0;
        holder3["shankHeight"] = 20.0;
        holder3["isRoundShank"] = false;
        holder3["insertSeatAngle"] = 90.0;
        holder3["insertSetback"] = 1.0;
        holder3["manufacturer"] = "Mitsubishi";
        tool3["holder"] = holder3;
        
        // Cutting Data
        QJsonObject cuttingData3;
        cuttingData3["constantSurfaceSpeed"] = true;
        cuttingData3["surfaceSpeed"] = 120.0;
        cuttingData3["spindleSpeed"] = 400;
        cuttingData3["feedPerRevolution"] = true;
        cuttingData3["cuttingFeedrate"] = 0.03;
        cuttingData3["plungeFeedrate"] = 0.01;
        cuttingData3["retractFeedrate"] = 1.0;
        cuttingData3["maxDepthOfCut"] = 25.0;
        cuttingData3["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool3["cuttingData"] = cuttingData3;
        
        toolsArray.append(tool3);
        
        // Tool 4: Right-hand tool: external metric threading
        QJsonObject tool4;
        tool4["id"] = "RH_EXTERNAL_THREADING_T04";
        tool4["name"] = "Right-Hand External Threading";
        tool4["manufacturer"] = "Sandvik";
        tool4["toolType"] = static_cast<int>(ToolType::THREADING);
        tool4["toolNumber"] = "T04";
        tool4["turretPosition"] = 4;
        tool4["isActive"] = true;
        tool4["toolOffset_X"] = 0.0;
        tool4["toolOffset_Z"] = 0.0;
        tool4["toolLengthOffset"] = 0.0;
        tool4["toolRadiusOffset"] = 0.0;
        tool4["expectedLifeMinutes"] = 240.0;
        tool4["usageMinutes"] = 0.0;
        tool4["cycleCount"] = 0;
        tool4["notes"] = "Right-hand external metric threading tool for M6-M30 threads";
        tool4["internalThreading"] = false;
        tool4["externalThreading"] = true;
        tool4["internalBoring"] = false;
        tool4["partingGrooving"] = false;
        tool4["longitudinalTurning"] = false;
        tool4["facing"] = false;
        tool4["chamfering"] = false;
        
        // Threading Insert
        QJsonObject threadingInsert4;
        threadingInsert4["isoCode"] = "16ER1.5ISO";
        threadingInsert4["thickness"] = 3.18;
        threadingInsert4["width"] = 6.0;
        threadingInsert4["minThreadPitch"] = 1.0;
        threadingInsert4["maxThreadPitch"] = 3.5;
        threadingInsert4["internalThreads"] = false;
        threadingInsert4["externalThreads"] = true;
        threadingInsert4["threadProfile"] = static_cast<int>(ThreadProfile::METRIC);
        threadingInsert4["threadProfileAngle"] = 60.0;
        threadingInsert4["threadTipType"] = static_cast<int>(ThreadTipType::SHARP_POINT);
        threadingInsert4["threadTipRadius"] = 0.0;
        threadingInsert4["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        threadingInsert4["manufacturer"] = "Sandvik";
        threadingInsert4["name"] = "16ER1.5ISO External Threading";
        threadingInsert4["isActive"] = true;
        tool4["threadingInsert"] = threadingInsert4;
        
        // Tool Holder
        QJsonObject holder4;
        holder4["isoCode"] = "SER2525M16";
        holder4["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder4["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
        holder4["cuttingWidth"] = 16.0;
        holder4["headLength"] = 40.0;
        holder4["overallLength"] = 150.0;
        holder4["shankWidth"] = 25.0;
        holder4["shankHeight"] = 25.0;
        holder4["isRoundShank"] = false;
        holder4["insertSeatAngle"] = 90.0;
        holder4["insertSetback"] = 1.5;
        holder4["manufacturer"] = "Sandvik";
        tool4["holder"] = holder4;
        
        // Cutting Data
        QJsonObject cuttingData4;
        cuttingData4["constantSurfaceSpeed"] = true;
        cuttingData4["surfaceSpeed"] = 100.0;
        cuttingData4["spindleSpeed"] = 350;
        cuttingData4["feedPerRevolution"] = true;
        cuttingData4["cuttingFeedrate"] = 1.5; // pitch dependent
        cuttingData4["plungeFeedrate"] = 0.04;
        cuttingData4["retractFeedrate"] = 2.0;
        cuttingData4["maxDepthOfCut"] = 0.5;
        cuttingData4["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool4["cuttingData"] = cuttingData4;
        
        toolsArray.append(tool4);
        
        // Tool 5: Left-hand tool: longitudinal turning
        QJsonObject tool5;
        tool5["id"] = "LH_LONGITUDINAL_TURNING_T05";
        tool5["name"] = "Left-Hand Longitudinal Turning";
        tool5["manufacturer"] = "Sandvik";
        tool5["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
        tool5["toolNumber"] = "T05";
        tool5["turretPosition"] = 5;
        tool5["isActive"] = true;
        tool5["toolOffset_X"] = 0.0;
        tool5["toolOffset_Z"] = 0.0;
        tool5["toolLengthOffset"] = 0.0;
        tool5["toolRadiusOffset"] = 0.4;
        tool5["expectedLifeMinutes"] = 480.0;
        tool5["usageMinutes"] = 0.0;
        tool5["cycleCount"] = 0;
        tool5["notes"] = "Left-hand longitudinal turning tool for roughing and finishing";
        tool5["internalThreading"] = false;
        tool5["externalThreading"] = false;
        tool5["internalBoring"] = false;
        tool5["partingGrooving"] = false;
        tool5["longitudinalTurning"] = true;
        tool5["facing"] = false;
        tool5["chamfering"] = false;
        
        // General Turning Insert
        QJsonObject turningInsert5;
        turningInsert5["isoCode"] = "CNMG120408";
        turningInsert5["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert5["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert5["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert5["sizeSpecifier"] = "12";
        turningInsert5["inscribedCircle"] = 12.7;
        turningInsert5["thickness"] = 4.76;
        turningInsert5["cornerRadius"] = 0.8;
        turningInsert5["cuttingEdgeLength"] = 12.7;
        turningInsert5["width"] = 12.7;
        turningInsert5["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert5["substrate"] = "WC-Co";
        turningInsert5["coating"] = "TiAlN";
        turningInsert5["manufacturer"] = "Sandvik";
        turningInsert5["partNumber"] = "CNMG 12 04 08-PM 4325";
        turningInsert5["rake_angle"] = 0.0;
        turningInsert5["inclination_angle"] = -6.0;
        turningInsert5["name"] = "CNMG120408 Left-Hand Turning";
        turningInsert5["isActive"] = true;
        tool5["turningInsert"] = turningInsert5;
        
        // Tool Holder
        QJsonObject holder5;
        holder5["isoCode"] = "MCLNL2525M12";
        holder5["handOrientation"] = static_cast<int>(HandOrientation::LEFT_HAND);
        holder5["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder5["cuttingWidth"] = 25.0;
        holder5["headLength"] = 50.0;
        holder5["overallLength"] = 150.0;
        holder5["shankWidth"] = 25.0;
        holder5["shankHeight"] = 25.0;
        holder5["isRoundShank"] = false;
        holder5["insertSeatAngle"] = 95.0;
        holder5["insertSetback"] = 2.0;
        holder5["manufacturer"] = "Sandvik";
        tool5["holder"] = holder5;
        
        // Cutting Data
        QJsonObject cuttingData5;
        cuttingData5["constantSurfaceSpeed"] = true;
        cuttingData5["surfaceSpeed"] = 250.0;
        cuttingData5["spindleSpeed"] = 800;
        cuttingData5["feedPerRevolution"] = true;
        cuttingData5["cuttingFeedrate"] = 0.25;
        cuttingData5["plungeFeedrate"] = 0.1;
        cuttingData5["retractFeedrate"] = 5.0;
        cuttingData5["maxDepthOfCut"] = 3.0;
        cuttingData5["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool5["cuttingData"] = cuttingData5;
        
        toolsArray.append(tool5);
        
        // Tool 6: Right-hand tool: longitudinal turning / facing / chamfering
        QJsonObject tool6;
        tool6["id"] = "RH_LONGIT_FACE_CHAMFER_T06";
        tool6["name"] = "Right-Hand Longitudinal/Facing/Chamfering";
        tool6["manufacturer"] = "Sandvik";
        tool6["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
        tool6["toolNumber"] = "T06";
        tool6["turretPosition"] = 6;
        tool6["isActive"] = true;
        tool6["toolOffset_X"] = 0.0;
        tool6["toolOffset_Z"] = 0.0;
        tool6["toolLengthOffset"] = 0.0;
        tool6["toolRadiusOffset"] = 0.4;
        tool6["expectedLifeMinutes"] = 480.0;
        tool6["usageMinutes"] = 0.0;
        tool6["cycleCount"] = 0;
        tool6["notes"] = "Multi-purpose right-hand turning, facing and chamfering tool";
        tool6["internalThreading"] = false;
        tool6["externalThreading"] = false;
        tool6["internalBoring"] = false;
        tool6["partingGrooving"] = false;
        tool6["longitudinalTurning"] = true;
        tool6["facing"] = true;
        tool6["chamfering"] = true;
        
        // General Turning Insert
        QJsonObject turningInsert6;
        turningInsert6["isoCode"] = "CNMG120408";
        turningInsert6["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert6["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert6["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert6["sizeSpecifier"] = "12";
        turningInsert6["inscribedCircle"] = 12.7;
        turningInsert6["thickness"] = 4.76;
        turningInsert6["cornerRadius"] = 0.8;
        turningInsert6["cuttingEdgeLength"] = 12.7;
        turningInsert6["width"] = 12.7;
        turningInsert6["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert6["substrate"] = "WC-Co";
        turningInsert6["coating"] = "TiAlN";
        turningInsert6["manufacturer"] = "Sandvik";
        turningInsert6["partNumber"] = "CNMG 12 04 08-PM 4325";
        turningInsert6["rake_angle"] = 0.0;
        turningInsert6["inclination_angle"] = -6.0;
        turningInsert6["name"] = "CNMG120408 Multi-Purpose";
        turningInsert6["isActive"] = true;
        tool6["turningInsert"] = turningInsert6;
        
        // Tool Holder
        QJsonObject holder6;
        holder6["isoCode"] = "MCLNR2525M12";
        holder6["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder6["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder6["cuttingWidth"] = 25.0;
        holder6["headLength"] = 50.0;
        holder6["overallLength"] = 150.0;
        holder6["shankWidth"] = 25.0;
        holder6["shankHeight"] = 25.0;
        holder6["isRoundShank"] = false;
        holder6["insertSeatAngle"] = 95.0;
        holder6["insertSetback"] = 2.0;
        holder6["manufacturer"] = "Sandvik";
        tool6["holder"] = holder6;
        
        // Cutting Data
        QJsonObject cuttingData6;
        cuttingData6["constantSurfaceSpeed"] = true;
        cuttingData6["surfaceSpeed"] = 250.0;
        cuttingData6["spindleSpeed"] = 800;
        cuttingData6["feedPerRevolution"] = true;
        cuttingData6["cuttingFeedrate"] = 0.25;
        cuttingData6["plungeFeedrate"] = 0.1;
        cuttingData6["retractFeedrate"] = 5.0;
        cuttingData6["maxDepthOfCut"] = 3.0;
        cuttingData6["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool6["cuttingData"] = cuttingData6;
        
        toolsArray.append(tool6);
        
        // Tool 7: Right-hand tool: longitudinal turning
        QJsonObject tool7;
        tool7["id"] = "RH_LONGITUDINAL_TURNING_T07";
        tool7["name"] = "Right-Hand Longitudinal Turning";
        tool7["manufacturer"] = "Sandvik";
        tool7["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
        tool7["toolNumber"] = "T07";
        tool7["turretPosition"] = 7;
        tool7["isActive"] = true;
        tool7["toolOffset_X"] = 0.0;
        tool7["toolOffset_Z"] = 0.0;
        tool7["toolLengthOffset"] = 0.0;
        tool7["toolRadiusOffset"] = 0.4;
        tool7["expectedLifeMinutes"] = 480.0;
        tool7["usageMinutes"] = 0.0;
        tool7["cycleCount"] = 0;
        tool7["notes"] = "Right-hand longitudinal turning tool for general machining";
        tool7["internalThreading"] = false;
        tool7["externalThreading"] = false;
        tool7["internalBoring"] = false;
        tool7["partingGrooving"] = false;
        tool7["longitudinalTurning"] = true;
        tool7["facing"] = false;
        tool7["chamfering"] = false;
        
        // General Turning Insert
        QJsonObject turningInsert7;
        turningInsert7["isoCode"] = "CNMG120408";
        turningInsert7["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert7["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert7["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert7["sizeSpecifier"] = "12";
        turningInsert7["inscribedCircle"] = 12.7;
        turningInsert7["thickness"] = 4.76;
        turningInsert7["cornerRadius"] = 0.8;
        turningInsert7["cuttingEdgeLength"] = 12.7;
        turningInsert7["width"] = 12.7;
        turningInsert7["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert7["substrate"] = "WC-Co";
        turningInsert7["coating"] = "TiAlN";
        turningInsert7["manufacturer"] = "Sandvik";
        turningInsert7["partNumber"] = "CNMG 12 04 08-PM 4325";
        turningInsert7["rake_angle"] = 0.0;
        turningInsert7["inclination_angle"] = -6.0;
        turningInsert7["name"] = "CNMG120408 Right-Hand Turning";
        turningInsert7["isActive"] = true;
        tool7["turningInsert"] = turningInsert7;
        
        // Tool Holder
        QJsonObject holder7;
        holder7["isoCode"] = "MCLNR2525M12";
        holder7["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder7["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder7["cuttingWidth"] = 25.0;
        holder7["headLength"] = 50.0;
        holder7["overallLength"] = 150.0;
        holder7["shankWidth"] = 25.0;
        holder7["shankHeight"] = 25.0;
        holder7["isRoundShank"] = false;
        holder7["insertSeatAngle"] = 95.0;
        holder7["insertSetback"] = 2.0;
        holder7["manufacturer"] = "Sandvik";
        tool7["holder"] = holder7;
        
        // Cutting Data
        QJsonObject cuttingData7;
        cuttingData7["constantSurfaceSpeed"] = true;
        cuttingData7["surfaceSpeed"] = 250.0;
        cuttingData7["spindleSpeed"] = 800;
        cuttingData7["feedPerRevolution"] = true;
        cuttingData7["cuttingFeedrate"] = 0.25;
        cuttingData7["plungeFeedrate"] = 0.1;
        cuttingData7["retractFeedrate"] = 5.0;
        cuttingData7["maxDepthOfCut"] = 3.0;
        cuttingData7["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool7["cuttingData"] = cuttingData7;
        
        toolsArray.append(tool7);
        
        // Tool 8: Right-hand tool: facing / chamfering
        QJsonObject tool8;
        tool8["id"] = "RH_FACING_CHAMFERING_T08";
        tool8["name"] = "Right-Hand Facing / Chamfering";
        tool8["manufacturer"] = "Sandvik";
        tool8["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
        tool8["toolNumber"] = "T08";
        tool8["turretPosition"] = 8;
        tool8["isActive"] = true;
        tool8["toolOffset_X"] = 0.0;
        tool8["toolOffset_Z"] = 0.0;
        tool8["toolLengthOffset"] = 0.0;
        tool8["toolRadiusOffset"] = 0.4;
        tool8["expectedLifeMinutes"] = 360.0;
        tool8["usageMinutes"] = 0.0;
        tool8["cycleCount"] = 0;
        tool8["notes"] = "Right-hand facing and chamfering tool for finishing operations";
        tool8["internalThreading"] = false;
        tool8["externalThreading"] = false;
        tool8["internalBoring"] = false;
        tool8["partingGrooving"] = false;
        tool8["longitudinalTurning"] = false;
        tool8["facing"] = true;
        tool8["chamfering"] = true;
        
        // General Turning Insert
        QJsonObject turningInsert8;
        turningInsert8["isoCode"] = "CNMG120404";
        turningInsert8["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert8["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert8["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert8["sizeSpecifier"] = "12";
        turningInsert8["inscribedCircle"] = 12.7;
        turningInsert8["thickness"] = 4.76;
        turningInsert8["cornerRadius"] = 0.4;
        turningInsert8["cuttingEdgeLength"] = 12.7;
        turningInsert8["width"] = 12.7;
        turningInsert8["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert8["substrate"] = "WC-Co";
        turningInsert8["coating"] = "TiAlN";
        turningInsert8["manufacturer"] = "Sandvik";
        turningInsert8["partNumber"] = "CNMG 12 04 04-PM 4325";
        turningInsert8["rake_angle"] = 0.0;
        turningInsert8["inclination_angle"] = -6.0;
        turningInsert8["name"] = "CNMG120404 Facing/Chamfering";
        turningInsert8["isActive"] = true;
        tool8["turningInsert"] = turningInsert8;
        
        // Tool Holder
        QJsonObject holder8;
        holder8["isoCode"] = "MCLNR2525M12";
        holder8["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
        holder8["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder8["cuttingWidth"] = 25.0;
        holder8["headLength"] = 50.0;
        holder8["overallLength"] = 150.0;
        holder8["shankWidth"] = 25.0;
        holder8["shankHeight"] = 25.0;
        holder8["isRoundShank"] = false;
        holder8["insertSeatAngle"] = 95.0;
        holder8["insertSetback"] = 2.0;
        holder8["manufacturer"] = "Sandvik";
        tool8["holder"] = holder8;
        
        // Cutting Data
        QJsonObject cuttingData8;
        cuttingData8["constantSurfaceSpeed"] = true;
        cuttingData8["surfaceSpeed"] = 220.0;
        cuttingData8["spindleSpeed"] = 700;
        cuttingData8["feedPerRevolution"] = true;
        cuttingData8["cuttingFeedrate"] = 0.15;
        cuttingData8["plungeFeedrate"] = 0.08;
        cuttingData8["retractFeedrate"] = 4.0;
        cuttingData8["maxDepthOfCut"] = 2.0;
        cuttingData8["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool8["cuttingData"] = cuttingData8;
        
        toolsArray.append(tool8);
        
        // Tool 9: Neutral tool: longitudinal turning
        QJsonObject tool9;
        tool9["id"] = "NEUTRAL_LONGITUDINAL_TURNING_T09";
        tool9["name"] = "Neutral Longitudinal Turning";
        tool9["manufacturer"] = "Iscar";
        tool9["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
        tool9["toolNumber"] = "T09";
        tool9["turretPosition"] = 9;
        tool9["isActive"] = true;
        tool9["toolOffset_X"] = 0.0;
        tool9["toolOffset_Z"] = 0.0;
        tool9["toolLengthOffset"] = 0.0;
        tool9["toolRadiusOffset"] = 0.4;
        tool9["expectedLifeMinutes"] = 480.0;
        tool9["usageMinutes"] = 0.0;
        tool9["cycleCount"] = 0;
        tool9["notes"] = "Neutral longitudinal turning tool for versatile machining";
        tool9["internalThreading"] = false;
        tool9["externalThreading"] = false;
        tool9["internalBoring"] = false;
        tool9["partingGrooving"] = false;
        tool9["longitudinalTurning"] = true;
        tool9["facing"] = false;
        tool9["chamfering"] = false;
        
        // General Turning Insert
        QJsonObject turningInsert9;
        turningInsert9["isoCode"] = "CNMG120408";
        turningInsert9["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
        turningInsert9["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
        turningInsert9["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
        turningInsert9["sizeSpecifier"] = "12";
        turningInsert9["inscribedCircle"] = 12.7;
        turningInsert9["thickness"] = 4.76;
        turningInsert9["cornerRadius"] = 0.8;
        turningInsert9["cuttingEdgeLength"] = 12.7;
        turningInsert9["width"] = 12.7;
        turningInsert9["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
        turningInsert9["substrate"] = "WC-Co";
        turningInsert9["coating"] = "TiAlN";
        turningInsert9["manufacturer"] = "Iscar";
        turningInsert9["partNumber"] = "CNMG 12 04 08-IC 928";
        turningInsert9["rake_angle"] = 0.0;
        turningInsert9["inclination_angle"] = -6.0;
        turningInsert9["name"] = "CNMG120408 Neutral Turning";
        turningInsert9["isActive"] = true;
        tool9["turningInsert"] = turningInsert9;
        
        // Tool Holder
        QJsonObject holder9;
        holder9["isoCode"] = "MCLNN2525M12";
        holder9["handOrientation"] = static_cast<int>(HandOrientation::NEUTRAL);
        holder9["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
        holder9["cuttingWidth"] = 25.0;
        holder9["headLength"] = 50.0;
        holder9["overallLength"] = 150.0;
        holder9["shankWidth"] = 25.0;
        holder9["shankHeight"] = 25.0;
        holder9["isRoundShank"] = false;
        holder9["insertSeatAngle"] = 95.0;
        holder9["insertSetback"] = 2.0;
        holder9["manufacturer"] = "Iscar";
        tool9["holder"] = holder9;
        
        // Cutting Data
        QJsonObject cuttingData9;
        cuttingData9["constantSurfaceSpeed"] = true;
        cuttingData9["surfaceSpeed"] = 250.0;
        cuttingData9["spindleSpeed"] = 800;
        cuttingData9["feedPerRevolution"] = true;
        cuttingData9["cuttingFeedrate"] = 0.25;
        cuttingData9["plungeFeedrate"] = 0.1;
        cuttingData9["retractFeedrate"] = 5.0;
        cuttingData9["maxDepthOfCut"] = 3.0;
        cuttingData9["coolantType"] = static_cast<int>(CoolantType::FLOOD);
        tool9["cuttingData"] = cuttingData9;
        
        toolsArray.append(tool9);
        
        // Save to database
        QJsonObject database;
        database["tools"] = toolsArray;
        database["version"] = "1.0";
        database["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        database["description"] = "IntuiCAM Default Tool Library";
        
        // Write to database file
        QString databasePath = getToolAssemblyDatabasePath();
        QJsonDocument doc(database);
        QFile file(databasePath);
        
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            
            qDebug() << "Successfully saved 9 default tools to database:" << databasePath;
            
            // Refresh the tool list to show the new tools
            refreshToolList();
            
            QMessageBox::information(this, "Default Tools Loaded", 
                "Successfully created and loaded 9 professional lathe tools:\n\n"
                "• Right-Hand Internal Threading (T01)\n"
                "• Right-Hand Internal Boring (T02)\n"
                "• Right-Hand Parting/Grooving (T03)\n"
                "• Right-Hand External Threading (T04)\n"
                "• Left-Hand Longitudinal Turning (T05)\n"
                "• Right-Hand Longitudinal/Facing/Chamfering (T06)\n"
                "• Right-Hand Longitudinal Turning (T07)\n"
                "• Right-Hand Facing/Chamfering (T08)\n"
                "• Neutral Longitudinal Turning (T09)\n\n"
                "All tools include complete insert, holder, and cutting parameters.");
        } else {
            QMessageBox::warning(this, "Error", 
                "Failed to save default tools to database: " + file.errorString());
        }
        
        emit toolLibraryChanged();
    }
}

void ToolManagementTab::ensureDefaultToolsExist() {
    QString databasePath = getToolAssemblyDatabasePath();
    qDebug() << "ToolManagementTab::ensureDefaultToolsExist() - Database path:" << databasePath;
    QFile file(databasePath);
    
    // If database file doesn't exist, create it with default tools
    if (!file.exists()) {
        qDebug() << "No tool database found, creating default tools on startup";
        createDefaultToolDatabase();
    } else {
        // If file exists, check if it has tools in it
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject database = doc.object();
                if (database.contains("tools")) {
                    QJsonArray toolsArray = database["tools"].toArray();
                    if (toolsArray.isEmpty()) {
                        qDebug() << "Tool database exists but is empty, creating default tools";
                        createDefaultToolDatabase();
                    } else {
                        qDebug() << "Tool database exists with" << toolsArray.size() << "tools";
                    }
                } else {
                    qDebug() << "Tool database exists but has no tools array, creating default tools";
                    createDefaultToolDatabase();
                }
            } else {
                qDebug() << "Tool database exists but is corrupted, recreating with default tools";
                createDefaultToolDatabase();
            }
        } else {
            qDebug() << "Could not read tool database, creating default tools";
            createDefaultToolDatabase();
        }
    }
}

void ToolManagementTab::createDefaultToolDatabase() {
    qDebug() << "ToolManagementTab::createDefaultToolDatabase() - Creating 9 default tools";
    // Create the same 9 default tools as in loadDefaultTools() but without user confirmation
    QJsonArray toolsArray;
    
    // Tool 1: Right-hand tool: internal metric threading
    QJsonObject tool1;
    tool1["id"] = "RH_INTERNAL_THREADING_T01";
    tool1["name"] = "Right-Hand Internal Threading";
    tool1["manufacturer"] = "Iscar";
    tool1["toolType"] = static_cast<int>(ToolType::THREADING);
    tool1["toolNumber"] = "T01";
    tool1["turretPosition"] = 1;
    tool1["isActive"] = true;
    tool1["toolOffset_X"] = 0.0;
    tool1["toolOffset_Z"] = 0.0;
    tool1["toolLengthOffset"] = 0.0;
    tool1["toolRadiusOffset"] = 0.0;
    tool1["expectedLifeMinutes"] = 240.0;
    tool1["usageMinutes"] = 0.0;
    tool1["cycleCount"] = 0;
    tool1["notes"] = "Right-hand internal metric threading tool for M8-M20 threads";
    tool1["internalThreading"] = true;
    tool1["externalThreading"] = false;
    tool1["internalBoring"] = false;
    tool1["partingGrooving"] = false;
    tool1["longitudinalTurning"] = false;
    tool1["facing"] = false;
    tool1["chamfering"] = false;
    
    // Threading Insert
    QJsonObject threadingInsert1;
    threadingInsert1["isoCode"] = "16IR1.0ISO";
    threadingInsert1["thickness"] = 3.18;
    threadingInsert1["width"] = 6.0;
    threadingInsert1["minThreadPitch"] = 0.75;
    threadingInsert1["maxThreadPitch"] = 2.5;
    threadingInsert1["internalThreads"] = true;
    threadingInsert1["externalThreads"] = false;
    threadingInsert1["threadProfile"] = static_cast<int>(ThreadProfile::METRIC);
    threadingInsert1["threadProfileAngle"] = 60.0;
    threadingInsert1["threadTipType"] = static_cast<int>(ThreadTipType::SHARP_POINT);
    threadingInsert1["threadTipRadius"] = 0.0;
    threadingInsert1["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    threadingInsert1["manufacturer"] = "Iscar";
    threadingInsert1["name"] = "16IR1.0ISO Internal Threading";
    threadingInsert1["isActive"] = true;
    tool1["threadingInsert"] = threadingInsert1;
    
    // Tool Holder
    QJsonObject holder1;
    holder1["isoCode"] = "SIR2020K16";
    holder1["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder1["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
    holder1["cuttingWidth"] = 16.0;
    holder1["headLength"] = 40.0;
    holder1["overallLength"] = 150.0;
    holder1["shankWidth"] = 20.0;
    holder1["shankHeight"] = 20.0;
    holder1["isRoundShank"] = false;
    holder1["insertSeatAngle"] = 90.0;
    holder1["insertSetback"] = 1.5;
    holder1["manufacturer"] = "Iscar";
    tool1["holder"] = holder1;
    
    // Cutting Data
    QJsonObject cuttingData1;
    cuttingData1["constantSurfaceSpeed"] = true;
    cuttingData1["surfaceSpeed"] = 80.0;
    cuttingData1["spindleSpeed"] = 300;
    cuttingData1["feedPerRevolution"] = true;
    cuttingData1["cuttingFeedrate"] = 1.25; // pitch dependent
    cuttingData1["plungeFeedrate"] = 0.03;
    cuttingData1["retractFeedrate"] = 1.5;
    cuttingData1["maxDepthOfCut"] = 0.4;
    cuttingData1["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool1["cuttingData"] = cuttingData1;
    
    toolsArray.append(tool1);
    
    // Tool 2: Right-hand tool: internal boring
    QJsonObject tool2;
    tool2["id"] = "RH_INTERNAL_BORING_T02";
    tool2["name"] = "Right-Hand Internal Boring";
    tool2["manufacturer"] = "Kennametal";
    tool2["toolType"] = static_cast<int>(ToolType::BORING);
    tool2["toolNumber"] = "T02";
    tool2["turretPosition"] = 2;
    tool2["isActive"] = true;
    tool2["toolOffset_X"] = 0.0;
    tool2["toolOffset_Z"] = 0.0;
    tool2["toolLengthOffset"] = 0.0;
    tool2["toolRadiusOffset"] = 0.8;
    tool2["expectedLifeMinutes"] = 360.0;
    tool2["usageMinutes"] = 0.0;
    tool2["cycleCount"] = 0;
    tool2["notes"] = "Right-hand internal boring tool, min bore 15mm";
    tool2["internalThreading"] = false;
    tool2["externalThreading"] = false;
    tool2["internalBoring"] = true;
    tool2["partingGrooving"] = false;
    tool2["longitudinalTurning"] = false;
    tool2["facing"] = false;
    tool2["chamfering"] = false;
    
    // General Turning Insert (used for boring)
    QJsonObject turningInsert2;
    turningInsert2["isoCode"] = "CCMT09T304";
    turningInsert2["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert2["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert2["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert2["sizeSpecifier"] = "09";
    turningInsert2["inscribedCircle"] = 9.525;
    turningInsert2["thickness"] = 3.97;
    turningInsert2["cornerRadius"] = 0.4;
    turningInsert2["cuttingEdgeLength"] = 9.525;
    turningInsert2["width"] = 9.525;
    turningInsert2["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert2["substrate"] = "WC-Co";
    turningInsert2["coating"] = "TiAlN";
    turningInsert2["manufacturer"] = "Kennametal";
    turningInsert2["partNumber"] = "CCMT 09 T3 04-HP KC5010";
    turningInsert2["rake_angle"] = 0.0;
    turningInsert2["inclination_angle"] = -6.0;
    turningInsert2["name"] = "CCMT09T304 Boring Insert";
    turningInsert2["isActive"] = true;
    tool2["turningInsert"] = turningInsert2;
    
    // Tool Holder (Boring Bar)
    QJsonObject holder2;
    holder2["isoCode"] = "S16R-SCLCR09";
    holder2["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder2["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
    holder2["cuttingWidth"] = 9.0;
    holder2["headLength"] = 120.0;
    holder2["overallLength"] = 200.0;
    holder2["shankWidth"] = 16.0;
    holder2["shankHeight"] = 16.0;
    holder2["isRoundShank"] = true;
    holder2["shankDiameter"] = 16.0;
    holder2["insertSeatAngle"] = 95.0;
    holder2["insertSetback"] = 1.0;
    holder2["manufacturer"] = "Kennametal";
    tool2["holder"] = holder2;
    
    // Cutting Data
    QJsonObject cuttingData2;
    cuttingData2["constantSurfaceSpeed"] = true;
    cuttingData2["surfaceSpeed"] = 180.0;
    cuttingData2["spindleSpeed"] = 600;
    cuttingData2["feedPerRevolution"] = true;
    cuttingData2["cuttingFeedrate"] = 0.15;
    cuttingData2["plungeFeedrate"] = 0.05;
    cuttingData2["retractFeedrate"] = 3.0;
    cuttingData2["maxDepthOfCut"] = 1.5;
    cuttingData2["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool2["cuttingData"] = cuttingData2;
    
    toolsArray.append(tool2);
    
    // Tool 3: Right-hand tool: parting / grooving
    QJsonObject tool3;
    tool3["id"] = "RH_PARTING_GROOVING_T03";
    tool3["name"] = "Right-Hand Parting / Grooving";
    tool3["manufacturer"] = "Mitsubishi";
    tool3["toolType"] = static_cast<int>(ToolType::GROOVING);
    tool3["toolNumber"] = "T03";
    tool3["turretPosition"] = 3;
    tool3["isActive"] = true;
    tool3["toolOffset_X"] = 0.0;
    tool3["toolOffset_Z"] = 0.0;
    tool3["toolLengthOffset"] = 0.0;
    tool3["toolRadiusOffset"] = 0.0;
    tool3["expectedLifeMinutes"] = 180.0;
    tool3["usageMinutes"] = 0.0;
    tool3["cycleCount"] = 0;
    tool3["notes"] = "Right-hand 3mm parting and grooving tool";
    tool3["internalThreading"] = false;
    tool3["externalThreading"] = false;
    tool3["internalBoring"] = false;
    tool3["partingGrooving"] = true;
    tool3["longitudinalTurning"] = false;
    tool3["facing"] = false;
    tool3["chamfering"] = false;
    
    // Grooving Insert
    QJsonObject groovingInsert3;
    groovingInsert3["isoCode"] = "MGMN300";
    groovingInsert3["thickness"] = 3.0;
    groovingInsert3["overallLength"] = 15.0;
    groovingInsert3["width"] = 3.0;
    groovingInsert3["cornerRadius"] = 0.05;
    groovingInsert3["headLength"] = 10.0;
    groovingInsert3["grooveWidth"] = 3.0;
    groovingInsert3["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    groovingInsert3["manufacturer"] = "Mitsubishi";
    groovingInsert3["name"] = "MGMN300 Parting/Grooving Insert";
    groovingInsert3["isActive"] = true;
    tool3["groovingInsert"] = groovingInsert3;
    
    // Tool Holder
    QJsonObject holder3;
    holder3["isoCode"] = "MGFHR2020-3";
    holder3["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder3["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder3["cuttingWidth"] = 3.0;
    holder3["headLength"] = 30.0;
    holder3["overallLength"] = 150.0;
    holder3["shankWidth"] = 20.0;
    holder3["shankHeight"] = 20.0;
    holder3["isRoundShank"] = false;
    holder3["insertSeatAngle"] = 90.0;
    holder3["insertSetback"] = 1.0;
    holder3["manufacturer"] = "Mitsubishi";
    tool3["holder"] = holder3;
    
    // Cutting Data
    QJsonObject cuttingData3;
    cuttingData3["constantSurfaceSpeed"] = true;
    cuttingData3["surfaceSpeed"] = 120.0;
    cuttingData3["spindleSpeed"] = 400;
    cuttingData3["feedPerRevolution"] = true;
    cuttingData3["cuttingFeedrate"] = 0.03;
    cuttingData3["plungeFeedrate"] = 0.01;
    cuttingData3["retractFeedrate"] = 1.0;
    cuttingData3["maxDepthOfCut"] = 25.0;
    cuttingData3["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool3["cuttingData"] = cuttingData3;
    
    toolsArray.append(tool3);
    
    // Tool 4: Right-hand tool: external metric threading
    QJsonObject tool4;
    tool4["id"] = "RH_EXTERNAL_THREADING_T04";
    tool4["name"] = "Right-Hand External Threading";
    tool4["manufacturer"] = "Sandvik";
    tool4["toolType"] = static_cast<int>(ToolType::THREADING);
    tool4["toolNumber"] = "T04";
    tool4["turretPosition"] = 4;
    tool4["isActive"] = true;
    tool4["toolOffset_X"] = 0.0;
    tool4["toolOffset_Z"] = 0.0;
    tool4["toolLengthOffset"] = 0.0;
    tool4["toolRadiusOffset"] = 0.0;
    tool4["expectedLifeMinutes"] = 240.0;
    tool4["usageMinutes"] = 0.0;
    tool4["cycleCount"] = 0;
    tool4["notes"] = "Right-hand external metric threading tool for M6-M30 threads";
    tool4["internalThreading"] = false;
    tool4["externalThreading"] = true;
    tool4["internalBoring"] = false;
    tool4["partingGrooving"] = false;
    tool4["longitudinalTurning"] = false;
    tool4["facing"] = false;
    tool4["chamfering"] = false;
    
    // Threading Insert
    QJsonObject threadingInsert4;
    threadingInsert4["isoCode"] = "16ER1.5ISO";
    threadingInsert4["thickness"] = 3.18;
    threadingInsert4["width"] = 6.0;
    threadingInsert4["minThreadPitch"] = 1.0;
    threadingInsert4["maxThreadPitch"] = 3.5;
    threadingInsert4["internalThreads"] = false;
    threadingInsert4["externalThreads"] = true;
    threadingInsert4["threadProfile"] = static_cast<int>(ThreadProfile::METRIC);
    threadingInsert4["threadProfileAngle"] = 60.0;
    threadingInsert4["threadTipType"] = static_cast<int>(ThreadTipType::SHARP_POINT);
    threadingInsert4["threadTipRadius"] = 0.0;
    threadingInsert4["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    threadingInsert4["manufacturer"] = "Sandvik";
    threadingInsert4["name"] = "16ER1.5ISO External Threading";
    threadingInsert4["isActive"] = true;
    tool4["threadingInsert"] = threadingInsert4;
    
    // Tool Holder
    QJsonObject holder4;
    holder4["isoCode"] = "SER2525M16";
    holder4["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder4["clampingStyle"] = static_cast<int>(ClampingStyle::SCREW_CLAMP);
    holder4["cuttingWidth"] = 16.0;
    holder4["headLength"] = 40.0;
    holder4["overallLength"] = 150.0;
    holder4["shankWidth"] = 25.0;
    holder4["shankHeight"] = 25.0;
    holder4["isRoundShank"] = false;
    holder4["insertSeatAngle"] = 90.0;
    holder4["insertSetback"] = 1.5;
    holder4["manufacturer"] = "Sandvik";
    tool4["holder"] = holder4;
    
    // Cutting Data
    QJsonObject cuttingData4;
    cuttingData4["constantSurfaceSpeed"] = true;
    cuttingData4["surfaceSpeed"] = 100.0;
    cuttingData4["spindleSpeed"] = 350;
    cuttingData4["feedPerRevolution"] = true;
    cuttingData4["cuttingFeedrate"] = 1.5; // pitch dependent
    cuttingData4["plungeFeedrate"] = 0.04;
    cuttingData4["retractFeedrate"] = 2.0;
    cuttingData4["maxDepthOfCut"] = 0.5;
    cuttingData4["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool4["cuttingData"] = cuttingData4;
    
    toolsArray.append(tool4);
    
    // Tool 5: Left-hand tool: longitudinal turning
    QJsonObject tool5;
    tool5["id"] = "LH_LONGITUDINAL_TURNING_T05";
    tool5["name"] = "Left-Hand Longitudinal Turning";
    tool5["manufacturer"] = "Sandvik";
    tool5["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
    tool5["toolNumber"] = "T05";
    tool5["turretPosition"] = 5;
    tool5["isActive"] = true;
    tool5["toolOffset_X"] = 0.0;
    tool5["toolOffset_Z"] = 0.0;
    tool5["toolLengthOffset"] = 0.0;
    tool5["toolRadiusOffset"] = 0.4;
    tool5["expectedLifeMinutes"] = 480.0;
    tool5["usageMinutes"] = 0.0;
    tool5["cycleCount"] = 0;
    tool5["notes"] = "Left-hand longitudinal turning tool for roughing and finishing";
    tool5["internalThreading"] = false;
    tool5["externalThreading"] = false;
    tool5["internalBoring"] = false;
    tool5["partingGrooving"] = false;
    tool5["longitudinalTurning"] = true;
    tool5["facing"] = false;
    tool5["chamfering"] = false;
    
    // General Turning Insert
    QJsonObject turningInsert5;
    turningInsert5["isoCode"] = "CNMG120408";
    turningInsert5["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert5["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert5["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert5["sizeSpecifier"] = "12";
    turningInsert5["inscribedCircle"] = 12.7;
    turningInsert5["thickness"] = 4.76;
    turningInsert5["cornerRadius"] = 0.8;
    turningInsert5["cuttingEdgeLength"] = 12.7;
    turningInsert5["width"] = 12.7;
    turningInsert5["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert5["substrate"] = "WC-Co";
    turningInsert5["coating"] = "TiAlN";
    turningInsert5["manufacturer"] = "Sandvik";
    turningInsert5["partNumber"] = "CNMG 12 04 08-PM 4325";
    turningInsert5["rake_angle"] = 0.0;
    turningInsert5["inclination_angle"] = -6.0;
    turningInsert5["name"] = "CNMG120408 Left-Hand Turning";
    turningInsert5["isActive"] = true;
    tool5["turningInsert"] = turningInsert5;
    
    // Tool Holder
    QJsonObject holder5;
    holder5["isoCode"] = "MCLNL2525M12";
    holder5["handOrientation"] = static_cast<int>(HandOrientation::LEFT_HAND);
    holder5["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder5["cuttingWidth"] = 25.0;
    holder5["headLength"] = 50.0;
    holder5["overallLength"] = 150.0;
    holder5["shankWidth"] = 25.0;
    holder5["shankHeight"] = 25.0;
    holder5["isRoundShank"] = false;
    holder5["insertSeatAngle"] = 95.0;
    holder5["insertSetback"] = 2.0;
    holder5["manufacturer"] = "Sandvik";
    tool5["holder"] = holder5;
    
    // Cutting Data
    QJsonObject cuttingData5;
    cuttingData5["constantSurfaceSpeed"] = true;
    cuttingData5["surfaceSpeed"] = 250.0;
    cuttingData5["spindleSpeed"] = 800;
    cuttingData5["feedPerRevolution"] = true;
    cuttingData5["cuttingFeedrate"] = 0.25;
    cuttingData5["plungeFeedrate"] = 0.1;
    cuttingData5["retractFeedrate"] = 5.0;
    cuttingData5["maxDepthOfCut"] = 3.0;
    cuttingData5["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool5["cuttingData"] = cuttingData5;
    
    toolsArray.append(tool5);
    
    // Tool 6: Right-hand tool: longitudinal turning / facing / chamfering
    QJsonObject tool6;
    tool6["id"] = "RH_LONGIT_FACE_CHAMFER_T06";
    tool6["name"] = "Right-Hand Longitudinal/Facing/Chamfering";
    tool6["manufacturer"] = "Sandvik";
    tool6["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
    tool6["toolNumber"] = "T06";
    tool6["turretPosition"] = 6;
    tool6["isActive"] = true;
    tool6["toolOffset_X"] = 0.0;
    tool6["toolOffset_Z"] = 0.0;
    tool6["toolLengthOffset"] = 0.0;
    tool6["toolRadiusOffset"] = 0.4;
    tool6["expectedLifeMinutes"] = 480.0;
    tool6["usageMinutes"] = 0.0;
    tool6["cycleCount"] = 0;
    tool6["notes"] = "Multi-purpose right-hand turning, facing and chamfering tool";
    tool6["internalThreading"] = false;
    tool6["externalThreading"] = false;
    tool6["internalBoring"] = false;
    tool6["partingGrooving"] = false;
    tool6["longitudinalTurning"] = true;
    tool6["facing"] = true;
    tool6["chamfering"] = true;
    
    // General Turning Insert
    QJsonObject turningInsert6;
    turningInsert6["isoCode"] = "CNMG120408";
    turningInsert6["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert6["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert6["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert6["sizeSpecifier"] = "12";
    turningInsert6["inscribedCircle"] = 12.7;
    turningInsert6["thickness"] = 4.76;
    turningInsert6["cornerRadius"] = 0.8;
    turningInsert6["cuttingEdgeLength"] = 12.7;
    turningInsert6["width"] = 12.7;
    turningInsert6["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert6["substrate"] = "WC-Co";
    turningInsert6["coating"] = "TiAlN";
    turningInsert6["manufacturer"] = "Sandvik";
    turningInsert6["partNumber"] = "CNMG 12 04 08-PM 4325";
    turningInsert6["rake_angle"] = 0.0;
    turningInsert6["inclination_angle"] = -6.0;
    turningInsert6["name"] = "CNMG120408 Multi-Purpose";
    turningInsert6["isActive"] = true;
    tool6["turningInsert"] = turningInsert6;
    
    // Tool Holder
    QJsonObject holder6;
    holder6["isoCode"] = "MCLNR2525M12";
    holder6["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder6["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder6["cuttingWidth"] = 25.0;
    holder6["headLength"] = 50.0;
    holder6["overallLength"] = 150.0;
    holder6["shankWidth"] = 25.0;
    holder6["shankHeight"] = 25.0;
    holder6["isRoundShank"] = false;
    holder6["insertSeatAngle"] = 95.0;
    holder6["insertSetback"] = 2.0;
    holder6["manufacturer"] = "Sandvik";
    tool6["holder"] = holder6;
    
    // Cutting Data
    QJsonObject cuttingData6;
    cuttingData6["constantSurfaceSpeed"] = true;
    cuttingData6["surfaceSpeed"] = 250.0;
    cuttingData6["spindleSpeed"] = 800;
    cuttingData6["feedPerRevolution"] = true;
    cuttingData6["cuttingFeedrate"] = 0.25;
    cuttingData6["plungeFeedrate"] = 0.1;
    cuttingData6["retractFeedrate"] = 5.0;
    cuttingData6["maxDepthOfCut"] = 3.0;
    cuttingData6["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool6["cuttingData"] = cuttingData6;
    
    toolsArray.append(tool6);
    
    // Tool 7: Right-hand tool: longitudinal turning
    QJsonObject tool7;
    tool7["id"] = "RH_LONGITUDINAL_TURNING_T07";
    tool7["name"] = "Right-Hand Longitudinal Turning";
    tool7["manufacturer"] = "Sandvik";
    tool7["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
    tool7["toolNumber"] = "T07";
    tool7["turretPosition"] = 7;
    tool7["isActive"] = true;
    tool7["toolOffset_X"] = 0.0;
    tool7["toolOffset_Z"] = 0.0;
    tool7["toolLengthOffset"] = 0.0;
    tool7["toolRadiusOffset"] = 0.4;
    tool7["expectedLifeMinutes"] = 480.0;
    tool7["usageMinutes"] = 0.0;
    tool7["cycleCount"] = 0;
    tool7["notes"] = "Right-hand longitudinal turning tool for general machining";
    tool7["internalThreading"] = false;
    tool7["externalThreading"] = false;
    tool7["internalBoring"] = false;
    tool7["partingGrooving"] = false;
    tool7["longitudinalTurning"] = true;
    tool7["facing"] = false;
    tool7["chamfering"] = false;
    
    // General Turning Insert
    QJsonObject turningInsert7;
    turningInsert7["isoCode"] = "CNMG120408";
    turningInsert7["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert7["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert7["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert7["sizeSpecifier"] = "12";
    turningInsert7["inscribedCircle"] = 12.7;
    turningInsert7["thickness"] = 4.76;
    turningInsert7["cornerRadius"] = 0.8;
    turningInsert7["cuttingEdgeLength"] = 12.7;
    turningInsert7["width"] = 12.7;
    turningInsert7["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert7["substrate"] = "WC-Co";
    turningInsert7["coating"] = "TiAlN";
    turningInsert7["manufacturer"] = "Sandvik";
    turningInsert7["partNumber"] = "CNMG 12 04 08-PM 4325";
    turningInsert7["rake_angle"] = 0.0;
    turningInsert7["inclination_angle"] = -6.0;
    turningInsert7["name"] = "CNMG120408 Right-Hand Turning";
    turningInsert7["isActive"] = true;
    tool7["turningInsert"] = turningInsert7;
    
    // Tool Holder
    QJsonObject holder7;
    holder7["isoCode"] = "MCLNR2525M12";
    holder7["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder7["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder7["cuttingWidth"] = 25.0;
    holder7["headLength"] = 50.0;
    holder7["overallLength"] = 150.0;
    holder7["shankWidth"] = 25.0;
    holder7["shankHeight"] = 25.0;
    holder7["isRoundShank"] = false;
    holder7["insertSeatAngle"] = 95.0;
    holder7["insertSetback"] = 2.0;
    holder7["manufacturer"] = "Sandvik";
    tool7["holder"] = holder7;
    
    // Cutting Data
    QJsonObject cuttingData7;
    cuttingData7["constantSurfaceSpeed"] = true;
    cuttingData7["surfaceSpeed"] = 250.0;
    cuttingData7["spindleSpeed"] = 800;
    cuttingData7["feedPerRevolution"] = true;
    cuttingData7["cuttingFeedrate"] = 0.25;
    cuttingData7["plungeFeedrate"] = 0.1;
    cuttingData7["retractFeedrate"] = 5.0;
    cuttingData7["maxDepthOfCut"] = 3.0;
    cuttingData7["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool7["cuttingData"] = cuttingData7;
    
    toolsArray.append(tool7);
    
    // Tool 8: Right-hand tool: facing / chamfering
    QJsonObject tool8;
    tool8["id"] = "RH_FACING_CHAMFERING_T08";
    tool8["name"] = "Right-Hand Facing / Chamfering";
    tool8["manufacturer"] = "Sandvik";
    tool8["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
    tool8["toolNumber"] = "T08";
    tool8["turretPosition"] = 8;
    tool8["isActive"] = true;
    tool8["toolOffset_X"] = 0.0;
    tool8["toolOffset_Z"] = 0.0;
    tool8["toolLengthOffset"] = 0.0;
    tool8["toolRadiusOffset"] = 0.4;
    tool8["expectedLifeMinutes"] = 360.0;
    tool8["usageMinutes"] = 0.0;
    tool8["cycleCount"] = 0;
    tool8["notes"] = "Right-hand facing and chamfering tool for finishing operations";
    tool8["internalThreading"] = false;
    tool8["externalThreading"] = false;
    tool8["internalBoring"] = false;
    tool8["partingGrooving"] = false;
    tool8["longitudinalTurning"] = false;
    tool8["facing"] = true;
    tool8["chamfering"] = true;
    
    // General Turning Insert
    QJsonObject turningInsert8;
    turningInsert8["isoCode"] = "CNMG120404";
    turningInsert8["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert8["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert8["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert8["sizeSpecifier"] = "12";
    turningInsert8["inscribedCircle"] = 12.7;
    turningInsert8["thickness"] = 4.76;
    turningInsert8["cornerRadius"] = 0.4;
    turningInsert8["cuttingEdgeLength"] = 12.7;
    turningInsert8["width"] = 12.7;
    turningInsert8["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert8["substrate"] = "WC-Co";
    turningInsert8["coating"] = "TiAlN";
    turningInsert8["manufacturer"] = "Sandvik";
    turningInsert8["partNumber"] = "CNMG 12 04 04-PM 4325";
    turningInsert8["rake_angle"] = 0.0;
    turningInsert8["inclination_angle"] = -6.0;
    turningInsert8["name"] = "CNMG120404 Facing/Chamfering";
    turningInsert8["isActive"] = true;
    tool8["turningInsert"] = turningInsert8;
    
    // Tool Holder
    QJsonObject holder8;
    holder8["isoCode"] = "MCLNR2525M12";
    holder8["handOrientation"] = static_cast<int>(HandOrientation::RIGHT_HAND);
    holder8["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder8["cuttingWidth"] = 25.0;
    holder8["headLength"] = 50.0;
    holder8["overallLength"] = 150.0;
    holder8["shankWidth"] = 25.0;
    holder8["shankHeight"] = 25.0;
    holder8["isRoundShank"] = false;
    holder8["insertSeatAngle"] = 95.0;
    holder8["insertSetback"] = 2.0;
    holder8["manufacturer"] = "Sandvik";
    tool8["holder"] = holder8;
    
    // Cutting Data
    QJsonObject cuttingData8;
    cuttingData8["constantSurfaceSpeed"] = true;
    cuttingData8["surfaceSpeed"] = 220.0;
    cuttingData8["spindleSpeed"] = 700;
    cuttingData8["feedPerRevolution"] = true;
    cuttingData8["cuttingFeedrate"] = 0.15;
    cuttingData8["plungeFeedrate"] = 0.08;
    cuttingData8["retractFeedrate"] = 4.0;
    cuttingData8["maxDepthOfCut"] = 2.0;
    cuttingData8["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool8["cuttingData"] = cuttingData8;
    
    toolsArray.append(tool8);
    
    // Tool 9: Neutral tool: longitudinal turning
    QJsonObject tool9;
    tool9["id"] = "NEUTRAL_LONGITUDINAL_TURNING_T09";
    tool9["name"] = "Neutral Longitudinal Turning";
    tool9["manufacturer"] = "Iscar";
    tool9["toolType"] = static_cast<int>(ToolType::GENERAL_TURNING);
    tool9["toolNumber"] = "T09";
    tool9["turretPosition"] = 9;
    tool9["isActive"] = true;
    tool9["toolOffset_X"] = 0.0;
    tool9["toolOffset_Z"] = 0.0;
    tool9["toolLengthOffset"] = 0.0;
    tool9["toolRadiusOffset"] = 0.4;
    tool9["expectedLifeMinutes"] = 480.0;
    tool9["usageMinutes"] = 0.0;
    tool9["cycleCount"] = 0;
    tool9["notes"] = "Neutral longitudinal turning tool for versatile machining";
    tool9["internalThreading"] = false;
    tool9["externalThreading"] = false;
    tool9["internalBoring"] = false;
    tool9["partingGrooving"] = false;
    tool9["longitudinalTurning"] = true;
    tool9["facing"] = false;
    tool9["chamfering"] = false;
    
    // General Turning Insert
    QJsonObject turningInsert9;
    turningInsert9["isoCode"] = "CNMG120408";
    turningInsert9["shape"] = static_cast<int>(InsertShape::DIAMOND_80);
    turningInsert9["reliefAngle"] = static_cast<int>(InsertReliefAngle::ANGLE_7);
    turningInsert9["tolerance"] = static_cast<int>(InsertTolerance::M_PRECISION);
    turningInsert9["sizeSpecifier"] = "12";
    turningInsert9["inscribedCircle"] = 12.7;
    turningInsert9["thickness"] = 4.76;
    turningInsert9["cornerRadius"] = 0.8;
    turningInsert9["cuttingEdgeLength"] = 12.7;
    turningInsert9["width"] = 12.7;
    turningInsert9["material"] = static_cast<int>(InsertMaterial::COATED_CARBIDE);
    turningInsert9["substrate"] = "WC-Co";
    turningInsert9["coating"] = "TiAlN";
    turningInsert9["manufacturer"] = "Iscar";
    turningInsert9["partNumber"] = "CNMG 12 04 08-IC 928";
    turningInsert9["rake_angle"] = 0.0;
    turningInsert9["inclination_angle"] = -6.0;
    turningInsert9["name"] = "CNMG120408 Neutral Turning";
    turningInsert9["isActive"] = true;
    tool9["turningInsert"] = turningInsert9;
    
    // Tool Holder
    QJsonObject holder9;
    holder9["isoCode"] = "MCLNN2525M12";
    holder9["handOrientation"] = static_cast<int>(HandOrientation::NEUTRAL);
    holder9["clampingStyle"] = static_cast<int>(ClampingStyle::TOP_CLAMP);
    holder9["cuttingWidth"] = 25.0;
    holder9["headLength"] = 50.0;
    holder9["overallLength"] = 150.0;
    holder9["shankWidth"] = 25.0;
    holder9["shankHeight"] = 25.0;
    holder9["isRoundShank"] = false;
    holder9["insertSeatAngle"] = 95.0;
    holder9["insertSetback"] = 2.0;
    holder9["manufacturer"] = "Iscar";
    tool9["holder"] = holder9;
    
    // Cutting Data
    QJsonObject cuttingData9;
    cuttingData9["constantSurfaceSpeed"] = true;
    cuttingData9["surfaceSpeed"] = 250.0;
    cuttingData9["spindleSpeed"] = 800;
    cuttingData9["feedPerRevolution"] = true;
    cuttingData9["cuttingFeedrate"] = 0.25;
    cuttingData9["plungeFeedrate"] = 0.1;
    cuttingData9["retractFeedrate"] = 5.0;
    cuttingData9["maxDepthOfCut"] = 3.0;
    cuttingData9["coolantType"] = static_cast<int>(CoolantType::FLOOD);
    tool9["cuttingData"] = cuttingData9;
    
    toolsArray.append(tool9);
    
    // Save to database
    QJsonObject database;
    database["tools"] = toolsArray;
    database["version"] = "1.0";
    database["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    database["description"] = "IntuiCAM Default Tool Library - Auto-created on first run";
    
    // Write to database file
    QString databasePath = getToolAssemblyDatabasePath();
    QJsonDocument doc(database);
    QFile file(databasePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Successfully created default tool database with 9 tools:" << databasePath;
    } else {
        qWarning() << "Failed to create default tool database:" << file.errorString();
    }
}

// Filter and utility methods
void ToolManagementTab::clearFilters() {
    showAllTools();
}

void ToolManagementTab::showAllTools() {
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        m_toolTreeWidget->topLevelItem(i)->setHidden(false);
    }
}

void ToolManagementTab::applyFilters() {
    QString searchText = m_searchBox->text().toLower();
    QString typeFilter = m_toolTypeFilter->currentText();
    QString materialFilter = m_materialFilter->currentText();
    QString statusFilter = m_statusFilter->currentText();
    
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_toolTreeWidget->topLevelItem(i);
        bool visible = true;
        
        // Apply search filter
        if (!searchText.isEmpty()) {
            QString itemText = item->text(COL_NAME).toLower() + " " +
                              item->text(COL_INSERT_TYPE).toLower() + " " +
                              item->text(COL_HOLDER_TYPE).toLower();
            visible = visible && itemText.contains(searchText);
        }
        
        // Apply type filter
        if (typeFilter != "All Types") {
            visible = visible && (item->text(COL_TYPE) == typeFilter);
        }
        
        // Apply status filter
        if (statusFilter != "All Status") {
            visible = visible && (item->text(COL_STATUS) == statusFilter);
        }
        
        item->setHidden(!visible);
    }
}

QString ToolManagementTab::getSelectedToolId() const {
    auto currentItem = m_toolTreeWidget->currentItem();
    return currentItem ? currentItem->data(COL_NAME, Qt::UserRole).toString() : QString();
}

void ToolManagementTab::displayToolInfo(const QString& toolId) {
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_toolTreeWidget->topLevelItem(i);
        if (item->data(COL_NAME, Qt::UserRole).toString() == toolId) {
            m_toolNameValue->setText(item->text(COL_NAME));
            m_toolTypeValue->setText(item->text(COL_TYPE));
            m_toolNumberValue->setText(item->text(COL_TOOL_NUMBER));
            m_turretPositionValue->setText(item->text(COL_TURRET_POS));
            m_toolStatusValue->setText(item->text(COL_STATUS));
            m_insertInfoValue->setText(item->text(COL_INSERT_TYPE));
            m_holderInfoValue->setText(item->text(COL_HOLDER_TYPE));
            m_toolLifeValue->setText(item->text(COL_USAGE));
            break;
        }
    }
}

void ToolManagementTab::clearToolInfo() {
    m_toolTypeValue->setText("-");
    m_toolNameValue->setText("-");
    m_toolNumberValue->setText("-");
    m_turretPositionValue->setText("-");
    m_toolStatusValue->setText("-");
    m_insertInfoValue->setText("-");
    m_holderInfoValue->setText("-");
    m_cuttingDataValue->setText("-");
    m_toolLifeValue->setText("-");
    m_lastUsedValue->setText("-");
}

void ToolManagementTab::updateToolCounts() {
    // Update status information
}

void ToolManagementTab::updateStatusBar() {
    // Update status bar information
}

QIcon ToolManagementTab::getToolTypeIcon(ToolType toolType) const {
    Q_UNUSED(toolType)
    return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
}

QColor ToolManagementTab::getToolStatusColor(bool isActive) const {
    return isActive ? QColor(0, 128, 0) : QColor(128, 128, 128);
}

QString ToolManagementTab::getToolStatusText(bool isActive) const {
    return isActive ? "Active" : "Inactive";
}

QString ToolManagementTab::formatToolType(ToolType toolType) const {
    switch (toolType) {
        case ToolType::GENERAL_TURNING: return "General Turning";
        case ToolType::BORING: return "Boring";
        case ToolType::THREADING: return "Threading";
        case ToolType::GROOVING: return "Grooving";
        case ToolType::PARTING: return "Parting";
        case ToolType::FORM_TOOL: return "Form Tool";
        case ToolType::LIVE_TOOLING: return "Live Tooling";
        default: return "Unknown";
    }
}

// Public slot implementations
void ToolManagementTab::onToolAdded(const QString& toolId) {
    Q_UNUSED(toolId)
    refreshToolList();
}

void ToolManagementTab::onToolModified(const QString& toolId) {
    Q_UNUSED(toolId)
    refreshToolList();
}

void ToolManagementTab::onToolDeleted(const QString& toolId) {
    Q_UNUSED(toolId)
    refreshToolList();
}

void ToolManagementTab::onToolLibraryUpdated() {
    refreshToolList();
}

// Filter implementations
void ToolManagementTab::filterByToolType(ToolType toolType) {
    QString typeText = formatToolType(toolType);
    int index = m_toolTypeFilter->findText(typeText);
    if (index >= 0) {
        m_toolTypeFilter->setCurrentIndex(index);
        applyFilters();
    }
}

void ToolManagementTab::filterByMaterial(InsertMaterial material) {
    QString materialText;
    switch (material) {
        case InsertMaterial::UNCOATED_CARBIDE: materialText = "Uncoated Carbide"; break;
        case InsertMaterial::COATED_CARBIDE: materialText = "Coated Carbide"; break;
        case InsertMaterial::CERMET: materialText = "Cermet"; break;
        case InsertMaterial::CERAMIC: materialText = "Ceramic"; break;
        case InsertMaterial::CBN: materialText = "CBN"; break;
        case InsertMaterial::PCD: materialText = "PCD"; break;
        case InsertMaterial::HSS: materialText = "HSS"; break;
        case InsertMaterial::CAST_ALLOY: materialText = "Cast Alloy"; break;
        case InsertMaterial::DIAMOND: materialText = "Diamond"; break;
        default: materialText = "All Materials"; break;
    }
    
    int index = m_materialFilter->findText(materialText);
    if (index >= 0) {
        m_materialFilter->setCurrentIndex(index);
        applyFilters();
    }
}

void ToolManagementTab::filterToolsByOperation(const QString& operationName) {
    // Clear current search and filters
    m_searchBox->clear();
    m_toolTypeFilter->setCurrentIndex(0); // "All Tool Types"
    m_materialFilter->setCurrentIndex(0); // "All Materials"
    
    // Set search text based on operation type to filter tools
    QString searchText;
    
    if (operationName == "Facing") {
        // Show tools suitable for facing operations
        m_toolTypeFilter->setCurrentText("General Turning");
        searchText = "facing";
    } else if (operationName == "Roughing" || operationName == "External Roughing") {
        // Show tools suitable for roughing operations  
        m_toolTypeFilter->setCurrentText("General Turning");
        searchText = "rough";
    } else if (operationName == "Finishing" || operationName == "External Finishing") {
        // Show tools suitable for finishing operations
        m_toolTypeFilter->setCurrentText("General Turning");
        searchText = "finish";
    } else if (operationName == "Internal Roughing") {
        // Show boring bars and internal turning tools
        m_toolTypeFilter->setCurrentText("Boring Bar");
        searchText = "internal";
    } else if (operationName == "Internal Finishing") {
        // Show boring bars and fine internal turning tools
        m_toolTypeFilter->setCurrentText("Boring Bar");
        searchText = "finish";
    } else if (operationName == "Drilling") {
        // Show drill tools
        m_toolTypeFilter->setCurrentText("Drill");
        searchText = "drill";
    } else if (operationName == "Grooving" || operationName == "Internal Grooving" || operationName == "External Grooving") {
        // Show grooving tools
        m_toolTypeFilter->setCurrentText("Grooving Tool");
        searchText = "groove";
    } else if (operationName == "Threading") {
        // Show threading tools
        m_toolTypeFilter->setCurrentText("Threading Tool");
        searchText = "thread";
    } else if (operationName == "Chamfering") {
        // Show chamfering and general turning tools
        m_toolTypeFilter->setCurrentText("General Turning");
        searchText = "chamfer";
    } else if (operationName == "Parting") {
        // Show parting tools
        m_toolTypeFilter->setCurrentText("Parting Tool");
        searchText = "part";
    }
    
    // Set the search text if we have one
    if (!searchText.isEmpty()) {
        m_searchBox->setText(searchText);
    }
    
    // Apply the filters
    applyFilters();
    
    // Show a message about the filtering
    if (m_toolTreeWidget->topLevelItemCount() == 0) {
        QMessageBox::information(this, "Tool Filter", 
            QString("No tools found suitable for %1 operation.\nConsider adding tools or clearing filters.").arg(operationName));
    }
}

// Missing method implementations for context menu actions
void ToolManagementTab::onEditToolAction() {
    editSelectedTool();
}

void ToolManagementTab::onDeleteToolAction() {
    deleteSelectedTool();
}

void ToolManagementTab::onDuplicateToolAction() {
    duplicateSelectedTool();
}

void ToolManagementTab::onToolPropertiesAction() {
    // Open a properties dialog for the selected tool
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        auto dialog = new ToolManagementDialog(toolId, this);
        
        // Get MaterialManager from parent and set it on the dialog
        auto mainWindow = qobject_cast<MainWindow*>(window());
        if (mainWindow && mainWindow->getMaterialManager()) {
            dialog->setMaterialManager(mainWindow->getMaterialManager());
        }
        
        // Connect save signal to refresh list
        connect(dialog, &ToolManagementDialog::toolSaved,
                this, [this](const QString&) {
                    refreshToolList();
                });
        connect(dialog, &ToolManagementDialog::toolNameChanged,
                this, [this](const QString& id, const QString& name) {
                    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
                        auto item = m_toolTreeWidget->topLevelItem(i);
                        if (item->data(COL_NAME, Qt::UserRole).toString() == id) {
                            item->setText(COL_NAME, name);
                            break;
                        }
                    }
                });
        
        dialog->exec();
        dialog->deleteLater();
    }
}

void ToolManagementTab::onSetActiveAction() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        // Find and update the selected tool item
        auto selectedItems = m_toolTreeWidget->selectedItems();
        if (!selectedItems.isEmpty()) {
            auto item = selectedItems.first();
            item->setText(COL_STATUS, "Active");
            
            // Update visual appearance
            QColor activeColor = getToolStatusColor(true);
            item->setForeground(COL_STATUS, QBrush(activeColor));
            
            // Update tool details if this tool is currently displayed
            if (item->text(COL_NAME) == m_toolNameValue->text()) {
                m_toolStatusValue->setText("Active");
                m_toolStatusValue->setStyleSheet("color: green; font-weight: bold;");
            }
            
            QMessageBox::information(this, "Tool Activated", 
                QString("Tool '%1' has been set to active status.").arg(toolId));
        }
        
        emit toolLibraryChanged();
    }
}

void ToolManagementTab::onSetInactiveAction() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        // Find and update the selected tool item
        auto selectedItems = m_toolTreeWidget->selectedItems();
        if (!selectedItems.isEmpty()) {
            auto item = selectedItems.first();
            item->setText(COL_STATUS, "Inactive");
            
            // Update visual appearance
            QColor inactiveColor = getToolStatusColor(false);
            item->setForeground(COL_STATUS, QBrush(inactiveColor));
            
            // Update tool details if this tool is currently displayed
            if (item->text(COL_NAME) == m_toolNameValue->text()) {
                m_toolStatusValue->setText("Inactive");
                m_toolStatusValue->setStyleSheet("color: gray; font-weight: bold;");
            }
            
            QMessageBox::information(this, "Tool Deactivated", 
                QString("Tool '%1' has been set to inactive status.").arg(toolId));
        }
        
        emit toolLibraryChanged();
    }
}

void ToolManagementTab::updateToolDetails() {
    QString selectedToolId = getSelectedToolId();
    if (!selectedToolId.isEmpty()) {
        displayToolInfo(selectedToolId);
    } else {
        clearToolInfo();
    }
}

QString ToolManagementTab::formatToolSummary(const ToolAssembly& tool) const {
    QString summary = QString("%1 - %2")
                        .arg(QString::fromStdString(tool.name))
                        .arg(formatToolType(tool.toolType));
    return summary;
}

bool ToolManagementTab::passesFilter(const ToolAssembly& tool) const {
    Q_UNUSED(tool)
    // Simple implementation - in a real system this would check against current filters
    return true;
}

void ToolManagementTab::updateToolListItem(const QString& toolId) {
    Q_UNUSED(toolId)
    // Find and update the specific tool item
    refreshToolList(); // For now, just refresh everything
}

void ToolManagementTab::removeToolListItem(const QString& toolId) {
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_toolTreeWidget->topLevelItem(i);
        if (item->data(COL_NAME, Qt::UserRole).toString() == toolId) {
            delete m_toolTreeWidget->takeTopLevelItem(i);
            break;
        }
    }
}

QStringList ToolManagementTab::getSelectedToolIds() const {
    QStringList toolIds;
    auto selectedItems = m_toolTreeWidget->selectedItems();
    for (auto item : selectedItems) {
        toolIds << item->data(COL_NAME, Qt::UserRole).toString();
    }
    return toolIds;
}

QString ToolManagementTab::getToolAssemblyDatabasePath() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(dataDir);
    }
    
    QString dbPath = dir.filePath("tool_assemblies.json");
    return dbPath;
}
