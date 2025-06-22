#ifndef TOOLMANAGEMENTTAB_H
#define TOOLMANAGEMENTTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QFrame>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QProgressBar>
#include <QTimer>

// Forward declarations
class ToolManagementDialog;

namespace IntuiCAM {
namespace Toolpath {
    struct ToolAssembly;
    enum class ToolType;
    enum class InsertMaterial;
}
}

class ToolManagementTab : public QWidget
{
    Q_OBJECT

public:
    explicit ToolManagementTab(QWidget *parent = nullptr);
    ~ToolManagementTab();

    // Tool management operations
    void refreshToolList();
    void selectTool(const QString& toolId);
    void addNewTool();
    void editSelectedTool();
    void deleteSelectedTool();
    void duplicateSelectedTool();
    
    // Tool filtering and display
    void filterByToolType(IntuiCAM::Toolpath::ToolType toolType);
    void filterByMaterial(IntuiCAM::Toolpath::InsertMaterial material);
    void clearFilters();
    void showAllTools();
    
    // Tool library operations
    void importToolLibrary();
    void exportToolLibrary();
    void loadDefaultTools();
    void ensureDefaultToolsExist();

signals:
    void toolSelected(const QString& toolId);
    void toolDoubleClicked(const QString& toolId);
    void toolContextMenuRequested(const QString& toolId, const QPoint& position);
    void toolLibraryChanged();
    void errorOccurred(const QString& message);
    
    // Tool management signals
    void toolAdded(const QString& toolId);
    void toolModified(const QString& toolId);
    void toolDeleted(const QString& toolId);

public slots:
    void onToolAdded(const QString& toolId);
    void onToolModified(const QString& toolId);
    void onToolDeleted(const QString& toolId);
    void onToolLibraryUpdated();

private slots:
    // UI interaction slots
    void onToolListSelectionChanged();
    void onToolListDoubleClicked();
    void onToolListContextMenuRequested(const QPoint& pos);
    void onSearchTextChanged(const QString& text);
    void onFilterChanged();
    void onRefreshRequested();
    
    // Toolbar actions
    void onAddToolTriggered();
    void onEditToolTriggered();
    void onDeleteToolTriggered();
    void onDuplicateToolTriggered();
    void onImportLibraryTriggered();
    void onExportLibraryTriggered();
    void onLoadDefaultsTriggered();
    
    // Context menu actions
    void onEditToolAction();
    void onDeleteToolAction();
    void onDuplicateToolAction();
    void onToolPropertiesAction();
    void onSetActiveAction();
    void onSetInactiveAction();
    
    // Update and refresh
    void updateToolDetails();
    void updateToolCounts();
    void updateStatusBar();

private:
    // UI setup methods
    void setupUI();
    void createToolbar();
    void createFilterPanel();
    void createToolListWidget();
    void createToolDetailsPanel();
    void createStatusPanel();
    void setupConnections();
    void setupContextMenu();
    
    // Tool list management
    void populateToolList();
    void createDefaultToolDatabase();
    void updateToolListItem(const QString& toolId);
    void removeToolListItem(const QString& toolId);
    QString getSelectedToolId() const;
    QStringList getSelectedToolIds() const;
    
    // Tool filtering
    void applyFilters();
    bool passesFilter(const IntuiCAM::Toolpath::ToolAssembly& tool) const;
    
    // Tool information display
    void displayToolInfo(const QString& toolId);
    void clearToolInfo();
    QString formatToolSummary(const IntuiCAM::Toolpath::ToolAssembly& tool) const;
    QString formatToolType(IntuiCAM::Toolpath::ToolType toolType) const;
    
    // Utility methods
    QIcon getToolTypeIcon(IntuiCAM::Toolpath::ToolType toolType) const;
    QColor getToolStatusColor(bool isActive) const;
    QString getToolStatusText(bool isActive) const;
    QString getToolAssemblyDatabasePath() const;
    
    // UI Components - Main Layout
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;
    
    // Toolbar
    QHBoxLayout* m_toolbarLayout;
    QFrame* m_toolbarFrame;
    QPushButton* m_addToolButton;
    QPushButton* m_editToolButton;
    QPushButton* m_deleteToolButton;
    QPushButton* m_duplicateToolButton;
    QToolButton* m_moreActionsButton;
    QMenu* m_moreActionsMenu;
    QAction* m_importLibraryAction;
    QAction* m_exportLibraryAction;
    QAction* m_loadDefaultsAction;
    QAction* m_refreshAction;
    
    // Filter Panel
    QGroupBox* m_filterPanel;
    QHBoxLayout* m_filterLayout;
    QLineEdit* m_searchBox;
    QComboBox* m_toolTypeFilter;
    QComboBox* m_materialFilter;
    QComboBox* m_statusFilter;
    QPushButton* m_clearFiltersButton;
    
    // Tool List Widget
    QWidget* m_toolListWidget;
    QVBoxLayout* m_toolListLayout;
    QTreeWidget* m_toolTreeWidget;
    
    // Tool Details Panel
    QWidget* m_toolDetailsPanel;
    QVBoxLayout* m_toolDetailsLayout;
    QLabel* m_toolDetailsTitle;
    QLabel* m_toolSummaryLabel;
    QFrame* m_toolInfoFrame;
    QGridLayout* m_toolInfoLayout;
    
    // Tool information labels
    QLabel* m_toolTypeLabel;
    QLabel* m_toolTypeValue;
    QLabel* m_toolNameLabel;
    QLabel* m_toolNameValue;
    QLabel* m_toolNumberLabel;
    QLabel* m_toolNumberValue;
    QLabel* m_turretPositionLabel;
    QLabel* m_turretPositionValue;
    QLabel* m_toolStatusLabel;
    QLabel* m_toolStatusValue;
    QLabel* m_insertInfoLabel;
    QLabel* m_insertInfoValue;
    QLabel* m_holderInfoLabel;
    QLabel* m_holderInfoValue;
    QLabel* m_cuttingDataLabel;
    QLabel* m_cuttingDataValue;
    QLabel* m_toolLifeLabel;
    QLabel* m_toolLifeValue;
    QLabel* m_lastUsedLabel;
    QLabel* m_lastUsedValue;
    QLabel* m_notesLabel;
    QLabel* m_notesValue;
    
    // Status Panel
    QFrame* m_statusPanel;
    QHBoxLayout* m_statusLayout;
    QLabel* m_toolCountLabel;
    QLabel* m_activeToolsLabel;
    QLabel* m_statusMessageLabel;
    QProgressBar* m_operationProgressBar;
    
    // Context Menu
    QMenu* m_contextMenu;
    QAction* m_editToolAction;
    QAction* m_deleteToolAction;
    QAction* m_duplicateToolAction;
    QAction* m_toolPropertiesAction;
    QAction* m_setActiveAction;
    QAction* m_setInactiveAction;
    
    // Tool Management Dialog
    ToolManagementDialog* m_toolDialog;
    
    // Data members
    QString m_currentToolId;
    QString m_currentSearchText;
    IntuiCAM::Toolpath::ToolType m_currentToolTypeFilter;
    IntuiCAM::Toolpath::InsertMaterial m_currentMaterialFilter;
    bool m_showActiveOnly;
    
    // Update timer
    QTimer* m_updateTimer;
    
    // Constants
    static const int UPDATE_DELAY_MS = 300;
    static const QString NO_TOOL_SELECTED_TEXT;
    static const QString LOADING_TEXT;
    
    // Tree widget columns
    enum ToolTreeColumns {
        COL_NAME = 0,
        COL_TYPE,
        COL_TOOL_NUMBER,
        COL_TURRET_POS,
        COL_STATUS,
        COL_INSERT_TYPE,
        COL_HOLDER_TYPE,
        COL_USAGE,
        COL_COUNT
    };
};

#endif // TOOLMANAGEMENTTAB_H 