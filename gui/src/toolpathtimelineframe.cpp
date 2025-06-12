#include "../include/toolpathtimelineframe.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPixmap>

ToolpathTimelineFrame::ToolpathTimelineFrame(int index, QWidget *parent)
    : QFrame(parent), m_index(index)
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);
    setObjectName(QString("toolpath-frame-%1").arg(index));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setObjectName("iconLabel");
    topLayout->addWidget(m_iconLabel);
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setObjectName("nameLabel");
    m_nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLayout->addWidget(m_nameLabel, 1);
    
    layout->addLayout(topLayout);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName("typeLabel");
    layout->addWidget(m_typeLabel);
    
    m_toolLabel = new QLabel(this);
    m_toolLabel->setObjectName("toolLabel");
    layout->addWidget(m_toolLabel);
    
    setMinimumSize(120, 80);
}

QString ToolpathTimelineFrame::getOperationName() const
{
    return m_nameLabel->text();
}

QString ToolpathTimelineFrame::getOperationType() const
{
    return m_typeLabel->text();
}

QString ToolpathTimelineFrame::getToolName() const
{
    return m_toolLabel->text();
}

void ToolpathTimelineFrame::setOperationName(const QString& name)
{
    m_nameLabel->setText(name);
}

void ToolpathTimelineFrame::setOperationType(const QString& type)
{
    m_typeLabel->setText(type);
}

void ToolpathTimelineFrame::setToolName(const QString& name)
{
    m_toolLabel->setText(name);
}

void ToolpathTimelineFrame::setIcon(const QString& iconPath)
{
    if (!iconPath.isEmpty()) {
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            m_iconLabel->setPixmap(pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void ToolpathTimelineFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_index);
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked(m_index, event->globalPosition().toPoint());
    }
    QFrame::mousePressEvent(event);
} 