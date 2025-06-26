#include "operationtilewidget.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QCursor>
#include <QToolTip>
#include <QTimer>
#include <QEasingCurve>
#include <QEnterEvent>

namespace IntuiCAM {
namespace GUI {

// ========== OperationTileWidget Implementation ==========

OperationTileWidget::OperationTileWidget(const QString& operationName, bool enabledByDefault, QWidget* parent)
    : QFrame(parent)
    , m_operationName(operationName)
    , m_enabled(enabledByDefault)
    , m_expanded(false)
    , m_isHovered(false)
    , m_subTileContainer(nullptr)
    , m_subTileLayout(nullptr)
    , m_mainLayout(nullptr)
    , m_iconLabel(nullptr)
    , m_nameLabel(nullptr)
    , m_descriptionLabel(nullptr)
    , m_toolLabel(nullptr)
    , m_colorAnimation(nullptr)
    , m_shadowEffect(nullptr)
    , m_contextMenu(nullptr)
    , m_selectToolAction(nullptr)
    , m_toggleAction(nullptr)
{
    // Set up colors based on operation type
    if (operationName == "Facing") {
        m_enabledColor = QColor("#4CAF50");  // Green
        m_description = "Face the front of the part";
    } else if (operationName == "Internal Features") {
        m_enabledColor = QColor("#FF9800");  // Orange
        m_description = "Drilling, boring, and internal operations";
    } else if (operationName == "Roughing") {
        m_enabledColor = QColor("#2196F3");  // Blue
        m_description = "Remove bulk material quickly";
    } else if (operationName == "Finishing") {
        m_enabledColor = QColor("#9C27B0");  // Purple
        m_description = "Achieve final surface finish";
    } else if (operationName == "Grooving") {
        m_enabledColor = QColor("#607D8B");  // Blue Grey
        m_description = "Cut grooves and undercuts";
    } else if (operationName == "Threading") {
        m_enabledColor = QColor("#E91E63");  // Pink
        m_description = "Cut internal and external threads";
    } else if (operationName == "Chamfering") {
        m_enabledColor = QColor("#795548");  // Brown
        m_description = "Add chamfers and bevels";
    } else if (operationName == "Parting") {
        m_enabledColor = QColor("#FF5722");  // Deep Orange
        m_description = "Cut off the finished part";
    } else {
        // Sub-operation colors
        if (operationName == "Drilling") {
            m_enabledColor = QColor("#FFC107");  // Amber
            m_description = "Drill holes";
        } else if (operationName == "Internal Roughing") {
            m_enabledColor = QColor("#03A9F4");  // Light Blue
            m_description = "Rough internal features";
        } else if (operationName == "Internal Finishing") {
            m_enabledColor = QColor("#673AB7");  // Deep Purple
            m_description = "Finish internal surfaces";
        } else if (operationName == "Internal Grooving") {
            m_enabledColor = QColor("#455A64");  // Blue Grey Dark
            m_description = "Cut internal grooves";
        } else {
            m_enabledColor = QColor("#757575");  // Grey
            m_description = "Custom operation";
        }
    }
    
    m_disabledColor = QColor("#E0E0E0");  // Light grey
    m_hoverColor = m_enabledColor.lighter(110);
    m_textColor = QColor("#212121");  // Dark grey
    m_borderColor = QColor("#BDBDBD");  // Medium grey
    
    setupUI();
    updateColors();
    
    // Set initial state
    setFrameStyle(QFrame::Box);
    setLineWidth(2);
    setCursor(Qt::PointingHandCursor);
    
    // Set size constraints
    setMinimumSize(120, 80);
    setMaximumSize(160, 120);
    
    // Create shadow effect
    m_shadowEffect = new QGraphicsDropShadowEffect(this);
    m_shadowEffect->setBlurRadius(8);
    m_shadowEffect->setOffset(2, 2);
    m_shadowEffect->setColor(QColor(0, 0, 0, 30));
    setGraphicsEffect(m_shadowEffect);
    
    // Create color animation
    m_colorAnimation = new QPropertyAnimation(this, "backgroundColor");
    m_colorAnimation->setDuration(200);
    m_colorAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_colorAnimation, &QPropertyAnimation::finished,
            this, &OperationTileWidget::onAnimationFinished);
    
    // Create context menu
    m_contextMenu = new QMenu(this);
    m_selectToolAction = m_contextMenu->addAction("Select Tool...");
    m_toggleAction = m_contextMenu->addAction(m_enabled ? "Disable" : "Enable");
    
    connect(m_selectToolAction, &QAction::triggered,
            this, &OperationTileWidget::onToolSelectionRequested);
    connect(m_toggleAction, &QAction::triggered, [this]() {
        setEnabled(!m_enabled);
    });
}

OperationTileWidget::~OperationTileWidget()
{
    // Qt handles cleanup automatically
}

void OperationTileWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);
    
    // Icon (placeholder for now)
    m_iconLabel = new QLabel();
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(32, 32);
    m_iconLabel->setStyleSheet("QLabel { border: 1px solid #ccc; border-radius: 16px; background-color: white; }");
    m_iconLabel->setText("🔧");  // Placeholder emoji
    m_mainLayout->addWidget(m_iconLabel);
    
    // Operation name
    m_nameLabel = new QLabel(m_operationName);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setWordWrap(true);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPixelSize(11);
    m_nameLabel->setFont(nameFont);
    m_mainLayout->addWidget(m_nameLabel);
    
    // Tool label (hidden by default)
    m_toolLabel = new QLabel();
    m_toolLabel->setAlignment(Qt::AlignCenter);
    m_toolLabel->setWordWrap(true);
    QFont toolFont = m_toolLabel->font();
    toolFont.setPixelSize(9);
    toolFont.setItalic(true);
    m_toolLabel->setFont(toolFont);
    m_toolLabel->hide();
    m_mainLayout->addWidget(m_toolLabel);
    
    m_mainLayout->addStretch();
    
    // Sub-tile container (for Internal Features)
    if (m_operationName == "Internal Features") {
        m_subTileContainer = new QWidget(this);
        m_subTileLayout = new QVBoxLayout(m_subTileContainer);
        m_subTileLayout->setContentsMargins(0, 0, 0, 0);
        m_subTileLayout->setSpacing(2);
        m_subTileContainer->hide();  // Hidden by default
    }
}

void OperationTileWidget::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    
    m_enabled = enabled;
    updateColors();
    animateToState(enabled);
    
    // Update context menu
    if (m_toggleAction) {
        m_toggleAction->setText(enabled ? "Disable" : "Enable");
    }
    
    // Update sub-tiles visibility
    updateSubTilesVisibility();
    
    emit enabledChanged(m_operationName, enabled);
}

void OperationTileWidget::setIcon(const QString& iconPath)
{
    m_iconPath = iconPath;
    if (m_iconLabel && !iconPath.isEmpty()) {
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            m_iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_iconLabel->setText("");
        }
    }
}

void OperationTileWidget::setDescription(const QString& description)
{
    m_description = description;
    setToolTip(QString("%1\n%2").arg(m_operationName, description));
}

void OperationTileWidget::setSelectedTool(const QString& toolName)
{
    m_selectedTool = toolName;
    if (m_toolLabel) {
        if (toolName.isEmpty()) {
            m_toolLabel->hide();
        } else {
            m_toolLabel->setText(QString("Tool: %1").arg(toolName));
            m_toolLabel->show();
        }
    }
}

void OperationTileWidget::addSubTile(OperationTileWidget* subTile)
{
    if (!subTile || m_operationName != "Internal Features") return;
    
    m_subTiles.append(subTile);
    if (m_subTileLayout) {
        m_subTileLayout->addWidget(subTile);
        
        // Connect sub-tile signals
        connect(subTile, &OperationTileWidget::enabledChanged,
                this, &OperationTileWidget::enabledChanged);
        connect(subTile, &OperationTileWidget::clicked,
                this, &OperationTileWidget::clicked);
        connect(subTile, &OperationTileWidget::toolSelectionRequested,
                this, &OperationTileWidget::toolSelectionRequested);
    }
    
    updateSubTilesVisibility();
}

void OperationTileWidget::removeSubTile(OperationTileWidget* subTile)
{
    if (!subTile) return;
    
    m_subTiles.removeAll(subTile);
    if (m_subTileLayout) {
        m_subTileLayout->removeWidget(subTile);
    }
    
    updateSubTilesVisibility();
}

void OperationTileWidget::setExpanded(bool expanded)
{
    if (m_expanded == expanded || m_operationName != "Internal Features") return;
    
    m_expanded = expanded;
    updateSubTilesVisibility();
    
    emit expandedChanged(m_operationName, expanded);
}

void OperationTileWidget::updateColors()
{
    QColor targetColor = m_enabled ? m_enabledColor : m_disabledColor;
    if (m_isHovered && m_enabled) {
        targetColor = m_hoverColor;
    }
    
    m_backgroundColor = targetColor;
    
    // Update text color based on background
    qreal luminance = 0.299 * targetColor.redF() + 0.587 * targetColor.greenF() + 0.114 * targetColor.blueF();
    QColor textColor = luminance > 0.6 ? QColor("#212121") : QColor("#FFFFFF");
    
    if (m_nameLabel) {
        m_nameLabel->setStyleSheet(QString("color: %1;").arg(textColor.name()));
    }
    if (m_toolLabel) {
        m_toolLabel->setStyleSheet(QString("color: %1;").arg(textColor.name()));
    }
    
    update();
}

void OperationTileWidget::animateToState(bool enabled)
{
    if (m_colorAnimation) {
        QColor targetColor = enabled ? m_enabledColor : m_disabledColor;
        m_colorAnimation->setStartValue(m_backgroundColor);
        m_colorAnimation->setEndValue(targetColor);
        m_colorAnimation->start();
    }
}

void OperationTileWidget::updateSubTilesVisibility()
{
    if (m_operationName != "Internal Features" || !m_subTileContainer) return;
    
    bool shouldShow = m_enabled && m_expanded && !m_subTiles.isEmpty();
    m_subTileContainer->setVisible(shouldShow);
}

void OperationTileWidget::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    update();
}

QSize OperationTileWidget::sizeHint() const
{
    return QSize(140, 100);
}

QSize OperationTileWidget::minimumSizeHint() const
{
    return QSize(120, 80);
}

void OperationTileWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    QRect rect = this->rect().adjusted(1, 1, -1, -1);
    painter.setBrush(QBrush(m_backgroundColor));
    
    QColor borderColor = m_enabled ? m_enabledColor.darker(120) : m_borderColor;
    painter.setPen(QPen(borderColor, 2));
    painter.drawRoundedRect(rect, 8, 8);
    
    // Draw selection indicator if enabled
    if (m_enabled) {
        QRect indicator = QRect(rect.right() - 12, rect.top() + 4, 8, 8);
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(QPen(borderColor, 1));
        painter.drawEllipse(indicator);
    }
    
    QFrame::paintEvent(event);
}

void OperationTileWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setEnabled(!m_enabled);
    }
    QFrame::mouseDoubleClickEvent(event);
}

void OperationTileWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Handle expansion for Internal Features
        if (m_operationName == "Internal Features" && m_enabled) {
            setExpanded(!m_expanded);
        } else if (m_enabled) {
            emit clicked(m_operationName);
        }
    }
    QFrame::mousePressEvent(event);
}

void OperationTileWidget::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    updateColors();
    
    // Show description tooltip
    if (!m_description.isEmpty()) {
        setToolTip(QString("%1\n%2").arg(m_operationName, m_description));
    }
    
    QFrame::enterEvent(event);
}

void OperationTileWidget::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    updateColors();
    QFrame::leaveEvent(event);
}

void OperationTileWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_contextMenu) {
        m_contextMenu->exec(event->globalPos());
    }
}

void OperationTileWidget::onAnimationFinished()
{
    updateColors();
}

void OperationTileWidget::onToolSelectionRequested()
{
    emit toolSelectionRequested(m_operationName);
}

// ========== OperationTileContainer Implementation ==========

OperationTileContainer::OperationTileContainer(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_primaryRowLayout(nullptr)
    , m_secondaryRowLayout(nullptr)
    , m_internalFeaturesTile(nullptr)
{
    setupUI();
}

OperationTileContainer::~OperationTileContainer()
{
    // Qt handles cleanup automatically
}

void OperationTileContainer::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // Primary row for main operations
    m_primaryRowLayout = new QHBoxLayout();
    m_primaryRowLayout->setSpacing(8);
    m_mainLayout->addLayout(m_primaryRowLayout);
    
    // Secondary row for sub-operations (shown when Internal Features is expanded)
    m_secondaryRowLayout = new QHBoxLayout();
    m_secondaryRowLayout->setSpacing(4);
    m_mainLayout->addLayout(m_secondaryRowLayout);
    
    // Create tiles in the specified order
    QStringList operations = {
        "Facing",
        "Internal Features", 
        "Roughing",
        "Finishing",
        "Grooving",
        "Threading", 
        "Chamfering",
        "Parting"
    };
    
    // Enable operations that should be active by default.  The internal
    // features tile needs to reflect the default configuration where
    // machining internal geometry is turned on, so include it here.
    QStringList defaultEnabled = {
        "Facing",
        "Internal Features",
        "Roughing",
        "Finishing",
        "Parting"
    };
    
    for (const QString& operation : operations) {
        bool enabled = defaultEnabled.contains(operation);
        OperationTileWidget* tile = new OperationTileWidget(operation, enabled, this);
        addTile(tile);
        
        // Special handling for Internal Features
        if (operation == "Internal Features") {
            m_internalFeaturesTile = tile;
            
            // Create sub-tiles
            QStringList subOperations = {
                "Drilling",
                "Internal Roughing", 
                "Internal Finishing",
                "Internal Grooving"
            };
            
            for (const QString& subOp : subOperations) {
                OperationTileWidget* subTile = new OperationTileWidget(subOp, true, this);
                subTile->setParent(nullptr);  // Will be managed by Internal Features tile
                tile->addSubTile(subTile);
                m_tiles.append(subTile);
                
                // Connect sub-tile signals
                connect(subTile, &OperationTileWidget::enabledChanged,
                        this, &OperationTileContainer::onTileEnabledChanged);
                connect(subTile, &OperationTileWidget::clicked,
                        this, &OperationTileContainer::onTileClicked);
                connect(subTile, &OperationTileWidget::toolSelectionRequested,
                        this, &OperationTileContainer::onTileToolSelectionRequested);
            }
        }
    }
    
    arrangeInternalFeatures();
}

void OperationTileContainer::addTile(OperationTileWidget* tile)
{
    if (!tile) return;
    
    m_tiles.append(tile);
    
    // Add to primary row
    m_primaryRowLayout->addWidget(tile);
    
    // Connect signals
    connect(tile, &OperationTileWidget::enabledChanged,
            this, &OperationTileContainer::onTileEnabledChanged);
    connect(tile, &OperationTileWidget::clicked,
            this, &OperationTileContainer::onTileClicked);
    connect(tile, &OperationTileWidget::toolSelectionRequested,
            this, &OperationTileContainer::onTileToolSelectionRequested);
    connect(tile, &OperationTileWidget::expandedChanged,
            this, &OperationTileContainer::onTileExpandedChanged);
}

void OperationTileContainer::removeTile(OperationTileWidget* tile)
{
    if (!tile) return;
    
    m_tiles.removeAll(tile);
    m_primaryRowLayout->removeWidget(tile);
    m_secondaryRowLayout->removeWidget(tile);
}

OperationTileWidget* OperationTileContainer::getTile(const QString& operationName) const
{
    for (OperationTileWidget* tile : m_tiles) {
        if (tile->operationName() == operationName) {
            return tile;
        }
    }
    return nullptr;
}

void OperationTileContainer::setTileEnabled(const QString& operationName, bool enabled)
{
    OperationTileWidget* tile = getTile(operationName);
    if (tile) {
        tile->setEnabled(enabled);
    }
}

bool OperationTileContainer::isTileEnabled(const QString& operationName) const
{
    OperationTileWidget* tile = getTile(operationName);
    return tile ? tile->isEnabled() : false;
}

QStringList OperationTileContainer::getEnabledOperations() const
{
    QStringList enabled;
    for (OperationTileWidget* tile : m_tiles) {
        if (tile->isEnabled()) {
            enabled.append(tile->operationName());
        }
    }
    return enabled;
}

void OperationTileContainer::setTileSelectedTool(const QString& operationName, const QString& toolName)
{
    OperationTileWidget* tile = getTile(operationName);
    if (tile) {
        tile->setSelectedTool(toolName);
    }
}

QString OperationTileContainer::getTileSelectedTool(const QString& operationName) const
{
    OperationTileWidget* tile = getTile(operationName);
    return tile ? tile->selectedTool() : QString();
}

void OperationTileContainer::arrangeInternalFeatures()
{
    if (!m_internalFeaturesTile) return;
    
    // When Internal Features is expanded, show sub-tiles in secondary row
    connect(m_internalFeaturesTile, &OperationTileWidget::expandedChanged,
            [this](const QString&, bool expanded) {
                // Move sub-tiles to secondary row when expanded
                for (OperationTileWidget* subTile : m_internalFeaturesTile->subTiles()) {
                    if (expanded) {
                        m_secondaryRowLayout->addWidget(subTile);
                        subTile->show();
                    } else {
                        m_secondaryRowLayout->removeWidget(subTile);
                        subTile->hide();
                    }
                }
            });
}

void OperationTileContainer::onTileEnabledChanged(const QString& operationName, bool enabled)
{
    emit operationEnabledChanged(operationName, enabled);
}

void OperationTileContainer::onTileClicked(const QString& operationName)
{
    emit operationClicked(operationName);
}

void OperationTileContainer::onTileToolSelectionRequested(const QString& operationName)
{
    emit operationToolSelectionRequested(operationName);
}

void OperationTileContainer::onTileExpandedChanged(const QString& operationName, bool expanded)
{
    emit operationExpandedChanged(operationName, expanded);
}

} // namespace GUI
} // namespace IntuiCAM 