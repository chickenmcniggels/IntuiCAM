#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <Standard_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <qwidget.h>
#include <TopoDS_Shape.hxx>

namespace IntuiCAM {
    namespace GUI {

        /**
         * @brief A Qt widget that displays 3D CAD models using OpenCASCADE.
         * It embeds a V3d_Viewer and AIS_InteractiveContext for rendering.
         */
        class IntuiCAMViewerWidget : public QOpenGLWidget {
            Q_OBJECT
        public:
            explicit IntuiCAMViewerWidget(QWidget* parent = nullptr);
            virtual ~IntuiCAMViewerWidget();

            /**
             * @brief Display a TopoDS_Shape in the viewer.
             */
            void displayShape(const TopoDS_Shape& shape);

            /**
             * @brief Remove all displayed objects.
             */
            void clear();

        protected:
            void initializeGL() override;
            void resizeGL(int w, int h) override;
            void paintGL() override;

        private:
            Handle(Aspect_DisplayConnection) m_displayConnection;
            Handle(V3d_Viewer)              m_viewer;
            Handle(V3d_View)                m_view;
            Handle(AIS_InteractiveContext)  m_context;
        };

    } // namespace GUI
} // namespace IntuiCAM
