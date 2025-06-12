#ifndef TOOLPATHTIMELINEFRAME_H
#define TOOLPATHTIMELINEFRAME_H

#include <QFrame>
#include <QString>

class QLabel;

class ToolpathTimelineFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ToolpathTimelineFrame(int index, QWidget *parent = nullptr);

    int getIndex() const { return m_index; }
    QString getOperationName() const;
    QString getOperationType() const;
    QString getToolName() const;

    void setOperationName(const QString& name);
    void setOperationType(const QString& type);
    void setToolName(const QString& name);
    void setIcon(const QString& iconPath);

signals:
    void clicked(int index);
    void rightClicked(int index, const QPoint& pos);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_index;
    QLabel* m_nameLabel;
    QLabel* m_typeLabel;
    QLabel* m_toolLabel;
    QLabel* m_iconLabel;
};

#endif // TOOLPATHTIMELINEFRAME_H 