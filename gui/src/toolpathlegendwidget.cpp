#include "toolpathlegendwidget.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QMouseEvent>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QDebug>

// =============================================================================
// ToolpathLegendWidget Implementation
// =============================================================================

ToolpathLegendWidget::ToolpathLegendWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_compactMode(false)
    , m_colorSquareSize(16)
{
    setupUI();
    
    // Initialize with all operation types visible by default
    for (int i = static_cast<int>(IntuiCAM::Toolpath::OperationType::Facing); 
         i <= static_cast<int>(IntuiCAM::Toolpath::OperationType::Parting); ++i) {
        auto operation = static_cast<IntuiCAM::Toolpath::OperationType>(i);
        if (operation != IntuiCAM::Toolpath::OperationType::Unknown) {
            m_operationVisibility[operation] = true;
            createOperationEntry(operation);
        }
    }
}

void ToolpathLegendWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(2);
    
    // Create title
    QLabel* titleLabel = new QLabel("Toolpath Operations", this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(titleLabel);
    
    // Create scroll area for the legend entries
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameStyle(QFrame::NoFrame);
    
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(2, 2, 2, 2);
    m_contentLayout->setSpacing(1);
    
    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);
    
    // Set reasonable size constraints
    setMinimumWidth(200);
    setMaximumWidth(300);
    setMinimumHeight(150);
}

void ToolpathLegendWidget::createOperationEntry(IntuiCAM::Toolpath::OperationType operation)
{
    if (m_operationWidgets.find(operation) != m_operationWidgets.end()) {
        return; // Already exists
    }
    
    QColor color = getOperationColor(operation);
    QString name = QString::fromStdString(IntuiCAM::Toolpath::operationTypeToString(operation));
    QString description = getOperationDescription(operation);
    
    OperationEntryWidget* entryWidget = new OperationEntryWidget(
        operation, color, name, description, m_compactMode, m_contentWidget);
    
    // Connect signals
    connect(entryWidget, &OperationEntryWidget::clicked,
            this, &ToolpathLegendWidget::operationClicked);
    connect(entryWidget, &OperationEntryWidget::visibilityToggled,
            this, &ToolpathLegendWidget::operationVisibilityChanged);
    
    // Set tooltip
    entryWidget->setToolTip(getOperationTooltip(operation));
    
    m_contentLayout->addWidget(entryWidget);
    m_operationWidgets[operation] = entryWidget;
    
    // Add spacer at the end
    m_contentLayout->addStretch();
}

QColor ToolpathLegendWidget::getOperationColor(IntuiCAM::Toolpath::OperationType operation) const
{
    // Professional CAM color scheme matching ToolpathDisplayObject
    switch (operation) {
        case IntuiCAM::Toolpath::OperationType::Facing:
            return QColor(0, 204, 51);      // Bright Green (0.0, 0.8, 0.2)
        case IntuiCAM::Toolpath::OperationType::ExternalRoughing:
            return QColor(230, 26, 26);     // Red (0.9, 0.1, 0.1)
        case IntuiCAM::Toolpath::OperationType::InternalRoughing:
            return QColor(179, 0, 77);      // Dark Red (0.7, 0.0, 0.3)
        case IntuiCAM::Toolpath::OperationType::ExternalFinishing:
            return QColor(0, 102, 230);     // Blue (0.0, 0.4, 0.9)
        case IntuiCAM::Toolpath::OperationType::InternalFinishing:
            return QColor(0, 153, 179);     // Teal (0.0, 0.6, 0.7)
        case IntuiCAM::Toolpath::OperationType::Drilling:
            return QColor(230, 230, 0);     // Yellow (0.9, 0.9, 0.0)
        case IntuiCAM::Toolpath::OperationType::Boring:
            return QColor(204, 204, 51);    // Olive (0.8, 0.8, 0.2)
        case IntuiCAM::Toolpath::OperationType::ExternalGrooving:
            return QColor(230, 0, 230);     // Magenta (0.9, 0.0, 0.9)
        case IntuiCAM::Toolpath::OperationType::InternalGrooving:
            return QColor(179, 0, 179);     // Purple (0.7, 0.0, 0.7)
        case IntuiCAM::Toolpath::OperationType::Chamfering:
            return QColor(0, 230, 230);     // Cyan (0.0, 0.9, 0.9)
        case IntuiCAM::Toolpath::OperationType::Threading:
            return QColor(128, 0, 230);     // Purple-Blue (0.5, 0.0, 0.9)
        case IntuiCAM::Toolpath::OperationType::Parting:
            return QColor(255, 128, 0);     // Orange (1.0, 0.5, 0.0)
        default:
            return QColor(128, 128, 128);   // Gray for unknown
    }
}

QString ToolpathLegendWidget::getOperationDescription(IntuiCAM::Toolpath::OperationType operation) const
{
    switch (operation) {
        case IntuiCAM::Toolpath::OperationType::Facing:
            return "Surface facing (always first)";
        case IntuiCAM::Toolpath::OperationType::ExternalRoughing:
            return "External material removal";
        case IntuiCAM::Toolpath::OperationType::InternalRoughing:
            return "Internal material removal";
        case IntuiCAM::Toolpath::OperationType::ExternalFinishing:
            return "External surface finishing";
        case IntuiCAM::Toolpath::OperationType::InternalFinishing:
            return "Internal surface finishing";
        case IntuiCAM::Toolpath::OperationType::Drilling:
            return "Hole drilling operations";
        case IntuiCAM::Toolpath::OperationType::Boring:
            return "Precision hole boring";
        case IntuiCAM::Toolpath::OperationType::ExternalGrooving:
            return "External groove cutting";
        case IntuiCAM::Toolpath::OperationType::InternalGrooving:
            return "Internal groove cutting";
        case IntuiCAM::Toolpath::OperationType::Chamfering:
            return "Edge chamfering";
        case IntuiCAM::Toolpath::OperationType::Threading:
            return "Thread cutting operations";
        case IntuiCAM::Toolpath::OperationType::Parting:
            return "Part separation (always last)";
        default:
            return "Unknown operation";
    }
}

QString ToolpathLegendWidget::getOperationTooltip(IntuiCAM::Toolpath::OperationType operation) const
{
    switch (operation) {
        case IntuiCAM::Toolpath::OperationType::Facing:
            return "Facing operations establish the reference surface and are always performed first.\n"
                   "Multi-pass facing from outside diameter to center with proper feeds and speeds.";
        case IntuiCAM::Toolpath::OperationType::ExternalRoughing:
            return "External roughing removes bulk material from the outside of the workpiece.\n"
                   "Progressive passes with appropriate clearances and chip loads.";
        case IntuiCAM::Toolpath::OperationType::InternalRoughing:
            return "Internal roughing removes material from internal features and bores.\n"
                   "Used for oversized holes that require boring operations.";
        case IntuiCAM::Toolpath::OperationType::ExternalFinishing:
            return "External finishing operations create the final surface finish.\n"
                   "Precision profile following with light cuts and appropriate feeds.";
        case IntuiCAM::Toolpath::OperationType::InternalFinishing:
            return "Internal finishing operations create precise internal geometries.\n"
                   "Multiple finish passes for dimensional accuracy and surface quality.";
        case IntuiCAM::Toolpath::OperationType::Drilling:
            return "Drilling operations create holes using standard drill bits.\n"
                   "Peck drilling with chip breaking cycles for optimal chip evacuation.";
        case IntuiCAM::Toolpath::OperationType::Boring:
            return "Boring operations create precise holes larger than standard drill sizes.\n"
                   "Used for holes requiring high dimensional accuracy and surface finish.";
        case IntuiCAM::Toolpath::OperationType::ExternalGrooving:
            return "External grooving cuts grooves on the outside surface.\n"
                   "Multi-plunge cutting with side cuts for proper chip formation.";
        case IntuiCAM::Toolpath::OperationType::InternalGrooving:
            return "Internal grooving cuts grooves on internal surfaces.\n"
                   "Specialized tooling for confined space operations.";
        case IntuiCAM::Toolpath::OperationType::Chamfering:
            return "Chamfering operations create beveled edges.\n"
                   "Typically 45-degree chamfers for part finishing and deburring.";
        case IntuiCAM::Toolpath::OperationType::Threading:
            return "Threading operations cut helical threads.\n"
                   "Multi-pass threading with synchronized spindle feed for precision.";
        case IntuiCAM::Toolpath::OperationType::Parting:
            return "Parting operations separate the finished part from stock material.\n"
                   "Always performed last with pecking cuts for clean separation.";
        default:
            return "Click to toggle visibility of this operation type in the 3D view.";
    }
}

void ToolpathLegendWidget::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}

void ToolpathLegendWidget::updateLegendForOperations(const std::vector<IntuiCAM::Toolpath::OperationType>& operations)
{
    // Hide all existing entries
    for (auto& pair : m_operationWidgets) {
        pair.second->hide();
    }
    
    // Show only the operations that are present
    for (auto operation : operations) {
        if (m_operationWidgets.find(operation) != m_operationWidgets.end()) {
            m_operationWidgets[operation]->show();
        }
    }
    
    // Update layout
    m_contentLayout->update();
}

void ToolpathLegendWidget::setOperationVisible(IntuiCAM::Toolpath::OperationType operation, bool visible)
{
    m_operationVisibility[operation] = visible;
    
    auto it = m_operationWidgets.find(operation);
    if (it != m_operationWidgets.end()) {
        OperationEntryWidget* entryWidget = qobject_cast<OperationEntryWidget*>(it->second);
        if (entryWidget) {
            entryWidget->setVisible(visible);
        }
    }
}

void ToolpathLegendWidget::setCompactMode(bool compact)
{
    if (m_compactMode == compact) return;
    
    m_compactMode = compact;
    
    // Update all existing entries
    for (auto& pair : m_operationWidgets) {
        // Would need to recreate entries for compact mode change
        // For now, just store the setting for new entries
    }
}

void ToolpathLegendWidget::setColorSquareSize(int size)
{
    m_colorSquareSize = size;
    
    // Update all color squares
    for (auto& pair : m_operationWidgets) {
        OperationEntryWidget* entryWidget = qobject_cast<OperationEntryWidget*>(pair.second);
        if (entryWidget) {
            // Would need access to color square to update size
        }
    }
}

void ToolpathLegendWidget::onOperationClicked()
{
    OperationEntryWidget* sender = qobject_cast<OperationEntryWidget*>(QObject::sender());
    if (sender) {
        emit operationClicked(sender->getOperationType());
    }
}

// =============================================================================
// ColorSquareWidget Implementation
// =============================================================================

ColorSquareWidget::ColorSquareWidget(const QColor& color, int size, QWidget* parent)
    : QWidget(parent)
    , m_color(color)
    , m_size(size)
{
    setFixedSize(m_size, m_size);
}

void ColorSquareWidget::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void ColorSquareWidget::setSize(int size)
{
    m_size = size;
    setFixedSize(m_size, m_size);
}

QSize ColorSquareWidget::sizeHint() const
{
    return QSize(m_size, m_size);
}

void ColorSquareWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw border
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QBrush(m_color));
    painter.drawRect(1, 1, m_size - 2, m_size - 2);
}

// =============================================================================
// OperationEntryWidget Implementation
// =============================================================================

OperationEntryWidget::OperationEntryWidget(IntuiCAM::Toolpath::OperationType operation,
                                          const QColor& color,
                                          const QString& name,
                                          const QString& description,
                                          bool compact,
                                          QWidget* parent)
    : QWidget(parent)
    , m_operation(operation)
    , m_color(color)
    , m_name(name)
    , m_description(description)
    , m_compact(compact)
    , m_operationVisible(true)
    , m_hovered(false)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 2, 4, 2);
    m_layout->setSpacing(6);
    
    // Color square
    m_colorSquare = new ColorSquareWidget(color, 16, this);
    m_layout->addWidget(m_colorSquare);
    
    // Operation name
    m_nameLabel = new QLabel(name, this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    m_layout->addWidget(m_nameLabel);
    
    if (!compact) {
        // Description (only in non-compact mode)
        m_descriptionLabel = new QLabel(description, this);
        QFont descFont = m_descriptionLabel->font();
        descFont.setPointSize(descFont.pointSize() - 1);
        m_descriptionLabel->setFont(descFont);
        m_descriptionLabel->setStyleSheet("color: gray;");
        m_layout->addWidget(m_descriptionLabel);
    } else {
        m_descriptionLabel = nullptr;
    }
    
    m_layout->addStretch();
    
    // Set cursor to indicate clickability
    setCursor(Qt::PointingHandCursor);
    
    // Set initial style
    setAutoFillBackground(true);
    updateStyle();
}

void OperationEntryWidget::setVisible(bool visible)
{
    m_operationVisible = visible;
    updateStyle();
    QWidget::setVisible(visible);
}

void OperationEntryWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_operation);
    }
    QWidget::mousePressEvent(event);
}

void OperationEntryWidget::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    updateStyle();
    QWidget::enterEvent(event);
}

void OperationEntryWidget::leaveEvent(QEvent* event)
{
    m_hovered = false;
    updateStyle();
    QWidget::leaveEvent(event);
}

void OperationEntryWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    // Draw custom styling if needed
    if (m_hovered) {
        QPainter painter(this);
        painter.setPen(QPen(QColor(100, 150, 255), 1));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
}

void OperationEntryWidget::updateStyle()
{
    QString styleSheet;
    
    if (!m_operationVisible) {
        styleSheet = "background-color: #f0f0f0; color: #888888;";
    } else if (m_hovered) {
        styleSheet = "background-color: #e6f3ff; color: black;";
    } else {
        styleSheet = "background-color: transparent; color: black;";
    }
    
    setStyleSheet(styleSheet);
} 