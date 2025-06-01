#include <iostream>
#include <memory>
#include <fstream>

// IntuiCAM Core includes
#include <IntuiCAM/Geometry/StepLoader.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/PostProcessor/Types.h>
#include <IntuiCAM/Common/Types.h>

using namespace IntuiCAM;

int main() {
    try {
        std::cout << "IntuiCAM Basic Usage Example - Simple Turning Operation\n";
        std::cout << "======================================================\n\n";
        
        // 1. Load a STEP file
        std::cout << "1. Loading STEP file...\n";
        auto importResult = Geometry::StepLoader::importStepFile("../sample_data/simple_shaft.step");
        
        if (!importResult.success) {
            std::cerr << "Failed to load STEP file: " << importResult.errorMessage << std::endl;
            return 1;
        }
        
        if (importResult.parts.empty()) {
            std::cerr << "No parts found in STEP file" << std::endl;
            return 1;
        }
        
        auto& part = *importResult.parts[0];
        std::cout << "   ✓ Successfully loaded part\n";
        std::cout << "   ✓ Part volume: " << part.getVolume() << " cubic mm\n\n";
        
        // 2. Create a turning tool
        std::cout << "2. Creating turning tool...\n";
        auto tool = std::make_shared<Toolpath::Tool>(
            Toolpath::Tool::Type::Turning, 
            "CNMG 120408 Carbide Insert"
        );
        
        // Configure tool parameters
        Toolpath::Tool::CuttingParameters params;
        params.feedRate = 0.15;        // mm/rev
        params.spindleSpeed = 1200;    // RPM
        params.depthOfCut = 2.0;       // mm
        params.stepover = 0.8;         // mm
        tool->setCuttingParameters(params);
        
        std::cout << "   ✓ Tool created: " << tool->getName() << "\n";
        std::cout << "   ✓ Feed rate: " << params.feedRate << " mm/rev\n";
        std::cout << "   ✓ Spindle speed: " << params.spindleSpeed << " RPM\n\n";
        
        // 3. Create roughing operation
        std::cout << "3. Creating roughing operation...\n";
        auto roughingOp = std::make_unique<Toolpath::RoughingOperation>("Roughing Pass", tool);
        
        // Configure operation parameters
        Toolpath::RoughingOperation::Parameters roughParams;
        roughParams.startDiameter = 50.0;     // mm
        roughParams.endDiameter = 20.0;       // mm
        roughParams.startZ = 0.0;             // mm
        roughParams.endZ = -80.0;             // mm
        roughParams.depthOfCut = 2.0;         // mm per pass
        roughParams.stockAllowance = 0.5;     // mm for finishing
        roughingOp->setParameters(roughParams);
        
        std::cout << "   ✓ Roughing operation configured\n";
        std::cout << "   ✓ Material removal: " << roughParams.startDiameter << "mm → " 
                  << roughParams.endDiameter << "mm diameter\n";
        std::cout << "   ✓ Length: " << (roughParams.endZ - roughParams.startZ) << "mm\n\n";
        
        // 4. Generate toolpath
        std::cout << "4. Generating toolpath...\n";
        if (!roughingOp->validate()) {
            std::cerr << "Operation validation failed" << std::endl;
            return 1;
        }
        
        auto toolpath = roughingOp->generateToolpath(part);
        if (!toolpath) {
            std::cerr << "Failed to generate toolpath" << std::endl;
            return 1;
        }
        
        std::cout << "   ✓ Toolpath generated successfully\n";
        std::cout << "   ✓ Total movements: " << toolpath->getMovementCount() << "\n";
        std::cout << "   ✓ Estimated machining time: " << toolpath->estimateMachiningTime() 
                  << " minutes\n\n";
        
        // 5. Create finishing operation
        std::cout << "5. Creating finishing operation...\n";
        auto finishingTool = std::make_shared<Toolpath::Tool>(
            Toolpath::Tool::Type::Turning, // Changed from Finishing to Turning
            "VCMT 160404 Finishing Insert"
        );
        
        Toolpath::Tool::CuttingParameters finishParams;
        finishParams.feedRate = 0.08;      // mm/rev - finer feed for finishing
        finishParams.spindleSpeed = 1800;  // RPM - higher speed for finishing
        finishingTool->setCuttingParameters(finishParams);
        
        auto finishingOp = std::make_unique<Toolpath::FinishingOperation>("Finishing Pass", finishingTool);
        
        Toolpath::FinishingOperation::Parameters finishOpParams;
        finishOpParams.targetDiameter = 20.0;    // mm - final diameter
        finishOpParams.startZ = 0.0;             // mm
        finishOpParams.endZ = -80.0;             // mm
        finishOpParams.surfaceSpeed = 180.0;     // m/min
        finishOpParams.feedRate = 0.08;          // mm/rev
        finishingOp->setParameters(finishOpParams);
        
        auto finishingToolpath = finishingOp->generateToolpath(part);
        
        std::cout << "   ✓ Finishing operation configured\n";
        std::cout << "   ✓ Finishing movements: " << finishingToolpath->getMovementCount() << "\n\n";
        
        // 6. Generate G-code
        std::cout << "6. Generating G-code...\n";
        auto postProcessor = PostProcessor::PostProcessor::createForMachine(
            PostProcessor::PostProcessor::MachineType::Fanuc
        );
        
        // Process roughing toolpath
        auto roughingResult = postProcessor->process(*toolpath);
        if (!roughingResult.success) {
            std::cerr << "Failed to generate G-code for roughing: ";
            for (const auto& error : roughingResult.errors) {
                std::cerr << error << " ";
            }
            std::cerr << std::endl;
            return 1;
        }
        
        // Process finishing toolpath
        auto finishingResult = postProcessor->process(*finishingToolpath);
        if (!finishingResult.success) {
            std::cerr << "Failed to generate G-code for finishing" << std::endl;
            return 1;
        }
        
        std::cout << "   ✓ G-code generated successfully\n";
        std::cout << "   ✓ Roughing G-code length: " << roughingResult.gcode.length() << " characters\n";
        std::cout << "   ✓ Finishing G-code length: " << finishingResult.gcode.length() << " characters\n";
        std::cout << "   ✓ Total estimated time: " << (roughingResult.estimatedTime + finishingResult.estimatedTime) 
                  << " minutes\n\n";
        
        // 7. Save G-code to files
        std::cout << "7. Saving G-code files...\n";
        
        // Save roughing G-code
        std::ofstream roughingFile("roughing_operation.nc");
        if (roughingFile.is_open()) {
            roughingFile << roughingResult.gcode;
            roughingFile.close();
            std::cout << "   ✓ Roughing G-code saved to: roughing_operation.nc\n";
        }
        
        // Save finishing G-code
        std::ofstream finishingFile("finishing_operation.nc");
        if (finishingFile.is_open()) {
            finishingFile << finishingResult.gcode;
            finishingFile.close();
            std::cout << "   ✓ Finishing G-code saved to: finishing_operation.nc\n";
        }
        
        std::cout << "\n✓ Example completed successfully!\n";
        std::cout << "\nGenerated files:\n";
        std::cout << "  - roughing_operation.nc  (Roughing toolpath)\n";
        std::cout << "  - finishing_operation.nc (Finishing toolpath)\n";
        
        return 0;
        
    } catch (const Common::Exception& e) {
        std::cerr << "IntuiCAM Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Error: " << e.what() << std::endl;
        return 1;
    }
} 