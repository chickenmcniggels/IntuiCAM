// Custom Postprocessor Example
// Demonstrates how to create a custom postprocessor for IntuiCAM

#include <iostream>
#include <string>

namespace IntuiCAM {
namespace Examples {

class CustomPostprocessor {
public:
    CustomPostprocessor() = default;
    ~CustomPostprocessor() = default;
    
    void initialize() {
        std::cout << "Custom postprocessor example initialized" << std::endl;
    }
    
    void processToolpath() {
        std::cout << "Processing toolpath with custom postprocessor..." << std::endl;
    }
    
    std::string generateGCode() {
        std::cout << "Generating custom G-code..." << std::endl;
        return "G0 X0 Y0 Z0\nM30\n";
    }
};

} // namespace Examples
} // namespace IntuiCAM

int main() {
    IntuiCAM::Examples::CustomPostprocessor processor;
    processor.initialize();
    processor.processToolpath();
    std::string gcode = processor.generateGCode();
    std::cout << "Generated G-code:\n" << gcode << std::endl;
    return 0;
} 