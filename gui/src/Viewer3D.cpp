#include "Viewer3D.h"

#include <QApplication>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMessageBox>

#ifdef _WIN32
#include <windows.h>
#include <wingdi.h>
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#endif

// OpenCASCADE includes
#include <Aspect_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_Drawer.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <Graphic3d_Camera.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Message.hxx>
#include <BRepPrimAPI_MakeBox.hxx>

//! OpenGL FBO subclass for wrapping FBO created by Qt using GL_RGBA8 texture format instead of GL_SRGB8_ALPHA8.
//! This FBO is set to OpenGl_Context::SetDefaultFrameBuffer() as a final target.
//! Subclass calls OpenGl_Context::SetFrameBufferSRGB() with sRGB=false flag,
//! which asks OCCT to disable GL_FRAMEBUFFER_SRGB and apply sRGB gamma correction manually.
class OcctQtFrameBuffer : public OpenGl_FrameBuffer
{
  DEFINE_STANDARD_RTTI_INLINE(OcctQtFrameBuffer, OpenGl_FrameBuffer)
public:
  //! Empty constructor.
  OcctQtFrameBuffer() {}

  //! Make this FBO active in context.
  virtual void BindBuffer (const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindBuffer (theGlCtx);
    theGlCtx->SetFrameBufferSRGB (true, false);
  }

  //! Make this FBO as drawing target in context.
  virtual void BindDrawBuffer (const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindDrawBuffer (theGlCtx);
    theGlCtx->SetFrameBufferSRGB (true, false);
  }

  //! Make this FBO as reading source in context.
  virtual void BindReadBuffer (const Handle(OpenGl_Context)& theGlCtx) override
  {
    OpenGl_FrameBuffer::BindReadBuffer (theGlCtx);
  }
};

//! Helper to get OpenGL context from OCCT view
class OcctGlTools
{
public:
  static Handle(OpenGl_Context) GetGlContext (const Handle(V3d_View)& theView)
  {
    Handle(OpenGl_View) aGlView = Handle(OpenGl_View)::DownCast(theView->View());
    return aGlView->GlWindow()->GetGlContext();
  }
};

Viewer3D::Viewer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      m_currentViewMode(Viewer3D::ViewMode::Mode3D),
      m_isInitialized(false),
      m_needsUpdate(false),
      m_isDragging(false),
      m_dragButton(Qt::NoButton),
      m_selectionMode(false),
      m_autoFitEnabled(true),
      m_hoverHighlightEnabled(true)
{
    // Create display connection and graphic driver with proper QOpenGLWidget integration
    Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);
    
    // Critical driver options for QOpenGLWidget integration
    aDriver->ChangeOptions().buffersNoSwap = true;      // lets QOpenGLWidget manage buffer swap
    aDriver->ChangeOptions().buffersOpaqueAlpha = true; // don't write into alpha channel
    aDriver->ChangeOptions().useSystemBuffer = false;   // offscreen FBOs should be always used

    // Create viewer
    m_viewer = new V3d_Viewer(aDriver);
    m_viewer->SetDefaultBackgroundColor(Quantity_NOC_GRAY30);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();

    // Create AIS context
    m_context = new AIS_InteractiveContext(m_viewer);

    // Create view (window will be created later in initializeGL)
    m_view = m_viewer->CreateView();
    m_view->SetImmediateUpdate(false);
#ifndef __APPLE__
    m_view->ChangeRenderingParams().NbMsaaSamples = 4; // warning - affects performance
#endif

    // Qt widget setup
    setMouseTracking(true);
    setBackgroundRole(QPalette::NoRole);
    setFocusPolicy(Qt::StrongFocus);
    setUpdatesEnabled(true);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // OpenGL setup managed by Qt
    QSurfaceFormat aGlFormat;
    aGlFormat.setDepthBufferSize(24);
    aGlFormat.setStencilBufferSize(8);
    // Use OpenGL 3.3 instead of 4.5 for better compatibility
    aGlFormat.setVersion(3, 3);
    aGlFormat.setProfile(QSurfaceFormat::CompatibilityProfile); // Use compatibility profile for better hardware support
    setFormat(aGlFormat);

#if defined(_WIN32)
    // never use ANGLE on Windows, since OCCT 3D Viewer does not expect this
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif
    
    qDebug() << "Viewer3D created with proper OCCT-Qt integration setup.";
}

Viewer3D::~Viewer3D()
{
    // hold on X11 display connection till making another connection active by glXMakeCurrent()
    // to workaround sudden crash in QOpenGLWidget destructor
    Handle(Aspect_DisplayConnection) aDisp = m_viewer->Driver()->GetDisplayConnection();

    // release OCCT viewer
    if (!m_context.IsNull()) {
        m_context->RemoveAll(false);
        m_context.Nullify();
    }
    if (!m_view.IsNull()) {
        m_view->Remove();
        m_view.Nullify();
    }
    if (!m_viewer.IsNull()) {
        m_viewer.Nullify();
    }

    // make active OpenGL context created by Qt
    makeCurrent();
    aDisp.Nullify();
    
    qDebug() << "Viewer3D destroyed.";
}

void Viewer3D::initializeGL()
{
    qDebug() << "Viewer3D::initializeGL() called";
    
    // Ensure Qt's OpenGL context is current before OCCT tries to wrap it
    makeCurrent();
    
    // Check if we have a valid OpenGL context
    QOpenGLContext* qtContext = QOpenGLWidget::context();
    if (!qtContext || !qtContext->isValid()) {
        qCritical() << "Qt OpenGL context is not valid!";
        QMessageBox::critical(0, "OpenGL Error", "Qt OpenGL context is not valid!");
        return;
    }
    
    qDebug() << "Qt OpenGL context is valid, version:" << qtContext->format().majorVersion() << "." << qtContext->format().minorVersion();
    
    const QRect aRect = rect();
    const Graphic3d_Vec2i aViewSize(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());

    Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();

    // Try to create OCCT OpenGL context without forcing core profile initially
    Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
    
    // Try compatibility profile first, then core profile if that fails
    bool contextInitialized = false;
    
    // First try: compatibility profile (more lenient)
    if (aGlCtx->Init(false)) { // false = compatibility profile
        contextInitialized = true;
        qDebug() << "OCCT OpenGL context initialized with compatibility profile";
    } else {
        qDebug() << "Compatibility profile failed, trying core profile...";
        // Second try: core profile
        aGlCtx = new OpenGl_Context(); // Create a new context
        if (aGlCtx->Init(true)) { // true = core profile
            contextInitialized = true;
            qDebug() << "OCCT OpenGL context initialized with core profile";
        }
    }
    
    if (!contextInitialized) {
        QString errorMsg = "Error: OpenGl_Context is unable to wrap OpenGL context. Please check your graphics drivers.";
        qCritical() << errorMsg;
        QMessageBox::critical(0, "OpenGL Context Error", errorMsg);
        return;
    }

    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(m_view->Window());
    if (!aWindow.IsNull())
    {
        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        m_view->SetWindow(aWindow, aGlCtx->RenderingContext());
    }
    else
    {
        aWindow = new Aspect_NeutralWindow();
        aWindow->SetVirtual(true);
        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        m_view->SetWindow(aWindow, aGlCtx->RenderingContext());
    }

    // Setup view configuration
    setupView();

    // Apply the current view mode
    apply3DView();
    if (m_currentViewMode == Viewer3D::ViewMode::Mode2DXZ) {
        applyXZPlaneView();
    }

    // Configure selection and display defaults
    m_context->SetDisplayMode(AIS_Shaded, Standard_True);
    m_context->Activate(0); // Activate default selection mode
    m_context->SetAutomaticHilight(Standard_True);

    // Display coordinate trihedron
    m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);

    m_isInitialized = true;
    emit viewerInitialized();
    
    qDebug() << "Viewer3D: OpenCASCADE viewer initialized successfully.";
}

void Viewer3D::setupView()
{
    if (m_view.IsNull()) return;

    // Configure view background
    m_view->SetBackgroundColor(Quantity_NOC_GRAY30);
    
    // Setup lighting
    Handle(V3d_DirectionalLight) dirLight = new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE, Standard_True);
    Handle(V3d_AmbientLight) ambLight = new V3d_AmbientLight(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    m_viewer->AddLight(dirLight);
    m_viewer->AddLight(ambLight);
    m_viewer->SetLightOn();
    
    // Set default projection
    m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
    
    qDebug() << "Viewer3D: View setup completed.";
}

void Viewer3D::paintGL()
{
    if (m_view->Window().IsNull())
    {
        return;
    }

    Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();
#ifdef _WIN32
    // Use Qt's winId directly instead of WGL functions
    aNativeWin = (Aspect_Drawable)winId();
#endif
    if (m_view->Window()->NativeHandle() != aNativeWin)
    {
        // workaround window recreation done by Qt on monitor (QScreen) disconnection
        Message::SendWarning() << "Native window handle has changed by QOpenGLWidget!";
        initializeGL();
        return;
    }

    // wrap FBO created by QOpenGLWidget
    Handle(OpenGl_Context) aGlCtx = OcctGlTools::GetGlContext(m_view);
    Handle(OpenGl_FrameBuffer) aDefaultFbo = aGlCtx->DefaultFrameBuffer();
    if (aDefaultFbo.IsNull())
    {
        aDefaultFbo = new OcctQtFrameBuffer();
        aGlCtx->SetDefaultFrameBuffer(aDefaultFbo);
    }
    if (!aDefaultFbo->InitWrapper(aGlCtx))
    {
        aDefaultFbo.Nullify();
        Message::DefaultMessenger()->Send("Default FBO wrapper creation failed", Message_Fail);
        QMessageBox::critical(0, "Failure", "Default FBO wrapper creation failed");
        QApplication::exit(1);
        return;
    }

    Graphic3d_Vec2i aViewSizeOld;
    Graphic3d_Vec2i aViewSizeNew = aDefaultFbo->GetVPSize();
    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(m_view->Window());
    aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
    if (aViewSizeNew != aViewSizeOld)
    {
        aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
        m_view->MustBeResized();
        m_view->Invalidate();

        for (const Handle(V3d_View)& aSubviewIter : m_view->Subviews())
        {
            aSubviewIter->MustBeResized();
            aSubviewIter->Invalidate();
            aDefaultFbo->SetupViewport(aGlCtx);
        }
    }

    // flush pending input events and redraw the viewer
    m_view->InvalidateImmediate();
    // Use OCCT's built-in event handling mechanism
    if (!m_context.IsNull()) {
        // Redraw the view content
        m_view->Redraw();
    }
    
    m_needsUpdate = false;
}

void Viewer3D::resizeGL(int width, int height)
{
    if (!m_isInitialized || m_view.IsNull() || width <= 0 || height <= 0) {
        qDebug() << "Viewer3D::resizeGL: Not ready or invalid size:" << width << "x" << height;
        return;
    }

    try {
        // Tell OCCT that the view needs to be resized
        m_view->MustBeResized();
        
        qDebug() << "Viewer3D: Resized to" << width << "x" << height;
    } catch (const Standard_Failure& e) {
        qDebug() << "Viewer3D: Error during resizeGL:" << e.GetMessageString();
    } catch (...) {
        qDebug() << "Viewer3D: Unknown error during resizeGL.";
    }
}

void Viewer3D::updateView()
{
    update(); // Schedule Qt widget update (calls paintGL)
}

void Viewer3D::scheduleRedraw()
{
    if (!m_isInitialized) return;
    
    m_needsUpdate = true;
    update(); // Schedule Qt widget update (calls paintGL)
}

// --- Event Handling ---
void Viewer3D::showEvent(QShowEvent *event)
{
    QOpenGLWidget::showEvent(event);
    if (m_isInitialized) {
        scheduleRedraw();
    }
    qDebug() << "Viewer3D: showEvent";
}

void Viewer3D::hideEvent(QHideEvent *event)
{
    QOpenGLWidget::hideEvent(event);
    qDebug() << "Viewer3D: hideEvent";
}

void Viewer3D::focusInEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusInEvent(event);
    if (m_isInitialized) {
        scheduleRedraw();
    }
    qDebug() << "Viewer3D: focusInEvent";
}

void Viewer3D::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    qDebug() << "Viewer3D: focusOutEvent";
}

void Viewer3D::enterEvent(QEnterEvent *event)
{
    QOpenGLWidget::enterEvent(event);
    // Could trigger hover effects if needed
}

void Viewer3D::leaveEvent(QEvent *event)
{
    QOpenGLWidget::leaveEvent(event);
    // Could clear hover effects if needed
}

// --- Shape Display and Management ---
void Viewer3D::displayShape(const TopoDS_Shape& shape, bool autoFit)
{
    if (!m_isInitialized || m_context.IsNull() || shape.IsNull()) return;

    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_context->Display(aisShape, false);
    
    if (autoFit && m_autoFitEnabled) {
        m_view->FitAll();
    }
    
    scheduleRedraw();
    qDebug() << "Viewer3D: Displayed shape.";
}

void Viewer3D::removeShape(const Handle(AIS_Shape)& aisShape)
{
    if (!m_isInitialized || m_context.IsNull() || aisShape.IsNull()) return;

    if (m_context->IsDisplayed(aisShape)) {
        m_context->Erase(aisShape, false);
        scheduleRedraw();
        qDebug() << "Viewer3D: Removed AIS_Shape.";
    }
}

void Viewer3D::removeShape(const TopoDS_Shape& shape)
{
    if (!m_isInitialized || m_context.IsNull() || shape.IsNull()) return;

    AIS_ListOfInteractive displayedObjects;
    m_context->DisplayedObjects(displayedObjects);
    for (const auto& obj : displayedObjects) {
        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
        if (!aisShape.IsNull() && aisShape->Shape().IsSame(shape)) {
            m_context->Erase(aisShape, false);
            qDebug() << "Viewer3D: Removed TopoDS_Shape.";
        }
    }
    scheduleRedraw();
}

void Viewer3D::clearAll()
{
    if (!m_isInitialized || m_context.IsNull()) return;

    m_context->EraseAll(false);
    scheduleRedraw();
    qDebug() << "Viewer3D: Cleared all shapes.";
}

void Viewer3D::fitAll()
{
    if (!m_isInitialized || m_view.IsNull()) return;

    try {
        m_view->FitAll();
        m_view->ZFitAll();
        scheduleRedraw();
        qDebug() << "Viewer3D: FitAll executed.";
    } catch (const Standard_Failure& e) {
        qDebug() << "Viewer3D: Error during FitAll:" << e.GetMessageString();
    }
}

void Viewer3D::fitSelected()
{
    if (!m_isInitialized || m_context.IsNull() || m_view.IsNull()) return;

    if (m_context->NbSelected() > 0) {
        m_context->FitSelected(m_view);
        scheduleRedraw();
        qDebug() << "Viewer3D: FitSelected executed.";
    } else {
        qDebug() << "Viewer3D: FitSelected called but no shape selected. Fitting all.";
        fitAll();
    }
}

// --- Interaction and Configuration ---
void Viewer3D::setSelectionMode(bool enabled)
{
    if (!m_isInitialized || m_context.IsNull()) return;

    m_selectionMode = enabled;
    if (m_selectionMode) {
        m_context->Activate(0);
        qDebug() << "Viewer3D: Selection mode ENABLED.";
    } else {
        m_context->Deactivate(0);
        qDebug() << "Viewer3D: Selection mode DISABLED.";
    }
    m_context->UpdateCurrentViewer();
}

void Viewer3D::setHoverHighlightEnabled(bool enabled)
{
    if (!m_isInitialized || m_context.IsNull()) return;

    m_hoverHighlightEnabled = enabled;
    m_context->SetAutomaticHilight(enabled ? Standard_True : Standard_False);
    
    qDebug() << "Viewer3D: Hover highlight" << (enabled ? "ENABLED" : "DISABLED");
    m_context->UpdateCurrentViewer();
}

// --- Mouse Interaction ---
void Viewer3D::mousePressEvent(QMouseEvent *event)
{
    if (!m_isInitialized || m_view.IsNull()) {
        event->ignore();
        return;
    }
    
    m_lastMousePos = event->pos();
    m_isDragging = true;
    m_dragButton = event->button();
    
    if (event->button() == Qt::LeftButton && m_selectionMode) {
        // Handle selection
        m_context->MoveTo(event->position().x(), event->position().y(), m_view, Standard_True);
        if (event->modifiers() & Qt::ControlModifier) {
            m_context->SelectDetected(AIS_SelectionScheme_XOR);
        } else {
            m_context->SelectDetected(AIS_SelectionScheme_Replace);
        }
        
        // Check if something was selected and emit signal
        if (m_context->HasSelectedShape()) {
            TopoDS_Shape selectedShape = m_context->SelectedShape();
            Standard_Real worldX, worldY, worldZ;
            m_view->Convert(event->position().x(), event->position().y(), worldX, worldY, worldZ);
            gp_Pnt clickPnt(worldX, worldY, worldZ);
            emit shapeSelected(selectedShape, clickPnt);
        }
    }
    
    event->accept();
}

void Viewer3D::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isInitialized || m_view.IsNull()) {
        event->ignore();
        return;
    }
    
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        
        if (m_dragButton == Qt::LeftButton && !m_selectionMode) {
            // Rotation
            m_view->StartRotation(m_lastMousePos.x(), m_lastMousePos.y());
            m_view->Rotation(event->position().x(), event->position().y());
        } else if (m_dragButton == Qt::MiddleButton) {
            // Panning
            m_view->Pan(delta.x(), -delta.y());
        } else if (m_dragButton == Qt::RightButton) {
            // Zooming - calculate zoom factor based on vertical mouse movement
            double zoomFactor = 1.0 + (event->position().y() - m_lastMousePos.y()) * 0.01;
            double currentZoom = m_view->Scale();
            m_view->SetZoom(currentZoom * zoomFactor);
        }
        
        scheduleRedraw();
    } else if (m_hoverHighlightEnabled) {
        // Handle hover highlighting
        m_context->MoveTo(event->position().x(), event->position().y(), m_view, Standard_True);
    }
    
    m_lastMousePos = event->pos();
    event->accept();
}

void Viewer3D::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_isInitialized || m_view.IsNull()) {
        event->ignore();
        return;
    }
    
    m_isDragging = false;
    m_dragButton = Qt::NoButton;
    
    event->accept();
}

void Viewer3D::wheelEvent(QWheelEvent *event)
{
    if (!m_isInitialized || m_view.IsNull()) {
        event->ignore();
        return;
    }
    
    const double zoomFactor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QPoint pos = event->position().toPoint();
#else
    const QPoint pos = event->pos();
#endif
    
    double currentZoom = m_view->Scale();
    m_view->SetZoom(currentZoom * zoomFactor);
    
    scheduleRedraw();
    event->accept();
}

// --- View Mode Management ---
void Viewer3D::setViewMode(ViewMode mode)
{
    if (!m_isInitialized || mode == m_currentViewMode) return;
    
    m_currentViewMode = mode;
    
    switch (mode) {
        case ViewMode::Mode3D:
            apply3DView();
            break;
        case ViewMode::Mode2DXZ:
            applyXZPlaneView();
            break;
    }
    
    scheduleRedraw();
    emit viewModeChanged(m_currentViewMode);
}

void Viewer3D::apply3DView()
{
    if (m_view.IsNull()) return;
    
    m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
    setProjection(V3d_XposYnegZpos);
}

void Viewer3D::applyXZPlaneView()
{
    if (m_view.IsNull()) return;
    
    m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
    setProjection(V3d_Ypos);
}

void Viewer3D::setProjection(V3d_TypeOfOrientation orientation)
{
    if (m_view.IsNull()) return;
    
    m_view->SetProj(orientation);
    emit viewModeChanged(m_currentViewMode);
}

// --- Double Click Handling ---
void Viewer3D::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_isInitialized || m_context.IsNull() || m_view.IsNull()) {
        event->ignore();
        return;
    }
    
    // Handle double-click on objects
    m_context->MoveTo(event->position().x(), event->position().y(), m_view, Standard_True);
    m_context->SelectDetected(AIS_SelectionScheme_Replace);
    
    if (m_context->HasSelectedShape()) {
        for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected()) {
            Handle(AIS_InteractiveObject) selectedObject = m_context->SelectedInteractive();
            emit objectDoubleClicked(selectedObject);
            
            // Check if this is a chuck file shape and emit chuck load request
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(selectedObject);
            if (!aisShape.IsNull()) {
                // For demonstration, emit a request for chuck file loading
                // In practice, you'd associate the shape with a file path
                QString chuckFilePath = "example_chuck.step"; // This would be stored with the shape
                emit chuckLoadRequested(chuckFilePath);
            }
        }
    }
    
    event->accept();
}

void Viewer3D::setViewCubeVisible(bool visible)
{
    // Implementation for view cube visibility
    // This would require integrating AIS_ViewCube similar to the OCCT sample
    qDebug() << "Viewer3D: View cube visibility set to:" << visible;
} 