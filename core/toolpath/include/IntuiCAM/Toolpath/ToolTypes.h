#ifndef INTUICAM_TOOLPATH_TOOLTYPES_H
#define INTUICAM_TOOLPATH_TOOLTYPES_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <unordered_map>

namespace IntuiCAM {
namespace Toolpath {

// ============================================================================
// ISO-compliant Tool Enumerations
// ============================================================================

enum class InsertShape {
    // ISO 5608 - Insert shapes
    TRIANGLE = 'T',      // 60° triangle
    SQUARE = 'S',        // 90° square  
    PENTAGON = 'P',      // 108° pentagon
    DIAMOND_80 = 'D',    // 80° diamond
    DIAMOND_55 = 'C',    // 55° diamond
    HEXAGON = 'H',       // 120° hexagon
    OCTAGON = 'O',       // 135° octagon
    RHOMBIC_86 = 'V',    // 86° rhombic
    RHOMBIC_75 = 'E',    // 75° rhombic
    RHOMBUS_80 = 'V',    // 80° rhombus (alias for RHOMBIC_86)
    RHOMBUS_55 = 'C',    // 55° rhombus (alias for DIAMOND_55)
    RHOMBUS_35 = 'E',    // 35° rhombus (alias for RHOMBIC_75)
    ROUND = 'R',         // Round insert
    TRIGON = 'W',        // Trigon 80°
    CUSTOM = 'X'         // Custom shape
};

enum class InsertReliefAngle {
    // ISO 5608 - Relief angles
    ANGLE_0 = 'N',       // 0°
    ANGLE_3 = 'A',       // 3°
    ANGLE_5 = 'B',       // 5°
    ANGLE_7 = 'C',       // 7°
    ANGLE_11 = 'D',      // 11°
    ANGLE_15 = 'E',      // 15°
    ANGLE_20 = 'F',      // 20°
    ANGLE_25 = 'G',      // 25°
    ANGLE_30 = 'H'       // 30°
};

enum class InsertTolerance {
    // ISO 1832 - Insert tolerances
    A_PRECISION,         // ±0.005mm
    B_PRECISION,         // ±0.008mm  
    C_PRECISION,         // ±0.013mm
    D_PRECISION,         // ±0.020mm
    E_PRECISION,         // ±0.025mm
    F_PRECISION,         // ±0.050mm
    G_PRECISION,         // ±0.080mm
    H_PRECISION,         // ±0.130mm
    K_PRECISION,         // ±0.200mm
    L_PRECISION,         // ±0.250mm
    M_PRECISION,         // ±0.380mm
    N_PRECISION          // ±0.500mm
};

enum class InsertMaterial {
    // ISO 513 - Cutting tool materials
    UNCOATED_CARBIDE,    // P, M, K grades
    COATED_CARBIDE,      // CVD/PVD coated
    CERMET,              // TiC/TiN based
    CERAMIC,             // Al2O3, Si3N4
    CBN,                 // Cubic Boron Nitride
    PCD,                 // Polycrystalline Diamond
    HSS,                 // High Speed Steel
    CAST_ALLOY,          // Stellite type
    DIAMOND              // Single crystal
};

enum class HandOrientation {
    RIGHT_HAND,          // R - Right hand
    LEFT_HAND,           // L - Left hand
    NEUTRAL              // N - Neutral
};

enum class ClampingStyle {
    // ISO 5610 - Clamping methods
    TOP_CLAMP,           // M - Top clamp with screw
    TOP_CLAMP_HOLE,      // G - Top clamp through hole
    LEVER_CLAMP,         // C - Lever/cam clamp
    SCREW_CLAMP,         // S - Central screw clamp
    WEDGE_CLAMP,         // W - Wedge clamp
    PIN_LOCK,            // P - Pin lock system
    CARTRIDGE            // K - Cartridge system
};

enum class ThreadProfile {
    METRIC,              // 60° metric thread
    UNIFIED,             // 60° unified thread
    WHITWORTH,           // 55° Whitworth thread
    ACME,                // 29° ACME thread  
    TRAPEZOIDAL,         // 30° trapezoidal thread
    SQUARE,              // Square thread
    BUTTRESS,            // Buttress thread
    CUSTOM               // Custom thread profile
};

enum class ThreadTipType {
    SHARP_POINT,         // Sharp pointed tip
    FLAT_TIP,            // Flat tip
    ROUND_TIP            // Rounded tip
};

enum class CoolantType {
    NONE,                // No coolant
    MIST,                // Mist coolant
    MIST_COOLANT,        // Alternative name for mist
    FLOOD,               // Flood coolant
    FLOOD_COOLANT,       // Alternative name for flood
    HIGH_PRESSURE,       // High pressure coolant
    INTERNAL,            // Through-tool coolant
    AIR_BLAST            // Air blast
};

// ============================================================================
// ISO Size Specifiers
// ============================================================================

struct ISOInsertSize {
    std::string code;                // E.g., "CNMG120408"
    double inscribedCircle;      // IC - mm
    double thickness;            // S - mm
    double cornerRadius;         // r - mm
    InsertShape shape;
    InsertReliefAngle reliefAngle;
    InsertTolerance tolerance;
    
    ISOInsertSize() : inscribedCircle(0), thickness(0), cornerRadius(0), 
                      shape(InsertShape::SQUARE), reliefAngle(InsertReliefAngle::ANGLE_7),
                      tolerance(InsertTolerance::M_PRECISION) {}
    
    ISOInsertSize(const std::string& c, double ic, double t, double cr, 
                  InsertShape s, InsertReliefAngle ra, InsertTolerance tol)
        : code(c), inscribedCircle(ic), thickness(t), cornerRadius(cr),
          shape(s), reliefAngle(ra), tolerance(tol) {}
};

// ============================================================================
// Cutting Insert Structures
// ============================================================================

struct GeneralTurningInsert {
    // ISO identification
    std::string isoCode;             // Complete ISO designation e.g., "CNMG120408"
    InsertShape shape;
    InsertReliefAngle reliefAngle;
    InsertTolerance tolerance;
    std::string sizeSpecifier;       // 4-digit size code
    
    // Physical dimensions (from ISO tables)
    double inscribedCircle;      // IC - mm
    double thickness;            // S - mm  
    double cornerRadius;         // r - mm
    double cuttingEdgeLength;    // l - mm
    double width;                // d1 - mm (for rectangular inserts)
    
    // Material properties
    InsertMaterial material;
    std::string substrate;           // Base carbide grade
    std::string coating;             // Coating type/thickness
    std::string manufacturer;
    std::string partNumber;
    
    // Cutting geometry
    double rake_angle;           // γ - degrees (chipbreaker dependent)
    double inclination_angle;    // λ - degrees
    
    // User properties
    std::string name;
    std::string vendor;
    std::string productId;
    std::string productLink;
    std::string notes;
    bool isActive;
    
    GeneralTurningInsert() : inscribedCircle(0), thickness(0), cornerRadius(0),
                            cuttingEdgeLength(0), width(0), rake_angle(0), inclination_angle(0),
                            shape(InsertShape::SQUARE), reliefAngle(InsertReliefAngle::ANGLE_7),
                            tolerance(InsertTolerance::M_PRECISION), material(InsertMaterial::UNCOATED_CARBIDE),
                            isActive(true) {}
};

struct ThreadingInsert {
    // ISO threading insert designation
    std::string isoCode;             // e.g., "16ER1.0ISO"
    InsertShape isoShape;        // Usually partial profile
    InsertShape shape;               // Insert shape
    InsertTolerance tolerance;
    std::string crossSection;        // Threading insert cross-section code
    InsertMaterial material;
    
    // Threading specific dimensions
    double thickness;            // mm
    double width;                // mm
    double minThreadPitch;       // mm
    double maxThreadPitch;       // mm
    bool internalThreads;        // true for internal, false for external
    bool externalThreads;        // can do both
    
    // Thread geometry
    ThreadProfile threadProfile;
    double threadProfileAngle;   // degrees (60° for metric)
    ThreadTipType threadTipType;
    double threadTipRadius;      // mm
    
    // User properties
    std::string name;
    std::string vendor;
    std::string productId;
    std::string productLink;
    std::string notes;
    bool isActive;
    
    ThreadingInsert() : thickness(0), width(0), minThreadPitch(0), maxThreadPitch(0),
                       internalThreads(false), externalThreads(true), threadProfileAngle(60),
                       threadTipRadius(0), isoShape(InsertShape::CUSTOM), tolerance(InsertTolerance::M_PRECISION),
                       material(InsertMaterial::UNCOATED_CARBIDE), threadProfile(ThreadProfile::METRIC),
                       threadTipType(ThreadTipType::SHARP_POINT), isActive(true) {}
};

struct GroovingInsert {
    // ISO grooving insert designation
    std::string isoCode;
    InsertShape shape;           // Usually rectangular
    InsertTolerance tolerance;
    std::string crossSection;
    InsertMaterial material;
    
    // Grooving specific dimensions
    double thickness;            // mm
    double overallLength;        // mm
    double width;                // Grooving width - mm
    double cornerRadius;         // mm
    double headLength;           // mm
    double grooveWidth;          // Cutting width - mm
    
    // User properties
    std::string name;
    std::string vendor;
    std::string productId;
    std::string productLink;
    std::string notes;
    bool isActive;
    
    GroovingInsert() : thickness(0), overallLength(0), width(0), cornerRadius(0),
                      headLength(0), grooveWidth(0), shape(InsertShape::CUSTOM),
                      tolerance(InsertTolerance::M_PRECISION), material(InsertMaterial::UNCOATED_CARBIDE),
                      isActive(true) {}
};

// ============================================================================
// Tool Holder Structures  
// ============================================================================

struct ToolHolder {
    // ISO holder designation
    std::string isoCode;
    HandOrientation handOrientation;
    ClampingStyle clampingStyle;
    
    // Physical dimensions
    double cuttingWidth;         // mm - insert cutting edge engagement
    double headLength;           // mm - holder head length
    double overallLength;        // mm - total holder length
    double shankWidth;           // mm - rectangular shank width
    double shankHeight;          // mm - rectangular shank height
    bool roundShank;             // true for round, false for rectangular
    bool isRoundShank;           // Alternative name for roundShank
    double shankDiameter;        // mm - for round shanks
    
    // Cutting geometry
    double insertSeatAngle;      // degrees - angle of insert seat
    double insertSetback;        // mm - insert setback from holder nose
    double sideAngle;            // degrees - side cutting edge angle
    double backAngle;            // degrees - back cutting edge angle
    
    // Compatibility
    std::vector<std::string> compatibleInserts; // List of compatible insert ISO codes
    
    // Holder capabilities
    bool isInternal;             // Internal vs external operations
    bool isGrooving;             // Grooving holder
    bool isThreading;            // Threading holder
    
    // User properties
    std::string name;
    std::string vendor; 
    std::string productId;
    std::string productLink;
    std::string notes;
    bool isActive;
    
    ToolHolder() : cuttingWidth(0), headLength(0), overallLength(0), shankWidth(0),
                  shankHeight(0), roundShank(false), shankDiameter(0), insertSeatAngle(0),
                  insertSetback(0), sideAngle(0), backAngle(0), isInternal(false),
                  isGrooving(false), isThreading(false), handOrientation(HandOrientation::RIGHT_HAND),
                  clampingStyle(ClampingStyle::TOP_CLAMP), isActive(true) {}
};

// ============================================================================
// Cutting Data Structures
// ============================================================================

struct CuttingData {
    // Speed control
    bool constantSurfaceSpeed;   // true for CSS, false for RPM control
    double surfaceSpeed;         // m/min - when CSS enabled
    double spindleRPM;           // RPM - when CSS disabled
    
    // Feed control  
    bool feedPerRevolution;      // true for mm/rev, false for mm/min
    double cuttingFeedrate;      // mm/rev or mm/min
    double leadInFeedrate;       // mm/rev or mm/min
    double leadOutFeedrate;      // mm/rev or mm/min
    
    // Cutting limits
    double maxDepthOfCut;        // mm - maximum radial/axial depth
    double maxFeedrate;          // mm/min - absolute maximum
    double minSurfaceSpeed;      // m/min - minimum for tool life
    double maxSurfaceSpeed;      // m/min - maximum for tool life
    
    // Coolant
    bool floodCoolant;           // Flood coolant enabled
    bool mistCoolant;            // Mist coolant enabled
    CoolantType preferredCoolant;
    CoolantType coolantType;     // Alternative name for preferred coolant
    double coolantPressure;      // bar - for high pressure coolant
    double coolantFlow;          // L/min
    
    CuttingData() : constantSurfaceSpeed(true), surfaceSpeed(200), spindleRPM(1000),
                   feedPerRevolution(true), cuttingFeedrate(0.2), leadInFeedrate(0.1),
                   leadOutFeedrate(0.1), maxDepthOfCut(2.0), maxFeedrate(1000),
                   minSurfaceSpeed(50), maxSurfaceSpeed(500), floodCoolant(false),
                   mistCoolant(false), preferredCoolant(CoolantType::NONE), 
                   coolantPressure(0), coolantFlow(0) {}
};

// ============================================================================
// Complete Tool Assembly
// ============================================================================

enum class ToolType {
    GENERAL_TURNING,     // General turning insert + holder
    BORING,              // Boring insert + boring bar  
    THREADING,           // Threading insert + holder
    GROOVING,            // Grooving insert + holder
    PARTING,             // Parting insert + holder
    FORM_TOOL,           // Custom form tool
    LIVE_TOOLING         // Driven tools for mill/drill ops
};

struct ToolAssembly {
    std::string id;                  // Unique tool assembly ID
    std::string name;                // User-defined name
    std::string manufacturer;        // Tool manufacturer/vendor
    ToolType toolType;
    
    // Component references (only one set will be used based on type)
    std::shared_ptr<GeneralTurningInsert> turningInsert;
    std::shared_ptr<ThreadingInsert> threadingInsert;
    std::shared_ptr<GroovingInsert> groovingInsert;
    std::shared_ptr<ToolHolder> holder;
    
    // Cutting parameters
    CuttingData cuttingData;
    
    // Tool positioning
    double toolOffset_X;         // mm - X offset from turret center
    double toolOffset_Z;         // mm - Z offset from turret center  
    double toolLengthOffset;     // mm - tool length compensation
    double toolRadiusOffset;     // mm - tool radius compensation
    
    // Tool management
    std::string toolNumber;          // Machine tool number (T01, T02, etc.)
    int turretPosition;          // Physical turret position
    bool isActive;               // Tool is available for use
    
    // Tool capabilities (moved from ToolHolder to apply to entire tool assembly)
    bool internalThreading;      // Can perform internal threading operations
    bool internalBoring;         // Can perform internal boring operations  
    bool partingGrooving;        // Can perform parting/grooving operations
    bool externalThreading;      // Can perform external threading operations
    bool longitudinalTurning;    // Can perform longitudinal turning operations
    bool facing;                 // Can perform facing operations
    bool chamfering;             // Can perform chamfering operations
    
    // Tool life management
    double expectedLifeMinutes;  // Expected tool life in minutes
    double usageMinutes;         // Accumulated usage time
    int cycleCount;              // Number of parts machined
    std::string lastMaintenanceDate; // ISO date string
    std::string nextMaintenanceDate; // ISO date string
    std::string lastUsedDate;        // Last usage timestamp
    
    // User properties
    std::string notes;
    std::map<std::string, std::string> customProperties; // Extensible properties
    
    ToolAssembly() : toolType(ToolType::GENERAL_TURNING), toolOffset_X(0), toolOffset_Z(0),
                    toolLengthOffset(0), toolRadiusOffset(0), turretPosition(1), isActive(true),
                    internalThreading(false), internalBoring(false), partingGrooving(false),
                    externalThreading(false), longitudinalTurning(true), facing(true), chamfering(false),
                    expectedLifeMinutes(480), usageMinutes(0), cycleCount(0) {}
};

// ============================================================================
// ISO Database Interface
// ============================================================================

class ISOToolDatabase {
public:
    // Insert database access
    static std::vector<ISOInsertSize> getAllInsertSizes(InsertShape shape);
    static ISOInsertSize getInsertSize(const std::string& isoCode);
    static bool isValidInsertCode(const std::string& isoCode);
    static std::string generateInsertCode(InsertShape shape, InsertReliefAngle relief, 
                                    InsertTolerance tolerance, const std::string& sizeSpec);
    
    // Holder database access
    static std::vector<std::string> getCompatibleHolders(const std::string& insertCode);
    static std::string generateHolderCode(HandOrientation hand, ClampingStyle clamp,
                                    const std::string& sizeSpec, InsertShape insertShape);
    
    // Material grade database
    static std::vector<std::string> getCarbideGrades(const std::string& application = "");
    static std::vector<std::string> getCoatingTypes();
    
    // Standard cutting data
    static CuttingData getRecommendedCuttingData(const std::string& insertCode,
                                               const std::string& workpieceMaterial,
                                               const std::string& operation);
    
    // Validation functions for Phase 2
    static bool validateThreadingInsert(const ThreadingInsert& insert);
    static bool validateGroovingInsert(const GroovingInsert& insert);
    static bool validateBoringInsert(const std::string& insertCode, double boringDiameter);
    static bool isThreadingInsertCode(const std::string& isoCode);
    static bool isGroovingInsertCode(const std::string& isoCode);
    static bool isBoringInsertCode(const std::string& isoCode);
    
    // Thread specification helpers
    static double getThreadPitchFromCode(const std::string& threadingInsertCode);
    static ThreadProfile getThreadProfileFromCode(const std::string& threadingInsertCode);
    static std::vector<double> getSupportedThreadPitches(const std::string& threadingInsertCode);
    
    // Grooving specification helpers
    static double getGrooveWidthFromCode(const std::string& groovingInsertCode);
    static double getMaxGroovingDepth(const std::string& groovingInsertCode);
    static bool isGrooveWidthCompatible(const std::string& insertCode, double requiredWidth);
    
    // Boring specification helpers
    static double getMinBoringDiameter(const std::string& boringInsertCode);
    static double getMaxBoringDiameter(const std::string& boringInsertCode);
    static std::vector<std::string> getBoringBarsForInsert(const std::string& insertCode);
    
    // ========================================================================
    // Phase 3: Holder System Enhancement
    // ========================================================================
    
    // Holder validation and specification functions
    static bool validateToolHolder(const ToolHolder& holder);
    static bool validateHolderInsertCompatibility(const std::string& holderCode, const std::string& insertCode);
    static std::vector<std::string> getSupportedInsertShapes(ClampingStyle clampingStyle);
    static std::vector<std::string> getHoldersForInsertShape(InsertShape shape);
    
    // Holder geometry calculations
    static double calculateHolderApproachAngle(const ToolHolder& holder);
    static double calculateMaxInsertSize(const ToolHolder& holder);
    static double calculateToolOverhang(const ToolHolder& holder);
    static bool isHolderOrientationValid(HandOrientation hand, const std::string& operation);
    
    // Clamping style specifications
    static std::string getClampingStyleDescription(ClampingStyle clampingStyle);
    static std::vector<std::string> getClampingStyleRequirements(ClampingStyle clampingStyle);
    static bool isClampingStyleCompatibleWithInsert(ClampingStyle clampingStyle, InsertShape insertShape);
    
    // Detailed holder database access
    static std::vector<ToolHolder> getAllHolders();
    static std::vector<ToolHolder> getHoldersByType(ClampingStyle clampingStyle, HandOrientation handOrientation);
    static ToolHolder getHolderByCode(const std::string& holderCode);
    static std::vector<std::string> getHolderVariants(const std::string& baseHolderCode);
    
    // Holder-insert interaction calculations
    static double calculateInsertSetbackFromNose(const ToolHolder& holder, const std::string& insertCode);
    static double calculateEffectiveCuttingAngle(const ToolHolder& holder, const std::string& insertCode);
    static std::vector<double> getHolderDimensionalConstraints(const std::string& holderCode);
    
    // Orientation-specific calculations
    static std::string getOrientationSpecificCode(const std::string& baseCode, HandOrientation orientation);
    static bool isOrientationApplicableForOperation(HandOrientation orientation, const std::string& operation);
    static std::string getMirroredHolderCode(const std::string& holderCode);
    
    // Advanced holder compatibility checking
    static bool checkHolderInsertPhysicalFit(const ToolHolder& holder, const std::string& insertCode);
    static bool checkHolderMachineClearance(const ToolHolder& holder, double spindleSize, double chuckSize);
    static std::vector<std::string> getIncompatibilityReasons(const std::string& holderCode, const std::string& insertCode);
    
private:
    static void initializeDatabase();
    static std::map<std::string, ISOInsertSize> s_insertDatabase;
    static std::map<std::string, std::vector<std::string>> s_holderCompatibility;
    static bool s_databaseInitialized;
    
    // Phase 3: Enhanced holder database
    static std::map<std::string, ToolHolder> s_holderDatabase;
    static std::map<ClampingStyle, std::vector<InsertShape>> s_clampingCompatibility;
    static std::map<std::string, std::string> s_holderDescriptions;
};

} // namespace Toolpath
} // namespace IntuiCAM

#endif // INTUICAM_TOOLPATH_TOOLTYPES_H 