#ifndef OPERATIONTILEWIDGET_H
#define OPERATIONTILEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QAction>

namespace IntuiCAM {
namespace GUI {

class OperationTileWidget : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

public:
    explicit OperationTileWidget(const QString& operationName, bool enabledByDefault = false, QWidget* parent = nullptr);
    ~OperationTileWidget();

    // State management
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    
    // Visual properties
    QString operationName() const { return m_operationName; }
    void setIcon(const QString& iconPath);
    void setDescription(const QString& description);
    
    // Tool selection
    void setSelectedTool(const QString& toolName);
    QString selectedTool() const { return m_selectedTool; }
    
    // Sub-tiles for Internal Features
    void addSubTile(OperationTileWidget* subTile);
    void removeSubTile(OperationTileWidget* subTile);
    QList<OperationTileWidget*> subTiles() const { return m_subTiles; }
    bool hasSubTiles() const { return !m_subTiles.isEmpty(); }
    
    // Expanded state for Internal Features
    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);
    
    // Selection state
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    // Color properties for animations
    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    // Size policy
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void enabledChanged(const QString& operationName, bool enabled);
    void clicked(const QString& operationName);
    void toolSelectionRequested(const QString& operationName);
    void expandedChanged(const QString& operationName, bool expanded);

private slots:
    void onAnimationFinished();
    void onToolSelectionRequested();

private:
    void setupUI();
    void updateColors();
    void animateToState(bool enabled);
    void updateSubTilesVisibility();
    void updateIconSize();
    
    // Visual state
    QString m_operationName;
    QString m_description;
    QString m_iconPath;
    QString m_selectedTool;
    bool m_enabled;
    bool m_expanded;
    bool m_isHovered;
    bool m_selected;
    
    // Sub-tiles for Internal Features
    QList<OperationTileWidget*> m_subTiles;
    QWidget* m_subTileContainer;
    QVBoxLayout* m_subTileLayout;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QLabel* m_iconLabel;
    QLabel* m_nameLabel;
    QLabel* m_descriptionLabel;
    QLabel* m_toolLabel;
    
    // Animation and effects
    QPropertyAnimation* m_colorAnimation;
    QGraphicsDropShadowEffect* m_shadowEffect;
    
    // Colors
    QColor m_backgroundColor;
    QColor m_enabledColor;
    QColor m_disabledColor;
    QColor m_hoverColor;
    QColor m_textColor;
    QColor m_borderColor;
    QColor m_selectionBorderColor;

    // Icon sizes
    int m_defaultIconSize;
    int m_highlightedIconSize;
    
    // Context menu
    QMenu* m_contextMenu;
    QAction* m_selectToolAction;
    QAction* m_toggleAction;
};

class OperationTileContainer : public QWidget
{
    Q_OBJECT

public:
    explicit OperationTileContainer(QWidget* parent = nullptr);
    ~OperationTileContainer();

    // Tile management
    void addTile(OperationTileWidget* tile);
    void removeTile(OperationTileWidget* tile);
    OperationTileWidget* getTile(const QString& operationName) const;
    QList<OperationTileWidget*> getAllTiles() const { return m_tiles; }
    
    // State management
    void setTileEnabled(const QString& operationName, bool enabled);
    bool isTileEnabled(const QString& operationName) const;
    QStringList getEnabledOperations() const;
    
    // Tool selection
    void setTileSelectedTool(const QString& operationName, const QString& toolName);
    QString getTileSelectedTool(const QString& operationName) const;
    
    // Selection management
    void setSelectedOperation(const QString& operationName);
    QString getSelectedOperation() const;
    void clearSelection();

signals:
    void operationEnabledChanged(const QString& operationName, bool enabled);
    void operationClicked(const QString& operationName);
    void operationToolSelectionRequested(const QString& operationName);
    void operationExpandedChanged(const QString& operationName, bool expanded);

private slots:
    void onTileEnabledChanged(const QString& operationName, bool enabled);
    void onTileClicked(const QString& operationName);
    void onTileToolSelectionRequested(const QString& operationName);
    void onTileExpandedChanged(const QString& operationName, bool expanded);

private:
    void setupUI();
    void arrangeInternalFeatures();
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_primaryRowLayout;
    QHBoxLayout* m_secondaryRowLayout;
    QList<OperationTileWidget*> m_tiles;
    
    // Special handling for Internal Features
    OperationTileWidget* m_internalFeaturesTile;
    
    // Selection tracking
    QString m_selectedOperation;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // OPERATIONTILEWIDGET_H 