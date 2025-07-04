# IntuiCAM/core/toolpath/CMakeLists.txt

set(CORE_TOOLPATH_LIB_NAME intuicam_core_toolpath)

# Explicitly list all source files - including new pipeline-based operations
set(CORE_TOOLPATH_SOURCES
    "src/Types.cpp"
    "src/ToolTypes.cpp"
    "src/Operations.cpp"
    "src/FacingOperation.cpp"
    "src/DrillingOperation.cpp"
    "src/InternalRoughingOperation.cpp"
    "src/ExternalRoughingOperation.cpp"
    "src/ChamferingOperation.cpp"
    "src/RoughingOperation.cpp"
    "src/FinishingOperation.cpp"
    "src/PartingOperation.cpp"
    "src/ThreadingOperation.cpp"
    "src/GroovingOperation.cpp"
    "src/DummyOperation.cpp"
    "src/ContouringOperation.cpp"
    "src/ToolpathPlanner.cpp"
    "src/LatheProfile.cpp"
    "src/ToolpathGenerationPipeline.cpp"
    "src/ProfileExtractor.cpp"
    "src/OperationParameterManager.cpp"
    "src/ToolpathDisplayObject.cpp"
    "include/IntuiCAM/Toolpath/Types.h"
    "include/IntuiCAM/Toolpath/ToolTypes.h"
    "include/IntuiCAM/Toolpath/Operations.h"
    "include/IntuiCAM/Toolpath/FacingOperation.h"
    "include/IntuiCAM/Toolpath/DrillingOperation.h"
    "include/IntuiCAM/Toolpath/InternalRoughingOperation.h"
    "include/IntuiCAM/Toolpath/ExternalRoughingOperation.h"
    "include/IntuiCAM/Toolpath/ChamferingOperation.h"
    "include/IntuiCAM/Toolpath/RoughingOperation.h"
    "include/IntuiCAM/Toolpath/FinishingOperation.h"
    "include/IntuiCAM/Toolpath/PartingOperation.h"
    "include/IntuiCAM/Toolpath/ThreadingOperation.h"
    "include/IntuiCAM/Toolpath/GroovingOperation.h"
    "include/IntuiCAM/Toolpath/DummyOperation.h"
    "include/IntuiCAM/Toolpath/ContouringOperation.h"
    "include/IntuiCAM/Toolpath/LatheProfile.h"
    "include/IntuiCAM/Toolpath/ToolpathPlanner.h"
    "include/IntuiCAM/Toolpath/ToolpathGenerationPipeline.h"
    "include/IntuiCAM/Toolpath/ProfileExtractor.h"
    "include/IntuiCAM/Toolpath/OperationParameterManager.h"
    "include/IntuiCAM/Toolpath/ToolpathDisplayObject.h"
)

add_library(${CORE_TOOLPATH_LIB_NAME} STATIC ${CORE_TOOLPATH_SOURCES})

target_include_directories(${CORE_TOOLPATH_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
)

# Link against other core libraries from this project and OpenCASCADE
target_link_libraries(${CORE_TOOLPATH_LIB_NAME} 
    PRIVATE
        intuicam_core_common
        ${OpenCASCADE_LIBRARIES}
    PUBLIC
        intuicam_core_geometry
)

# Link against necessary OpenCASCADE modules for toolpath algorithms
# ModelingAlgorithms group:
set(OCCT_MODELING_ALGORITHMS_LIBS
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKBO      # Boolean Operations
    TKShHealing
    TKBool
    TKFillet
    TKOffset
    TKFeat
    TKMesh
    # Add other specific OCCT ModelingAlgorithms libraries if needed
)

target_link_libraries(${CORE_TOOLPATH_LIB_NAME} PRIVATE
    ${OCCT_MODELING_ALGORITHMS_LIBS}
)

# Publicly link OCCT include dirs if its headers are used in our public headers
# target_include_directories(${CORE_TOOLPATH_LIB_NAME} PUBLIC ${OpenCASCADE_INCLUDE_DIR}) 

# Export the target for use by other CMake projects/modules if necessary
# install(TARGETS ${CORE_TOOLPATH_LIB_NAME} EXPORT ${CORE_TOOLPATH_LIB_NAME}_Targets ...)
# export(TARGETS ${CORE_TOOLPATH_LIB_NAME} FILE ${CMAKE_BINARY_DIR}/cmake/${CORE_TOOLPATH_LIB_NAME}.cmake) 