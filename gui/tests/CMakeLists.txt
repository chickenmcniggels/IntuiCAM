find_package(Qt6 REQUIRED COMPONENTS Test Widgets Core Gui)

qt_add_executable(gui_toolpath_widget_tests
    test_toolpathtimelinewidget.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelinewidget.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelineframe.cpp
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelinewidget.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelineframe.h
)

target_link_libraries(gui_toolpath_widget_tests
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        ${OpenCASCADE_LIBRARIES}
)

target_include_directories(gui_toolpath_widget_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/gui/include
        ${OpenCASCADE_INCLUDE_DIR}
)

qt_add_executable(gui_toolpath_manager_tests
    test_toolpathmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/workpiecemanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathmanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/workpiecemanager.h
)

target_link_libraries(gui_toolpath_manager_tests
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        intuicam_core_toolpath
        intuicam_core_geometry
        intuicam_core_common
        ${OpenCASCADE_LIBRARIES}
)

target_include_directories(gui_toolpath_manager_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/gui/include
        ${CMAKE_SOURCE_DIR}/core/toolpath/include
        ${CMAKE_SOURCE_DIR}/core/geometry/include
        ${CMAKE_SOURCE_DIR}/core/common/include
        ${OpenCASCADE_INCLUDE_DIR}
)

find_package(GTest REQUIRED)

qt_add_executable(gui_toolpath_generation_tests
    test_toolpathgenerationcontroller.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathgenerationcontroller.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelinewidget.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelineframe.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/workpiecemanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/rawmaterialmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/chuckmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/workspacecontroller.cpp
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathgenerationcontroller.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelinewidget.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelineframe.h
    ${CMAKE_SOURCE_DIR}/gui/include/workpiecemanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/rawmaterialmanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/chuckmanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/workspacecontroller.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathmanager.h
)

target_link_libraries(gui_toolpath_generation_tests
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        intuicam_core_toolpath
        intuicam_core_geometry
        intuicam_core_common
        ${OpenCASCADE_LIBRARIES}
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
)

target_include_directories(gui_toolpath_generation_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/gui/include
        ${OpenCASCADE_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
)

# -----------------------------------------------------------------------------
# New test target for end-to-end toolpath addition workflow
# -----------------------------------------------------------------------------

qt_add_executable(gui_toolpath_addition_tests
    test_toolpath_addition.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathgenerationcontroller.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelinewidget.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/toolpathtimelineframe.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/workpiecemanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/rawmaterialmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/chuckmanager.cpp
    ${CMAKE_SOURCE_DIR}/gui/src/workspacecontroller.cpp
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathgenerationcontroller.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelinewidget.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathtimelineframe.h
    ${CMAKE_SOURCE_DIR}/gui/include/workpiecemanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/rawmaterialmanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/chuckmanager.h
    ${CMAKE_SOURCE_DIR}/gui/include/workspacecontroller.h
    ${CMAKE_SOURCE_DIR}/gui/include/toolpathmanager.h
)

target_link_libraries(gui_toolpath_addition_tests
    PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        intuicam_core_toolpath
        intuicam_core_geometry
        intuicam_core_common
        ${OpenCASCADE_LIBRARIES}
)

target_include_directories(gui_toolpath_addition_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/gui/include
        ${CMAKE_SOURCE_DIR}/core/toolpath/include
        ${CMAKE_SOURCE_DIR}/core/geometry/include
        ${CMAKE_SOURCE_DIR}/core/common/include
)

# Register tests
add_test(NAME GuiToolpathGenerationTests COMMAND gui_toolpath_generation_tests)
add_test(NAME GuiToolpathAdditionTests COMMAND gui_toolpath_addition_tests) 