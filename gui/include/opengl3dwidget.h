#ifndef OPENGL3DWIDGET_H
#define OPENGL3DWIDGET_H

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>

// OpenCASCADE includes
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <WNT_Window.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

class ChuckManager;

class OpenGL3DWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGL3DWidget(QWidget *parent = nullptr);
    ~OpenGL3DWidget();

    // Display a shape in the viewer
    void displayShape(const TopoDS_Shape& shape);
    
    // Clear all displayed objects
    void clearAll();
    
    // Fit all objects in view
    void fitAll();
    
    // Get the AIS context for advanced operations
    Handle(AIS_InteractiveContext) getContext() const { return m_context; }
    
    // Chuck management
    ChuckManager* getChuckManager() const { return m_chuckManager; }
    
    // Initialize chuck with default chuck file
    void initializeChuck(const QString& chuckFilePath);
    
    // Add workpiece and auto-align with chuck
    void addWorkpiece(const TopoDS_Shape& workpiece);

protected:
    // Qt OpenGL widget overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    // Mouse interaction
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void initializeViewer();
    void updateView();
    
    // OpenCASCADE objects
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(WNT_Window) m_window;
    
    // Mouse interaction state
    bool m_isDragging;
    QPoint m_lastMousePos;
    Qt::MouseButton m_dragButton;
    
    // Chuck management
    ChuckManager* m_chuckManager;
};

#endif // OPENGL3DWIDGET_H 