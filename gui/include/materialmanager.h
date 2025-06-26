#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

namespace IntuiCAM {
namespace GUI {

struct MaterialProperties {
    QString name;
    QString displayName;
    QString category;           // e.g., "Aluminum", "Steel", "Stainless Steel", "Brass", "Titanium", "Plastic"
    double density;             // kg/m³
    double thermalConductivity; // W/m·K
    double specificHeat;        // J/kg·K
    double yieldStrength;       // MPa
    double ultimateStrength;    // MPa
    double hardnessBHN;         // Brinell Hardness Number
    QString description;
    bool isCustom;

    // Machining properties
    double recommendedSurfaceSpeed;    // m/min (for 1mm diameter tool)
    double recommendedFeedRate;        // mm/rev (for 1mm diameter tool)
    double maxDepthOfCut;             // mm (maximum recommended)
    double machinabilityRating;       // 1.0 = reference material (1018 steel)
    
    MaterialProperties()
        : density(0.0), thermalConductivity(0.0), specificHeat(0.0)
        , yieldStrength(0.0), ultimateStrength(0.0), hardnessBHN(0.0)
        , recommendedSurfaceSpeed(0.0), recommendedFeedRate(0.0)
        , maxDepthOfCut(0.0), machinabilityRating(1.0), isCustom(false) {}
};

struct CuttingParameters {
    double surfaceSpeed;      // m/min
    double spindleSpeed;      // RPM
    double feedRate;          // mm/rev
    double depthOfCut;        // mm
    double stepover;          // mm
    bool climbMilling;        // true for climb, false for conventional
    bool useConstantSurfaceSpeed; // true for CSS mode, false for constant RPM
    QString coolantType;      // "None", "Flood", "Mist", "Air"
    
    CuttingParameters()
        : surfaceSpeed(0.0), spindleSpeed(0.0), feedRate(0.0)
        , depthOfCut(0.0), stepover(0.0), climbMilling(true)
        , useConstantSurfaceSpeed(false), coolantType("None") {}
};

enum class MaterialCategory {
    Aluminum,
    Steel,
    StainlessSteel,
    Brass,
    Bronze,
    Titanium,
    Plastic,
    Composite,
    Custom,
    Unknown
};

class MaterialManager : public QObject
{
    Q_OBJECT

public:
    explicit MaterialManager(QObject *parent = nullptr);
    ~MaterialManager();

    // Material database access
    QStringList getAllMaterialNames() const;
    QStringList getMaterialsByCategory(MaterialCategory category) const;
    MaterialProperties getMaterialProperties(const QString& materialName) const;
    bool hasMaterial(const QString& materialName) const;
    
    // Material categories
    MaterialCategory getMaterialCategory(const QString& materialName) const;
    QString getCategoryDisplayName(MaterialCategory category) const;
    static MaterialCategory stringToCategory(const QString& categoryStr);
    static QString categoryToString(MaterialCategory category);

    // Custom materials
    bool addCustomMaterial(const MaterialProperties& properties);
    bool updateCustomMaterial(const QString& materialName, const MaterialProperties& properties);
    bool removeCustomMaterial(const QString& materialName);
    QStringList getCustomMaterialNames() const;

    // Cutting parameter calculation
    CuttingParameters calculateCuttingParameters(
        const QString& materialName,
        double toolDiameter,           // mm
        const QString& operation,      // "facing", "roughing", "finishing", "parting"
        double surfaceFinishTarget = 8.0  // μm Ra
    ) const;

    // Surface speed and spindle speed calculations
    double calculateSpindleSpeed(double surfaceSpeed, double toolDiameter) const;
    double calculateSurfaceSpeed(double spindleSpeed, double toolDiameter) const;

    // Material validation and recommendations
    bool validateMaterialForOperation(const QString& materialName, const QString& operation) const;
    QStringList getRecommendedMaterials(const QString& operation) const;
    QString getMaterialRecommendations(const QString& materialName) const;

    // Data persistence
    bool loadMaterialDatabase();
    bool saveMaterialDatabase();
    bool exportMaterials(const QString& filePath, const QStringList& materialNames = QStringList()) const;
    bool importMaterials(const QString& filePath);

signals:
    void materialAdded(const QString& materialName);
    void materialUpdated(const QString& materialName);
    void materialRemoved(const QString& materialName);
    void databaseLoaded();
    void databaseError(const QString& error);

private slots:
    void onDatabaseChanged();

private:
    void initializeDefaultMaterials();
    void setupAluminumMaterials();
    void setupSteelMaterials();
    void setupStainlessSteelMaterials();
    void setupBrassMaterials();
    void setupTitaniumMaterials();
    void setupPlasticMaterials();
    
    MaterialProperties createMaterial(
        const QString& name,
        const QString& displayName,
        const QString& category,
        double density,
        double thermalConductivity,
        double specificHeat,
        double yieldStrength,
        double ultimateStrength,
        double hardnessBHN,
        double recommendedSurfaceSpeed,
        double recommendedFeedRate,
        double maxDepthOfCut,
        double machinabilityRating,
        const QString& description
    ) const;

    QString getDatabaseFilePath() const;
    QJsonObject materialPropertiesToJson(const MaterialProperties& props) const;
    MaterialProperties materialPropertiesFromJson(const QJsonObject& json) const;

    // Material database
    QMap<QString, MaterialProperties> m_materials;
    QMap<MaterialCategory, QStringList> m_categorizedMaterials;
    QString m_databasePath;
    bool m_databaseLoaded;

    // Operation-specific multipliers for cutting parameters
    static const QMap<QString, double> OPERATION_SURFACE_SPEED_MULTIPLIERS;
    static const QMap<QString, double> OPERATION_FEED_RATE_MULTIPLIERS;
    static const QMap<QString, double> OPERATION_DEPTH_MULTIPLIERS;
};

} // namespace GUI
} // namespace IntuiCAM

#endif // MATERIALMANAGER_H 