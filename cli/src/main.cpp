#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    std::cout << "IntuiCAM Command Line Interface" << std::endl;
    std::cout << "Computer Aided Manufacturing Tool" << std::endl;
    
    if (argc > 1) {
        std::cout << "Processing arguments:" << std::endl;
        for (int i = 1; i < argc; ++i) {
            std::cout << "  " << i << ": " << argv[i] << std::endl;
        }
    } else {
        std::cout << "Usage: " << argv[0] << " [options] [files...]" << std::endl;
        std::cout << "No arguments provided - starting interactive mode" << std::endl;
    }
    
    return 0;
} 