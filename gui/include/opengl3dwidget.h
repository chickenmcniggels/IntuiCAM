#ifndef OPENGL3DWIDGET_H
#define OPENGL3DWIDGET_H

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <WNT_Window.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

enum class ViewMode
{
    Mode3D,
    LatheXZ
};

class OpenGL3DWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit OpenGL3DWidget(QWidget* parent = nullptr);
    ~OpenGL3DWidget();

    Handle(AIS_InteractiveContext) getContext() const { return m_context; }

    void displayShape(const TopoDS_Shape& shape);
    void clearAll();
    void fitAll();

    void setSelectionMode(bool enabled);
    bool isSelectionModeActive() const { return m_selectionMode; }

    void setAutoFitEnabled(bool enabled) { m_autoFitEnabled = enabled; }
    bool isAutoFitEnabled() const { return m_autoFitEnabled; }

    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }
    void toggleViewMode();

signals:
    void viewerInitialized();
    void shapeSelected(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint);
    void viewModeChanged(ViewMode mode);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initViewer();
    void applyViewMode();

    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(WNT_Window) m_window;

    QPoint m_lastPos;
    Qt::MouseButton m_lastButton;

    bool m_selectionMode;
    bool m_autoFitEnabled;

    ViewMode m_viewMode;
};

#endif // OPENGL3DWIDGET_H
