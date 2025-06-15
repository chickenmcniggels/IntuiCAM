#include <IntuiCAM/Toolpath/OperationSequence.h>

namespace IntuiCAM {
namespace Toolpath {

void OperationSequence::addOperation(std::shared_ptr<Operation> op, bool active) {
    operations_.push_back({std::move(op), active});
}

void OperationSequence::setActive(size_t index, bool active) {
    if (index < operations_.size()) {
        operations_[index].active = active;
    }
}

bool OperationSequence::isActive(size_t index) const {
    return index < operations_.size() ? operations_[index].active : false;
}

} // namespace Toolpath
} // namespace IntuiCAM
