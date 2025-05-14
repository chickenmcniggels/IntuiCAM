#include <iostream>
#include <string>
#include <cassert>

#include "Core.h"
#include "io/StepLoader.h"

using namespace IntuiCAM;

void testCoreInitialization() {
    std::cout << "Testing core initialization..." << std::endl;
    
    Core core;
    bool result = core.initialize();
    
    assert(result && "Core initialization should succeed");
    std::cout << "Core initialized successfully." << std::endl;
    
    std::string version = core.getVersion();
    std::cout << "IntuiCAM version: " << version << std::endl;
    
    std::string occtVersion = core.getOCCTVersion();
    std::cout << "OpenCASCADE version: " << occtVersion << std::endl;
    
    core.shutdown();
    std::cout << "Core shutdown successfully." << std::endl;
}

void testStepLoader() {
    std::cout << "Testing STEP loader (no file loading)..." << std::endl;
    
    IO::StepLoader loader;
    // Just test creation and destruction - we don't have a sample file to load yet
    
    std::cout << "STEP loader created and destroyed successfully." << std::endl;
}

int main() {
    try {
        testCoreInitialization();
        testStepLoader();
        
        std::cout << "All tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception!" << std::endl;
        return 1;
    }
}
