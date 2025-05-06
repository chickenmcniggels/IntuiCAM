#include "../include/IntuiCAMViewerWidget.h"

// OpenCASCADE includes for viewer and context
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Quantity_Color.hxx>

// Platform-specific includes for windowing
#ifdef _WIN32
  #include <WNT_Window.hxx>
#elif defined(__APPLE__) && !defined(MACOSX_USE_GLX)
  #include <Cocoa_Window.hxx>
#else
  #include <Xw_Window.hxx>
#endif

using namespace IntuiCAM::GUI;

IntuiCAMViewerWidget::IntuiCAMViewerWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    // Enable depth buffer and anti-aliasing if needed (for future enhancements)
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(fmt);
}

IntuiCAMViewerWidget::~IntuiCAMViewerWidget() {
    // Resources are managed by OpenCASCADE handles and Qt parent-child; no explicit delete needed
}

void IntuiCAMViewerWidget::initializeGL() {
    // Initialize OpenCASCADE 3D viewer components
    m_displayConnection = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(m_displayConnection);

    // Create a V3d viewer (in a default interactive mode)
    m_viewer = new V3d_Viewer(graphicDriver);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();

    // Create a 3D view
    m_view = m_viewer->CreateView();

    // Bind the V3d_View to this QOpenGLWidget's native window
    WId windowHandle = winId();
    Handle(Aspect_Window) wind;
#ifdef _WIN32
    wind = new WNT_Window((Aspect_Handle)windowHandle);
#elif defined(__APPLE__) && !defined(MACOSX_USE_GLX)
    wind = new Cocoa_Window((void*)windowHandle);
#else
    wind = new Xw_Window(m_displayConnection, (Window)windowHandle);
#endif
    m_view->SetWindow(wind);
    if (!wind->IsMapped()) wind->Map();  // Ensure the window is ready for rendering:contentReference[oaicite:1]{index=1}

    // Create the interactive context for managing displayed objects
    m_context = new AIS_InteractiveContext(m_viewer);  //:contentReference[oaicite:2]{index=2}

    // Set an initial background and viewing mode (optional)
    m_view->TriedronDisplay(Aspect_TOTP_RIGHT_LOWER, Quantity_NOC_GRAY90, 0.1);
    m_view->SetBackgroundColor(Quantity_NOC_BLACK);  // Black background
}

void IntuiCAMViewerWidget::resizeGL(int w, int h) {
    if (!m_view.IsNull()) {
        // Notify the V3d_View that the window has resized
        m_view->MustBeResized();
    }
}

void IntuiCAMViewerWidget::paintGL() {
    if (!m_view.IsNull()) {
        // Redraw the OpenCASCADE view
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::displayShape(const TopoDS_Shape& shape) {
    if (m_context.IsNull() || shape.IsNull()) {
        return;
    }
    // Create an AIS_Shape to represent the TopoDS_Shape in the viewer
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_context->Display(aisShape, false);    // Add shape to context (deferred draw)
    m_view->FitAll();                      // Adjust camera to fit the new shape
    update();                              // Schedule a repaint to update the view
}

void IntuiCAMViewerWidget::clear() {
    if (!m_context.IsNull()) {
        m_context->RemoveAll(false);  // Remove all interactive objects from context
        m_view->ZFitAll();           // Reset view if needed (optional)
        update();
    }
}
