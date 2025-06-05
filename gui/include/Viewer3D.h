#ifndef VIEWER_3D_H
#define VIEWER_3D_H

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
#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <V3d_TypeOfOrientation.hxx>

#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#include <Xw_Window.hxx>
#endif

/**
 * @brief Unified 2D/3D visualization widget using OpenCASCADE.
 *
 * This widget combines the functionalities of a 3D viewer and a 2D (XZ plane)
 * viewer. It allows dynamic switching between viewing modes and aims for
 * robust rendering and interaction, inspired by best practices from projects
 * like fougue/mayo and the official OCCT QOpenGLWidget samples.
 */
class Viewer3D : public QOpenGLWidget
{
    Q_OBJECT

public:
    /**
     * @brief Defines the available viewing modes.
     */
    enum class ViewMode {
        Mode3D,      ///< Full 3D perspective view
        Mode2DXZ     ///< 2D orthographic view of the XZ plane
    };

    explicit Viewer3D(QWidget *parent = nullptr);
    ~Viewer3D();

    // --- Core View Management ---
    ViewMode currentViewMode() const { return m_currentViewMode; }
    void setViewMode(ViewMode mode);

    // --- Shape Display ---
    void displayShape(const TopoDS_Shape& shape, bool autoFit = true);
    void removeShape(const Handle(AIS_Shape)& aisShape);
    void removeShape(const TopoDS_Shape& shape);
    void clearAll();

    // --- Camera and View Control ---
    void fitAll();
    void fitSelected();
    void setProjection(V3d_TypeOfOrientation orientation);

    // --- Interaction Configuration ---
    void setSelectionMode(bool enabled);
    bool selectionMode() const { return m_selectionMode; }
    void setHoverHighlightEnabled(bool enabled);
    bool hoverHighlightEnabled() const { return m_hoverHighlightEnabled; }

    // --- Auto-fit Control ---
    void setAutoFitEnabled(bool enabled) { m_autoFitEnabled = enabled; }
    bool autoFitEnabled() const { return m_autoFitEnabled; }

    // --- View Cube ---
    void setViewCubeVisible(bool visible);

    // --- Access to OCCT Objects (for advanced use) ---
    Handle(V3d_View) view() const { return m_view; }
    Handle(V3d_Viewer) viewer() const { return m_viewer; }
    Handle(AIS_InteractiveContext) context() const { return m_context; }

    // --- State Queries ---
    bool isInitialized() const { return m_isInitialized; }

    // --- Specific Features (e.g., from previous viewers) ---
    void setTurningAxisFace(const TopoDS_Shape& axisShape);
    void clearTurningAxisFace();

    // --- Chuck Management ---
    void loadAndDisplayChuck(const QString& chuckFilePath);
    void displayChuck(const TopoDS_Shape& chuckShape);
    void clearChuck();
    bool isChuckDisplayed() const { return !m_chuckAIS.IsNull(); }
    
    // --- Lathe-specific Camera Setup ---
    void setLatheCameraOrientation();

signals:
    void viewerInitialized();
    void viewModeChanged(Viewer3D::ViewMode newMode);
    void shapeSelected(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint);
    void objectDoubleClicked(const Handle(AIS_InteractiveObject)& object);
    void chuckLoadRequested(const QString& filePath);

protected:
    // Qt OpenGL widget overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    // Event handling 
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void setupView();
    void updateView();
    void scheduleRedraw();

    // --- Internal View Setup ---
    void applyXZPlaneView();
    void apply3DView();

    // --- OpenCASCADE Objects ---
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;

    // --- State ---
    ViewMode m_currentViewMode;
    bool m_isInitialized;
    bool m_needsUpdate;
    bool m_isDragging;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    bool m_selectionMode;
    bool m_autoFitEnabled;
    bool m_hoverHighlightEnabled;
    Handle(AIS_Shape) m_hoveredObject;
    Handle(AIS_Shape) m_turningAxisFaceAIS;
    Handle(AIS_Shape) m_chuckAIS;
};

#endif // VIEWER_3D_H 