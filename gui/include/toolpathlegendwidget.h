#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QPainter>
#include <QPaintEvent>

#include <IntuiCAM/Toolpath/Types.h>

class ToolpathLegendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ToolpathLegendWidget(QWidget *parent = nullptr);
    ~ToolpathLegendWidget() override = default;

    // Control visibility
    void setVisible(bool visible) override;
    
    // Update legend based on available operations
    void updateLegendForOperations(const std::vector<IntuiCAM::Toolpath::OperationType>& operations);
    
    // Show/hide specific operation types
    void setOperationVisible(IntuiCAM::Toolpath::OperationType operation, bool visible);
    
    // Legend appearance
    void setCompactMode(bool compact);
    void setColorSquareSize(int size);

signals:
    void operationClicked(IntuiCAM::Toolpath::OperationType operation);
    void operationVisibilityChanged(IntuiCAM::Toolpath::OperationType operation, bool visible);

private slots:
    void onOperationClicked();

private:
    // UI setup
    void setupUI();
    void createOperationEntry(IntuiCAM::Toolpath::OperationType operation);
    
    // Color utilities  
    QColor getOperationColor(IntuiCAM::Toolpath::OperationType operation) const;
    QString getOperationDescription(IntuiCAM::Toolpath::OperationType operation) const;
    QString getOperationTooltip(IntuiCAM::Toolpath::OperationType operation) const;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    
    // Settings
    bool m_compactMode;
    int m_colorSquareSize;
    
    // Operation tracking
    std::map<IntuiCAM::Toolpath::OperationType, QWidget*> m_operationWidgets;
    std::map<IntuiCAM::Toolpath::OperationType, bool> m_operationVisibility;
};

// Color square widget for displaying operation colors
class ColorSquareWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorSquareWidget(const QColor& color, int size = 16, QWidget* parent = nullptr);
    
    void setColor(const QColor& color);
    void setSize(int size);
    
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_color;
    int m_size;
};

// Clickable operation entry widget
class OperationEntryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperationEntryWidget(IntuiCAM::Toolpath::OperationType operation, 
                                  const QColor& color, 
                                  const QString& name, 
                                  const QString& description,
                                  bool compact = false,
                                  QWidget* parent = nullptr);
    
    IntuiCAM::Toolpath::OperationType getOperationType() const { return m_operation; }
    void setVisible(bool visible);
    bool isOperationVisible() const { return m_operationVisible; }

signals:
    void clicked(IntuiCAM::Toolpath::OperationType operation);
    void visibilityToggled(IntuiCAM::Toolpath::OperationType operation, bool visible);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle();
    
    IntuiCAM::Toolpath::OperationType m_operation;
    QColor m_color;
    QString m_name;
    QString m_description;
    bool m_compact;
    bool m_operationVisible;
    bool m_hovered;
    
    ColorSquareWidget* m_colorSquare;
    QLabel* m_nameLabel;
    QLabel* m_descriptionLabel;
    QHBoxLayout* m_layout;
}; 