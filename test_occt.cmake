cmake_minimum_required(VERSION 3.16)
project(TestOCCT)

set(OpenCASCADE_DIR "C:/OpenCASCADE/occt-vc144-64-with-debug/cmake")
find_package(OpenCASCADE REQUIRED COMPONENTS FoundationClasses ModelingData ModelingAlgorithms)

message("OpenCASCADE_FOUND: ${OpenCASCADE_FOUND}")
message("OpenCASCADE_INCLUDE_DIR: ${OpenCASCADE_INCLUDE_DIR}")
message("OpenCASCADE_LIBRARIES: ${OpenCASCADE_LIBRARIES}")
message("OpenCASCADE_FoundationClasses_LIBRARIES: ${OpenCASCADE_FoundationClasses_LIBRARIES}")
message("OpenCASCADE_ModelingData_LIBRARIES: ${OpenCASCADE_ModelingData_LIBRARIES}")
message("OpenCASCADE_ModelingAlgorithms_LIBRARIES: ${OpenCASCADE_ModelingAlgorithms_LIBRARIES}")

# Print individual Modeling Data libraries if available
if(DEFINED OpenCASCADE_ModelingData_LIBRARIES)
  foreach(lib ${OpenCASCADE_ModelingData_LIBRARIES})
    message("ModelingData lib: ${lib}")
  endforeach()
endif() 