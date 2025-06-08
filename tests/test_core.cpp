#include <iostream>
#include <cassert>

// Include IntuiCAM core headers
#include <IntuiCAM/Common/Types.h>
#include <IntuiCAM/Common/Version.h>
#include <IntuiCAM/Geometry/Types.h>

// Basic test function for core functionality
void test_common_types() {
    std::cout << "Testing IntuiCAM Common Types..." << std::endl;
    
    // Test basic types and functionality
    IntuiCAM::Geometry::Point3D point(1.0, 2.0, 3.0);
    assert(point.x == 1.0);
    assert(point.y == 2.0);
    assert(point.z == 3.0);
    
    std::cout << "✓ Point3D test passed" << std::endl;
}

void test_version_info() {
    std::cout << "Testing IntuiCAM Version Info..." << std::endl;

    // Test that version constants are defined
    std::cout << "IntuiCAM Version: "
              << IntuiCAM::Common::Version::MAJOR << "."
              << IntuiCAM::Common::Version::MINOR << "."
              << IntuiCAM::Common::Version::PATCH << std::endl;

    // Verify getVersionString() returns the expected value
    std::string version = IntuiCAM::Common::Version::getVersionString();
    assert(version == "0.1.0");

    std::cout << "✓ Version info test passed" << std::endl;
}

int main() {
    std::cout << "==================================" << std::endl;
    std::cout << "IntuiCAM Core Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        test_common_types();
        test_version_info();
        
        std::cout << "==================================" << std::endl;
        std::cout << "✓ All core tests passed!" << std::endl;
        std::cout << "==================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
} 