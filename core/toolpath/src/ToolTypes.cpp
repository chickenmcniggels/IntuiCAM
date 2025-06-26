#include "IntuiCAM/Toolpath/ToolTypes.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <cassert>

namespace IntuiCAM {
namespace Toolpath {

// ============================================================================
// Static Database Variables
// ============================================================================

std::map<std::string, ISOInsertSize> ISOToolDatabase::s_insertDatabase;
std::map<std::string, std::vector<std::string>> ISOToolDatabase::s_holderCompatibility;
bool ISOToolDatabase::s_databaseInitialized = false;

// Phase 3: Enhanced holder database
std::map<std::string, ToolHolder> ISOToolDatabase::s_holderDatabase;
std::map<ClampingStyle, std::vector<InsertShape>> ISOToolDatabase::s_clampingCompatibility;
std::map<std::string, std::string> ISOToolDatabase::s_holderDescriptions;

// ============================================================================
// ISO Tool Database Implementation
// ============================================================================

void ISOToolDatabase::initializeDatabase() {
    if (s_databaseInitialized) return;
    
    // Initialize common turning insert sizes (ISO 1832/5608)
    // CNMG - Square inserts with 7° relief angle, M tolerance
    s_insertDatabase["CNMG120408"] = {
        "CNMG120408", 12.7, 4.76, 0.8, 
        InsertShape::SQUARE, InsertReliefAngle::ANGLE_7, InsertTolerance::M_PRECISION
    };
    s_insertDatabase["CNMG120412"] = {
        "CNMG120412", 12.7, 4.76, 1.2, 
        InsertShape::SQUARE, InsertReliefAngle::ANGLE_7, InsertTolerance::M_PRECISION
    };
    
    // Initialize holder compatibility
    s_holderCompatibility["CNMG120408"] = {
        "MCLNR2525M12", "MCLNR2020K12", "MCLNR1616H12"
    };
    
    s_databaseInitialized = true;
    std::cout << "ISO Tool Database initialized with " << s_insertDatabase.size() << " insert sizes" << std::endl;
}

std::vector<ISOInsertSize> ISOToolDatabase::getAllInsertSizes(InsertShape shape) {
    initializeDatabase();
    
    std::vector<ISOInsertSize> result;
    for (const auto& [code, insert] : s_insertDatabase) {
        if (insert.shape == shape) {
            result.push_back(insert);
        }
    }
    return result;
}

ISOInsertSize ISOToolDatabase::getInsertSize(const std::string& isoCode) {
    initializeDatabase();
    
    auto it = s_insertDatabase.find(isoCode);
    return (it != s_insertDatabase.end()) ? it->second : ISOInsertSize();
}

bool ISOToolDatabase::isValidInsertCode(const std::string& isoCode) {
    initializeDatabase();
    
    return s_insertDatabase.find(isoCode) != s_insertDatabase.end();
}

std::string ISOToolDatabase::generateInsertCode(InsertShape shape, InsertReliefAngle relief, 
                                          InsertTolerance tolerance, const std::string& sizeSpec) {
    std::string shapeCode = std::string(1, static_cast<char>(shape));
    std::string reliefCode = std::string(1, static_cast<char>(relief));
    std::string toleranceCode = "M"; // Default to M tolerance
    
    return shapeCode + reliefCode + toleranceCode + "G" + sizeSpec;
}

std::vector<std::string> ISOToolDatabase::getCompatibleHolders(const std::string& insertCode) {
    initializeDatabase();
    
    auto it = s_holderCompatibility.find(insertCode);
    return (it != s_holderCompatibility.end()) ? it->second : std::vector<std::string>();
}

std::string ISOToolDatabase::generateHolderCode(HandOrientation hand, ClampingStyle clamp,
                                          const std::string& sizeSpec, InsertShape insertShape) {
    std::string handCode = (hand == HandOrientation::RIGHT_HAND) ? "R" : "L";
    std::string clampCode = "M"; // Default to top clamp
    std::string insertCode = "C"; // Default to square
    
    return "M" + insertCode + clampCode + handCode + sizeSpec;
}

std::vector<std::string> ISOToolDatabase::getCarbideGrades(const std::string& application) {
    std::vector<std::string> grades;
    
    if (application.empty() || application.find("general") != std::string::npos) {
        grades.insert(grades.end(), {"P10", "P20", "P30", "P40", "P50"});
        grades.insert(grades.end(), {"M10", "M20", "M30", "M40"});
        grades.insert(grades.end(), {"K10", "K20", "K30", "K40"});
    }
    
    return grades;
}

std::vector<std::string> ISOToolDatabase::getCoatingTypes() {
    return {"Uncoated", "TiN", "TiAlN", "TiCN", "Al2O3", "TiAlSiN", "AlCrN", "CrN", "DLC", "MultiLayer"};
}

CuttingData ISOToolDatabase::getRecommendedCuttingData(const std::string& insertCode,
                                                     const std::string& workpieceMaterial,
                                                     const std::string& operation) {
    CuttingData data;
    
    ISOInsertSize insertSize = getInsertSize(insertCode);
    if (insertSize.code.empty()) {
        return data; // Return default if insert not found
    }
    
    // Basic cutting data recommendations
    data.surfaceSpeed = 200;
    data.cuttingFeedrate = 0.2;
    data.maxDepthOfCut = 2.0;
    
    return data;
}

// Minimal validation functions
bool ISOToolDatabase::validateThreadingInsert(const ThreadingInsert& insert) {
    return !insert.isoCode.empty() && insert.thickness > 0 && insert.width > 0;
}

bool ISOToolDatabase::validateGroovingInsert(const GroovingInsert& insert) {
    return !insert.isoCode.empty() && insert.thickness > 0 && insert.grooveWidth > 0;
}

bool ISOToolDatabase::validateBoringInsert(const std::string& insertCode, double boringDiameter) {
    return !insertCode.empty() && boringDiameter > 0;
}

bool ISOToolDatabase::isThreadingInsertCode(const std::string& isoCode) {
    return isoCode.find("ER") != std::string::npos || isoCode.find("IR") != std::string::npos;
}

bool ISOToolDatabase::isGroovingInsertCode(const std::string& isoCode) {
    return isoCode.find("GTN") != std::string::npos || isoCode.find("GN") != std::string::npos;
}

bool ISOToolDatabase::isBoringInsertCode(const std::string& isoCode) {
    return isoCode.find("CCMT") != std::string::npos || isoCode.find("CCGT") != std::string::npos;
}

double ISOToolDatabase::getThreadPitchFromCode(const std::string& threadingInsertCode) {
    std::regex pitchRegex(R"((\d+\.?\d*)(?:ISO|UN|WHT|ACME))");
    std::smatch match;
    
    if (std::regex_search(threadingInsertCode, match, pitchRegex)) {
        return std::stod(match[1].str());
    }
    
    return 0.0;
}

ThreadProfile ISOToolDatabase::getThreadProfileFromCode(const std::string& threadingInsertCode) {
    if (threadingInsertCode.find("ISO") != std::string::npos) {
        return ThreadProfile::METRIC;
    } else if (threadingInsertCode.find("UN") != std::string::npos) {
        return ThreadProfile::UNIFIED;
    } else if (threadingInsertCode.find("WHT") != std::string::npos) {
        return ThreadProfile::WHITWORTH;
    } else if (threadingInsertCode.find("ACME") != std::string::npos) {
        return ThreadProfile::ACME;
    }
    
    return ThreadProfile::METRIC;
}

std::vector<double> ISOToolDatabase::getSupportedThreadPitches(const std::string& threadingInsertCode) {
    std::vector<double> pitches;
    
    if (threadingInsertCode.find("16ER") != std::string::npos) {
        pitches.insert(pitches.end(), {0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0});
    } else if (threadingInsertCode.find("22ER") != std::string::npos) {
        pitches.insert(pitches.end(), {0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0});
    } else if (threadingInsertCode.find("27ER") != std::string::npos) {
        pitches.insert(pitches.end(), {1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 3.5, 4.0});
    }
    
    return pitches;
}

double ISOToolDatabase::getGrooveWidthFromCode(const std::string& groovingInsertCode) {
    std::regex widthRegex(R"(GTN(\d+))");
    std::smatch match;
    
    if (std::regex_search(groovingInsertCode, match, widthRegex)) {
        return std::stod(match[1].str());
    }
    
    return 0.0;
}

double ISOToolDatabase::getMaxGroovingDepth(const std::string& groovingInsertCode) {
    double grooveWidth = getGrooveWidthFromCode(groovingInsertCode);
    return grooveWidth * 3.0; // Typical depth is 3x width
}

bool ISOToolDatabase::isGrooveWidthCompatible(const std::string& insertCode, double requiredWidth) {
    double insertWidth = getGrooveWidthFromCode(insertCode);
    return std::abs(insertWidth - requiredWidth) <= 0.1;
}

double ISOToolDatabase::getMinBoringDiameter(const std::string& boringInsertCode) {
    initializeDatabase();
    
    auto it = s_insertDatabase.find(boringInsertCode);
    if (it == s_insertDatabase.end()) {
        return 0.0;
    }
    
    ISOInsertSize insertSize = it->second;
    return insertSize.inscribedCircle * 3.0;
}

double ISOToolDatabase::getMaxBoringDiameter(const std::string& boringInsertCode) {
    initializeDatabase();
    
    auto it = s_insertDatabase.find(boringInsertCode);
    if (it == s_insertDatabase.end()) {
        return 0.0;
    }
    
    ISOInsertSize insertSize = it->second;
    
    if (insertSize.inscribedCircle >= 12.0) {
        return 200.0;
    } else if (insertSize.inscribedCircle >= 9.0) {
        return 100.0;
    } else {
        return 50.0;
    }
}

std::vector<std::string> ISOToolDatabase::getBoringBarsForInsert(const std::string& insertCode) {
    initializeDatabase();
    
    std::vector<std::string> boringBars;
    auto it = s_holderCompatibility.find(insertCode);
    if (it != s_holderCompatibility.end()) {
        for (const std::string& holder : it->second) {
            if (holder.find("SCLCR") != std::string::npos || holder.find("SCLCL") != std::string::npos) {
                boringBars.push_back(holder);
            }
        }
    }
    
    return boringBars;
}

// ============================================================================
// Phase 3: Holder System Enhancement Functions
// ============================================================================

bool ISOToolDatabase::validateToolHolder(const ToolHolder& holder) {
    return !holder.isoCode.empty() && holder.cuttingWidth > 0 && holder.headLength > 0;
}

bool ISOToolDatabase::validateHolderInsertCompatibility(const std::string& holderCode, const std::string& insertCode) {
    if (!s_databaseInitialized) {
        initializeDatabase();
    }
    
    auto holderIt = s_holderDatabase.find(holderCode);
    if (holderIt == s_holderDatabase.end()) {
        return false;
    }
    
    ToolHolder holder = holderIt->second;
    auto compatIt = std::find(holder.compatibleInserts.begin(), holder.compatibleInserts.end(), insertCode);
    if (compatIt != holder.compatibleInserts.end()) {
        return true;
    }
    
    ISOInsertSize insertSize = getInsertSize(insertCode);
    if (insertSize.code.empty()) {
        return false;
    }
    
    return isClampingStyleCompatibleWithInsert(holder.clampingStyle, insertSize.shape);
}

std::vector<std::string> ISOToolDatabase::getSupportedInsertShapes(ClampingStyle clampingStyle) {
    std::vector<std::string> shapeNames;
    auto shapesIt = s_clampingCompatibility.find(clampingStyle);
    if (shapesIt != s_clampingCompatibility.end()) {
        for (InsertShape shape : shapesIt->second) {
            switch (shape) {
                case InsertShape::SQUARE: shapeNames.push_back("Square (C)"); break;
                case InsertShape::TRIANGLE: shapeNames.push_back("Triangle (T)"); break;
                case InsertShape::DIAMOND_55: shapeNames.push_back("Diamond 55° (D)"); break;
                case InsertShape::DIAMOND_80: shapeNames.push_back("Diamond 80° (V)"); break;
                case InsertShape::TRIGON: shapeNames.push_back("Trigon (W)"); break;
                case InsertShape::ROUND: shapeNames.push_back("Round (R)"); break;
                default: break;
            }
        }
    }
    
    return shapeNames;
}

std::vector<std::string> ISOToolDatabase::getHoldersForInsertShape(InsertShape shape) {
    initializeDatabase();
    
    std::vector<std::string> holderCodes;
    for (const auto& [code, holder] : s_holderDatabase) {
        // Simple compatibility check
        if (holder.clampingStyle == ClampingStyle::TOP_CLAMP) {
            holderCodes.push_back(code);
        }
    }
    
    return holderCodes;
}

bool ISOToolDatabase::isClampingStyleCompatibleWithInsert(ClampingStyle clampingStyle, InsertShape insertShape) {
    // Simple compatibility - most clamping styles work with most insert shapes
    return true;
}

std::vector<ToolHolder> ISOToolDatabase::getAllHolders() {
    initializeDatabase();
    
    std::vector<ToolHolder> holders;
    for (const auto& [code, holder] : s_holderDatabase) {
        holders.push_back(holder);
    }
    
    return holders;
}

std::vector<ToolHolder> ISOToolDatabase::getHoldersByType(ClampingStyle clampingStyle, HandOrientation handOrientation) {
    initializeDatabase();
    
    std::vector<ToolHolder> holders;
    for (const auto& [code, holder] : s_holderDatabase) {
        if (holder.clampingStyle == clampingStyle && holder.handOrientation == handOrientation) {
            holders.push_back(holder);
        }
    }
    
    return holders;
}

ToolHolder ISOToolDatabase::getHolderByCode(const std::string& holderCode) {
    initializeDatabase();
    
    auto it = s_holderDatabase.find(holderCode);
    return (it != s_holderDatabase.end()) ? it->second : ToolHolder();
}

std::vector<std::string> ISOToolDatabase::getHolderVariants(const std::string& baseHolderCode) {
    initializeDatabase();
    
    std::vector<std::string> variants;
    for (const auto& [code, holder] : s_holderDatabase) {
        if (code.find(baseHolderCode.substr(0, 6)) != std::string::npos) {
            variants.push_back(code);
        }
    }
    
    return variants;
}

// Minimal implementations for remaining functions
double ISOToolDatabase::calculateHolderApproachAngle(const ToolHolder& holder) {
    return holder.sideAngle;
}

double ISOToolDatabase::calculateMaxInsertSize(const ToolHolder& holder) {
    return holder.cuttingWidth;
}

double ISOToolDatabase::calculateToolOverhang(const ToolHolder& holder) {
    return holder.overallLength - holder.headLength;
}

bool ISOToolDatabase::isHolderOrientationValid(HandOrientation hand, const std::string& operation) {
    return true; // Simplified - all orientations valid
}

std::string ISOToolDatabase::getClampingStyleDescription(ClampingStyle clampingStyle) {
    switch (clampingStyle) {
        case ClampingStyle::TOP_CLAMP: return "Top clamp with screw";
        case ClampingStyle::LEVER_CLAMP: return "Lever/cam clamp";
        case ClampingStyle::SCREW_CLAMP: return "Central screw clamp";
        default: return "Unknown";
    }
}

std::vector<std::string> ISOToolDatabase::getClampingStyleRequirements(ClampingStyle clampingStyle) {
    std::vector<std::string> requirements;
    switch (clampingStyle) {
        case ClampingStyle::TOP_CLAMP:
            requirements.push_back("Top clamp screw");
            requirements.push_back("Insert hole");
            break;
        default:
            requirements.push_back("Standard mounting");
            break;
    }
    return requirements;
}

double ISOToolDatabase::calculateInsertSetbackFromNose(const ToolHolder& holder, const std::string& insertCode) {
    return holder.insertSetback;
}

double ISOToolDatabase::calculateEffectiveCuttingAngle(const ToolHolder& holder, const std::string& insertCode) {
    return holder.sideAngle;
}

std::vector<double> ISOToolDatabase::getHolderDimensionalConstraints(const std::string& holderCode) {
    std::vector<double> constraints;
    ToolHolder holder = getHolderByCode(holderCode);
    constraints.push_back(holder.overallLength);
    constraints.push_back(holder.shankWidth);
    constraints.push_back(holder.shankHeight);
    return constraints;
}

std::string ISOToolDatabase::getOrientationSpecificCode(const std::string& baseCode, HandOrientation orientation) {
    std::string code = baseCode;
    if (orientation == HandOrientation::LEFT_HAND) {
        // Replace R with L in code
        size_t pos = code.find('R');
        if (pos != std::string::npos) {
            code[pos] = 'L';
        }
    }
    return code;
}

bool ISOToolDatabase::isOrientationApplicableForOperation(HandOrientation orientation, const std::string& operation) {
    return true; // Simplified
}

std::string ISOToolDatabase::getMirroredHolderCode(const std::string& holderCode) {
    std::string mirrored = holderCode;
    size_t pos = mirrored.find('R');
    if (pos != std::string::npos) {
        mirrored[pos] = 'L';
    } else {
        pos = mirrored.find('L');
        if (pos != std::string::npos) {
            mirrored[pos] = 'R';
        }
    }
    return mirrored;
}

bool ISOToolDatabase::checkHolderInsertPhysicalFit(const ToolHolder& holder, const std::string& insertCode) {
    ISOInsertSize insertSize = getInsertSize(insertCode);
    return insertSize.inscribedCircle <= holder.cuttingWidth;
}

bool ISOToolDatabase::checkHolderMachineClearance(const ToolHolder& holder, double spindleSize, double chuckSize) {
    return holder.shankDiameter < spindleSize * 0.8; // 80% of spindle size
}

std::vector<std::string> ISOToolDatabase::getIncompatibilityReasons(const std::string& holderCode, const std::string& insertCode) {
    std::vector<std::string> reasons;
    
    ToolHolder holder = getHolderByCode(holderCode);
    ISOInsertSize insertSize = getInsertSize(insertCode);
    
    if (insertSize.inscribedCircle > holder.cuttingWidth) {
        reasons.push_back("Insert IC exceeds holder cutting width");
    }
    
    if (insertSize.thickness > holder.headLength) {
        reasons.push_back("Insert thickness too large for holder head");
    }
    
    return reasons;
}

} // namespace Toolpath
} // namespace IntuiCAM 