#include "toolmanagementtab.h"
#include "toolmanagementdialog.h"
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
    
    // Sample tools for demonstration
    auto item1 = new QTreeWidgetItem(m_toolTreeWidget);
    item1->setText(COL_NAME, "CNMG120408 General Turn");
    item1->setText(COL_TYPE, "General Turning");
    item1->setText(COL_TOOL_NUMBER, "T01");
    item1->setText(COL_TURRET_POS, "1");
    item1->setText(COL_STATUS, "Active");
    item1->setText(COL_INSERT_TYPE, "CNMG120408");
    item1->setText(COL_HOLDER_TYPE, "MCLNR2525M12");
    item1->setText(COL_USAGE, "85%");
    
    auto item2 = new QTreeWidgetItem(m_toolTreeWidget);
    item2->setText(COL_NAME, "16ER Threading Tool");
    item2->setText(COL_TYPE, "Threading");
    item2->setText(COL_TOOL_NUMBER, "T02");
    item2->setText(COL_TURRET_POS, "2");
    item2->setText(COL_STATUS, "Active");
    item2->setText(COL_INSERT_TYPE, "16ER1.0ISO");
    item2->setText(COL_HOLDER_TYPE, "SER1616H16");
    item2->setText(COL_USAGE, "45%");
    
    m_toolTreeWidget->resizeColumnToContents(0);
}

// Slot implementations
void ToolManagementTab::onToolListSelectionChanged() {
    auto currentItem = m_toolTreeWidget->currentItem();
    bool hasSelection = (currentItem != nullptr);
    
    m_editToolButton->setEnabled(hasSelection);
    m_deleteToolButton->setEnabled(hasSelection);
    m_duplicateToolButton->setEnabled(hasSelection);
    
    if (hasSelection) {
        QString toolId = currentItem->text(COL_NAME);
        displayToolInfo(toolId);
        emit toolSelected(toolId);
    } else {
        clearToolInfo();
    }
}

void ToolManagementTab::onToolListDoubleClicked() {
    auto currentItem = m_toolTreeWidget->currentItem();
    if (currentItem) {
        QString toolId = currentItem->text(COL_NAME);
        emit toolDoubleClicked(toolId);
        editSelectedTool();
    }
}

void ToolManagementTab::onToolListContextMenuRequested(const QPoint& pos) {
    auto item = m_toolTreeWidget->itemAt(pos);
    if (item) {
        QString toolId = item->text(COL_NAME);
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
        if (item->text(COL_NAME) == toolId) {
            m_toolTreeWidget->setCurrentItem(item);
            break;
        }
    }
}

void ToolManagementTab::addNewTool() {
    auto dialog = new ToolManagementDialog(this);
    
    // Set dialog window properties
    dialog->setWindowTitle("Add New Tool");
    dialog->setModal(true);
    dialog->resize(800, 600);
    
    // Connect signals to handle new tool creation
    connect(dialog, &ToolManagementDialog::toolAdded,
            this, [this](const QString& toolId) {
                qDebug() << "Tool added signal received for:" << toolId;
                refreshToolList();
                selectTool(toolId); // Select the newly added tool
                emit toolAdded(toolId);
            });
    
    connect(dialog, &ToolManagementDialog::toolLibraryChanged,
            this, &ToolManagementTab::onToolLibraryUpdated);
    
    // Connect error handling
    connect(dialog, &ToolManagementDialog::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Tool Management Error", error);
                emit errorOccurred(error);
            });
    
    // Set up for new tool creation
    dialog->addNewTool(IntuiCAM::Toolpath::ToolType::GENERAL_TURNING);
    
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

void ToolManagementTab::editSelectedTool() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        auto dialog = new ToolManagementDialog(this);
        
        // Set dialog window properties
        dialog->setWindowTitle(QString("Edit Tool: %1").arg(toolId));
        dialog->setModal(true);
        dialog->resize(800, 600);
        
        // Connect signals
        connect(dialog, &ToolManagementDialog::toolModified,
                this, [this](const QString& modifiedToolId) {
                    qDebug() << "Tool modified signal received for:" << modifiedToolId;
                    refreshToolList();
                    selectTool(modifiedToolId); // Re-select the modified tool
                    emit toolModified(modifiedToolId);
                });
        
        connect(dialog, &ToolManagementDialog::toolLibraryChanged,
                this, &ToolManagementTab::onToolLibraryUpdated);
        
        // Connect error handling
        connect(dialog, &ToolManagementDialog::errorOccurred,
                this, [this](const QString& error) {
                    QMessageBox::warning(this, "Tool Management Error", error);
                    emit errorOccurred(error);
                });
        
        // Set up for editing existing tool
        dialog->editTool(toolId);
        
        // Show dialog and handle result
        int result = dialog->exec();
        if (result == QDialog::Accepted) {
            qDebug() << "Tool edit dialog accepted, refreshing tool list";
            refreshToolList();
            updateToolCounts();
            emit toolLibraryChanged();
        }
        
        dialog->deleteLater();
    } else {
        QMessageBox::information(this, "No Tool Selected", 
            "Please select a tool to edit from the list.");
    }
}

void ToolManagementTab::deleteSelectedTool() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        auto reply = QMessageBox::question(this, "Delete Tool",
            QString("Are you sure you want to delete tool '%1'?").arg(toolId),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            // Remove tool from the tree widget
            auto selectedItems = m_toolTreeWidget->selectedItems();
            if (!selectedItems.isEmpty()) {
                auto item = selectedItems.first();
                int index = m_toolTreeWidget->indexOfTopLevelItem(item);
                if (index >= 0) {
                    delete m_toolTreeWidget->takeTopLevelItem(index);
                }
            }
            
            // Clear tool details display
            clearToolInfo();
            
            // Update button states
            m_editToolButton->setEnabled(false);
            m_deleteToolButton->setEnabled(false);
            m_duplicateToolButton->setEnabled(false);
            
            emit toolDeleted(toolId);
            updateToolCounts();
            emit toolLibraryChanged();
        }
    }
}

void ToolManagementTab::duplicateSelectedTool() {
    QString toolId = getSelectedToolId();
    if (!toolId.isEmpty()) {
        // Find the original tool item
        QTreeWidgetItem* originalItem = nullptr;
        for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
            auto item = m_toolTreeWidget->topLevelItem(i);
            if (item->text(COL_NAME) == toolId) {
                originalItem = item;
                break;
            }
        }
        
        if (originalItem) {
            // Create duplicate with modified name
            QTreeWidgetItem* duplicateItem = new QTreeWidgetItem(m_toolTreeWidget);
            duplicateItem->setText(COL_NAME, originalItem->text(COL_NAME) + " Copy");
            duplicateItem->setText(COL_TYPE, originalItem->text(COL_TYPE));
            duplicateItem->setText(COL_TOOL_NUMBER, "T99"); // Temporary tool number
            duplicateItem->setText(COL_TURRET_POS, "99");
            duplicateItem->setText(COL_STATUS, "Inactive"); // Start as inactive
            duplicateItem->setText(COL_INSERT_TYPE, originalItem->text(COL_INSERT_TYPE));
            duplicateItem->setText(COL_HOLDER_TYPE, originalItem->text(COL_HOLDER_TYPE));
            duplicateItem->setText(COL_USAGE, "0/480 min"); // Reset usage
            
            // Select the new item
            m_toolTreeWidget->setCurrentItem(duplicateItem);
            
            emit toolAdded(duplicateItem->text(COL_NAME));
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
        "This will replace your current tool library with default tools. Continue?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Clear existing tools
        m_toolTreeWidget->clear();
        
        // Add comprehensive default tool set with realistic specifications
        QTreeWidgetItem* tool1 = new QTreeWidgetItem(m_toolTreeWidget);
        tool1->setText(COL_NAME, "General Turning CNMG");
        tool1->setText(COL_TYPE, "General Turning");
        tool1->setText(COL_TOOL_NUMBER, "T01");
        tool1->setText(COL_TURRET_POS, "1");
        tool1->setText(COL_STATUS, "Active");
        tool1->setText(COL_INSERT_TYPE, "CNMG120408");
        tool1->setText(COL_HOLDER_TYPE, "MCLNR2525M12");
        tool1->setText(COL_USAGE, "120/480 min");
        tool1->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
        
        QTreeWidgetItem* tool2 = new QTreeWidgetItem(m_toolTreeWidget);
        tool2->setText(COL_NAME, "Threading 16ER");
        tool2->setText(COL_TYPE, "Threading");
        tool2->setText(COL_TOOL_NUMBER, "T02");
        tool2->setText(COL_TURRET_POS, "2");
        tool2->setText(COL_STATUS, "Active");
        tool2->setText(COL_INSERT_TYPE, "16ER1.0ISO");
        tool2->setText(COL_HOLDER_TYPE, "SER2525M16");
        tool2->setText(COL_USAGE, "45/240 min");
        tool2->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
        
        QTreeWidgetItem* tool3 = new QTreeWidgetItem(m_toolTreeWidget);
        tool3->setText(COL_NAME, "Grooving GTN");
        tool3->setText(COL_TYPE, "Grooving");
        tool3->setText(COL_TOOL_NUMBER, "T03");
        tool3->setText(COL_TURRET_POS, "3");
        tool3->setText(COL_STATUS, "Active");
        tool3->setText(COL_INSERT_TYPE, "GTN3");
        tool3->setText(COL_HOLDER_TYPE, "MGEHR2525-3");
        tool3->setText(COL_USAGE, "30/180 min");
        tool3->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
        
        QTreeWidgetItem* tool4 = new QTreeWidgetItem(m_toolTreeWidget);
        tool4->setText(COL_NAME, "Parting MGMN");
        tool4->setText(COL_TYPE, "Parting");
        tool4->setText(COL_TOOL_NUMBER, "T04");
        tool4->setText(COL_TURRET_POS, "4");
        tool4->setText(COL_STATUS, "Inactive");
        tool4->setText(COL_INSERT_TYPE, "MGMN300");
        tool4->setText(COL_HOLDER_TYPE, "MGEHR2525-3");
        tool4->setText(COL_USAGE, "0/120 min");
        tool4->setForeground(COL_STATUS, QBrush(getToolStatusColor(false)));
        
        QTreeWidgetItem* tool5 = new QTreeWidgetItem(m_toolTreeWidget);
        tool5->setText(COL_NAME, "Boring CCMT");
        tool5->setText(COL_TYPE, "Boring");
        tool5->setText(COL_TOOL_NUMBER, "T05");
        tool5->setText(COL_TURRET_POS, "5");
        tool5->setText(COL_STATUS, "Active");
        tool5->setText(COL_INSERT_TYPE, "CCMT09T308");
        tool5->setText(COL_HOLDER_TYPE, "A20R-SCLCR09");
        tool5->setText(COL_USAGE, "85/360 min");
        tool5->setForeground(COL_STATUS, QBrush(getToolStatusColor(true)));
        
        // Resize columns to fit content
        for (int i = 0; i < m_toolTreeWidget->columnCount(); ++i) {
            m_toolTreeWidget->resizeColumnToContents(i);
        }
        
        updateToolCounts();
        clearToolInfo();
        emit toolLibraryChanged();
        
        QMessageBox::information(this, "Default Tools Loaded", 
            "Successfully loaded 5 default tools into the library.");
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
    return currentItem ? currentItem->text(COL_NAME) : QString();
}

void ToolManagementTab::displayToolInfo(const QString& toolId) {
    // Update tool details display
    m_toolNameValue->setText(toolId);
    
    // Find the tool item
    for (int i = 0; i < m_toolTreeWidget->topLevelItemCount(); ++i) {
        auto item = m_toolTreeWidget->topLevelItem(i);
        if (item->text(COL_NAME) == toolId) {
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
        auto dialog = new ToolManagementDialog(this);
        dialog->editTool(toolId);
        dialog->exec();
        dialog->deleteLater();
        
        refreshToolList();
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
        if (item->text(COL_NAME) == toolId) {
            delete m_toolTreeWidget->takeTopLevelItem(i);
            break;
        }
    }
}

QStringList ToolManagementTab::getSelectedToolIds() const {
    QStringList toolIds;
    auto selectedItems = m_toolTreeWidget->selectedItems();
    for (auto item : selectedItems) {
        toolIds << item->text(COL_NAME);
    }
    return toolIds;
}
