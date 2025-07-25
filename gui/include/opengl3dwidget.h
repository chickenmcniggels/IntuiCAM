#ifndef OPENGL3DWIDGET_H
#define OPENGL3DWIDGET_H

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QEvent>
#include <QEnterEvent>
#include <QPaintEvent>
#include <QFocusEvent>
#include <QShowEvent>
#include <QHideEvent>

// Standard includes
#include <vector>

// OpenCASCADE includes
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <WNT_Window.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <Aspect_Grid.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Structure.hxx>
#include <Graphic3d_Camera.hxx>
#include <AIS_Line.hxx>
#include <Geom_Line.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Prs3d_LineAspect.hxx>
#include <AIS_InteractiveObject.hxx>

// IntuiCAM toolpath includes for visibility management
#include <IntuiCAM/Toolpath/ToolpathDisplayObject.h>

// Qt includes for the overlay button
#include <QPushButton>

/**
 * @brief Viewing modes for the 3D widget
 */
enum class ViewMode {
    Mode3D,     ///< Full 3D viewing with free rotation
    LatheXZ     ///< Locked to XZ plane for lathe operations (X top to bottom, Z left to right)
};

/**
 * @brief Pure 3D visualization widget using OpenCASCADE
 * 
 * This widget is now a focused visualization component that:
 * - Handles OpenGL rendering and OpenCASCADE integration
 * - Manages user interaction (mouse, wheel events)
 * - Provides basic display operations (show shape, clear, fit view)
 * - Supports multiple viewing modes (3D and XZ lathe plane)
 * - Maintains clean separation from business logic
 * 
 * Business logic and workflow coordination are handled by WorkspaceController.
 * This follows the modular architecture principle of clear component separation.
 */
class OpenGL3DWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGL3DWidget(QWidget *parent = nullptr);
    ~OpenGL3DWidget();

    /**
     * @brief Get the AIS context for manager initialization
     * @return OpenCASCADE AIS interactive context
     */
    Handle(AIS_InteractiveContext) getContext() const { return m_context; }
    
    /**
     * @brief Display a shape in the viewer (basic display operation)
     * @param shape The shape to display
     */
    void displayShape(const TopoDS_Shape& shape);
    
    /**
     * @brief Clear all displayed objects
     */
    void clearAll();
    
    /**
     * @brief Fit all objects in view
     */
    void fitAll();
    
    /**
     * @brief Check if the 3D viewer is properly initialized
     * @return True if viewer is ready for use
     */
    bool isViewerInitialized() const { return !m_context.IsNull(); }
    
    /**
     * @brief Enable or disable continuous updates
     * @param enabled True to enable continuous updates (useful for animations)
     */
    void setContinuousUpdate(bool enabled);

    /**
     * @brief Enable interactive selection mode for manual axis selection
     * @param enabled True to enable selection mode
     */
    void setSelectionMode(bool enabled);

    /**
     * @brief Check if selection mode is currently active
     * @return True if selection mode is enabled
     */
    bool isSelectionModeActive() const { return m_selectionMode; }

    /**
     * @brief Enable or disable auto-fit when displaying new shapes
     * @param enabled True to automatically fit view when displaying shapes
     */
    void setAutoFitEnabled(bool enabled) { m_autoFitEnabled = enabled; }

    /**
     * @brief Check if auto-fit is enabled
     * @return True if auto-fit is enabled
     */
    bool isAutoFitEnabled() const { return m_autoFitEnabled; }

    /**
     * @brief Set the turning axis face for special highlighting
     * @param axisShape The face shape that defines the turning axis
     */
    void setTurningAxisFace(const TopoDS_Shape& axisShape);

    /**
     * @brief Clear the turning axis face highlighting
     */
    void clearTurningAxisFace();

    /**
     * @brief Set the viewing mode (3D or XZ plane)
     * @param mode The desired viewing mode
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief Get the current viewing mode
     * @return Current viewing mode
     */
    ViewMode getViewMode() const { return m_currentViewMode; }

    /**
     * @brief Toggle between 3D and XZ plane viewing modes
     */
    void toggleViewMode();

    /**
     * @brief Set the workspace controller for checking object selectability
     * @param controller Pointer to the workspace controller
     */
    void setWorkspaceController(class WorkspaceController* controller) { m_workspaceController = controller; }

    /**
     * @brief Enable or disable toolpath visibility
     * @param visible True to show toolpaths, false to hide
     */
    void setToolpathsVisible(bool visible);

    /**
     * @brief Enable or disable profile visibility
     * @param visible True to show profiles, false to hide
     */
    void setProfilesVisible(bool visible);

    /**
     * @brief Check if toolpaths are currently visible
     * @return True if toolpaths are visible
     */
    bool areToolpathsVisible() const { return m_toolpathsVisible; }

    /**
     * @brief Check if profiles are currently visible
     * @return True if profiles are visible
     */
    bool areProfilesVisible() const { return m_profilesVisible; }

signals:
    /**
     * @brief Emitted when the viewer is successfully initialized
     */
    void viewerInitialized();

    /**
     * @brief Emitted when a shape is selected in selection mode
     * @param selectedShape The selected shape
     * @param clickPoint The 3D point where the selection occurred
     */
    void shapeSelected(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint);

    /**
     * @brief Emitted when the view mode changes
     * @param mode The new viewing mode
     */
    void viewModeChanged(ViewMode mode);

protected:
    // Qt OpenGL widget overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void resizeEvent(QResizeEvent *event) override;
    
    // Mouse interaction
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    
    // Focus handling - call update() on focus changes to avoid black screens
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    
    // Basic QOpenGLWidget lifecycle events are sufficient. Extra overrides
    // were removed for a leaner and more stable implementation.

private:
    /**
     * @brief Initialize the OpenCASCADE viewer system
     */
    void initializeViewer();
    
    /**
     * @brief Update the 3D view rendering
     */
    void updateView();
    
    /**
     * @brief Apply camera settings based on current view mode
     */
    void applyCameraForViewMode();
    
    /**
     * @brief Set up 3D orthographic camera
     */
    void setupCamera3D();
    
    /**
     * @brief Set up XZ plane orthographic camera for lathe view
     */
    void setupCameraXZ();
    
    /**
     * @brief Store current 3D camera state for restoration
     */
    void store3DCameraState();
    
    /**
     * @brief Restore previously stored 3D camera state
     */
    void restore3DCameraState();

    /**
     * @brief Create and display a grid for the lathe XZ view
     * @param spacing Spacing between grid lines in mm
     * @param extent Maximum distance from origin for grid lines
     */
    void createLatheGrid(double spacing = 10.0, double extent = 200.0);

    /**
     * @brief Remove the lathe grid from display
     */
    void removeLatheGrid();
    
    /**
     * @brief Throttled redraw to prevent excessive render calls
     */
    void throttledRedraw();

    // OpenCASCADE handles
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(WNT_Window) m_window;
    
    // Mouse interaction state
    bool m_isDragging;
    bool m_isDragStarted;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    bool m_isMousePressed;
    
    // Update management
    bool m_continuousUpdate;
    QTimer* m_updateTimer;
    QTimer* m_redrawThrottleTimer;
    QTimer* m_deferredResizeTimer; ///< Delay heavy resize handling
    
    // State tracking
    bool m_isInitialized;

    // Selection mode
    bool m_selectionMode;

    // Auto-fit
    bool m_autoFitEnabled;

    // Hover highlighting
    Handle(AIS_Shape) m_hoveredObject;
    bool m_hoverHighlightEnabled;

    // Turning axis face highlighting
    Handle(AIS_Shape) m_turningAxisFaceAIS;
    TopoDS_Shape m_turningAxisFace;

    // View mode management
    ViewMode m_currentViewMode;
    
    // Camera state storage for 3D mode
    gp_Pnt m_stored3DEye;
    gp_Pnt m_stored3DAt;
    gp_Dir m_stored3DUp;
    double m_stored3DScale;
    Graphic3d_Camera::Projection m_stored3DProjection;
    bool m_has3DCameraState;
    
    // XZ view locking parameters
    gp_Pnt m_lockedXZEye;
    gp_Pnt m_lockedXZAt;
    gp_Dir m_lockedXZUp;

    // Workspace controller
    class WorkspaceController* m_workspaceController;

    // Lathe grid display elements
    bool m_gridVisible;
    Standard_Real m_gridSpacing;
    Standard_Real m_gridExtent;
    
    // Visibility control for toolpaths and profiles
    bool m_toolpathsVisible;
    bool m_profilesVisible;
    
    // Grid objects are managed by the AIS context, no need to store references
};

#endif // OPENGL3DWIDGET_H 