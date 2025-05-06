#pragma once

#include <TopoDS_Shape.hxx>
#include <string>

namespace IntuiCAM {
    namespace Core {
        namespace IO {

            /**
             * @brief StepLoader provides functionality to import STEP files into IntuiCAM.
             * This is part of the Core library and uses OpenCASCADE to read the file.
             */
            class StepLoader {
            public:
                /**
                 * Load a STEP file from the given path.
                 * @param filePath Path to the .step/.stp file.
                 * @param outShape Reference to a TopoDS_Shape where the result will be stored.
                 * @return true on success, false if loading or translation failed.
                 */
                static bool loadStep(const std::string& filePath, TopoDS_Shape& outShape);
            };

        } // namespace IO
    } // namespace Core
} // namespace IntuiCAM
