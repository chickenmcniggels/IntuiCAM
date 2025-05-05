find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(IntuiCAM_GUI
    src/main.cpp
)

target_link_libraries(IntuiCAM_GUI
    PRIVATE
        IntuiCAMCore
        Qt5::Widgets
)

# Ressourcen (Icons, QML, ...)
qt5_add_resources(GUI_RESOURCES resources/icons.qrc)
target_sources(IntuiCAM_GUI PRIVATE ${GUI_RESOURCES})

