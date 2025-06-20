#include <IntuiCAM/Toolpath/DummyOperation.h>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Toolpath {

DummyOperation::DummyOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) {
}

std::unique_ptr<Toolpath> DummyOperation::generateToolpath(const Geometry::Part& part) {
    // Create a simple dummy toolpath
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    
    // Add a simple move pattern
    toolpath->addRapidMove(Geometry::Point3D(0.0, 0.0, 10.0));
    toolpath->addLinearMove(Geometry::Point3D(0.0, 0.0, 0.0), 100.0);
    toolpath->addRapidMove(Geometry::Point3D(0.0, 0.0, 10.0));
    
    return toolpath;
}

bool DummyOperation::validate() const {
    return true; // Always valid for dummy operation
}

} // namespace Toolpath
} // namespace IntuiCAM
