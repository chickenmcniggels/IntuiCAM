#include "opengl3dwidget.h"
#include "workspacecontroller.h"
#include "rawmaterialmanager.h"

#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

// Additional OpenCASCADE includes
#include <Aspect_Handle.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <SelectMgr_SortCriterion.hxx>
#include <Prs3d_Drawer.hxx>

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
    , m_robustRefreshTimer(new QTimer(this))
    , m_isInitialized(false)
    , m_needsRefresh(false)
    , m_selectionMode(false)
    , m_autoFitEnabled(true)
    , m_hoverHighlightEnabled(true)
    , m_currentViewMode(ViewMode::Mode3D)
    , m_stored3DScale(1.0)
    , m_has3DCameraState(false)
    , m_workspaceController(nullptr)
{
    // Enable mouse tracking for proper interaction
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Set update behavior to ensure consistent rendering
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    
    // Ensure the widget gets proper resize events
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen, false);  // Important for proper rendering
    setAttribute(Qt::WA_NativeWindow, true);    // Helps with focus management
    
    // Setup continuous update timer
    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    
    // Setup robust refresh timer for recovery from black screens
    m_robustRefreshTimer->setSingleShot(true);
    m_robustRefreshTimer->setInterval(50); // Quick recovery
    connect(m_robustRefreshTimer, &QTimer::timeout, this, [this]() {
        forceRedraw();
    });
    
    // Connect to application focus changes to handle external app switching
    connect(qApp, &QApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        qDebug() << "Application state changed to:" << state;
        if (state == Qt::ApplicationActive && !m_view.IsNull() && m_isInitialized) {
            qDebug() << "Application became active - ensuring viewer ready";
            ensureViewerReady();
            // Use a longer delay for application reactivation
            QTimer::singleShot(100, this, [this]() {
                forceRedraw();
            });
        }
    });
    
    // Also monitor focus changes at application level
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now) {
        Q_UNUSED(old)
        if (now == this && !m_view.IsNull() && m_isInitialized) {
            qDebug() << "Focus changed to this widget - ensuring ready";
            ensureViewerReady();
        }
    });
    
    qDebug() << "OpenGL3DWidget created as pure visualization component with view mode support";
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
    if (!m_view.IsNull() && !m_window.IsNull() && m_isInitialized)
    {
        // Ensure we have a valid context and proper size
        if (width <= 0 || height <= 0) {
            return;
        }
        
        makeCurrent();
        
        try {
            // Tell OpenCASCADE that the view must be resized
            m_view->MustBeResized();
            
            // Update the window size immediately
            m_window->DoResize();
            
            // Force immediate redraw with proper viewport
            m_view->Redraw();
            
            // Ensure immediate flush
            if (context()) {
                context()->functions()->glFlush();
            }
            
            qDebug() << "OpenGL3DWidget resized to:" << width << "x" << height;
        } catch (...) {
            qDebug() << "Error during resize, marking for refresh";
            m_needsRefresh = true;
            m_robustRefreshTimer->start();
        }
    }
}

void OpenGL3DWidget::resizeEvent(QResizeEvent *event)
{
    // Call the base class implementation first
    QOpenGLWidget::resizeEvent(event);
    
    // Additional resize handling for smooth OpenCASCADE integration
    if (!m_view.IsNull() && !m_window.IsNull() && m_isInitialized)
    {
        QSize newSize = event->size();
        
        // Ensure minimum size and valid dimensions
        if (newSize.width() > 0 && newSize.height() > 0)
        {
            try {
                makeCurrent();
                
                // Immediate resize handling for smoother experience
                m_view->MustBeResized();
                m_window->DoResize();
                
                // Schedule multiple deferred updates for ultra-smooth resizing
                QTimer::singleShot(0, this, [this]() {
                    if (!m_view.IsNull() && m_isInitialized) {
                        forceRedraw();
                    }
                });
                
                QTimer::singleShot(16, this, [this]() {
                    if (!m_view.IsNull() && m_isInitialized) {
                        forceRedraw();
                    }
                });
            } catch (...) {
                qDebug() << "Error during resize event, scheduling recovery";
                m_needsRefresh = true;
                m_robustRefreshTimer->start();
            }
        }
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
        
        // Mark as initialized
        m_isInitialized = true;
        
        // Emit initialization signal
        emit viewerInitialized();
        
        qDebug() << "OpenCASCADE 3D viewer initialized successfully";
        
    } catch (const std::exception& e) {
        qDebug() << "Error initializing OpenCASCADE viewer:" << e.what();
        m_isInitialized = false;
    } catch (...) {
        qDebug() << "Unknown error initializing OpenCASCADE viewer";
        m_isInitialized = false;
    }
}

void OpenGL3DWidget::updateView()
{
    if (!m_view.IsNull() && !m_window.IsNull() && m_isInitialized)
    {
        // Ensure the OpenGL context is current before updating
        makeCurrent();
        
        // Check if the widget is visible and has a valid size
        if (isVisible() && width() > 0 && height() > 0)
        {
            try {
                // Ensure the view is properly sized
                if (m_needsRefresh) {
                    m_view->MustBeResized();
                    m_window->DoResize();
                    m_needsRefresh = false;
                }
                
                // Force redraw regardless of focus state with error handling
                m_view->Redraw();
                
                // Ensure immediate flush for better responsiveness without full swap
                if (context() && context()->surface()) {
                    context()->functions()->glFlush();
                    // Use doneCurrent() instead of swapBuffers to prevent frame buffer issues
                    doneCurrent();
                    makeCurrent();
                }
            } catch (...) {
                qDebug() << "Error during view update, attempting recovery";
                // Mark for refresh and attempt recovery
                m_needsRefresh = true;
                m_robustRefreshTimer->start();
            }
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
        
        // Only auto-fit if enabled
        if (m_autoFitEnabled) {
            fitAll();
        } else {
            // Just update the view without fitting
            updateView();
        }
        
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
    if (!m_view.IsNull())
    {
        if (m_selectionMode && event->button() == Qt::LeftButton)
        {
            // Handle selection in selection mode
            if (!m_context.IsNull())
            {
                // Perform detection at mouse position
                m_context->MoveTo(event->pos().x(), event->pos().y(), m_view, Standard_False);
                
                if (m_context->HasDetected())
                {
                    // Select the detected entity
                    m_context->SelectDetected();
                    
                    // Process all selected entities
                    for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected())
                    {
                        Handle(SelectMgr_EntityOwner) anOwner = m_context->SelectedOwner();
                        Handle(AIS_InteractiveObject) selectedObject = Handle(AIS_InteractiveObject)::DownCast(anOwner->Selectable());
                        
                        if (!selectedObject.IsNull())
                        {
                            // Try to get the shape from the selected object
                            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(selectedObject);
                            if (!aisShape.IsNull())
                            {
                                TopoDS_Shape selectedShape;
                                
                                // Check if we have a sub-shape owner (face, edge, etc.)
                                Handle(StdSelect_BRepOwner) aBRepOwner = Handle(StdSelect_BRepOwner)::DownCast(anOwner);
                                if (!aBRepOwner.IsNull()) {
                                    // We selected a specific face/edge/vertex
                                    selectedShape = aBRepOwner->Shape();
                                    qDebug() << "Selected sub-shape type:" << selectedShape.ShapeType();
                                } else {
                                    // We selected the whole shape
                                    selectedShape = aisShape->Shape();
                                    qDebug() << "Selected whole shape";
                                }
                                
                                // Calculate 3D point from screen coordinates
                                gp_Pnt clickPoint;
                                if (m_context->HasDetected()) {
                                    // Get the picked point from the detection
                                    Handle(SelectMgr_ViewerSelector) aSelector = m_context->MainSelector();
                                    if (aSelector->NbPicked() > 0) {
                                        SelectMgr_SortCriterion aPickedData = aSelector->PickedData(1);
                                        clickPoint = aPickedData.Point;
                                    } else {
                                        // Fallback to screen coordinate conversion
                                        Standard_Real xv, yv, zv;
                                        m_view->Convert(event->pos().x(), event->pos().y(), xv, yv, zv);
                                        clickPoint = gp_Pnt(xv, yv, zv);
                                    }
                                } else {
                                    // Fallback conversion
                                    Standard_Real xv, yv, zv;
                                    m_view->Convert(event->pos().x(), event->pos().y(), xv, yv, zv);
                                    clickPoint = gp_Pnt(xv, yv, zv);
                                }
                                
                                emit shapeSelected(selectedShape, clickPoint);
                                qDebug() << "Shape selected at point:" << clickPoint.X() << clickPoint.Y() << clickPoint.Z();
                                
                                // Break after first successful selection
                                break;
                            }
                        }
                    }
                    
                    // Clear selection after processing
                    m_context->ClearSelected(Standard_False);
                    updateView();
                } else {
                    qDebug() << "No object detected at mouse position";
                }
            }
            return; // Don't process as normal interaction
        }
        
        // Normal interaction mode
        if (event->button() == Qt::LeftButton)
        {
            // In XZ plane mode, left click does panning instead of rotation
            if (m_currentViewMode == ViewMode::LatheXZ) {
                // Initialize panning for XZ mode
            } else {
                // Normal 3D rotation
                m_view->StartRotation(event->pos().x(), event->pos().y());
            }
        }
    }
    
    m_isDragging = true;
    m_lastMousePos = event->pos();
    m_dragButton = event->button();
}

void OpenGL3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_view.IsNull()) {
        if (m_isDragging) {
            if (m_dragButton == Qt::LeftButton)
            {
                if (m_currentViewMode == ViewMode::LatheXZ) {
                    // In XZ mode, left click performs panning (no rotation allowed)
                    m_view->Pan(event->pos().x() - m_lastMousePos.x(), 
                               m_lastMousePos.y() - event->pos().y());
                } else {
                    // Normal 3D rotation
                    m_view->Rotation(event->pos().x(), event->pos().y());
                }
            }
            else if (m_dragButton == Qt::MiddleButton)
            {
                // Panning (works the same in both modes)
                m_view->Pan(event->pos().x() - m_lastMousePos.x(), 
                           m_lastMousePos.y() - event->pos().y());
            }
            else if (m_dragButton == Qt::RightButton)
            {
                // Zooming (works the same in both modes)
                m_view->Zoom(m_lastMousePos.x(), m_lastMousePos.y(), 
                            event->pos().x(), event->pos().y());
            }
            
            updateView();
        } else if (m_hoverHighlightEnabled && !m_context.IsNull()) {
            // Handle hover highlighting when not dragging
            m_context->MoveTo(event->pos().x(), event->pos().y(), m_view, Standard_True);
            
            // Check if we're hovering over something different
            if (m_context->HasDetected()) {
                Handle(AIS_InteractiveObject) detectedObj = m_context->DetectedInteractive();
                
                // Highlight the detected object if it's not the turning axis face
                if (!detectedObj.IsNull() && detectedObj != m_turningAxisFaceAIS) {
                    // The context will automatically handle highlighting with default colors
                    updateView();
                }
            } else {
                // Clear any previous hover highlighting
                m_context->ClearDetected(Standard_False);
                updateView();
            }
        }
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

void OpenGL3DWidget::setSelectionMode(bool enabled)
{
    m_selectionMode = enabled;
    
    if (enabled) {
        // Change cursor to indicate selection mode
        setCursor(Qt::CrossCursor);
        
        // Enable proper selection modes for the context
        if (!m_context.IsNull()) {
            // Set automatic highlighting for better visual feedback
            m_context->SetAutomaticHilight(Standard_True);
            
            // Enable default selection mode (0) for whole objects
            m_context->SetAutoActivateSelection(Standard_True);
            
            // Activate selection for all displayed shapes except raw material
            AIS_ListOfInteractive allObjects;
            m_context->DisplayedObjects(allObjects);
            
            // Get raw material AIS object if workspace controller is available
            Handle(AIS_Shape) rawMaterialAIS;
            if (m_workspaceController) {
                RawMaterialManager* rawMaterialManager = m_workspaceController->getRawMaterialManager();
                if (rawMaterialManager) {
                    rawMaterialAIS = rawMaterialManager->getCurrentRawMaterialAIS();
                }
            }
            
            for (AIS_ListOfInteractive::Iterator anIter(allObjects); anIter.More(); anIter.Next()) {
                Handle(AIS_InteractiveObject) anObj = anIter.Value();
                Handle(AIS_Shape) aShape = Handle(AIS_Shape)::DownCast(anObj);
                if (!aShape.IsNull()) {
                    // Skip raw material - it should remain non-selectable
                    if (!rawMaterialAIS.IsNull() && aShape == rawMaterialAIS) {
                        qDebug() << "Skipping selection activation for raw material";
                        continue;
                    }
                    
                    // Activate selection for whole shape (mode 0)
                    m_context->Activate(aShape, 0, Standard_False);
                    // Also activate face selection (mode 4) for cylindrical faces
                    m_context->Activate(aShape, 4, Standard_False);
                    // And edge selection (mode 2) for cylindrical edges
                    m_context->Activate(aShape, 2, Standard_False);
                }
            }
            
            qDebug() << "Selection mode enabled - enhanced detection for shapes, faces, and edges";
        }
    } else {
        // Restore normal cursor
        setCursor(Qt::ArrowCursor);
        
        // Deactivate all selection modes
        if (!m_context.IsNull()) {
            m_context->Deactivate();
            m_context->ClearSelected(Standard_False);
            updateView();
        }
        
        qDebug() << "Selection mode disabled";
    }
}

void OpenGL3DWidget::focusInEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusInEvent(event);
    qDebug() << "OpenGL3DWidget gained focus from:" << event->reason();
    
    // Immediate update when gaining focus to prevent black screen
    if (!m_view.IsNull() && m_isInitialized)
    {
        ensureViewerReady();
        
        // Additional immediate redraw for critical focus scenarios
        QTimer::singleShot(1, this, [this]() {
            forceRedraw();
        });
    }
}

void OpenGL3DWidget::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    qDebug() << "OpenGL3DWidget lost focus due to:" << event->reason();
    
    // Enhanced focus loss handling to prevent black screen
    if (!m_view.IsNull() && m_isInitialized)
    {
        // Mark for refresh when focus returns
        m_needsRefresh = true;
        
        // Use a very short timer to ensure context is still valid
        QTimer::singleShot(1, this, [this]() {
            if (!m_view.IsNull() && isVisible()) {
                forceRedraw();
            }
        });
    }
}

void OpenGL3DWidget::showEvent(QShowEvent *event)
{
    QOpenGLWidget::showEvent(event);
    qDebug() << "OpenGL3DWidget show event";
    
    if (m_continuousUpdate) {
        m_updateTimer->start();
    }
    
    // Enhanced show event to ensure proper display
    if (!m_view.IsNull() && m_isInitialized)
    {
        ensureViewerReady();
        
        // Force immediate redraw with slight delay to ensure context is ready
        QTimer::singleShot(10, this, [this]() {
            forceRedraw();
        });
    }
}

void OpenGL3DWidget::hideEvent(QHideEvent *event)
{
    QOpenGLWidget::hideEvent(event);
    m_updateTimer->stop();
}

void OpenGL3DWidget::forceRedraw()
{
    if (!m_view.IsNull() && isVisible() && width() > 0 && height() > 0)
    {
        try {
            makeCurrent();
            
            // Ensure proper sizing before redraw
            if (m_needsRefresh) {
                m_view->MustBeResized();
                m_window->DoResize();
                m_needsRefresh = false;
            }
            
            m_view->Redraw();
            
            // Use a more conservative flush approach
            if (context()) {
                context()->functions()->glFlush();
                context()->functions()->glFinish(); // Ensure completion
            }
        } catch (...) {
            qDebug() << "Error during force redraw - will retry";
            m_needsRefresh = true;
            // Schedule another attempt with a short delay
            QTimer::singleShot(10, this, [this]() {
                if (!m_view.IsNull() && isVisible()) {
                    try {
                        makeCurrent();
                        m_view->Redraw();
                        if (context()) {
                            context()->functions()->glFlush();
                        }
                    } catch (...) {
                        qDebug() << "Backup redraw also failed";
                    }
                }
            });
        }
    }
}

void OpenGL3DWidget::ensureViewerReady()
{
    if (!m_view.IsNull() && !m_window.IsNull() && isVisible())
    {
        makeCurrent();
        m_view->MustBeResized();
        m_window->DoResize();
        m_needsRefresh = true;
        m_robustRefreshTimer->start();
    }
}

void OpenGL3DWidget::handleActivationChange(bool active)
{
    qDebug() << "OpenGL3DWidget activation changed:" << active;
    if (active && !m_view.IsNull())
    {
        ensureViewerReady();
    }
}

void OpenGL3DWidget::changeEvent(QEvent *event)
{
    QOpenGLWidget::changeEvent(event);
    
    if (event->type() == QEvent::ActivationChange)
    {
        handleActivationChange(isActiveWindow());
    }
    else if (event->type() == QEvent::WindowStateChange)
    {
        qDebug() << "OpenGL3DWidget window state changed";
        if (!isMinimized() && !m_view.IsNull())
        {
            ensureViewerReady();
        }
    }
}

void OpenGL3DWidget::paintEvent(QPaintEvent *event)
{
    // Ensure we always have a valid rendering state
    if (!m_view.IsNull() && isVisible())
    {
        makeCurrent();
        // Let the base class handle the actual painting
        QOpenGLWidget::paintEvent(event);
        
        // Force immediate flush for responsiveness
        if (context()) {
            context()->functions()->glFlush();
        }
    }
    else
    {
        QOpenGLWidget::paintEvent(event);
    }
}

void OpenGL3DWidget::enterEvent(QEnterEvent *event)
{
    QOpenGLWidget::enterEvent(event);
    // Ensure the viewer is ready when mouse enters
    if (!m_view.IsNull())
    {
        m_robustRefreshTimer->start();
    }
}

void OpenGL3DWidget::leaveEvent(QEvent *event)
{
    QOpenGLWidget::leaveEvent(event);
    // Optional: Could implement specific leave behavior if needed
}

bool OpenGL3DWidget::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Show:
        case QEvent::WindowActivate:
        case QEvent::FocusIn:
            qDebug() << "OpenGL3DWidget critical event:" << event->type();
            if (!m_view.IsNull())
            {
                ensureViewerReady();
            }
            break;
            
        case QEvent::WindowDeactivate:
            // Prepare for potential reactivation
            m_needsRefresh = true;
            break;
            
        case QEvent::UpdateRequest:
            // Handle update requests immediately
            if (!m_view.IsNull() && isVisible())
            {
                forceRedraw();
            }
            break;
            
        default:
            break;
    }
    
    return QOpenGLWidget::event(event);
}

void OpenGL3DWidget::setTurningAxisFace(const TopoDS_Shape& axisShape)
{
    if (m_context.IsNull() || axisShape.IsNull()) {
        return;
    }
    
    // Clear any existing turning axis face display
    clearTurningAxisFace();
    
    try {
        // Create AIS shape for the turning axis face
        m_turningAxisFaceAIS = new AIS_Shape(axisShape);
        m_turningAxisFace = axisShape;
        
        // Set special highlighting material for turning axis (orange/gold color)
        Graphic3d_MaterialAspect axisMaterial(Graphic3d_NOM_GOLD);
        axisMaterial.SetColor(Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB)); // Orange
        axisMaterial.SetTransparency(Standard_ShortReal(0.3));
        
        m_turningAxisFaceAIS->SetMaterial(axisMaterial);
        m_turningAxisFaceAIS->SetTransparency(Standard_ShortReal(0.3));
        
        // Display with special highlighting
        m_context->Display(m_turningAxisFaceAIS, AIS_Shaded, 0, false);
        
        // Create a drawer for permanent highlight with orange color
        Handle(Prs3d_Drawer) highlightDrawer = new Prs3d_Drawer();
        highlightDrawer->SetColor(Quantity_NOC_ORANGE);
        highlightDrawer->SetTransparency(Standard_ShortReal(0.3));
        
        // Set permanent highlight for the turning axis face
        m_context->HilightWithColor(m_turningAxisFaceAIS, highlightDrawer, Standard_False);
        
        updateView();
        
        qDebug() << "Turning axis face highlighted";
        
    } catch (const std::exception& e) {
        qDebug() << "Error highlighting turning axis face:" << e.what();
    }
}

void OpenGL3DWidget::clearTurningAxisFace()
{
    if (!m_context.IsNull() && !m_turningAxisFaceAIS.IsNull()) {
        try {
            // Clear highlight and remove from display
            m_context->Unhilight(m_turningAxisFaceAIS, Standard_False);
            m_context->Remove(m_turningAxisFaceAIS, Standard_False);
            
            m_turningAxisFaceAIS.Nullify();
            m_turningAxisFace.Nullify();
            
            updateView();
            
            qDebug() << "Turning axis face highlighting cleared";
            
        } catch (const std::exception& e) {
            qDebug() << "Error clearing turning axis face:" << e.what();
        }
    }
}

void OpenGL3DWidget::setViewMode(ViewMode mode)
{
    if (!m_view.IsNull() && m_currentViewMode != mode) {
        // Store current 3D camera state if we're switching away from 3D mode
        if (m_currentViewMode == ViewMode::Mode3D) {
            store3DCameraState();
        }
        
        m_currentViewMode = mode;
        applyCameraForViewMode();
        
        // Emit signal for UI updates
        emit viewModeChanged(mode);
        
        // Update the view
        updateView();
        
        qDebug() << "View mode changed to:" << (mode == ViewMode::Mode3D ? "3D" : "Lathe XZ");
    }
}

void OpenGL3DWidget::toggleViewMode()
{
    ViewMode newMode = (m_currentViewMode == ViewMode::Mode3D) ? ViewMode::LatheXZ : ViewMode::Mode3D;
    setViewMode(newMode);
}

void OpenGL3DWidget::applyCameraForViewMode()
{
    if (m_view.IsNull()) {
        return;
    }
    
    try {
        switch (m_currentViewMode) {
            case ViewMode::Mode3D:
                setupCamera3D();
                break;
                
            case ViewMode::LatheXZ:
                setupCameraXZ();
                break;
        }
        
        m_view->ZFitAll();
        m_view->Redraw();
        
    } catch (const std::exception& e) {
        qDebug() << "Error applying camera for view mode:" << e.what();
    }
}

void OpenGL3DWidget::setupCamera3D()
{
    if (m_view.IsNull()) {
        return;
    }
    
    // Restore previous 3D camera state if available
    if (m_has3DCameraState) {
        restore3DCameraState();
    } else {
        // Set up default 3D perspective view
        m_view->SetAt(0.0, 0.0, 0.0);
        m_view->SetEye(100.0, 100.0, 100.0);
        m_view->SetUp(0.0, 0.0, 1.0);
        // Set perspective view (default for 3D mode)
        
        // Ensure the view shows the coordinate trihedron
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
    }
    
    qDebug() << "3D camera mode configured";
}

void OpenGL3DWidget::setupCameraXZ()
{
    if (m_view.IsNull()) {
        return;
    }
    
    // Set up XZ plane view for lathe operations
    // Standard lathe coordinate system: X increases top to bottom, Z increases right to left
    // This means we need to look from the Y-negative direction toward the origin
    // and orient the view so X goes down and Z goes left
    
    // Camera position: Look from negative Y toward origin (flips Z axis)
    gp_Pnt eye(0.0, -200.0, 0.0);    // Camera position on negative Y axis
    gp_Pnt at(0.0, 0.0, 0.0);       // Look at origin
    gp_Dir up(-1.0, 0.0, 0.0);      // X axis points down (negative X direction as up vector)
    
    m_view->SetAt(at.X(), at.Y(), at.Z());
    m_view->SetEye(eye.X(), eye.Y(), eye.Z());
    m_view->SetUp(up.X(), up.Y(), up.Z());
    
    // For orthographic projection in XZ mode, we'll use view manipulation
    // The view type is set during view creation, but we can achieve orthographic-like
    // behavior by using appropriate camera positioning and scaling
    
    // Position camera very far away to approximate orthographic projection
    gp_Pnt farEye(0.0, -10000.0, 0.0);  // Very far away on negative Y axis (matches flipped Z)
    m_view->SetEye(farEye.X(), farEye.Y(), farEye.Z());
    
    // Hides the 3D trihedron as it's not relevant for 2D lathe view
    // m_view->TriedronErase();
    
    // Fit all to ensure objects are visible in the new view
    m_view->FitAll();
    
    qDebug() << "XZ lathe camera mode configured (X: top to bottom, Z: right to left)";
}

void OpenGL3DWidget::store3DCameraState()
{
    if (m_view.IsNull()) {
        return;
    }
    
    try {
        // Store current camera parameters
        Standard_Real eyeX, eyeY, eyeZ;
        Standard_Real atX, atY, atZ;
        Standard_Real upX, upY, upZ;
        
        m_view->Eye(eyeX, eyeY, eyeZ);
        m_view->At(atX, atY, atZ);
        m_view->Up(upX, upY, upZ);
        
        m_stored3DEye = gp_Pnt(eyeX, eyeY, eyeZ);
        m_stored3DAt = gp_Pnt(atX, atY, atZ);
        m_stored3DUp = gp_Dir(upX, upY, upZ);
        m_stored3DScale = m_view->Scale();
        
        m_has3DCameraState = true;
        
        qDebug() << "3D camera state stored";
        
    } catch (const std::exception& e) {
        qDebug() << "Error storing 3D camera state:" << e.what();
        m_has3DCameraState = false;
    }
}

void OpenGL3DWidget::restore3DCameraState()
{
    if (m_view.IsNull() || !m_has3DCameraState) {
        return;
    }
    
    try {
        // Restore stored camera parameters
        m_view->SetAt(m_stored3DAt.X(), m_stored3DAt.Y(), m_stored3DAt.Z());
        m_view->SetEye(m_stored3DEye.X(), m_stored3DEye.Y(), m_stored3DEye.Z());
        m_view->SetUp(m_stored3DUp.X(), m_stored3DUp.Y(), m_stored3DUp.Z());
        m_view->SetScale(m_stored3DScale);
        
        // Restore perspective projection (default for 3D mode)
        
        // Restore trihedron display
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
        
        qDebug() << "3D camera state restored";
        
    } catch (const std::exception& e) {
        qDebug() << "Error restoring 3D camera state:" << e.what();
    }
} 