#include <IntuiCAM/Toolpath/GroovingOperation.h>
#include <IntuiCAM/Geometry/Types.h>

namespace IntuiCAM {
namespace Toolpath {

GroovingOperation::GroovingOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Grooving, name, tool) {
}

std::unique_ptr<Toolpath> GroovingOperation::generateToolpath(const Geometry::Part& part) {
    // Return empty toolpath - Grooving operation not part of core focus
    // Core focus: external roughing, external finishing, facing, and parting only
    auto toolpath = std::make_unique<Toolpath>(getName(), getTool());
    return toolpath;
}

bool GroovingOperation::validate() const {
    return true; // Basic validation
}

} // namespace Toolpath
} // namespace IntuiCAM
