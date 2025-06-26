#define _USE_MATH_DEFINES
#include "materialmanager.h"
#include <QDebug>
#include <QJsonArray>
#include <QCoreApplication>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace GUI {

// Static member definitions
const QMap<QString, double> MaterialManager::OPERATION_SURFACE_SPEED_MULTIPLIERS = {
    {"facing", 1.0},      // Base speed
    {"roughing", 1.2},    // 20% higher for roughing
    {"finishing", 0.8},   // 20% lower for finishing (better surface)
    {"parting", 0.6}      // 40% lower for parting (stability)
};

const QMap<QString, double> MaterialManager::OPERATION_FEED_RATE_MULTIPLIERS = {
    {"facing", 1.0},      // Base feed
    {"roughing", 1.5},    // 50% higher for material removal
    {"finishing", 0.4},   // 60% lower for surface finish
    {"parting", 0.3}      // 70% lower for parting (precision)
};

const QMap<QString, double> MaterialManager::OPERATION_DEPTH_MULTIPLIERS = {
    {"facing", 1.0},      // Base depth
    {"roughing", 1.0},    // Full depth for roughing
    {"finishing", 0.2},   // Light cuts for finishing
    {"parting", 0.5}      // Moderate depth for parting
};

MaterialManager::MaterialManager(QObject *parent)
    : QObject(parent)
    , m_databaseLoaded(false)
{
    m_databasePath = getDatabaseFilePath();
    
    // Try to load existing database, create default if none exists
    if (!loadMaterialDatabase()) {
        qDebug() << "Creating default material database...";
        initializeDefaultMaterials();
        saveMaterialDatabase();
    }
    
    connect(this, &MaterialManager::materialAdded, this, &MaterialManager::onDatabaseChanged);
    connect(this, &MaterialManager::materialUpdated, this, &MaterialManager::onDatabaseChanged);
    connect(this, &MaterialManager::materialRemoved, this, &MaterialManager::onDatabaseChanged);
}

MaterialManager::~MaterialManager()
{
    if (m_databaseLoaded) {
        saveMaterialDatabase();
    }
}

QStringList MaterialManager::getAllMaterialNames() const
{
    return m_materials.keys();
}

QStringList MaterialManager::getMaterialsByCategory(MaterialCategory category) const
{
    return m_categorizedMaterials.value(category, QStringList());
}

MaterialProperties MaterialManager::getMaterialProperties(const QString& materialName) const
{
    return m_materials.value(materialName, MaterialProperties());
}

bool MaterialManager::hasMaterial(const QString& materialName) const
{
    return m_materials.contains(materialName);
}

MaterialCategory MaterialManager::getMaterialCategory(const QString& materialName) const
{
    if (!m_materials.contains(materialName)) {
        return MaterialCategory::Unknown;
    }
    
    const QString categoryStr = m_materials[materialName].category;
    return stringToCategory(categoryStr);
}

QString MaterialManager::getCategoryDisplayName(MaterialCategory category) const
{
    switch (category) {
        case MaterialCategory::Aluminum: return "Aluminum Alloys";
        case MaterialCategory::Steel: return "Carbon Steel";
        case MaterialCategory::StainlessSteel: return "Stainless Steel";
        case MaterialCategory::Brass: return "Brass & Bronze";
        case MaterialCategory::Bronze: return "Bronze";
        case MaterialCategory::Titanium: return "Titanium Alloys";
        case MaterialCategory::Plastic: return "Engineering Plastics";
        case MaterialCategory::Composite: return "Composite Materials";
        case MaterialCategory::Custom: return "Custom Materials";
        default: return "Unknown";
    }
}

MaterialCategory MaterialManager::stringToCategory(const QString& categoryStr)
{
    if (categoryStr == "Aluminum") return MaterialCategory::Aluminum;
    if (categoryStr == "Steel") return MaterialCategory::Steel;
    if (categoryStr == "Stainless Steel") return MaterialCategory::StainlessSteel;
    if (categoryStr == "Brass") return MaterialCategory::Brass;
    if (categoryStr == "Bronze") return MaterialCategory::Bronze;
    if (categoryStr == "Titanium") return MaterialCategory::Titanium;
    if (categoryStr == "Plastic") return MaterialCategory::Plastic;
    if (categoryStr == "Composite") return MaterialCategory::Composite;
    if (categoryStr == "Custom") return MaterialCategory::Custom;
    return MaterialCategory::Unknown;
}

QString MaterialManager::categoryToString(MaterialCategory category)
{
    switch (category) {
        case MaterialCategory::Aluminum: return "Aluminum";
        case MaterialCategory::Steel: return "Steel";
        case MaterialCategory::StainlessSteel: return "Stainless Steel";
        case MaterialCategory::Brass: return "Brass";
        case MaterialCategory::Bronze: return "Bronze";
        case MaterialCategory::Titanium: return "Titanium";
        case MaterialCategory::Plastic: return "Plastic";
        case MaterialCategory::Composite: return "Composite";
        case MaterialCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

bool MaterialManager::addCustomMaterial(const MaterialProperties& properties)
{
    if (properties.name.isEmpty() || m_materials.contains(properties.name)) {
        return false;
    }
    
    MaterialProperties customProps = properties;
    customProps.isCustom = true;
    
    m_materials[properties.name] = customProps;
    
    MaterialCategory category = stringToCategory(properties.category);
    if (!m_categorizedMaterials[category].contains(properties.name)) {
        m_categorizedMaterials[category].append(properties.name);
    }
    
    emit materialAdded(properties.name);
    return true;
}

CuttingParameters MaterialManager::calculateCuttingParameters(
    const QString& materialName,
    double toolDiameter,
    const QString& operation,
    double surfaceFinishTarget) const
{
    CuttingParameters params;
    
    if (!m_materials.contains(materialName)) {
        qWarning() << "Material not found:" << materialName;
        return params;
    }
    
    const MaterialProperties& material = m_materials[materialName];
    
    // Base parameters from material database
    double baseSurfaceSpeed = material.recommendedSurfaceSpeed;
    double baseFeedRate = material.recommendedFeedRate;
    double baseDepthOfCut = material.maxDepthOfCut;
    
    // Apply operation-specific multipliers
    double speedMultiplier = OPERATION_SURFACE_SPEED_MULTIPLIERS.value(operation, 1.0);
    double feedMultiplier = OPERATION_FEED_RATE_MULTIPLIERS.value(operation, 1.0);
    double depthMultiplier = OPERATION_DEPTH_MULTIPLIERS.value(operation, 1.0);
    
    // Apply surface finish adjustments
    double finishMultiplier = 1.0;
    if (surfaceFinishTarget <= 2.0) {
        finishMultiplier = 0.6;  // Very fine finish - reduce speeds significantly
    } else if (surfaceFinishTarget <= 4.0) {
        finishMultiplier = 0.7;  // Fine finish
    } else if (surfaceFinishTarget <= 8.0) {
        finishMultiplier = 0.85; // Medium finish
    } else if (surfaceFinishTarget <= 16.0) {
        finishMultiplier = 1.0;  // Standard finish
    } else {
        finishMultiplier = 1.2;  // Rough finish - can go faster
    }
    
    // Calculate final parameters
    params.surfaceSpeed = baseSurfaceSpeed * speedMultiplier * finishMultiplier;
    params.spindleSpeed = calculateSpindleSpeed(params.surfaceSpeed, toolDiameter);
    params.feedRate = baseFeedRate * feedMultiplier * finishMultiplier;
    params.depthOfCut = baseDepthOfCut * depthMultiplier;
    params.stepover = toolDiameter * 0.6; // 60% stepover default
    
    // Operation-specific settings
    if (operation == "finishing") {
        params.climbMilling = true;
        params.coolantType = "Mist";
        params.stepover = toolDiameter * 0.3; // Finer stepover for finishing
    } else if (operation == "roughing") {
        params.climbMilling = true;
        params.coolantType = "Flood";
        params.stepover = toolDiameter * 0.8; // Aggressive stepover for roughing
    } else if (operation == "parting") {
        params.climbMilling = false; // Conventional for stability
        params.coolantType = "Flood";
        params.stepover = toolDiameter * 0.1; // Very fine for parting
    } else { // facing
        params.climbMilling = true;
        params.coolantType = material.thermalConductivity > 100 ? "Mist" : "Flood";
    }
    
    // Material-specific adjustments
    if (material.category == "Aluminum") {
        params.coolantType = "Mist"; // Aluminum doesn't need heavy cooling
        params.surfaceSpeed *= 1.5;  // Aluminum can run faster
    } else if (material.category == "Titanium") {
        params.surfaceSpeed *= 0.3;  // Titanium requires very slow speeds
        params.coolantType = "Flood";
        params.depthOfCut *= 0.5;    // Light cuts for titanium
    } else if (material.category == "Stainless Steel") {
        params.feedRate *= 1.2;      // Higher feed rates for stainless
        params.coolantType = "Flood";
    }
    
    return params;
}

double MaterialManager::calculateSpindleSpeed(double surfaceSpeed, double toolDiameter) const
{
    // Formula: RPM = (Surface Speed * 1000) / (π * Diameter)
    // Surface speed in m/min, diameter in mm
    if (toolDiameter <= 0.0) {
        return 0.0;
    }
    
    return (surfaceSpeed * 1000.0) / (M_PI * toolDiameter);
}

double MaterialManager::calculateSurfaceSpeed(double spindleSpeed, double toolDiameter) const
{
    // Formula: Surface Speed = (RPM * π * Diameter) / 1000
    return (spindleSpeed * M_PI * toolDiameter) / 1000.0;
}

QString MaterialManager::getMaterialRecommendations(const QString& materialName) const
{
    if (!m_materials.contains(materialName)) {
        return "Material not found in database.";
    }
    
    const MaterialProperties& material = m_materials[materialName];
    QString recommendations;
    
    recommendations += QString("Material: %1\n").arg(material.displayName);
    recommendations += QString("Category: %1\n").arg(material.category);
    recommendations += QString("Machinability Rating: %1/10\n\n").arg(material.machinabilityRating * 10, 0, 'f', 1);
    
    if (material.machinabilityRating >= 0.8) {
        recommendations += "✓ Excellent machinability - suitable for all operations\n";
    } else if (material.machinabilityRating >= 0.6) {
        recommendations += "✓ Good machinability - suitable for most operations\n";
    } else if (material.machinabilityRating >= 0.4) {
        recommendations += "⚠ Moderate machinability - use conservative parameters\n";
    } else {
        recommendations += "⚠ Difficult to machine - requires expertise and special tooling\n";
    }
    
    return recommendations;
}

bool MaterialManager::loadMaterialDatabase()
{
    QFile file(m_databasePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open material database file:" << m_databasePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in material database:" << error.errorString();
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray materialsArray = root["materials"].toArray();
    
    m_materials.clear();
    m_categorizedMaterials.clear();
    
    for (const QJsonValue& value : materialsArray) {
        QJsonObject materialObj = value.toObject();
        MaterialProperties props = materialPropertiesFromJson(materialObj);
        
        if (!props.name.isEmpty()) {
            m_materials[props.name] = props;
            
            MaterialCategory category = stringToCategory(props.category);
            if (!m_categorizedMaterials[category].contains(props.name)) {
                m_categorizedMaterials[category].append(props.name);
            }
        }
    }
    
    m_databaseLoaded = true;
    emit databaseLoaded();
    qDebug() << "Loaded" << m_materials.size() << "materials from database";
    return true;
}

bool MaterialManager::saveMaterialDatabase()
{
    // Ensure directory exists
    QDir dir = QFileInfo(m_databasePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonArray materialsArray;
    for (auto it = m_materials.constBegin(); it != m_materials.constEnd(); ++it) {
        QJsonObject materialObj = materialPropertiesToJson(it.value());
        materialsArray.append(materialObj);
    }
    
    QJsonObject root;
    root["version"] = "1.0";
    root["materials"] = materialsArray;
    
    QJsonDocument doc(root);
    
    QFile file(m_databasePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write material database file:" << m_databasePath;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    qDebug() << "Saved" << m_materials.size() << "materials to database";
    return true;
}

void MaterialManager::onDatabaseChanged()
{
    // Auto-save when database changes
    saveMaterialDatabase();
}

void MaterialManager::initializeDefaultMaterials()
{
    m_materials.clear();
    m_categorizedMaterials.clear();
    
    setupAluminumMaterials();
    setupSteelMaterials();
    setupStainlessSteelMaterials();
    setupBrassMaterials();
    setupTitaniumMaterials();
    setupPlasticMaterials();
    
    qDebug() << "Initialized" << m_materials.size() << "default materials";
}

void MaterialManager::setupAluminumMaterials()
{
    // Aluminum 6061-T6
    MaterialProperties al6061 = createMaterial(
        "AL6061", "Aluminum 6061-T6", "Aluminum",
        2700, 167, 896, 276, 310, 95,
        300, 0.15, 3.0, 1.0,
        "General purpose aluminum alloy with good machinability"
    );
    m_materials["AL6061"] = al6061;
    m_categorizedMaterials[MaterialCategory::Aluminum].append("AL6061");
    
    // Aluminum 7075-T6
    MaterialProperties al7075 = createMaterial(
        "AL7075", "Aluminum 7075-T6", "Aluminum",
        2810, 130, 960, 503, 572, 150,
        250, 0.12, 2.5, 0.8,
        "High strength aluminum alloy, harder to machine"
    );
    m_materials["AL7075"] = al7075;
    m_categorizedMaterials[MaterialCategory::Aluminum].append("AL7075");
}

void MaterialManager::setupSteelMaterials()
{
    // Steel 1018
    MaterialProperties steel1018 = createMaterial(
        "STEEL1018", "Steel 1018 (Low Carbon)", "Steel",
        7870, 51.9, 486, 370, 440, 126,
        120, 0.20, 2.0, 1.0,
        "Reference material for machinability ratings"
    );
    m_materials["STEEL1018"] = steel1018;
    m_categorizedMaterials[MaterialCategory::Steel].append("STEEL1018");
    
    // Steel 4140
    MaterialProperties steel4140 = createMaterial(
        "STEEL4140", "Steel 4140 (Alloy Steel)", "Steel",
        7850, 42.6, 475, 655, 850, 302,
        80, 0.15, 1.5, 0.6,
        "Medium carbon alloy steel, heat treatable"
    );
    m_materials["STEEL4140"] = steel4140;
    m_categorizedMaterials[MaterialCategory::Steel].append("STEEL4140");
}

void MaterialManager::setupStainlessSteelMaterials()
{
    // Stainless Steel 304
    MaterialProperties ss304 = createMaterial(
        "SS304", "Stainless Steel 304", "Stainless Steel",
        8000, 16.2, 500, 205, 515, 201,
        100, 0.10, 1.2, 0.5,
        "Austenitic stainless steel, work hardens rapidly"
    );
    m_materials["SS304"] = ss304;
    m_categorizedMaterials[MaterialCategory::StainlessSteel].append("SS304");
    
    // Stainless Steel 316
    MaterialProperties ss316 = createMaterial(
        "SS316", "Stainless Steel 316", "Stainless Steel",
        8000, 16.2, 500, 205, 515, 217,
        90, 0.08, 1.0, 0.45,
        "Marine grade stainless steel with molybdenum"
    );
    m_materials["SS316"] = ss316;
    m_categorizedMaterials[MaterialCategory::StainlessSteel].append("SS316");
}

void MaterialManager::setupBrassMaterials()
{
    // Brass 360 (Free Machining Brass)
    MaterialProperties brass360 = createMaterial(
        "BRASS360", "Brass 360 (Free Machining)", "Brass",
        8500, 115, 380, 124, 310, 62,
        200, 0.20, 3.0, 1.5,
        "Free machining brass with excellent machinability"
    );
    m_materials["BRASS360"] = brass360;
    m_categorizedMaterials[MaterialCategory::Brass].append("BRASS360");
}

void MaterialManager::setupTitaniumMaterials()
{
    // Titanium Grade 2
    MaterialProperties ti_gr2 = createMaterial(
        "TI_GR2", "Titanium Grade 2", "Titanium",
        4500, 17.0, 523, 275, 345, 215,
        30, 0.05, 0.5, 0.2,
        "Commercially pure titanium, excellent corrosion resistance"
    );
    m_materials["TI_GR2"] = ti_gr2;
    m_categorizedMaterials[MaterialCategory::Titanium].append("TI_GR2");
}

void MaterialManager::setupPlasticMaterials()
{
    // ABS Plastic
    MaterialProperties abs = createMaterial(
        "ABS", "ABS Plastic", "Plastic",
        1050, 0.25, 1400, 41, 55, 0,
        500, 0.30, 5.0, 2.0,
        "Thermoplastic, easy to machine with sharp tools"
    );
    m_materials["ABS"] = abs;
    m_categorizedMaterials[MaterialCategory::Plastic].append("ABS");
    
    // Delrin (POM)
    MaterialProperties delrin = createMaterial(
        "DELRIN", "Delrin (POM)", "Plastic",
        1410, 0.31, 1460, 69, 89, 0,
        400, 0.25, 4.0, 1.8,
        "Excellent dimensional stability and machinability"
    );
    m_materials["DELRIN"] = delrin;
    m_categorizedMaterials[MaterialCategory::Plastic].append("DELRIN");
}

MaterialProperties MaterialManager::createMaterial(
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
    const QString& description) const
{
    MaterialProperties props;
    props.name = name;
    props.displayName = displayName;
    props.category = category;
    props.density = density;
    props.thermalConductivity = thermalConductivity;
    props.specificHeat = specificHeat;
    props.yieldStrength = yieldStrength;
    props.ultimateStrength = ultimateStrength;
    props.hardnessBHN = hardnessBHN;
    props.recommendedSurfaceSpeed = recommendedSurfaceSpeed;
    props.recommendedFeedRate = recommendedFeedRate;
    props.maxDepthOfCut = maxDepthOfCut;
    props.machinabilityRating = machinabilityRating;
    props.description = description;
    props.isCustom = false;
    
    return props;
}

QString MaterialManager::getDatabaseFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("materials.json");
}

QJsonObject MaterialManager::materialPropertiesToJson(const MaterialProperties& props) const
{
    QJsonObject obj;
    obj["name"] = props.name;
    obj["displayName"] = props.displayName;
    obj["category"] = props.category;
    obj["density"] = props.density;
    obj["thermalConductivity"] = props.thermalConductivity;
    obj["specificHeat"] = props.specificHeat;
    obj["yieldStrength"] = props.yieldStrength;
    obj["ultimateStrength"] = props.ultimateStrength;
    obj["hardnessBHN"] = props.hardnessBHN;
    obj["recommendedSurfaceSpeed"] = props.recommendedSurfaceSpeed;
    obj["recommendedFeedRate"] = props.recommendedFeedRate;
    obj["maxDepthOfCut"] = props.maxDepthOfCut;
    obj["machinabilityRating"] = props.machinabilityRating;
    obj["description"] = props.description;
    obj["isCustom"] = props.isCustom;
    return obj;
}

MaterialProperties MaterialManager::materialPropertiesFromJson(const QJsonObject& json) const
{
    MaterialProperties props;
    props.name = json["name"].toString();
    props.displayName = json["displayName"].toString();
    props.category = json["category"].toString();
    props.density = json["density"].toDouble();
    props.thermalConductivity = json["thermalConductivity"].toDouble();
    props.specificHeat = json["specificHeat"].toDouble();
    props.yieldStrength = json["yieldStrength"].toDouble();
    props.ultimateStrength = json["ultimateStrength"].toDouble();
    props.hardnessBHN = json["hardnessBHN"].toDouble();
    props.recommendedSurfaceSpeed = json["recommendedSurfaceSpeed"].toDouble();
    props.recommendedFeedRate = json["recommendedFeedRate"].toDouble();
    props.maxDepthOfCut = json["maxDepthOfCut"].toDouble();
    props.machinabilityRating = json["machinabilityRating"].toDouble();
    props.description = json["description"].toString();
    props.isCustom = json["isCustom"].toBool();
    return props;
}

} // namespace GUI
} // namespace IntuiCAM 