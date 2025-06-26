#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>

namespace IntuiCAM {
namespace GUI {

enum class ToolType {
    TurningInsert,
    FacingTool,
    PartingTool,
    BoringBar,
    ThreadingTool,
    FormTool,
    Custom
};

enum class ToolMaterial {
    HighSpeedSteel,    // HSS
    Carbide,           // Uncoated carbide
    CoatedCarbide,     // TiN, TiAlN, etc.
    Ceramic,           // Al2O3, Si3N4
    CBN,               // Cubic Boron Nitride
    PCD,               // Polycrystalline Diamond
    Diamond            // Single crystal diamond
};

enum class InsertGeometry {
    Square,            // S - 90° corners
    Triangle,          // T - 60° corners
    Diamond_80,        // D - 80° diamond
    Diamond_55,        // C - 55° diamond
    Round,             // R - round insert
    Hexagon,           // H - hexagon
    Octagon,           // O - octagon
    Rhombic,           // V - rhombic
    Custom
};

struct ToolCapabilities {
    QStringList supportedOperations;   // "facing", "roughing", "finishing", "parting", etc.
    QStringList suitableMaterials;     // Compatible workpiece materials
    double minDiameter;                // mm
    double maxDiameter;                // mm
    double maxDepthOfCut;              // mm
    double maxFeedRate;                // mm/rev
    double maxSurfaceSpeed;            // m/min
    bool supportsClimbMilling;
    bool supportsConventionalMilling;
    QString coolantRequirement;        // "None", "Mist", "Flood", "Required"
};

struct ToolGeometry {
    double diameter;                   // mm
    double length;                     // mm
    double insertSize;                 // mm (IC)
    double cornerRadius;               // mm
    double cuttingEdgeAngle;          // degrees
    double reliefAngle;               // degrees
    double rakeAngle;                 // degrees
    InsertGeometry insertShape;
    QString coating;                   // "None", "TiN", "TiAlN", "TiCN", etc.
};

struct CuttingTool {
    QString id;                        // Unique identifier
    QString name;                      // Display name
    QString manufacturer;              // Tool manufacturer
    QString partNumber;                // Manufacturer part number
    QString description;               // Tool description
    ToolType type;
    ToolMaterial material;
    ToolGeometry geometry;
    ToolCapabilities capabilities;
    double cost;                       // Tool cost in local currency
    QString notes;                     // User notes
    bool isActive;                     // Tool availability
    bool isCustom;                     // User-defined tool
    
    CuttingTool()
        : type(ToolType::TurningInsert), material(ToolMaterial::Carbide)
        , cost(0.0), isActive(true), isCustom(false) {}
};

struct ToolRecommendation {
    QString toolId;
    QString reason;                    // Why this tool was recommended
    double suitabilityScore;          // 0.0 to 1.0
    bool isPrimary;                   // Primary vs alternative recommendation
};

class ToolManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolManager(QObject *parent = nullptr);
    ~ToolManager();

    // Tool database access
    QStringList getAllToolIds() const;
    QStringList getToolsByType(ToolType type) const;
    QStringList getToolsByMaterial(ToolMaterial material) const;
    CuttingTool getTool(const QString& toolId) const;
    bool hasTool(const QString& toolId) const;

    // Tool type and material utilities
    QString getToolTypeDisplayName(ToolType type) const;
    QString getToolMaterialDisplayName(ToolMaterial material) const;
    QString getInsertGeometryDisplayName(InsertGeometry geometry) const;
    static ToolType stringToToolType(const QString& typeStr);
    static ToolMaterial stringToToolMaterial(const QString& materialStr);
    static InsertGeometry stringToInsertGeometry(const QString& geometryStr);
    static QString toolTypeToString(ToolType type);
    static QString toolMaterialToString(ToolMaterial material);
    static QString insertGeometryToString(InsertGeometry geometry);

    // Tool management
    bool addTool(const CuttingTool& tool);
    bool updateTool(const QString& toolId, const CuttingTool& tool);
    bool removeTool(const QString& toolId);
    bool setToolActive(const QString& toolId, bool active);

    // Material-specific tool filtering
    QStringList getToolsWithEnabledMaterial(const QString& materialName) const;
    QStringList getToolsEnabledForAnyMaterial() const;
    bool isToolEnabledForMaterial(const QString& toolId, const QString& materialName) const;
    QStringList getEnabledMaterialsForTool(const QString& toolId) const;

    // Tool recommendations
    QList<ToolRecommendation> recommendTools(
        const QString& operation,           // "facing", "roughing", "finishing", "parting"
        const QString& workpieceMaterial,   // Material name
        double workpieceDiameter,           // mm
        double requiredSurfaceFinish = 8.0, // μm Ra
        bool preferHighPerformance = false  // Prefer expensive/high-performance tools
    ) const;

    CuttingTool getBestTool(
        const QString& operation,
        const QString& workpieceMaterial,
        double workpieceDiameter,
        double requiredSurfaceFinish = 8.0
    ) const;

    // Tool validation
    bool validateToolForOperation(const QString& toolId, const QString& operation) const;
    bool validateToolForMaterial(const QString& toolId, const QString& material) const;
    bool validateToolCapabilities(const QString& toolId, double diameter, const QString& operation) const;

    // Tool library management
    QStringList getToolLibraries() const;
    bool createToolLibrary(const QString& libraryName);
    bool deleteToolLibrary(const QString& libraryName);
    bool exportToolLibrary(const QString& libraryName, const QString& filePath) const;
    bool importToolLibrary(const QString& filePath, const QString& libraryName = QString());

    // Tool performance analysis
    struct ToolPerformanceData {
        QString toolId;
        int usageCount;
        double averageToolLife;        // minutes
        double averageSurfaceFinish;   // μm Ra achieved
        double averageRemovalRate;     // cm³/min
        QString notes;
    };

    ToolPerformanceData getToolPerformance(const QString& toolId) const;
    void recordToolUsage(const QString& toolId, double toolLife, double surfaceFinish, double removalRate);

    // Data persistence
    bool loadToolDatabase();
    bool saveToolDatabase();

signals:
    void toolAdded(const QString& toolId);
    void toolUpdated(const QString& toolId);
    void toolRemoved(const QString& toolId);
    void toolActiveChanged(const QString& toolId, bool active);
    void databaseLoaded();
    void databaseError(const QString& error);

private slots:
    void onDatabaseChanged();

private:
    void initializeDefaultTools();
    void setupTurningInserts();
    void setupFacingTools();
    void setupPartingTools();
    void setupBoringBars();
    
    CuttingTool createTool(
        const QString& id,
        const QString& name,
        const QString& manufacturer,
        const QString& partNumber,
        ToolType type,
        ToolMaterial material,
        const ToolGeometry& geometry,
        const ToolCapabilities& capabilities,
        const QString& description,
        double cost = 0.0
    ) const;

    ToolCapabilities createCapabilities(
        const QStringList& operations,
        const QStringList& materials,
        double minDia, double maxDia, double maxDoc, double maxFeed, double maxSpeed,
        bool climb, bool conventional,
        const QString& coolant
    ) const;

    ToolGeometry createGeometry(
        double diameter, double length, double insertSize, double cornerRadius,
        double cuttingAngle, double reliefAngle, double rakeAngle,
        InsertGeometry shape, const QString& coating
    ) const;

    double calculateSuitabilityScore(
        const CuttingTool& tool,
        const QString& operation,
        const QString& workpieceMaterial,
        double workpieceDiameter,
        double surfaceFinishTarget
    ) const;

    QString getDatabaseFilePath() const;
    QJsonObject toolToJson(const CuttingTool& tool) const;
    CuttingTool toolFromJson(const QJsonObject& json) const;
    QJsonObject capabilitiesToJson(const ToolCapabilities& caps) const;
    ToolCapabilities capabilitiesFromJson(const QJsonObject& json) const;
    QJsonObject geometryToJson(const ToolGeometry& geom) const;
    ToolGeometry geometryFromJson(const QJsonObject& json) const;

    // Tool database
    QMap<QString, CuttingTool> m_tools;
    QMap<ToolType, QStringList> m_toolsByType;
    QMap<ToolMaterial, QStringList> m_toolsByMaterial;
    QMap<QString, ToolPerformanceData> m_performanceData;
    QString m_databasePath;
    bool m_databaseLoaded;

    // Operation requirements (used for tool recommendations)
    static const QMap<QString, double> OPERATION_PRECISION_REQUIREMENTS;
    static const QMap<QString, double> OPERATION_SURFACE_FINISH_TARGETS;
    static const QMap<QString, QStringList> OPERATION_PREFERRED_TOOL_TYPES;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // TOOLMANAGER_H 