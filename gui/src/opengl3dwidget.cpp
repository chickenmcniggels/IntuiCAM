#include "opengl3dwidget.h"

#include <QApplication>
#include <QDebug>
#include <QTimer>

// Additional OpenCASCADE includes
#include <Aspect_Handle.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>

#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

OpenGL3DWidget::OpenGL3DWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_isDragging(false)
    , m_dragButton(Qt::NoButton)
    , m_continuousUpdate(false)
    , m_updateTimer(new QTimer(this))
{
    // Enable mouse tracking for proper interaction
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Set update behavior to always update, regardless of focus
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    
    // Setup continuous update timer
    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    
    qDebug() << "OpenGL3DWidget created as pure visualization component";
}

OpenGL3DWidget::~OpenGL3DWidget()
{
    // OpenCASCADE handles cleanup automatically through smart pointers
}

void OpenGL3DWidget::initializeGL()
{
    initializeViewer();
}

void OpenGL3DWidget::paintGL()
{
    updateView();
}

void OpenGL3DWidget::resizeGL(int width, int height)
{
    if (!m_view.IsNull())
    {
        m_view->MustBeResized();
    }
}

void OpenGL3DWidget::initializeViewer()
{
    try {
        // Create display connection
        Handle(Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection();
        
        // Create graphic driver
        Handle(OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
        
        // Create viewer
        m_viewer = new V3d_Viewer(aGraphicDriver);
        m_viewer->SetDefaultBackgroundColor(Quantity_NOC_GRAY30);
        
        // Create window handle
#ifdef _WIN32
        Aspect_Handle aWindowHandle = (Aspect_Handle)winId();
        m_window = new WNT_Window(aWindowHandle);
#else
        Aspect_Handle aWindowHandle = (Aspect_Handle)winId();
        m_window = new Xw_Window(aDisplayConnection, aWindowHandle);
#endif
        
        // Create view
        m_view = m_viewer->CreateView();
        m_view->SetWindow(m_window);
        
        // Set up lighting
        Handle(V3d_DirectionalLight) aLight = new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE, Standard_True);
        Handle(V3d_AmbientLight) aAmbientLight = new V3d_AmbientLight(Quantity_NOC_GRAY50);
        m_viewer->AddLight(aLight);
        m_viewer->AddLight(aAmbientLight);
        m_viewer->SetLightOn();
        
        // Create interactive context
        m_context = new AIS_InteractiveContext(m_viewer);
        
        // Configure the view
        m_view->SetBackgroundColor(Quantity_NOC_GRAY30);
        m_view->MustBeResized();
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
        
        // Emit initialization signal
        emit viewerInitialized();
        
        qDebug() << "OpenCASCADE 3D viewer initialized successfully";
        
    } catch (const std::exception& e) {
        qDebug() << "Error initializing OpenCASCADE viewer:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error initializing OpenCASCADE viewer";
    }
}

void OpenGL3DWidget::updateView()
{
    if (!m_view.IsNull() && !m_window.IsNull())
    {
        // Ensure the OpenGL context is current before updating
        makeCurrent();
        
        // Force redraw even if widget doesn't have focus
        m_view->Redraw();
        
        // Make sure the rendering is complete
        if (context()) {
            context()->swapBuffers(context()->surface());
        }
    }
}

void OpenGL3DWidget::displayShape(const TopoDS_Shape& shape)
{
    if (m_context.IsNull() || shape.IsNull())
        return;
        
    try {
        // Create AIS shape
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        
        // Display the shape
        m_context->Display(aisShape, AIS_Shaded, 0, false);
        
        // Update the view
        fitAll();
        
        qDebug() << "Shape displayed successfully";
        
    } catch (const std::exception& e) {
        qDebug() << "Error displaying shape:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error displaying shape";
    }
}

void OpenGL3DWidget::clearAll()
{
    if (!m_context.IsNull())
    {
        m_context->RemoveAll(false);
        updateView();
    }
}

void OpenGL3DWidget::fitAll()
{
    if (!m_view.IsNull())
    {
        m_view->FitAll();
        m_view->ZFitAll();
        updateView();
    }
}

void OpenGL3DWidget::mousePressEvent(QMouseEvent *event)
{
    m_isDragging = true;
    m_lastMousePos = event->pos();
    m_dragButton = event->button();
    
    if (!m_view.IsNull())
    {
        if (event->button() == Qt::LeftButton)
        {
            m_view->StartRotation(event->pos().x(), event->pos().y());
        }
    }
}

void OpenGL3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_view.IsNull() && m_isDragging)
    {
        if (m_dragButton == Qt::LeftButton)
        {
            // Rotation
            m_view->Rotation(event->pos().x(), event->pos().y());
        }
        else if (m_dragButton == Qt::MiddleButton)
        {
            // Panning
            m_view->Pan(event->pos().x() - m_lastMousePos.x(), 
                       m_lastMousePos.y() - event->pos().y());
        }
        else if (m_dragButton == Qt::RightButton)
        {
            // Zooming
            m_view->Zoom(m_lastMousePos.x(), m_lastMousePos.y(), 
                        event->pos().x(), event->pos().y());
        }
        
        updateView();
    }
    
    m_lastMousePos = event->pos();
}

void OpenGL3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_isDragging = false;
    m_dragButton = Qt::NoButton;
}

void OpenGL3DWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_view.IsNull())
    {
        Standard_Real aFactor = 16;
        Standard_Real aX = event->position().x();
        Standard_Real aY = event->position().y();
        
        if (event->angleDelta().y() > 0)
        {
            aX += aFactor;
            aY += aFactor;
        }
        else
        {
            aX -= aFactor;
            aY -= aFactor;
        }
        
        m_view->Zoom(event->position().x(), event->position().y(), aX, aY);
        updateView();
    }
}

void OpenGL3DWidget::setContinuousUpdate(bool enabled)
{
    m_continuousUpdate = enabled;
    if (enabled && isVisible()) {
        m_updateTimer->start();
    } else {
        m_updateTimer->stop();
    }
}

void OpenGL3DWidget::focusInEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusInEvent(event);
    // Ensure we update when gaining focus
    update();
    qDebug() << "OpenGL3DWidget gained focus";
}

void OpenGL3DWidget::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    // Force an update even when losing focus to prevent black screen
    update();
    qDebug() << "OpenGL3DWidget lost focus";
}

void OpenGL3DWidget::showEvent(QShowEvent *event)
{
    QOpenGLWidget::showEvent(event);
    if (m_continuousUpdate) {
        m_updateTimer->start();
    }
    // Force initial update when shown
    update();
}

void OpenGL3DWidget::hideEvent(QHideEvent *event)
{
    QOpenGLWidget::hideEvent(event);
    m_updateTimer->stop();
} 