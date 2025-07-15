#include <IntuiCAM/Toolpath/ThreadingOperation.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Toolpath/OperationParameterManager.h>
#include <IntuiCAM/Geometry/Types.h>

#include <algorithm>
#include <cmath>
#include <regex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace Toolpath {

ThreadingOperation::ThreadingOperation(const std::string& name, std::shared_ptr<Tool> tool) 
    : Operation(Operation::Type::Threading, name, tool) {
    // Initialize with default parameters
    params_ = Parameters();
}

std::unique_ptr<Toolpath> ThreadingOperation::generateToolpath(const Geometry::Part& part) {
    // Use the advanced interface with stored parameters and tool
    auto result = generateToolpaths(part, getTool(), params_);
    
    // Return the main threading toolpath (standard interface expects single toolpath)
    if (result.success && result.threadingToolpath) {
        return std::move(result.threadingToolpath);
    }
    
    return nullptr;
}

bool ThreadingOperation::validate() const {
    // Validate that we have a tool and parameters are reasonable
    if (!getTool()) {
        return false;
    }
    
    // Validate parameters using static method
    std::string validationError = validateParameters(params_);
    return validationError.empty();
}

ThreadingOperation::Result ThreadingOperation::generateToolpaths(
    const IntuiCAM::Geometry::Part& part,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    // Return placeholder result - Threading operation implementation placeholder
    Result result;
    result.success = true;
    result.usedParameters = params;
    result.estimatedTime = 0.0;
    result.totalPasses = params.numberOfPasses;
    result.actualThreadDepth = params.threadDepth;
    result.materialRemoved = 0.0;
    
    // Create simple threading toolpath
    auto toolpath = std::make_unique<Toolpath>("Threading", tool, OperationType::Threading);
    
    // Add basic threading movements
    IntuiCAM::Geometry::Point3D startPos(params.startZ, 0.0, params.majorDiameter / 2.0);
    IntuiCAM::Geometry::Point3D endPos(params.endZ, 0.0, params.majorDiameter / 2.0 - params.threadDepth);
    
    toolpath->addRapidMove(startPos);
    toolpath->addLinearMove(endPos, params.feedRate);
    
    result.threadingToolpath = std::move(toolpath);
    
    return result;
}

std::vector<ThreadingOperation::ThreadFeature> ThreadingOperation::detectThreadFeatures(
    const LatheProfile::Profile2D& profile,
    const Parameters& params) {
    
    std::vector<ThreadFeature> features;
    
    // Convert segments to points for thread detection
    std::vector<IntuiCAM::Geometry::Point2D> points = profile.toPointArray(0.1);
    
    if (points.size() < 10) {
        return features; // Not enough points for thread detection
    }
    
    // Analyze profile for thread-like patterns - placeholder implementation
    const double minThreadLength = params.pitch * 3; // Minimum 3 pitches
    const double radiusTolerance = 0.1; // mm
    
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& point = points[i];
        
        // Look for cylindrical sections that could be threads
        if (point.x > 0.1 && i + 10 < points.size()) { // Valid radius and enough points ahead
            bool potentialThread = true;
            double avgRadius = point.x;
            
            // Check next several points for consistent radius
            for (size_t j = i + 1; j < std::min(i + 10, points.size()); ++j) {
                if (std::abs(points[j].x - avgRadius) > radiusTolerance) {
                    potentialThread = false;
                    break;
                }
            }
            
            if (potentialThread) {
                ThreadFeature feature;
                feature.position = gp_Pnt(point.z, 0.0, point.x);
                feature.type = (point.x < 5.0) ? ThreadType::Internal : ThreadType::External;
                feature.diameter = point.x * 2.0;
                feature.pitch = params.pitch; // Use provided pitch as guess
                feature.length = minThreadLength;
                feature.isMetric = (params.threadForm == ThreadForm::Metric);
                feature.designation = "Detected Thread";
                feature.confidence = 0.7;
                
                features.push_back(feature);
                i += 10; // Skip ahead to avoid duplicate detections
            }
        }
    }
    
    return features;
}

ThreadingOperation::Parameters ThreadingOperation::calculateThreadParameters(const std::string& threadDesignation) {
    Parameters params;
    
    // Parse common thread designations
    std::regex metricRegex(R"(M(\d+(?:\.\d+)?)(?:x(\d+(?:\.\d+)?))?)");
    std::regex uncRegex(R"((\d+(?:\.\d+)?)-(\d+))");
    
    std::smatch match;
    
    if (std::regex_match(threadDesignation, match, metricRegex)) {
        // Metric thread (e.g., "M20x1.5" or "M20")
        params.threadForm = ThreadForm::Metric;
        params.majorDiameter = std::stod(match[1].str());
        
        if (match[2].matched) {
            params.pitch = std::stod(match[2].str());
        } else {
            // Standard metric pitch based on diameter
            if (params.majorDiameter <= 6.0) params.pitch = 1.0;
            else if (params.majorDiameter <= 12.0) params.pitch = 1.25;
            else if (params.majorDiameter <= 20.0) params.pitch = 1.5;
            else params.pitch = 2.0;
        }
        
        params.threadDepth = 0.613 * params.pitch; // Standard metric thread depth
        
    } else if (std::regex_match(threadDesignation, match, uncRegex)) {
        // UNC thread (e.g., "1/4-20")
        params.threadForm = ThreadForm::UNC;
        params.majorDiameter = std::stod(match[1].str()) * 25.4; // Convert inches to mm
        double tpi = std::stod(match[2].str());
        params.pitch = 25.4 / tpi; // Convert TPI to mm
        params.threadDepth = 0.613 * params.pitch;
        
    } else {
        // Default to M20x1.5
        params.threadForm = ThreadForm::Metric;
        params.majorDiameter = 20.0;
        params.pitch = 1.5;
        params.threadDepth = 0.613 * params.pitch;
    }
    
    return params;
}

std::string ThreadingOperation::validateParameters(const Parameters& params) {
    if (params.majorDiameter <= 0.0) {
        return "Major diameter must be positive";
    }
    
    if (params.pitch <= 0.0) {
        return "Thread pitch must be positive";
    }
    
    if (params.threadDepth <= 0.0) {
        return "Thread depth must be positive";
    }
    
    if (params.threadLength <= 0.0) {
        return "Thread length must be positive";
    }
    
    if (params.numberOfPasses < 1) {
        return "Number of passes must be at least 1";
    }
    
    if (params.feedRate <= 0.0) {
        return "Feed rate must be positive";
    }
    
    if (params.spindleSpeed <= 0.0) {
        return "Spindle speed must be positive";
    }
    
    return ""; // Valid
}

ThreadingOperation::Parameters ThreadingOperation::getDefaultParameters(
    ThreadForm threadForm,
    double diameter,
    const std::string& materialType) {
    
    Parameters params;
    params.threadForm = threadForm;
    params.majorDiameter = diameter;
    
    // Set pitch based on thread form and diameter
    switch (threadForm) {
        case ThreadForm::Metric:
            if (diameter <= 6.0) params.pitch = 1.0;
            else if (diameter <= 12.0) params.pitch = 1.25;
            else if (diameter <= 20.0) params.pitch = 1.5;
            else params.pitch = 2.0;
            params.threadDepth = 0.613 * params.pitch;
            break;
            
        case ThreadForm::UNC:
        case ThreadForm::UNF:
            // Unified threads - pitch based on diameter
            params.pitch = std::max(0.5, diameter * 0.1);
            params.threadDepth = 0.613 * params.pitch;
            break;
            
        case ThreadForm::ACME:
            params.pitch = std::max(1.5, diameter * 0.15);
            params.threadDepth = params.pitch / 2.0;
            break;
            
        case ThreadForm::Trapezoidal:
            params.pitch = std::max(1.5, diameter * 0.08);
            params.threadDepth = params.pitch / 2.0;
            break;
            
        case ThreadForm::BSW:
            params.pitch = std::max(1.0, diameter * 0.075);
            params.threadDepth = 0.96 * params.pitch;
            break;
            
        case ThreadForm::Custom:
            // User will set custom parameters
            break;
    }
    
    // Material-specific cutting parameters
    if (materialType == "aluminum") {
        params.feedRate = 80.0;
        params.spindleSpeed = 500.0;
    } else if (materialType == "stainless_steel") {
        params.feedRate = 40.0;
        params.spindleSpeed = 200.0;
    } else { // steel
        params.feedRate = 60.0;
        params.spindleSpeed = 300.0;
    }
    
    return params;
}

std::vector<std::unique_ptr<Toolpath>> ThreadingOperation::generateThreadingPasses(
    const IntuiCAM::Geometry::Part& part,
    std::shared_ptr<Tool> tool,
    const Parameters& params) {
    
    std::vector<std::unique_ptr<Toolpath>> passes;
    
    // Create simple threading passes - placeholder implementation
    for (int pass = 0; pass < params.numberOfPasses; ++pass) {
        auto toolpath = std::make_unique<Toolpath>("Threading Pass " + std::to_string(pass + 1), tool, OperationType::Threading);
        
        double depth = params.threadDepth * (pass + 1) / params.numberOfPasses;
        double radius = (params.majorDiameter / 2.0) - depth;
        
        IntuiCAM::Geometry::Point3D startPos(params.startZ, 0.0, radius);
        IntuiCAM::Geometry::Point3D endPos(params.endZ, 0.0, radius);
        
        toolpath->addRapidMove(startPos);
        toolpath->addLinearMove(endPos, params.feedRate);
        
        passes.push_back(std::move(toolpath));
    }
    
    return passes;
}

std::unique_ptr<Toolpath> ThreadingOperation::generateThreadChamfer(
    const Parameters& params,
    std::shared_ptr<Tool> tool,
    bool isStart) {
    
    if ((!params.chamferThreadStart && isStart) || (!params.chamferThreadEnd && !isStart)) {
        return nullptr;
    }
    
    auto toolpath = std::make_unique<Toolpath>("Thread Chamfer", tool, OperationType::Threading);
    
    double chamferZ = isStart ? params.startZ : params.endZ;
    double radius = params.majorDiameter / 2.0;
    
    IntuiCAM::Geometry::Point3D chamferPos(chamferZ, 0.0, radius);
    toolpath->addRapidMove(chamferPos);
    toolpath->addLinearMove(chamferPos, params.feedRate * 0.8);
    
    return toolpath;
}

std::vector<double> ThreadingOperation::calculateDepthProgression(const Parameters& params) {
    std::vector<double> depths;
    
    if (params.constantDepthPasses) {
        // Constant depth per pass
        double depthPerPass = params.threadDepth / params.numberOfPasses;
        for (int i = 1; i <= params.numberOfPasses; ++i) {
            depths.push_back(i * depthPerPass);
        }
    } else {
        // Variable depth with degression
        double remainingDepth = params.threadDepth;
        double currentDepth = 0.0;
        
        for (int i = 0; i < params.numberOfPasses; ++i) {
            if (i == params.numberOfPasses - 1) {
                currentDepth = params.threadDepth;
            } else {
                double passDepth = remainingDepth * (1.0 - params.degression);
                currentDepth += passDepth;
                remainingDepth -= passDepth;
            }
            depths.push_back(currentDepth);
        }
    }
    
    return depths;
}

std::unique_ptr<Toolpath> ThreadingOperation::generateSinglePass(
    const Parameters& params,
    std::shared_ptr<Tool> tool,
    double depth,
    int passNumber) {
    
    auto toolpath = std::make_unique<Toolpath>("Threading Pass " + std::to_string(passNumber), tool, OperationType::Threading);
    
    double radius = (params.majorDiameter / 2.0) - depth;
    IntuiCAM::Geometry::Point3D startPos(params.startZ, 0.0, radius);
    IntuiCAM::Geometry::Point3D endPos(params.endZ, 0.0, radius);
    
    toolpath->addRapidMove(startPos);
    toolpath->addLinearMove(endPos, params.feedRate);
    
    return toolpath;
}

void ThreadingOperation::calculateThreadGeometry(const Parameters& params,
                                               double& minorDiameter,
                                               double& pitchDiameter,
                                               double& threadAngle) {
    
    // Basic thread geometry calculations
    minorDiameter = params.majorDiameter - 2.0 * params.threadDepth;
    pitchDiameter = params.majorDiameter - params.threadDepth;
    
    switch (params.threadForm) {
        case ThreadForm::Metric:
        case ThreadForm::UNC:
        case ThreadForm::UNF:
            threadAngle = 60.0;
            break;
        case ThreadForm::BSW:
            threadAngle = 55.0;
            break;
        case ThreadForm::ACME:
            threadAngle = 29.0;
            break;
        case ThreadForm::Trapezoidal:
            threadAngle = 30.0;
            break;
        case ThreadForm::Custom:
            threadAngle = 60.0; // Default
            break;
    }
}

std::string ThreadingOperation::validateManufacturingConstraints(const Parameters& params,
                                                                std::shared_ptr<Tool> tool) {
    
    if (!tool) {
        return "Threading tool is required";
    }
    
    // Check tool compatibility
    if (tool->getType() != Tool::Type::Threading && tool->getType() != Tool::Type::Turning) {
        return "Tool must be threading or turning type";
    }
    
    // Check geometric constraints
    if (params.threadDepth > params.majorDiameter / 4.0) {
        return "Thread depth is too large for diameter";
    }
    
    if (params.pitch > params.majorDiameter / 3.0) {
        return "Thread pitch is too large for diameter";
    }
    
    return ""; // Valid
}

} // namespace Toolpath
} // namespace IntuiCAM
