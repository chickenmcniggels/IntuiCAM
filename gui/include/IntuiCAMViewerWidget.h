#pragma once

#include <QWidget>
#include <QPoint>
#include <memory>

// Forward declarations for Qt classes
class QMenu;
class QTimer;

// Forward declarations for OpenCASCADE classes
class AIS_InteractiveContext;
class V3d_View;
class V3d_Viewer;
class TopoDS_Shape;

/**
 * @brief The IntuiCAMViewerWidget class provides OpenCASCADE 3D visualization.
 * 
 * This class integrates OpenCASCADE visualization into a Qt widget,
 * allowing for 3D model viewing and interaction.
 */
class IntuiCAMViewerWidget : public QWidget {
    Q_OBJECT
    
public:
    /**
     * @brief Constructs a new IntuiCAMViewerWidget instance.
     * 
     * @param parent The parent widget (optional).
     */
    explicit IntuiCAMViewerWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destroys the IntuiCAMViewerWidget instance.
     */
    ~IntuiCAMViewerWidget();
    
    /**
     * @brief Displays a shape in the 3D viewer.
     * 
     * @param shape The OpenCASCADE shape to display.
     */
    void displayShape(const TopoDS_Shape& shape);
    
    /**
     * @brief Clears all displayed shapes from the 3D viewer.
     */
    void clearDisplay();
    
    /**
     * @brief Resets the view to the default orientation.
     */
    void resetView();
    
    /**
     * @brief Sets the view to front orientation.
     */
    void viewFront();
    
    /**
     * @brief Sets the view to top orientation.
     */
    void viewTop();
    
    /**
     * @brief Sets the view to side orientation.
     */
    void viewSide();
    
    /**
     * @brief Fits all displayed shapes in the view.
     */
    void fitAll();
    
protected:
    // Override Qt event handlers
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    
private:
    void initializeOCCViewer();
    void createActions();
    void createContextMenu();
    
    // OpenCASCADE viewer objects
    std::shared_ptr<V3d_Viewer> m_viewer;
    std::shared_ptr<V3d_View> m_view;
    std::shared_ptr<AIS_InteractiveContext> m_context;
    
    // Mouse state
    QPoint m_lastMousePos;
    bool m_isDragging;
    
    // View control
    enum class ViewMode {
        Rotate,
        Pan,
        Zoom
    };
    
    ViewMode m_currentViewMode;
    
    // Context menu
    QMenu* m_contextMenu;
    
    // Timer for continuous updates
    QTimer* m_updateTimer;
}; 