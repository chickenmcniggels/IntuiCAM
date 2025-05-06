#pragma once
#include <QMainWindow>

namespace IntuiCAM {
    namespace GUI {

        /**
         * @brief The main application window for IntuiCAM.
         * Provides File→Import menu to load STEP files and hosts the 3D viewer.
         */
        class IntuiCAMMainWindow : public QMainWindow {
            Q_OBJECT
        public:
            explicit IntuiCAMMainWindow(QWidget* parent = nullptr);
            ~IntuiCAMMainWindow() override = default;

        private slots:
            void onImportStepFile();

        private:
            class IntuiCAMViewerWidget* m_viewerWidget;
        };

    } // namespace GUI
} // namespace IntuiCAM
