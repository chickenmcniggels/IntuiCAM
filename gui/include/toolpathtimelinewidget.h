#ifndef TOOLPATHTIMELINEWIDGET_H
#define TOOLPATHTIMELINEWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QVector>
#include <QMap>
#include <QMessageBox>

namespace IntuiCAM {
namespace GUI {
class OperationParameterDialog;
}
namespace Toolpath {
class Operation;
}
}

/**
 * @brief A timeline widget that displays toolpaths in sequence order
 * 
 * This widget displays a horizontal timeline of toolpath operations in
 * their execution order. Each toolpath can be clicked to open a parameter
 * dialog to adjust settings. The timeline provides a visual representation
 * of the machining sequence and allows for adding new toolpaths.
 */
class ToolpathTimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ToolpathTimelineWidget(QWidget *parent = nullptr);
    ~ToolpathTimelineWidget();

    /**
     * @brief Add a toolpath to the timeline
     * @param operationName The display name of the operation
     * @param operationType The type of operation (facing, roughing, etc.)
     * @param toolName The name of the tool used
     * @param icon Optional icon path for the operation
     * @return Index of the added toolpath
     */
    int addToolpath(const QString& operationName, 
                   const QString& operationType,
                   const QString& toolName,
                   const QString& icon = QString());

    /**
     * @brief Remove a toolpath from the timeline
     * @param index The index of the toolpath to remove
     */
    void removeToolpath(int index);

    /**
     * @brief Clear all toolpaths from the timeline
     */
    void clearToolpaths();

    /**
     * @brief Update an existing toolpath entry
     * @param index The index of the toolpath to update
     * @param operationName The new operation name
     * @param operationType The new operation type
     * @param toolName The new tool name
     * @param icon Optional new icon path
     */
    void updateToolpath(int index,
                       const QString& operationName,
                       const QString& operationType,
                       const QString& toolName,
                       const QString& icon = QString());

    /**
     * @brief Get the number of toolpaths in the timeline
     * @return Count of toolpath entries
     */
    int getToolpathCount() const { return m_toolpathFrames.size(); }

    /**
     * @brief Get the type of a toolpath at the specified index
     * @param index The index of the toolpath
     * @return The type of the toolpath
     */
    QString getToolpathType(int index) const;

    /**
     * @brief Get the name of a toolpath at the specified index
     * @param index The index of the toolpath
     * @return The name of the toolpath
     */
    QString getToolpathName(int index) const;

    /**
     * @brief Check if a toolpath is currently visible/enabled
     * @param index The index of the toolpath to check
     * @return True if the toolpath is visible, false otherwise
     */
    bool isToolpathVisible(int index) const { return index >= 0 && index < m_toolpathFrames.size(); }

    /**
     * @brief Set the current active toolpath
     * @param index The index of the toolpath to set as active
     */
    void setActiveToolpath(int index);

    /**
     * @brief Check if a toolpath is enabled
     */
    bool isToolpathEnabled(int index) const;

    /**
     * @brief Enable or disable a toolpath
     */
    void setToolpathEnabled(int index, bool enabled);

public slots:
    /**
     * @brief Handle click on the add toolpath button
     */
    void onAddToolpathClicked();
    
    /**
     * @brief Handle parameter editing for a toolpath
     * @param index The index of the toolpath to edit parameters for
     */
    void onToolpathParameterEdit(int index);

signals:
    /**
     * @brief Emitted when a toolpath is selected
     * @param index The index of the selected toolpath
     */
    void toolpathSelected(int index);

    /**
     * @brief Emitted when a toolpath's parameters should be edited
     * @param index The index of the toolpath to edit
     * @param operationType The type of operation to edit
     */
    void toolpathParametersRequested(int index, const QString& operationType);

    /**
     * @brief Emitted when a new toolpath should be added
     * @param operationType The type of operation to add
     */
    void addToolpathRequested(const QString& operationType);

    /**
     * @brief Emitted when a toolpath should be removed
     * @param index The index of the toolpath to remove
     */
    void removeToolpathRequested(int index);

    /**
     * @brief Emitted when toolpaths are reordered
     * @param fromIndex Original position
     * @param toIndex New position
     */
    void toolpathReordered(int fromIndex, int toIndex);
    
    /**
     * @brief Emitted when a toolpath should be regenerated
     * @param index The index of the toolpath to regenerate
     */
    void toolpathRegenerateRequested(int index);

    /**
     * @brief Emitted when a toolpath is enabled or disabled
     */
    void toolpathEnabledChanged(int index, bool enabled);

private slots:
    /**
     * @brief Handle click on a toolpath frame
     * @param index The index of the clicked toolpath
     */
    void onToolpathClicked(int index);

    /**
     * @brief Handle right click on a toolpath frame
     * @param index The index of the right-clicked toolpath
     * @param pos The position of the right click
     */
    void onToolpathRightClicked(int index, const QPoint& pos);

    /**
     * @brief Handle selection of an operation type from the add menu
     */
    void onOperationTypeSelected();

private:
    /**
     * @brief Create a new toolpath frame
     * @param operationName The name of the operation
     * @param operationType The type of operation (facing, roughing, etc.)
     * @param toolName The name of the tool used
     * @param icon Optional icon path for the operation
     * @return The created frame widget
     */
    QFrame* createToolpathFrame(const QString& operationName,
                              const QString& operationType,
                              const QString& toolName,
                              const QString& icon);

    /**
     * @brief Update the visual appearance of toolpath frames
     */
    void updateToolpathFrameStyles();

    /**
     * @brief Create the add toolpath button and menu
     */
    void createAddToolpathButton();

    /**
     * @brief Event filter for handling events on child widgets
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    // UI components
    QScrollArea* m_scrollArea;
    QWidget* m_timelineContainer;
    QHBoxLayout* m_timelineLayout;
    QPushButton* m_addToolpathButton;
    QMenu* m_addToolpathMenu;

    // Toolpath data
    QVector<QFrame*> m_toolpathFrames;
    QVector<QString> m_toolpathTypes;
    QVector<QString> m_toolpathNames;
    QVector<QCheckBox*> m_enabledChecks;
    int m_activeToolpathIndex;

    // Standard operation types
    QStringList m_standardOperations;

protected:
    /**
     * @brief Process events for the widget
     */
    bool event(QEvent* event) override;
};

#endif // TOOLPATHTIMELINEWIDGET_H 