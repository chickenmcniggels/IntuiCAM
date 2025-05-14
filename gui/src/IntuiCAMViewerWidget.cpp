#include "IntuiCAMViewerWidget.h"

// Qt includes
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QDebug>

// OpenCASCADE includes
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <Graphic3d_Camera.hxx>

#if defined(_WIN32)
#include <WNT_Window.hxx>
#elif defined(__APPLE__)
#include <Cocoa_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

IntuiCAMViewerWidget::IntuiCAMViewerWidget(QWidget *parent)
    : QWidget(parent)
    , m_isDragging(false)
    , m_currentViewMode(ViewMode::Rotate)
{
    // Set attributes for OpenGL rendering
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Create context menu and actions
    createActions();
    createContextMenu();
    
    // Initialize OpenCASCADE viewer
    initializeOCCViewer();
    
    // Create update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        if (m_view) {
            update();
        }
    });
    m_updateTimer->start(30); // 30ms = ~33fps
}

IntuiCAMViewerWidget::~IntuiCAMViewerWidget() {
    // OpenCASCADE cleanup handled by shared_ptr
}

void IntuiCAMViewerWidget::displayShape(const TopoDS_Shape& shape) {
    if (!m_context || shape.IsNull()) {
        return;
    }
    
    // Display the shape
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_context->Display(aisShape, Standard_True);
    fitAll();
}

void IntuiCAMViewerWidget::clearDisplay() {
    if (m_context) {
        m_context->RemoveAll(Standard_True);
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::resetView() {
    if (m_view) {
        m_view->ResetViewOrientation();
        m_view->FitAll();
        m_view->ZFitAll();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::viewFront() {
    if (m_view) {
        m_view->SetProj(V3d_Xpos);
        m_view->FitAll();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::viewTop() {
    if (m_view) {
        m_view->SetProj(V3d_Zpos);
        m_view->FitAll();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::viewSide() {
    if (m_view) {
        m_view->SetProj(V3d_Ypos);
        m_view->FitAll();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::fitAll() {
    if (m_view) {
        m_view->FitAll();
        m_view->ZFitAll();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::paintEvent(QPaintEvent* /*event*/) {
    if (m_view) {
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    if (m_view) {
        m_view->MustBeResized();
        m_view->Redraw();
    }
}

void IntuiCAMViewerWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_view) {
        return;
    }
    
    m_lastMousePos = event->pos();
    m_isDragging = true;
    
    if (event->button() == Qt::LeftButton) {
        m_currentViewMode = ViewMode::Rotate;
    } else if (event->button() == Qt::MiddleButton) {
        m_currentViewMode = ViewMode::Pan;
    } else if (event->button() == Qt::RightButton) {
        // Right button shows context menu (handled by contextMenuEvent)
        m_isDragging = false;
    }
}

void IntuiCAMViewerWidget::mouseReleaseEvent(QMouseEvent* /*event*/) {
    m_isDragging = false;
}

void IntuiCAMViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_view || !m_isDragging) {
        return;
    }
    
    QPoint currentPos = event->pos();
    int dx = currentPos.x() - m_lastMousePos.x();
    int dy = currentPos.y() - m_lastMousePos.y();
    
    switch (m_currentViewMode) {
        case ViewMode::Rotate:
            m_view->Rotation(currentPos.x(), currentPos.y());
            break;
        case ViewMode::Pan:
            m_view->Pan(dx, -dy);
            break;
        case ViewMode::Zoom:
            m_view->Zoom(m_lastMousePos.x(), m_lastMousePos.y(), currentPos.x(), currentPos.y());
            break;
    }
    
    m_lastMousePos = currentPos;
}

void IntuiCAMViewerWidget::wheelEvent(QWheelEvent* event) {
    if (!m_view) {
        return;
    }
    
    // Get the delta (positive = zoom in, negative = zoom out)
    double delta = event->angleDelta().y();
    
    // Scale factor (adjust as needed)
    double scaleFactor = 0.1;
    
    if (delta > 0) {
        // Zoom in
        m_view->SetScale(m_view->Scale() * (1.0 + scaleFactor));
    } else {
        // Zoom out
        m_view->SetScale(m_view->Scale() * (1.0 - scaleFactor));
    }
    
    m_view->Redraw();
}

void IntuiCAMViewerWidget::contextMenuEvent(QContextMenuEvent* event) {
    m_contextMenu->exec(event->globalPos());
}

void IntuiCAMViewerWidget::initializeOCCViewer() {
    try {
        // Create a OpenCASCADE graphic driver
        Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
        Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);

        // Create viewer
        m_viewer = std::make_shared<V3d_Viewer>(graphicDriver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();
        
        // Create view
        m_view = std::make_shared<V3d_View>(m_viewer);
        
        // Create native window for OpenCASCADE rendering
#if defined(_WIN32)
        Handle(WNT_Window) window = new WNT_Window((Aspect_Handle)winId());
#elif defined(__APPLE__)
        Handle(Cocoa_Window) window = new Cocoa_Window((NSView*)winId());
#else
        Handle(Xw_Window) window = new Xw_Window(displayConnection, (Window)winId());
#endif
        
        // Set the window for the view
        m_view->SetWindow(window);
        
        if (!window->IsMapped()) {
            window->Map();
        }
        
        // Create interactive context
        m_context = std::make_shared<AIS_InteractiveContext>(m_viewer);
        
        // Set up the view
        m_view->SetBackgroundColor(Quantity_NOC_BLACK);
        m_view->MustBeResized();
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
        
        resetView();
    } catch (const Standard_Failure& e) {
        qCritical() << "OpenCASCADE exception:" << e.GetMessageString();
    } catch (const std::exception& e) {
        qCritical() << "Exception:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception during OpenCASCADE initialization";
    }
}

void IntuiCAMViewerWidget::createActions() {
    // Will be used in context menu
}

void IntuiCAMViewerWidget::createContextMenu() {
    m_contextMenu = new QMenu(this);
    
    // View actions
    QAction* resetViewAction = m_contextMenu->addAction("Reset View");
    connect(resetViewAction, &QAction::triggered, this, &IntuiCAMViewerWidget::resetView);
    
    QAction* fitAllAction = m_contextMenu->addAction("Fit All");
    connect(fitAllAction, &QAction::triggered, this, &IntuiCAMViewerWidget::fitAll);
    
    m_contextMenu->addSeparator();
    
    // View orientation submenu
    QMenu* viewOrientMenu = m_contextMenu->addMenu("View Orientation");
    
    QAction* frontViewAction = viewOrientMenu->addAction("Front");
    connect(frontViewAction, &QAction::triggered, this, &IntuiCAMViewerWidget::viewFront);
    
    QAction* topViewAction = viewOrientMenu->addAction("Top");
    connect(topViewAction, &QAction::triggered, this, &IntuiCAMViewerWidget::viewTop);
    
    QAction* sideViewAction = viewOrientMenu->addAction("Side");
    connect(sideViewAction, &QAction::triggered, this, &IntuiCAMViewerWidget::viewSide);
    
    m_contextMenu->addSeparator();
    
    // Display mode submenu (can be expanded with more options)
    QMenu* displayModeMenu = m_contextMenu->addMenu("Display Mode");
    
    QAction* wireframeAction = displayModeMenu->addAction("Wireframe");
    connect(wireframeAction, &QAction::triggered, [this]() {
        if (m_context) {
            m_context->SetDisplayMode(AIS_WireFrame, Standard_True);
        }
    });
    
    QAction* shadedAction = displayModeMenu->addAction("Shaded");
    connect(shadedAction, &QAction::triggered, [this]() {
        if (m_context) {
            m_context->SetDisplayMode(AIS_Shaded, Standard_True);
        }
    });
} 