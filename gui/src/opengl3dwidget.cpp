#include "opengl3dwidget.h"

#include <QTimer>
#include <QApplication>

OpenGL3DWidget::OpenGL3DWidget(QWidget* parent)
    : QOpenGLWidget(parent),
      m_lastButton(Qt::NoButton),
      m_selectionMode(false),
      m_autoFitEnabled(true),
      m_viewMode(ViewMode::Mode3D)
{
    setMouseTracking(true);
}

OpenGL3DWidget::~OpenGL3DWidget() = default;

void OpenGL3DWidget::initializeGL()
{
    initViewer();
}

void OpenGL3DWidget::initViewer()
{
    try {
        Handle(Aspect_DisplayConnection) conn = new Aspect_DisplayConnection();
        Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(conn);

        m_viewer = new V3d_Viewer(driver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();

        m_context = new AIS_InteractiveContext(m_viewer);
        m_view = m_viewer->CreateView();

#ifdef _WIN32
        Aspect_Handle win = (Aspect_Handle)winId();
        m_window = new WNT_Window(win);
#else
        Aspect_Handle win = (Aspect_Handle)winId();
        m_window = new Xw_Window(conn, win);
#endif
        m_view->SetWindow(m_window);
        m_view->SetBackgroundColor(Quantity_NOC_GRAY30);
        m_view->MustBeResized();

        applyViewMode();

        emit viewerInitialized();
    } catch (...) {
        // ignore errors for now
    }
}

void OpenGL3DWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    if (!m_view.IsNull()) {
        m_view->MustBeResized();
    }
}

void OpenGL3DWidget::paintGL()
{
    if (!m_view.IsNull()) {
        m_view->Redraw();
    }
}

void OpenGL3DWidget::displayShape(const TopoDS_Shape& shape)
{
    if (m_context.IsNull() || shape.IsNull())
        return;
    Handle(AIS_Shape) ais = new AIS_Shape(shape);
    m_context->Display(ais, AIS_Shaded, 0, false);
    if (m_autoFitEnabled)
        fitAll();
    else
        update();
}

void OpenGL3DWidget::clearAll()
{
    if (!m_context.IsNull()) {
        m_context->RemoveAll(false);
        update();
    }
}

void OpenGL3DWidget::fitAll()
{
    if (!m_view.IsNull()) {
        m_view->FitAll();
        m_view->ZFitAll();
        update();
    }
}

void OpenGL3DWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastPos = event->pos();
    m_lastButton = event->button();

    if (m_selectionMode && event->button() == Qt::LeftButton && !m_context.IsNull()) {
        m_context->MoveTo(event->pos().x(), event->pos().y(), m_view, Standard_True);
        if (m_context->HasDetected()) {
            m_context->Select(Standard_True);
            for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected()) {
                Handle(AIS_Shape) ais = Handle(AIS_Shape)::DownCast(m_context->SelectedInteractive());
                if (!ais.IsNull()) {
                    Standard_Real x, y, z;
                    m_view->Convert(event->pos().x(), event->pos().y(), x, y, z);
                    emit shapeSelected(ais->Shape(), gp_Pnt(x, y, z));
                }
            }
        }
    }
}

void OpenGL3DWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_view.IsNull())
        return;

    if (!m_selectionMode && (event->buttons() & Qt::LeftButton) && m_lastButton == Qt::LeftButton) {
        m_view->Rotate(event->pos().x(), event->pos().y(), m_lastPos.x(), m_lastPos.y());
    } else if (event->buttons() & Qt::MidButton ||
               ((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ShiftModifier))) {
        m_view->Pan(event->pos().x() - m_lastPos.x(), m_lastPos.y() - event->pos().y());
    }

    m_lastPos = event->pos();
    update();
}

void OpenGL3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_lastButton = Qt::NoButton;
}

void OpenGL3DWidget::wheelEvent(QWheelEvent* event)
{
    if (m_view.IsNull())
        return;
    double factor = event->angleDelta().y() > 0 ? 0.9 : 1.1;
    m_view->SetScale(m_view->Scale() * factor);
    update();
}

void OpenGL3DWidget::setSelectionMode(bool enabled)
{
    m_selectionMode = enabled;
    if (!enabled && !m_context.IsNull()) {
        m_context->ClearSelected(Standard_False);
        update();
    }
}

void OpenGL3DWidget::setViewMode(ViewMode mode)
{
    if (mode == m_viewMode || m_view.IsNull())
        return;
    m_viewMode = mode;
    applyViewMode();
    emit viewModeChanged(mode);
    update();
}

void OpenGL3DWidget::applyViewMode()
{
    if (m_view.IsNull())
        return;
    if (m_viewMode == ViewMode::Mode3D) {
        m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
        m_view->SetProj(V3d_Zpos);
    } else {
        m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
        m_view->SetProj(V3d_Yneg);
    }
}

void OpenGL3DWidget::toggleViewMode()
{
    setViewMode(m_viewMode == ViewMode::Mode3D ? ViewMode::LatheXZ : ViewMode::Mode3D);
}


