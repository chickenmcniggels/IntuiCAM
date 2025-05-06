#ifndef INTUICAM_CORE_H
#define INTUICAM_CORE_H

#include <string>
#include <TopoDS_Edge.hxx>
#include <vector>

namespace IntuiCAM {
namespace Core {

/// Simple stub interface for CAM core
class CAMEngine {
private:
    TopoDS_Shape my_shape;
public:
    CAMEngine();
    ~CAMEngine();

    /// Load a STEP model from file
    bool loadSTEP(const std::string& path);

    /// Compute toolpaths (stub)
    std::vector<std::string> computeToolpaths();

    /// Export G-Code to file
    bool exportGCode(const std::string& outPath);
};

} // namespace Core
} // namespace IntuiCAM

#endif // INTUICAM_CORE_H
