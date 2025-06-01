// Milling Operation Implementation
// Placeholder implementation for milling operations

#include <iostream>

namespace IntuiCAM {
namespace Operations {

class MillingOperation {
public:
    MillingOperation() = default;
    ~MillingOperation() = default;
    
    void initialize() {
        std::cout << "Milling operation initialized" << std::endl;
    }
    
    void generateToolpath() {
        std::cout << "Generating milling toolpath..." << std::endl;
    }
};

} // namespace Operations
} // namespace IntuiCAM

// Export function for plugin loading
extern "C" {
    void* create_operation() {
        return new IntuiCAM::Operations::MillingOperation();
    }
} 