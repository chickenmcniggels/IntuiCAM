#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[])
{

    // Share OpenGL contexts across widgets to prevent black screens when focus
    // changes. This follows guidelines from Context7 research.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    
    // CRITICAL: Set global OpenGL surface format BEFORE creating OpenGL widgets
    // This prevents many black screen issues with QOpenGLWidget
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // Anti-aliasing
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setVersion(3, 3); // Minimum OpenGL 3.3 for OpenCASCADE
    
    // ESSENTIAL: Set as default format for all QOpenGLWidget instances
    QSurfaceFormat::setDefaultFormat(format);
    
    // Optional: Force OpenGL backend on Qt6 (prevents DirectX issues on Windows)
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qputenv("QSG_RHI_BACKEND", "opengl");
    #endif
    
    // Set application properties
    app.setApplicationName("IntuiCAM");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("IntuiCAM Project");
    
    std::cout << "Starting IntuiCAM GUI application..." << std::endl;
    std::cout << "Qt Version: " << qVersion() << std::endl;
    
    try {
        MainWindow window;
        window.show();
        
        std::cout << "Main window created and shown successfully." << std::endl;
        std::cout << "Starting application event loop..." << std::endl;
        
        int result = app.exec();
        std::cout << "Application exiting with code: " << result << std::endl;
        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown exception caught!" << std::endl;
        return -2;
    }
} 