#include "../include/toolpathtimelinewidget.h"

#include <QApplication>
#include <QPalette>
#include <QColor>
#include <QIcon>
#include <QStyle>
#include <QPixmap>
#include <QContextMenuEvent>
#include <QChildEvent>
#include <QMouseEvent>
#include <QMessageBox>

ToolpathTimelineWidget::ToolpathTimelineWidget(QWidget *parent)
    : QWidget(parent),
      m_activeToolpathIndex(-1)
{
    // Create the main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create a scroll area to contain the timeline
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    // Create container widget for the timeline
    m_timelineContainer = new QWidget(m_scrollArea);
    m_timelineLayout = new QHBoxLayout(m_timelineContainer);
    m_timelineLayout->setAlignment(Qt::AlignLeft);
    m_timelineLayout->setContentsMargins(5, 5, 5, 5);
    m_timelineLayout->setSpacing(10);
    
    m_scrollArea->setWidget(m_timelineContainer);
    mainLayout->addWidget(m_scrollArea);
    
    // Set standard operations
    m_standardOperations << "Contouring" << "Threading" << "Chamfering" << "Parting";

    // Create default tiles for each standard operation
    for (const QString& op : m_standardOperations) {
        addToolpath(op, op, "Default Tool");
    }
    
    // Set stylesheet
    setStyleSheet(
        "QFrame.toolpath-frame {"
        "  border: 1px solid #808080;"
        "  border-radius: 4px;"
        "  background-color: #E0E0E0;"
        "  padding: 4px;"
        "  min-width: 120px;"
        "  max-width: 180px;"
        "  min-height: 70px;"
        "}"
        "QFrame.toolpath-frame:hover {"
        "  border: 1px solid #4080C0;"
        "  background-color: #E8F0FF;"
        "}"
        "QFrame.toolpath-frame-active {"
        "  border: 2px solid #2060A0;"
        "  background-color: #D0E0F8;"
        "}"
        "QFrame.toolpath-frame-disabled {"
        "  border: 1px solid #A0A0A0;"
        "  background-color: #F5F5F5;"
        "}"
        "QFrame.toolpath-frame-disabled QLabel {"
        "  color: #A0A0A0;"
        "}"
        "QLabel.operation-name {"
        "  font-weight: bold;"
        "  color: #303030;"
        "  font-size: 11pt;"
        "}"
        "QLabel.operation-type {"
        "  color: #606060;"
        "  font-size: 9pt;"
        "}"
        "QLabel.tool-name {"
        "  color: #505050;"
        "  font-style: italic;"
        "  font-size: 9pt;"
        "}"
    );
}

ToolpathTimelineWidget::~ToolpathTimelineWidget()
{
    // Clean up is handled by Qt's parent-child mechanism
}

int ToolpathTimelineWidget::addToolpath(const QString& operationName, 
                                       const QString& operationType,
                                       const QString& toolName,
                                       const QString& icon)
{
    // Create a new frame for the toolpath
    QFrame* frame = createToolpathFrame(operationName, operationType, toolName, icon);
    
    // Insert before the add button
    int index = m_timelineLayout->count();
    m_timelineLayout->insertWidget(index, frame);
    
    // Store the frame, operation type, and operation name
    m_toolpathFrames.append(frame);
    m_toolpathTypes.append(operationType);
    m_toolpathNames.append(operationName);
    
    // Update styles
    updateToolpathFrameStyles();
    
    // Return the index of the added toolpath
    return m_toolpathFrames.size() - 1;
}

void ToolpathTimelineWidget::removeToolpath(int index)
{
    if (index < 0 || index >= m_toolpathFrames.size())
        return;
    
    // Remove from layout and delete
    QFrame* frame = m_toolpathFrames.at(index);
    m_timelineLayout->removeWidget(frame);
    m_toolpathFrames.remove(index);
    m_toolpathTypes.remove(index);
    m_toolpathNames.remove(index);
    QCheckBox* chk = m_enabledChecks.takeAt(index);
    delete chk;
    delete frame;
    
    // Update active toolpath index if needed
    if (m_activeToolpathIndex == index) {
        m_activeToolpathIndex = -1;
    } else if (m_activeToolpathIndex > index) {
        m_activeToolpathIndex--;
    }
    
    // Update styles
    updateToolpathFrameStyles();
}

void ToolpathTimelineWidget::clearToolpaths()
{
    // Remove all toolpath frames
    for (QFrame* frame : m_toolpathFrames) {
        m_timelineLayout->removeWidget(frame);
        delete frame;
    }

    // Clear checkbox list. The actual QCheckBox objects are deleted together
    // with their parent frames above, so we must not delete them again here to
    // avoid a double free crash when reloading parts.
    m_enabledChecks.clear();
    
    m_toolpathFrames.clear();
    m_toolpathTypes.clear();
    m_toolpathNames.clear();
    m_activeToolpathIndex = -1;
}

void ToolpathTimelineWidget::updateToolpath(int index,
                                           const QString& operationName,
                                           const QString& operationType,
                                           const QString& toolName,
                                           const QString& icon)
{
    if (index < 0 || index >= m_toolpathFrames.size())
        return;
    
    // Update the toolpath frame
    QFrame* frame = m_toolpathFrames.at(index);
    
    // Find the labels in the frame
    QLabel* nameLabel = frame->findChild<QLabel*>("nameLabel");
    QLabel* typeLabel = frame->findChild<QLabel*>("typeLabel");
    QLabel* toolLabel = frame->findChild<QLabel*>("toolLabel");
    QLabel* iconLabel = frame->findChild<QLabel*>("iconLabel");
    
    if (nameLabel)
        nameLabel->setText(operationName);
    
    if (typeLabel)
        typeLabel->setText(operationType);
    
    if (toolLabel)
        toolLabel->setText(toolName);
    
    if (iconLabel && !icon.isEmpty()) {
        QPixmap pixmap(icon);
        if (!pixmap.isNull()) {
            iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    
    // Update the stored operation type and name
    m_toolpathTypes[index] = operationType;
    m_toolpathNames[index] = operationName;
}

void ToolpathTimelineWidget::setActiveToolpath(int index)
{
    if (index < -1 || index >= m_toolpathFrames.size())
        return;
    
    m_activeToolpathIndex = index;
    updateToolpathFrameStyles();
    
    if (index >= 0) {
        emit toolpathSelected(index);
    }
}

bool ToolpathTimelineWidget::isToolpathEnabled(int index) const
{
    if (index < 0 || index >= m_enabledChecks.size())
        return false;
    return m_enabledChecks.at(index)->isChecked();
}

void ToolpathTimelineWidget::setToolpathEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_enabledChecks.size())
        return;
    m_enabledChecks.at(index)->setChecked(enabled);
    updateToolpathFrameStyles();
}


void ToolpathTimelineWidget::onToolpathClicked(int index)
{
    setActiveToolpath(index);
    
    // Request to edit the parameters for this toolpath
    if (index >= 0 && index < m_toolpathTypes.size()) {
        emit toolpathParametersRequested(index, m_toolpathTypes.at(index));
    }
}

void ToolpathTimelineWidget::onToolpathRightClicked(int index, const QPoint& pos)
{
    if (index < 0 || index >= m_toolpathFrames.size())
        return;
    
    // Create context menu
    QMenu contextMenu(this);
    
    QAction* editAction = contextMenu.addAction("Edit Parameters");
    QAction* regenerateAction = contextMenu.addAction("Regenerate Toolpath");
    QAction* showAction = contextMenu.addAction("Show Toolpath");
    QAction* hideAction = contextMenu.addAction("Hide Toolpath");
    contextMenu.addSeparator();
    QAction* removeAction = contextMenu.addAction("Remove");
    
    // Show context menu
    QAction* selectedAction = contextMenu.exec(pos);
    
    // Handle selected action
    if (selectedAction == editAction) {
        onToolpathParameterEdit(index);
    }
    else if (selectedAction == regenerateAction) {
        // Emit signal to regenerate toolpath
        emit toolpathRegenerateRequested(index);
    }
    else if (selectedAction == showAction) {
        // Emit signal to show toolpath
        setActiveToolpath(index);
    }
    else if (selectedAction == hideAction) {
        // Clear active toolpath
        m_activeToolpathIndex = -1;
        updateToolpathFrameStyles();
        // Emit signal with -1 to hide all
        emit toolpathSelected(-1);
    }
    else if (selectedAction == removeAction) {
        // Ask for confirmation
        QMessageBox::StandardButton confirmation = 
            QMessageBox::question(this, "Remove Toolpath", 
                                "Are you sure you want to remove this toolpath?",
                                QMessageBox::Yes | QMessageBox::No);
        
        if (confirmation == QMessageBox::Yes) {
            emit removeToolpathRequested(index);
        }
    }
}

void ToolpathTimelineWidget::onOperationTypeSelected()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString operationType = action->text();
        emit addToolpathRequested(operationType);
    }
}

QFrame* ToolpathTimelineWidget::createToolpathFrame(const QString& operationName,
                                                  const QString& operationType,
                                                  const QString& toolName,
                                                  const QString& icon)
{
    // Create a frame for the toolpath
    QFrame* frame = new QFrame(m_timelineContainer);
    frame->setObjectName("toolpathFrame");
    frame->setProperty("index", m_toolpathFrames.size());
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    frame->setProperty("class", "toolpath-frame");
    
    // Create layout for the frame
    QVBoxLayout* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(4, 4, 4, 4);
    frameLayout->setSpacing(2);
    
    // Create header with enable checkbox, icon and operation name
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    // Enable checkbox
    QCheckBox* enableCheck = new QCheckBox(frame);
    enableCheck->setChecked(true);
    headerLayout->addWidget(enableCheck);
    m_enabledChecks.append(enableCheck);
    connect(enableCheck, &QCheckBox::toggled, this, [this, frame](bool checked) {
        int idx = frame->property("index").toInt();
        emit toolpathEnabledChanged(idx, checked);
        Q_UNUSED(checked);
        updateToolpathFrameStyles();
    });
    
    // Icon label
    QLabel* iconLabel = new QLabel(frame);
    iconLabel->setObjectName("iconLabel");
    iconLabel->setFixedSize(24, 24);
    
    if (!icon.isEmpty()) {
        QPixmap pixmap(icon);
        if (!pixmap.isNull()) {
            iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            // Default icons based on operation type
            if (operationType == "Contouring") {
                iconLabel->setText("C");
            } else if (operationType == "Threading") {
                iconLabel->setText("T");
            } else if (operationType == "Chamfering") {
                iconLabel->setText("Ch");
            } else if (operationType == "Parting") {
                iconLabel->setText("P");
            } else {
                iconLabel->setText("T");
            }
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet("background-color: #4080C0; color: white; border-radius: 3px;");
        }
    } else {
        // Default icons based on operation type
        if (operationType == "Contouring") {
            iconLabel->setText("C");
        } else if (operationType == "Threading") {
            iconLabel->setText("T");
        } else if (operationType == "Chamfering") {
            iconLabel->setText("Ch");
        } else if (operationType == "Parting") {
            iconLabel->setText("P");
        } else {
            iconLabel->setText("T");
        }
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("background-color: #4080C0; color: white; border-radius: 3px;");
    }
    
    headerLayout->addWidget(iconLabel);
    
    // Operation name label
    QLabel* nameLabel = new QLabel(operationName, frame);
    nameLabel->setObjectName("nameLabel");
    nameLabel->setProperty("class", "operation-name");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    headerLayout->addWidget(nameLabel);
    
    frameLayout->addLayout(headerLayout);
    
    // Operation type label
    QLabel* typeLabel = new QLabel(operationType, frame);
    typeLabel->setObjectName("typeLabel");
    typeLabel->setProperty("class", "operation-type");
    frameLayout->addWidget(typeLabel);
    
    // Tool name label
    QLabel* toolLabel = new QLabel(toolName, frame);
    toolLabel->setObjectName("toolLabel");
    toolLabel->setProperty("class", "tool-name");
    frameLayout->addWidget(toolLabel);
    
    // Make the frame clickable
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);
    
    return frame;
}

void ToolpathTimelineWidget::updateToolpathFrameStyles()
{
    for (int i = 0; i < m_toolpathFrames.size(); ++i) {
        QFrame* frame = m_toolpathFrames.at(i);

        QString cls = "toolpath-frame";
        if (!m_enabledChecks.at(i)->isChecked()) {
            cls += " toolpath-frame-disabled";
        }
        if (i == m_activeToolpathIndex) {
            cls += " toolpath-frame-active";
        }
        frame->setProperty("class", cls.trimmed());

        // Force style update
        frame->style()->unpolish(frame);
        frame->style()->polish(frame);
        frame->update();
    }
}


// Override event filter to handle mouse events on toolpath frames
bool ToolpathTimelineWidget::event(QEvent* event)
{
    if (event->type() == QEvent::ChildAdded) {
        // Make sure new frames have the event filter installed
        QChildEvent* childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()->inherits("QFrame")) {
            QFrame* frame = qobject_cast<QFrame*>(childEvent->child());
            if (frame && !frame->objectName().isEmpty() && frame->objectName() == "toolpathFrame") {
                frame->installEventFilter(this);
            }
        }
    }
    return QWidget::event(event);
}

bool ToolpathTimelineWidget::eventFilter(QObject* watched, QEvent* event)
{
    QFrame* frame = qobject_cast<QFrame*>(watched);
    if (frame && frame->objectName() == "toolpathFrame") {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            int index = frame->property("index").toInt();
            
            if (mouseEvent->button() == Qt::LeftButton) {
                onToolpathClicked(index);
                return true;
            } else if (mouseEvent->button() == Qt::RightButton) {
                onToolpathRightClicked(index, mouseEvent->globalPosition().toPoint());
                return true;
            }
        }
    }
    
    return QWidget::eventFilter(watched, event);
}

QString ToolpathTimelineWidget::getToolpathType(int index) const
{
    if (index >= 0 && index < m_toolpathTypes.size()) {
        return m_toolpathTypes.at(index);
    }
    return QString();
}

QString ToolpathTimelineWidget::getToolpathName(int index) const
{
    if (index >= 0 && index < m_toolpathNames.size()) {
        return m_toolpathNames.at(index);
    }
    return QString();
}

// Add this new method to handle parameter editing
void ToolpathTimelineWidget::onToolpathParameterEdit(int index)
{
    if (index < 0 || index >= m_toolpathFrames.size())
        return;
    
    QString operationType = m_toolpathTypes.at(index);
    QString operationName = m_toolpathNames.at(index);
    
    // Emit signal to request parameter editing
    emit toolpathParametersRequested(index, operationType);
} 