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

// OpenCASCADE includes
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <WNT_Window.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

/**
 * @brief Pure 3D visualization widget using OpenCASCADE
 * 
 * This widget is now a focused visualization component that:
 * - Handles OpenGL rendering and OpenCASCADE integration
 * - Manages user interaction (mouse, wheel events)
 * - Provides basic display operations (show shape, clear, fit view)
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

signals:
    /**
     * @brief Emitted when the viewer is successfully initialized
     */
    void viewerInitialized();

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
    
    // Comprehensive event handling to prevent black screen
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    /**
     * @brief Initialize the OpenCASCADE viewer system
     */
    void initializeViewer();
    
    /**
     * @brief Update the 3D view
     */
    void updateView();
    
    /**
     * @brief Force a robust redraw of the viewer
     */
    void forceRedraw();
    
    /**
     * @brief Ensure the viewer context is valid and ready
     */
    void ensureViewerReady();
    
    /**
     * @brief Handle window activation changes
     */
    void handleActivationChange(bool active);
    
    // OpenCASCADE objects
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(WNT_Window) m_window;
    
    // Mouse interaction state
    bool m_isDragging;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    
    // Update management
    bool m_continuousUpdate;
    QTimer* m_updateTimer;
    QTimer* m_robustRefreshTimer;  // For preventing persistent black screens
    
    // State tracking
    bool m_isInitialized;
    bool m_needsRefresh;
};

#endif // OPENGL3DWIDGET_H 