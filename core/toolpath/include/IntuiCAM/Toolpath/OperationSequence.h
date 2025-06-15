#pragma once

#include <vector>
#include <memory>
#include <IntuiCAM/Toolpath/Operations.h>

namespace IntuiCAM {
namespace Toolpath {

struct OperationEntry {
    std::shared_ptr<Operation> operation;
    bool active = true;
};

class OperationSequence {
private:
    std::vector<OperationEntry> operations_;

public:
    void addOperation(std::shared_ptr<Operation> op, bool active = true);
    const std::vector<OperationEntry>& getOperations() const { return operations_; }
    void setActive(size_t index, bool active);
    bool isActive(size_t index) const;
};

} // namespace Toolpath
} // namespace IntuiCAM
