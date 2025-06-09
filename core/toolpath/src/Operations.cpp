#include <IntuiCAM/Toolpath/Operations.h>

namespace IntuiCAM {
namespace Toolpath {

// Factory method implementation for Operation
std::unique_ptr<Operation> Operation::createOperation(Type type, const std::string& name, 
                                                    std::shared_ptr<Tool> tool) {
    switch (type) {
        case Type::Facing:
            return std::make_unique<FacingOperation>(name, tool);
        case Type::Roughing:
            return std::make_unique<RoughingOperation>(name, tool);
        case Type::Finishing:
            return std::make_unique<FinishingOperation>(name, tool);
        case Type::Parting:
            return std::make_unique<PartingOperation>(name, tool);
        case Type::Threading:
            return std::make_unique<ThreadingOperation>(name, tool);
        case Type::Grooving:
            return std::make_unique<GroovingOperation>(name, tool);
        default:
            return nullptr;
    }
}

} // namespace Toolpath
} // namespace IntuiCAM 