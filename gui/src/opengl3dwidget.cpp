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
    , m_dragButton(Qt::NoButton)
    , m_continuousUpdate(false)
    , m_updateTimer(new QTimer(this))
    , m_isInitialized(false)
    , m_selectionMode(false)
    , m_autoFitEnabled(true)
    , m_hoverHighlightEnabled(true)
    , m_currentViewMode(ViewMode::Mode3D)
    , m_stored3DScale(1.0)
    , m_stored3DProjection(Graphic3d_Camera::Projection_Perspective)
    , m_has3DCameraState(false)
    , m_workspaceController(nullptr)
    , m_gridVisible(false)
{
    // Enable mouse tracking for proper interaction
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Use the default full update behavior. Partial updates caused brief
    // black flashes when the widget regained focus or another widget was
    // interacted with. Following working OCCT viewer examples like
    // gkv311/occt-samples-qopenglwidget, we stick with NoPartialUpdate to
    // ensure consistent rendering.
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    
    // Ensure the widget gets proper resize events
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen, false);  // Important for proper rendering
    // Do NOT use WA_NativeWindow. It caused the viewer to turn black when menus
    // or other widgets gained focus.
    
    // Optional timer for continuous updates (e.g. animations)
    m_updateTimer->setSingleShot(false);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    
    qDebug() << "OpenGL3DWidget created as pure visualization component with simplified rendering";
}

OpenGL3DWidget::~OpenGL3DWidget()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }
    
    // Clean up lathe grid if present
    removeLatheGrid();
    
    // Release OpenCASCADE resources
    m_context.Nullify();
    m_view.Nullify();
    m_viewer.Nullify();
    m_window.Nullify();
    
    qDebug() << "OpenGL3DWidget destroyed and resources released";
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
            qDebug() << "Error during resize";
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
        // Simplified update logic to reduce context switching and flickering
        if (isVisible() && width() > 0 && height() > 0)
        {
            try {
                m_view->Redraw();
            } catch (...) {
                qDebug() << "Error during view update";
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
    m_lastMousePos = event->pos();
    m_dragButton = event->button();

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
    
    m_isDragging = true;
}

void OpenGL3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_view.IsNull())
        return;

    if (!m_selectionMode && (event->buttons() & Qt::LeftButton) && m_dragButton == Qt::LeftButton) {
        m_view->Rotate(event->pos().x(), event->pos().y(), m_lastMousePos.x(), m_lastMousePos.y());
    } else if (event->buttons() & Qt::MiddleButton ||
               ((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ShiftModifier))) {
        m_view->Pan(event->pos().x() - m_lastMousePos.x(), m_lastMousePos.y() - event->pos().y());
    }

    m_lastMousePos = event->pos();
    update();
}

void OpenGL3DWidget::mouseReleaseEvent(QMouseEvent* event)
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
        
        // Show or hide grid based on view mode
        if (mode == ViewMode::LatheXZ) {
            // Create grid for lathe view
            createLatheGrid();
        } else {
            // Remove grid when switching to 3D view
            removeLatheGrid();
        }
        
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
        // Remove any existing grid
        if (m_gridVisible) {
            removeLatheGrid();
        }
        
        // Restore previous 3D camera state if available
        if (m_has3DCameraState) {
            restore3DCameraState();
        } else {
            // Set up default 3D perspective view
            m_view->SetAt(0.0, 0.0, 0.0);
            m_view->SetEye(100.0, 100.0, 100.0);
            m_view->SetUp(0.0, 0.0, 1.0);
            
            // Explicitly set perspective projection for 3D mode
            m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
            
            // Ensure the view shows the coordinate trihedron
            m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);
            
            // Update the current view mode
            m_currentViewMode = ViewMode::Mode3D;
        }
        
        // Fit all to ensure objects are visible
        m_view->FitAll();
        m_view->Redraw();
        
        qDebug() << "Set up 3D camera view with perspective projection";
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
    
    // Set up XZ plane view for lathe operations
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
        
        // Set orthographic projection for 2D view
        m_view->Camera()->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
        
        // Adjust the view to show the entire grid
        double extent = 250.0;  // Match or exceed grid extent
        m_view->SetSize(extent * 1.2);  // Scale to leave some margin
        
        // Change trihedron display to match lathe coordinate system
        m_view->TriedronErase();
        m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_RED, 0.08, V3d_ZBUFFER);
        
        // Create the grid
        createLatheGrid(10.0, 200.0);
        
        // Update the current view mode
        m_currentViewMode = ViewMode::LatheXZ;
        
        // Refresh the view
        m_view->Redraw();
        
        qDebug() << "Switched to XZ plane view (lathe coordinate system)";
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
        // Remove the grid if it's visible
        if (m_gridVisible) {
            removeLatheGrid();
        }
        
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