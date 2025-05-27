#include <QApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <iostream>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
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