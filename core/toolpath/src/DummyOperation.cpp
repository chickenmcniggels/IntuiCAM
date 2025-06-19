#include <IntuiCAM/Toolpath/DummyOperation.h>

namespace IntuiCAM {
namespace Toolpath {

DummyOperation::DummyOperation(const std::string& name, std::shared_ptr<Tool> tool)
    : Operation(Operation::Type::Facing, name, tool) { }

std::unique_ptr<Toolpath> DummyOperation::generateToolpath(const Geometry::Part& /*part*/) {
    auto tp = std::make_unique<Toolpath>(name_, tool_);

    // Rapid approach to the start position
    tp->addRapidMove(params_.startPosition);
    // Single cutting move
    tp->addLinearMove(params_.endPosition, params_.feedRate);

    return tp;
}

bool DummyOperation::validate() const {
    return params_.feedRate > 0.0;
}

} // namespace Toolpath
} // namespace IntuiCAM
