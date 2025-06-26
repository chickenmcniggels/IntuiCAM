#include "opengl3dwidget.h"
#include "workspacecontroller.h"
#include "rawmaterialmanager.h"
#include "chuckmanager.h"
#include "workpiecemanager.h"

#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QSet>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QDateTime>
#include <QSurfaceFormat>

// Additional OpenCASCADE includes
#include <Aspect_Handle.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_AmbientLight.hxx>
#include <Quantity_Color.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_SelectionScheme.hxx>
#include <AIS_DisplayStatus.hxx>
#include <NCollection_Mat4.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <SelectMgr_SortCriterion.hxx>
#include <Prs3d_Drawer.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

OpenGL3DWidget::OpenGL3DWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_isDragging(false)
    , m_isDragStarted(false)
    , m_dragButton(Qt::NoButton)
    , m_isMousePressed(false)
    , m_continuousUpdate(false)
    , m_updateTimer(new QTimer(this))
    , m_redrawThrottleTimer(nullptr)
    , m_isInitialized(false)
    , m_selectionMode(false)
    , m_autoFitEnabled(true)
    , m_hoverHighlightEnabled(true)
    , m_currentViewMode(ViewMode::Mode3D)
    , m_stored3DScale(1.0)
    , m_stored3DProjection(Graphic3d_Camera::Projection_Orthographic)
    , m_has3DCameraState(false)
    , m_lockedXZEye(0.0, -1000.0, 0.0)
    , m_lockedXZAt(0.0, 0.0, 0.0)
    , m_lockedXZUp(-1.0, 0.0, 0.0)
    , m_workspaceController(nullptr)
    , m_gridVisible(false)
    , m_toolpathsVisible(true)
    , m_profilesVisible(true)
{
    // Enable mouse tracking for proper interaction
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Preserve the previous frame so the view does not clear when the widget
    // temporarily loses focus. This follows the Qt documentation which states
    // that in non-preserved mode the buffers are invalidated after each frame
    // and the view must be fully redrawn. Using PartialUpdate avoids a black
    // screen when other widgets gain focus.
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    
    // Proper OpenGL context attributes for stable rendering
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // Anti-aliasing
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setVersion(3, 3); // Minimum OpenGL 3.3
    setFormat(format);
    
    // Widget attributes for stable rendering - ESSENTIAL for preventing black screen
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_PaintOnScreen, false);
    setAttribute(Qt::WA_AlwaysStackOnTop, false);
    setAttribute(Qt::WA_NativeWindow, false); // Prevents some focus-related black screen issues
    setAttribute(Qt::WA_DontCreateNativeAncestors, true); // Improves widget stability
    
    // Setup redraw throttling timer to prevent excessive redraws
    m_redrawThrottleTimer = new QTimer(this);
    m_redrawThrottleTimer->setSingleShot(true);
    m_redrawThrottleTimer->setInterval(16); // ~60 FPS max
    connect(m_redrawThrottleTimer, &QTimer::timeout, this, [this]() {
        if (!m_view.IsNull() && !m_window.IsNull() && m_isInitialized) {
            // CRITICAL: Ensure context is current before redraw
            makeCurrent();
            m_view->Redraw();
            // Don't call doneCurrent() here as it can cause black screens
        }
    });
    
    // Optional timer for continuous updates (animations)
    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        throttledRedraw();
    });
    
    qDebug() << "OpenGL3DWidget created with improved stability and performance";
}

OpenGL3DWidget::~OpenGL3DWidget()
{
    // CRITICAL: Make current before cleanup to prevent crashes
    makeCurrent();
    
    // Stop and cleanup timers
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    if (m_redrawThrottleTimer) {
        m_redrawThrottleTimer->stop();
        delete m_redrawThrottleTimer;
        m_redrawThrottleTimer = nullptr;
    }
    
    // Clean up lathe grid if present
    removeLatheGrid();
    
    // Release OpenCASCADE resources in proper order
    if (!m_context.IsNull()) {
        m_context->RemoveAll(Standard_False);
    }
    m_context.Nullify();
    m_view.Nullify();
    m_viewer.Nullify();
    m_window.Nullify();
    
    doneCurrent();
    
    qDebug() << "OpenGL3DWidget destroyed and resources released";
}

void OpenGL3DWidget::initializeGL()
{
    // ESSENTIAL: Ensure context is current
    makeCurrent();
    initializeViewer();
}

void OpenGL3DWidget::paintGL()
{
    // ESSENTIAL: Always ensure context is current before rendering
    // This prevents black screen when other widgets take focus
    makeCurrent(); // makeCurrent() returns void, so we can't check its return value
    
    // Additional validation to prevent black screen
    if (!m_isInitialized || m_view.IsNull() || m_context.IsNull()) {
        // Clear to background color if not properly initialized
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    
    updateView();
    
    // DON'T call doneCurrent() here - it can cause black screens
    // Qt will handle context switching automatically
}

void OpenGL3DWidget::resizeGL(int width, int height)
{
    if (!m_view.IsNull() && !m_window.IsNull() && m_isInitialized)
    {
        // Validate dimensions
        if (width <= 0 || height <= 0) {
            qDebug() << "Invalid resize dimensions:" << width << "x" << height;
            return;
        }
        
        try {
            // CRITICAL: Ensure current OpenGL context - prevents black screen during resize
            makeCurrent(); // makeCurrent() returns void
            
            // Tell OpenCASCADE about the resize
            m_view->MustBeResized();
            m_window->DoResize();
            
            // Update viewport and redraw immediately (resize needs immediate update)
            updateView();
            
            qDebug() << "OpenGL3DWidget successfully resized to:" << width << "x" << height;
            
        } catch (const std::exception& e) {
            qDebug() << "Error during resize:" << e.what();
            // Fallback: clear screen to prevent artifacts
            glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } catch (...) {
            qDebug() << "Unknown error during resize";
            // Fallback: clear screen to prevent artifacts
            glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
                m_view->MustBeResized();
                m_window->DoResize();
                update();
            } catch (...) {
                qDebug() << "Error during resize event";
            }
        }
    }
}

void OpenGL3DWidget::initializeViewer()
{
    try {
        // CRITICAL: Ensure context is current
        makeCurrent();
        
        // Create display connection
        Handle(Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection();
        
        // Create graphic driver
        Handle(OpenGl_GraphicDriver) aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
        
        // Create viewer
        m_viewer = new V3d_Viewer(aGraphicDriver);
        
        // Create view
        m_view = m_viewer->CreateView();
        
        // Create window wrapper
#ifdef _WIN32
        m_window = new WNT_Window((Aspect_Handle)winId());
#else
        m_window = new Xw_Window(aDisplayConnection, (Window)winId());
#endif
        
        // Set the window
        m_view->SetWindow(m_window);
        
        // Create interactive context
        m_context = new AIS_InteractiveContext(m_viewer);
        
        // Set up lighting
        Handle(V3d_DirectionalLight) aLight = new V3d_DirectionalLight(V3d_Zneg, Quantity_NOC_WHITE, Standard_True);
        m_viewer->AddLight(aLight);
        m_viewer->SetLightOn();
        
        // Basic view setup
        m_view->SetBackgroundColor(Quantity_NOC_GRAY90);
        m_view->MustBeResized();
        
        // Set up default camera for 3D view
        setupCamera3D();
        
        // Initialize the window
        if (!m_window->IsMapped()) {
            m_window->Map();
        }
        
        m_isInitialized = true;
        
        qDebug() << "OpenGL3DWidget viewer initialized successfully";
        emit viewerInitialized();
        
    } catch (const Standard_Failure& ex) {
        qDebug() << "Error initializing viewer:" << ex.GetMessageString();
        m_isInitialized = false;
    } catch (const std::exception& e) {
        qDebug() << "Error initializing viewer:" << e.what();
        m_isInitialized = false;
    } catch (...) {
        qDebug() << "Unknown error during viewer initialization";
        m_isInitialized = false;
    }
}

void OpenGL3DWidget::updateView()
{
    // ESSENTIAL: Ensure context is current before any OpenCASCADE operations
    // This is the critical fix for black screen issues
    if (!m_view.IsNull() && m_isInitialized) {
        // Always make context current before OpenCASCADE operations
        makeCurrent(); // makeCurrent() returns void
        
        try {
            m_view->Invalidate();
            m_view->Redraw();
            
            // Ensure the frame is flushed to prevent partial renders
            glFlush();
            
        } catch (const std::exception& e) {
            qDebug() << "Error in updateView:" << e.what();
        }
        
        // DON'T call doneCurrent() - let Qt handle context management
    }
}

void OpenGL3DWidget::mousePressEvent(QMouseEvent *event)
{
    // Store mouse position and button state
    m_lastMousePos = event->pos();
    m_dragButton = event->button();
    m_isMousePressed = true;

    // Handle selection mode first
    if (m_selectionMode && event->button() == Qt::LeftButton && !m_context.IsNull()) {
        makeCurrent(); // makeCurrent() returns void
        
        m_context->MoveTo(event->pos().x(), event->pos().y(), m_view, Standard_True);
        if (m_context->HasDetected()) {
            m_context->SelectDetected(AIS_SelectionScheme_Replace);
            for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected()) {
                Handle(AIS_Shape) ais = Handle(AIS_Shape)::DownCast(m_context->SelectedInteractive());
                if (!ais.IsNull()) {
                    Standard_Real x, y, z;
                    m_view->Convert(event->pos().x(), event->pos().y(), x, y, z);
                    emit shapeSelected(ais->Shape(), gp_Pnt(x, y, z));
                }
            }
        }
        // In selection mode, don't start dragging
        m_isDragging = false;
        return;
    }
    
    // Only start dragging for navigation (not in selection mode)
    m_isDragging = true;
    
    // Accept the event to ensure we receive move and release events
    event->accept();
}

void OpenGL3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_view.IsNull()) {
        return;
    }

    // Calculate smooth mouse delta
    QPoint currentPos = event->pos();
    QPoint delta = currentPos - m_lastMousePos;
    
    // Only process mouse movement if we're actually dragging and not in selection mode
    if (m_isDragging && !m_selectionMode && m_isMousePressed) {
        
        // CRITICAL: Ensure context is current for interaction - prevents black screen
        makeCurrent(); // makeCurrent() returns void
        
        // Check if we're in XZ lathe mode and restrict rotation
        if (m_currentViewMode == ViewMode::LatheXZ) {
            // In XZ lathe mode, ONLY allow panning and zooming - NO rotation
            if ((event->buttons() & Qt::MiddleButton) || 
                ((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ShiftModifier)) ||
                (event->buttons() & Qt::LeftButton)) { // All mouse drags become panning in XZ mode
                
                // Use OpenCASCADE's Pan method with proper delta calculation
                m_view->Pan(currentPos.x() - m_lastMousePos.x(), m_lastMousePos.y() - currentPos.y());
                throttledRedraw();
            }
            // Rotation is completely disabled in XZ mode to maintain plane lock
        } else {
            // Full 3D mode - allow all navigation
            // Left mouse button: Rotation (improved handling)
            if ((event->buttons() & Qt::LeftButton) && m_dragButton == Qt::LeftButton) {
                // Use OpenCASCADE's StartRotation and Rotation methods for smooth rotation
                if (!m_isDragStarted) {
                    m_view->StartRotation(m_lastMousePos.x(), m_lastMousePos.y());
                    m_isDragStarted = true;
                }
                m_view->Rotation(currentPos.x(), currentPos.y());
                throttledRedraw();
            }
            // Middle mouse button OR Shift+Left: Panning (fixed implementation)
            else if ((event->buttons() & Qt::MiddleButton) || 
                     ((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ShiftModifier))) {
                // Use OpenCASCADE's Pan method with proper delta calculation
                m_view->Pan(currentPos.x() - m_lastMousePos.x(), m_lastMousePos.y() - currentPos.y());
                throttledRedraw();
            }
        }
        
    } else if (m_selectionMode && !m_context.IsNull()) {
        // In selection mode, just provide hover feedback
        makeCurrent(); // makeCurrent() returns void
        m_context->MoveTo(currentPos.x(), currentPos.y(), m_view, Standard_False);
    }

    // Update last mouse position for next move event
    m_lastMousePos = currentPos;
    
    // Accept the event
    event->accept();
}

void OpenGL3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // Clear mouse state
    m_isDragging = false;
    m_isDragStarted = false;
    m_dragButton = Qt::NoButton;
    m_isMousePressed = false;
    
    // Accept the event
    event->accept();
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

// CRITICAL: Focus event handling to prevent black screen
void OpenGL3DWidget::focusInEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusInEvent(event);
    
    // Schedule an update so paintGL() is invoked. According to the Qt
    // documentation, update() should be used when a repaint is required from
    // places outside of paintGL(). This prevents the viewer from going black
    // when focus changes.
    if (m_isInitialized && !m_view.IsNull()) {
        update();
    }
}

void OpenGL3DWidget::focusOutEvent(QFocusEvent *event)
{
    QOpenGLWidget::focusOutEvent(event);
    // Schedule a repaint when focus is lost to ensure the framebuffer remains
    // valid. Qt's documentation recommends calling update() when a repaint is
    // needed outside of paintGL(). This avoids the viewer turning black when
    // interacting with other widgets.
    if (m_isInitialized && !m_view.IsNull()) {
        update();
    }
    // Don't call doneCurrent() here as it can cause black screen
}

void OpenGL3DWidget::showEvent(QShowEvent *event)
{
    QOpenGLWidget::showEvent(event);
    
    // Request a repaint when the widget becomes visible. update() will trigger
    // paintGL() where the actual rendering occurs.
    if (m_isInitialized && !m_view.IsNull()) {
        update();
    }
}

void OpenGL3DWidget::hideEvent(QHideEvent *event)
{
    QOpenGLWidget::hideEvent(event);
    // Don't do anything special here to avoid context issues
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

            // Get raw material, chuck and workpiece objects
            Handle(AIS_Shape) rawMaterialAIS;
            Handle(AIS_Shape) chuckAIS;
            QSet<Handle(AIS_Shape)> workpieceSet;
            if (m_workspaceController) {
                RawMaterialManager* rawMaterialManager = m_workspaceController->getRawMaterialManager();
                if (rawMaterialManager) {
                    rawMaterialAIS = rawMaterialManager->getCurrentRawMaterialAIS();
                }
                ChuckManager* chuckManager = m_workspaceController->getChuckManager();
                if (chuckManager) {
                    chuckAIS = chuckManager->getChuckAIS();
                }
                WorkpieceManager* wpMgr = m_workspaceController->getWorkpieceManager();
                if (wpMgr) {
                    QVector<Handle(AIS_Shape)> wp = wpMgr->getWorkpieces();
                    for (const Handle(AIS_Shape)& s : wp) {
                        workpieceSet.insert(s);
                    }
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

                    // Skip chuck - it should remain non-selectable
                    if (!chuckAIS.IsNull() && aShape == chuckAIS) {
                        qDebug() << "Skipping selection activation for chuck";
                        continue;
                    }
                    
                    if (workpieceSet.contains(aShape)) {
                        // Activate selection for part shapes only
                        m_context->Activate(aShape, 0, Standard_False);
                        m_context->Activate(aShape, 4, Standard_False);
                    }
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

// Focus/activation handling and custom paint routines have been removed for
// simplicity. QOpenGLWidget manages the context reliably in modern Qt versions
// and excessive event handling was a primary source of flickering and lag.

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
    
    try {
      // Restore previous 3D camera state if available
      if (m_has3DCameraState) {
        restore3DCameraState();
      } else {
        // Set up default 3D view with Z axis horizontal
        m_view->SetAt(0.0, 0.0, 0.0);
        m_view->SetEye(200.0, -300.0, 0.0);
        m_view->SetUp(-1.0, 0.0, 0.0);

        // Use orthographic projection for consistent visualization
        m_view->Camera()->SetProjectionType(
            Graphic3d_Camera::Projection_Orthographic);

        // Ensure the view shows the coordinate trihedron
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08,
                                V3d_ZBUFFER);

        // Update the current view mode
        m_currentViewMode = ViewMode::Mode3D;
      }

        // Fit all to ensure objects are visible
        m_view->FitAll();
        m_view->Redraw();
        
        qDebug() << "Set up 3D camera view with orthographic projection";
    }
    catch (const Standard_Failure& ex) {
        qDebug() << "Error setting up 3D camera:" << ex.GetMessageString();
    }
}

void OpenGL3DWidget::setupCameraXZ()
{
    if (m_view.IsNull()) {
        return;
    }
    
    // Set up XZ plane view for lathe operations with strict plane locking
    // Standard lathe coordinate system: X increases top to bottom, Z increases right to left
    // This means we need to look from the Y-negative direction toward the origin
    
    try {
        // Store current camera state if not already stored
        if (!m_has3DCameraState) {
            store3DCameraState();
        }
        
        // Camera position: Look from negative Y toward origin
        gp_Pnt eye(0.0, -1000.0, 0.0);  // Camera position on negative Y axis
        gp_Pnt at(0.0, 0.0, 0.0);       // Look at origin
        gp_Dir up(-1.0, 0.0, 0.0);      // X axis points down (negative X direction as up vector)
        
        m_view->SetAt(at.X(), at.Y(), at.Z());
        m_view->SetEye(eye.X(), eye.Y(), eye.Z());
        m_view->SetUp(up.X(), up.Y(), up.Z());
        
        // Set orthographic projection for 2D view - critical for lathe operations
        m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
        
        // Adjust the view to a reasonable default extent
        double extent = 250.0;
        m_view->SetSize(extent * 1.2);  // Scale to leave some margin
        
        // Change trihedron display to match lathe coordinate system
        m_view->TriedronErase();
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_RED, 0.08, V3d_ZBUFFER);
        // Update the current view mode
        m_currentViewMode = ViewMode::LatheXZ;
        
        // Store the XZ view parameters for enforcement
        m_lockedXZEye = eye;
        m_lockedXZAt = at;
        m_lockedXZUp = up;
        
        // Refresh the view
        m_view->Redraw();
        
        qDebug() << "Switched to XZ plane view (lathe coordinate system) with strict plane locking";
    }
    catch (const Standard_Failure& ex) {
        qDebug() << "Error setting up XZ camera:" << ex.GetMessageString();
    }
}

void OpenGL3DWidget::store3DCameraState()
{
    if (m_view.IsNull()) {
        return;
    }
    
    try {
        // Store current camera parameters
        Standard_Real atX, atY, atZ;
        Standard_Real eyeX, eyeY, eyeZ;
        Standard_Real upX, upY, upZ;
        
        m_view->At(atX, atY, atZ);
        m_view->Eye(eyeX, eyeY, eyeZ);
        m_view->Up(upX, upY, upZ);

        m_stored3DAt = gp_Pnt(atX, atY, atZ);
        m_stored3DEye = gp_Pnt(eyeX, eyeY, eyeZ);
        m_stored3DUp = gp_Dir(upX, upY, upZ);
        m_stored3DScale = m_view->Scale();
        m_stored3DProjection = m_view->Camera()->ProjectionType();
        
        m_has3DCameraState = true;
        
        qDebug() << "Stored 3D camera state";
    }
    catch (const Standard_Failure& ex) {
        qDebug() << "Error storing 3D camera state:" << ex.GetMessageString();
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
        
        // Restore stored projection type for 3D mode
        m_view->Camera()->SetProjectionType(m_stored3DProjection);
        
        // Restore trihedron display with standard orientation
        m_view->TriedronErase();
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
        
        // Update the current view mode
        m_currentViewMode = ViewMode::Mode3D;
        
        // Refresh the view
        m_view->Redraw();
        
        qDebug() << "Restored 3D camera state";
    }
    catch (const Standard_Failure& ex) {
        qDebug() << "Error restoring 3D camera:" << ex.GetMessageString();
    }
}

void OpenGL3DWidget::createLatheGrid(double /*spacing*/, double /*extent*/)
{
    // Grid creation disabled per latest requirements – keep existing scene objects intact.
    m_gridVisible = false;
    return;
}

void OpenGL3DWidget::removeLatheGrid()
{
    // Grid removal disabled because grid is not created anymore, prevents inadvertent RemoveAll().
    return;
}

void OpenGL3DWidget::throttledRedraw()
{
    if (!m_redrawThrottleTimer->isActive()) {
        m_redrawThrottleTimer->start();
    }
}

void OpenGL3DWidget::displayShape(const TopoDS_Shape& shape)
{
    if (m_context.IsNull() || shape.IsNull()) {
        return;
    }
    
    try {
        // Create AIS shape for display
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        
        // Display the shape
        m_context->Display(aisShape, Standard_False);
        
        // Auto-fit if enabled
        if (m_autoFitEnabled) {
            fitAll();
        } else {
            updateView();
        }
        
        qDebug() << "Shape displayed successfully";
        
    } catch (const std::exception& e) {
        qDebug() << "Error displaying shape:" << e.what();
    }
}

void OpenGL3DWidget::clearAll()
{
    if (!m_context.IsNull()) {
        try {
            m_context->RemoveAll(Standard_False);
            updateView();
            qDebug() << "All objects cleared from display";
        } catch (const std::exception& e) {
            qDebug() << "Error clearing display:" << e.what();
        }
    }
}

void OpenGL3DWidget::fitAll()
{
    if (!m_view.IsNull()) {
        try {
            m_view->FitAll();
            updateView();
            qDebug() << "View fitted to all objects";
        } catch (const std::exception& e) {
            qDebug() << "Error fitting view:" << e.what();
        }
    }
}

void OpenGL3DWidget::setToolpathsVisible(bool visible)
{
    if (m_toolpathsVisible == visible) {
        return; // No change needed
    }
    
    m_toolpathsVisible = visible;
    
    if (!m_context.IsNull()) {
        try {
            // Find all toolpath objects in the context and update their visibility
            AIS_ListOfInteractive allObjects;
            m_context->DisplayedObjects(allObjects);
            
            for (AIS_ListOfInteractive::Iterator anIter(allObjects); anIter.More(); anIter.Next()) {
                Handle(AIS_InteractiveObject) obj = anIter.Value();
                if (!obj.IsNull()) {
                    // Check if this is a toolpath display object
                    Handle(IntuiCAM::Toolpath::ToolpathDisplayObject) toolpathObj = 
                        Handle(IntuiCAM::Toolpath::ToolpathDisplayObject)::DownCast(obj);
                    
                    if (!toolpathObj.IsNull()) {
                        if (visible) {
                            if (!m_context->IsDisplayed(toolpathObj)) {
                                m_context->Display(toolpathObj, Standard_False);
                            }
                        } else {
                            if (m_context->IsDisplayed(toolpathObj)) {
                                m_context->Erase(toolpathObj, Standard_False);
                            }
                        }
                    }
                }
            }
            
            updateView();
            qDebug() << "Toolpaths visibility set to:" << visible;
            
        } catch (const std::exception& e) {
            qDebug() << "Error setting toolpath visibility:" << e.what();
        }
    }
}

void OpenGL3DWidget::setProfilesVisible(bool visible)
{
    if (m_profilesVisible == visible) {
        return; // No change needed
    }
    
    m_profilesVisible = visible;
    
    if (!m_context.IsNull()) {
        try {
            // Find all profile objects in the context and update their visibility
            AIS_ListOfInteractive allObjects;
            m_context->DisplayedObjects(allObjects);
            
            for (AIS_ListOfInteractive::Iterator anIter(allObjects); anIter.More(); anIter.Next()) {
                Handle(AIS_InteractiveObject) obj = anIter.Value();
                if (!obj.IsNull()) {
                    // Check if this is a profile display object
                    Handle(IntuiCAM::Toolpath::ProfileDisplayObject) profileObj = 
                        Handle(IntuiCAM::Toolpath::ProfileDisplayObject)::DownCast(obj);
                    
                    if (!profileObj.IsNull()) {
                        if (visible) {
                            if (!m_context->IsDisplayed(profileObj)) {
                                m_context->Display(profileObj, Standard_False);
                            }
                        } else {
                            if (m_context->IsDisplayed(profileObj)) {
                                m_context->Erase(profileObj, Standard_False);
                            }
                        }
                    }
                }
            }
            
            updateView();
            qDebug() << "Profiles visibility set to:" << visible;
            
        } catch (const std::exception& e) {
            qDebug() << "Error setting profile visibility:" << e.what();
        }
    }
} 