#include "toolmanager.h"
#include <QDebug>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

namespace IntuiCAM {
namespace GUI {

// Static member definitions
const QMap<QString, double> ToolManager::OPERATION_PRECISION_REQUIREMENTS = {
    {"facing", 0.1},      // mm tolerance
    {"roughing", 0.5},    // mm tolerance
    {"finishing", 0.02},  // mm tolerance
    {"parting", 0.05}     // mm tolerance
};

const QMap<QString, double> ToolManager::OPERATION_SURFACE_FINISH_TARGETS = {
    {"facing", 16.0},     // μm Ra
    {"roughing", 32.0},   // μm Ra
    {"finishing", 4.0},   // μm Ra
    {"parting", 8.0}      // μm Ra
};

const QMap<QString, QStringList> ToolManager::OPERATION_PREFERRED_TOOL_TYPES = {
    {"facing", {"FacingTool", "TurningInsert"}},
    {"roughing", {"TurningInsert"}},
    {"finishing", {"TurningInsert", "FacingTool"}},
    {"parting", {"PartingTool"}}
};

ToolManager::ToolManager(QObject *parent)
    : QObject(parent), m_databaseLoaded(false)
{
    m_databasePath = getDatabaseFilePath();
    
    // Try to load existing database, create default if none exists
    if (!loadToolDatabase()) {
        qDebug() << "Creating default tool database...";
        initializeDefaultTools();
        saveToolDatabase();
    }
    
    connect(this, &ToolManager::toolAdded, this, &ToolManager::onDatabaseChanged);
    connect(this, &ToolManager::toolUpdated, this, &ToolManager::onDatabaseChanged);
    connect(this, &ToolManager::toolRemoved, this, &ToolManager::onDatabaseChanged);
}

ToolManager::~ToolManager()
{
    if (m_databaseLoaded) {
        saveToolDatabase();
    }
}

QStringList ToolManager::getAllToolIds() const
{
    return m_tools.keys();
}

QStringList ToolManager::getToolsByType(ToolType type) const
{
    return m_toolsByType.value(type, QStringList());
}

CuttingTool ToolManager::getTool(const QString& toolId) const
{
    return m_tools.value(toolId, CuttingTool());
}

bool ToolManager::hasTool(const QString& toolId) const
{
    return m_tools.contains(toolId);
}

QString ToolManager::getToolTypeDisplayName(ToolType type) const
{
    switch (type) {
        case ToolType::TurningInsert: return "Turning Insert";
        case ToolType::FacingTool: return "Facing Tool";
        case ToolType::PartingTool: return "Parting Tool";
        case ToolType::BoringBar: return "Boring Bar";
        case ToolType::ThreadingTool: return "Threading Tool";
        case ToolType::FormTool: return "Form Tool";
        default: return "Custom Tool";
    }
}

QString ToolManager::getToolMaterialDisplayName(ToolMaterial material) const
{
    switch (material) {
        case ToolMaterial::HighSpeedSteel: return "High Speed Steel";
        case ToolMaterial::Carbide: return "Carbide";
        case ToolMaterial::CoatedCarbide: return "Coated Carbide";
        case ToolMaterial::Ceramic: return "Ceramic";
        case ToolMaterial::CBN: return "CBN";
        case ToolMaterial::PCD: return "PCD";
        default: return "Diamond";
    }
}

QList<ToolRecommendation> ToolManager::recommendTools(
    const QString& operation,
    const QString& workpieceMaterial,
    double workpieceDiameter,
    double requiredSurfaceFinish,
    bool preferHighPerformance) const
{
    QList<ToolRecommendation> recommendations;
    QStringList preferredTypes = OPERATION_PREFERRED_TOOL_TYPES.value(operation);
    
    // Score all available tools
    QList<QPair<double, QString>> scoredTools;
    for (auto it = m_tools.constBegin(); it != m_tools.constEnd(); ++it) {
        const CuttingTool& tool = it.value();
        if (!tool.isActive) continue;
        
        double score = calculateSuitabilityScore(tool, operation, workpieceMaterial, 
                                                workpieceDiameter, requiredSurfaceFinish);
        if (score > 0.1) { // Only include reasonably suitable tools
            scoredTools.append(qMakePair(score, it.key()));
        }
    }
    
    // Sort by score (highest first)
    std::sort(scoredTools.begin(), scoredTools.end(), 
              [](const QPair<double, QString>& a, const QPair<double, QString>& b) {
                  return a.first > b.first;
              });
    
    // Create recommendations
    int primaryCount = 0;
    for (const auto& scoredTool : scoredTools) {
        ToolRecommendation rec;
        rec.toolId = scoredTool.second;
        rec.suitabilityScore = scoredTool.first;
        rec.isPrimary = (primaryCount < 3); // Top 3 as primary recommendations
        
        const CuttingTool& tool = m_tools[rec.toolId];
        rec.reason = QString("Score: %1 - %2").arg(rec.suitabilityScore, 0, 'f', 2)
                                               .arg(tool.description);
        
        recommendations.append(rec);
        if (rec.isPrimary) primaryCount++;
        
        if (recommendations.size() >= 10) break; // Limit to top 10 recommendations
    }
    
    return recommendations;
}

CuttingTool ToolManager::getBestTool(
    const QString& operation,
    const QString& workpieceMaterial,
    double workpieceDiameter,
    double requiredSurfaceFinish) const
{
    auto recommendations = recommendTools(operation, workpieceMaterial, 
                                        workpieceDiameter, requiredSurfaceFinish);
    
    if (!recommendations.isEmpty()) {
        return getTool(recommendations.first().toolId);
    }
    
    return CuttingTool(); // Return empty tool if no recommendations
}

bool ToolManager::validateToolForOperation(const QString& toolId, const QString& operation) const
{
    if (!m_tools.contains(toolId)) return false;
    
    const CuttingTool& tool = m_tools[toolId];
    return tool.capabilities.supportedOperations.contains(operation);
}

void ToolManager::initializeDefaultTools()
{
    m_tools.clear();
    m_toolsByType.clear();
    m_toolsByMaterial.clear();
    
    setupTurningInserts();
    setupFacingTools();
    setupPartingTools();
    setupBoringBars();
    
    qDebug() << "Initialized" << m_tools.size() << "default tools";
}

void ToolManager::setupTurningInserts()
{
    // Basic carbide turning insert
    ToolGeometry basicGeom = createGeometry(12.7, 4.76, 12.7, 0.4, 80, 7, 0, 
                                           InsertGeometry::Diamond_80, "None");
    ToolCapabilities basicCaps = createCapabilities(
        {"roughing", "finishing"}, {"AL6061", "STEEL1018", "BRASS360"},
        10, 100, 3.0, 0.3, 200, true, true, "Mist");
    
    CuttingTool basicInsert = createTool("CNMG120408", "CNMG 12 04 08 Carbide Insert",
                                        "Generic", "CNMG120408", ToolType::TurningInsert,
                                        ToolMaterial::Carbide, basicGeom, basicCaps,
                                        "General purpose carbide turning insert", 15.0);
    
    m_tools["CNMG120408"] = basicInsert;
    m_toolsByType[ToolType::TurningInsert].append("CNMG120408");
    m_toolsByMaterial[ToolMaterial::Carbide].append("CNMG120408");
}

void ToolManager::setupFacingTools()
{
    ToolGeometry faceGeom = createGeometry(25, 150, 16, 0.8, 90, 5, 5,
                                          InsertGeometry::Square, "TiAlN");
    ToolCapabilities faceCaps = createCapabilities(
        {"facing", "roughing"}, {"AL6061", "STEEL1018", "SS304"},
        20, 200, 2.5, 0.25, 250, true, true, "Flood");
    
    CuttingTool faceTool = createTool("FACE001", "Face Milling Cutter 25mm",
                                     "Generic", "FACE001", ToolType::FacingTool,
                                     ToolMaterial::CoatedCarbide, faceGeom, faceCaps,
                                     "25mm face milling cutter with TiAlN coating", 85.0);
    
    m_tools["FACE001"] = faceTool;
    m_toolsByType[ToolType::FacingTool].append("FACE001");
    m_toolsByMaterial[ToolMaterial::CoatedCarbide].append("FACE001");
}

void ToolManager::setupPartingTools()
{
    ToolGeometry partGeom = createGeometry(3, 100, 3, 0, 90, 5, 0,
                                          InsertGeometry::Square, "None");
    ToolCapabilities partCaps = createCapabilities(
        {"parting"}, {"AL6061", "STEEL1018", "BRASS360"},
        5, 50, 0.5, 0.05, 80, false, true, "Flood");
    
    CuttingTool partTool = createTool("PART001", "Parting Tool 3mm",
                                     "Generic", "PART001", ToolType::PartingTool,
                                     ToolMaterial::Carbide, partGeom, partCaps,
                                     "3mm carbide parting tool", 25.0);
    
    m_tools["PART001"] = partTool;
    m_toolsByType[ToolType::PartingTool].append("PART001");
    m_toolsByMaterial[ToolMaterial::Carbide].append("PART001");
}

void ToolManager::setupBoringBars()
{
    ToolGeometry boreGeom = createGeometry(12, 150, 12, 0.2, 93, 7, -5,
                                          InsertGeometry::Diamond_55, "TiN");
    ToolCapabilities boreCaps = createCapabilities(
        {"roughing", "finishing"}, {"AL6061", "STEEL1018", "SS304"},
        15, 80, 2.0, 0.2, 150, true, true, "Mist");
    
    CuttingTool boreTool = createTool("BORE001", "Boring Bar 12mm",
                                     "Generic", "BORE001", ToolType::BoringBar,
                                     ToolMaterial::CoatedCarbide, boreGeom, boreCaps,
                                     "12mm boring bar with TiN coated insert", 65.0);
    
    m_tools["BORE001"] = boreTool;
    m_toolsByType[ToolType::BoringBar].append("BORE001");
    m_toolsByMaterial[ToolMaterial::CoatedCarbide].append("BORE001");
}

CuttingTool ToolManager::createTool(
    const QString& id, const QString& name, const QString& manufacturer,
    const QString& partNumber, ToolType type, ToolMaterial material,
    const ToolGeometry& geometry, const ToolCapabilities& capabilities,
    const QString& description, double cost) const
{
    CuttingTool tool;
    tool.id = id;
    tool.name = name;
    tool.manufacturer = manufacturer;
    tool.partNumber = partNumber;
    tool.type = type;
    tool.material = material;
    tool.geometry = geometry;
    tool.capabilities = capabilities;
    tool.description = description;
    tool.cost = cost;
    tool.isActive = true;
    tool.isCustom = false;
    return tool;
}

ToolCapabilities ToolManager::createCapabilities(
    const QStringList& operations, const QStringList& materials,
    double minDia, double maxDia, double maxDoc, double maxFeed, double maxSpeed,
    bool climb, bool conventional, const QString& coolant) const
{
    ToolCapabilities caps;
    caps.supportedOperations = operations;
    caps.suitableMaterials = materials;
    caps.minDiameter = minDia;
    caps.maxDiameter = maxDia;
    caps.maxDepthOfCut = maxDoc;
    caps.maxFeedRate = maxFeed;
    caps.maxSurfaceSpeed = maxSpeed;
    caps.supportsClimbMilling = climb;
    caps.supportsConventionalMilling = conventional;
    caps.coolantRequirement = coolant;
    return caps;
}

ToolGeometry ToolManager::createGeometry(
    double diameter, double length, double insertSize, double cornerRadius,
    double cuttingAngle, double reliefAngle, double rakeAngle,
    InsertGeometry shape, const QString& coating) const
{
    ToolGeometry geom;
    geom.diameter = diameter;
    geom.length = length;
    geom.insertSize = insertSize;
    geom.cornerRadius = cornerRadius;
    geom.cuttingEdgeAngle = cuttingAngle;
    geom.reliefAngle = reliefAngle;
    geom.rakeAngle = rakeAngle;
    geom.insertShape = shape;
    geom.coating = coating;
    return geom;
}

double ToolManager::calculateSuitabilityScore(
    const CuttingTool& tool, const QString& operation,
    const QString& workpieceMaterial, double workpieceDiameter,
    double surfaceFinishTarget) const
{
    double score = 0.0;
    
    // Check operation compatibility (30% of score)
    if (tool.capabilities.supportedOperations.contains(operation)) {
        score += 0.3;
    } else {
        return 0.0; // Tool not suitable for operation
    }
    
    // Check material compatibility (25% of score)
    if (tool.capabilities.suitableMaterials.contains(workpieceMaterial)) {
        score += 0.25;
    } else if (workpieceMaterial.contains("AL") && 
               tool.capabilities.suitableMaterials.contains("AL6061")) {
        score += 0.15; // Partial compatibility for aluminum family
    }
    
    // Check diameter range (20% of score)
    if (workpieceDiameter >= tool.capabilities.minDiameter && 
        workpieceDiameter <= tool.capabilities.maxDiameter) {
        score += 0.2;
    } else if (workpieceDiameter < tool.capabilities.minDiameter) {
        score += 0.1; // Can still work but not optimal
    }
    
    // Check surface finish capability (15% of score)
    double targetFinish = OPERATION_SURFACE_FINISH_TARGETS.value(operation, 16.0);
    if (surfaceFinishTarget <= targetFinish) {
        score += 0.15;
    } else {
        score += 0.05; // Tool may struggle with required finish
    }
    
    // Tool material bonus (10% of score)
    if (tool.material == ToolMaterial::CoatedCarbide || 
        tool.material == ToolMaterial::CBN) {
        score += 0.1;
    } else if (tool.material == ToolMaterial::Carbide) {
        score += 0.08;
    } else {
        score += 0.05;
    }
    
    return qMin(score, 1.0);
}

QString ToolManager::getDatabaseFilePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("tools.json");
}

bool ToolManager::loadToolDatabase()
{
    QFile file(m_databasePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    // Parse and load tools (simplified for space)
    m_databaseLoaded = true;
    emit databaseLoaded();
    return true;
}

bool ToolManager::saveToolDatabase()
{
    // Simplified save implementation
    QDir dir = QFileInfo(m_databasePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject root;
    root["version"] = "1.0";
    
    QJsonDocument doc(root);
    QFile file(m_databasePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    return true;
}

void ToolManager::onDatabaseChanged()
{
    saveToolDatabase();
}

// Static method implementations
QString ToolManager::toolTypeToString(ToolType type)
{
    switch (type) {
        case ToolType::TurningInsert: return "TurningInsert";
        case ToolType::FacingTool: return "FacingTool";
        case ToolType::PartingTool: return "PartingTool";
        case ToolType::BoringBar: return "BoringBar";
        case ToolType::ThreadingTool: return "ThreadingTool";
        case ToolType::FormTool: return "FormTool";
        default: return "Custom";
    }
}

ToolType ToolManager::stringToToolType(const QString& typeStr)
{
    if (typeStr == "TurningInsert") return ToolType::TurningInsert;
    if (typeStr == "FacingTool") return ToolType::FacingTool;
    if (typeStr == "PartingTool") return ToolType::PartingTool;
    if (typeStr == "BoringBar") return ToolType::BoringBar;
    if (typeStr == "ThreadingTool") return ToolType::ThreadingTool;
    if (typeStr == "FormTool") return ToolType::FormTool;
    return ToolType::Custom;
}

} // namespace GUI
} // namespace IntuiCAM 