add_library(IntuiCAMCore
    src/Core.cpp
)

target_include_directories(IntuiCAMCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Beispiel für externe Abhängigkeit (OCCT etc.)
# find_package(OpenCASCADE REQUIRED)
# target_link_libraries(IntuiCAMCore PUBLIC OpenCASCADE)

# Install-Regeln (optional)
install(TARGETS IntuiCAMCore
        EXPORT IntuiCAMPackages
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)
install(DIRECTORY include/IntuiCAMCore
        DESTINATION include)

