#include "../include/toolpathmanager.h"

#include <QDebug>

// OpenCASCADE includes
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_Arrow.hxx>
#include <Prs3d_ArrowAspect.hxx>
#include <V3d_View.hxx>

ToolpathManager::ToolpathManager(QObject *parent)
    : QObject(parent)
{
}

ToolpathManager::~ToolpathManager()
{
    clearAllToolpaths();
}

void ToolpathManager::initialize(Handle(AIS_InteractiveContext) context)
{
    m_context = context;
    
    if (m_context.IsNull()) {
        emit errorOccurred("Failed to initialize ToolpathManager: AIS context is null");
    } else {
        qDebug() << "ToolpathManager initialized successfully";
    }
}

bool ToolpathManager::displayToolpath(const IntuiCAM::Toolpath::Toolpath& toolpath, const QString& name)
{
    if (m_context.IsNull()) {
        emit errorOccurred("Cannot display toolpath: AIS context not initialized");
        return false;
    }
    
    if (m_displayedToolpaths.contains(name)) {
        // If this toolpath name is already in use, remove the old one first
        removeToolpath(name);
    }
    
    // Create the toolpath shape
    TopoDS_Shape toolpathShape = createToolpathShape(toolpath);
    
    if (toolpathShape.IsNull()) {
        emit errorOccurred(QString("Failed to create toolpath shape for '%1'").arg(name));
        return false;
    }
    
    // Create AIS shape for visualization
    Handle(AIS_Shape) toolpathAIS = new AIS_Shape(toolpathShape);
    
    // Set display properties
    setToolpathDisplayProperties(toolpathAIS, false); // Default to cutting movement style
    
    // Display the toolpath
    m_context->Display(toolpathAIS, Standard_False);
    
    // Store the toolpath AIS object
    m_displayedToolpaths[name] = toolpathAIS;
    
    // Force view update
    m_context->UpdateCurrentViewer();
    
    emit toolpathDisplayed(name);
    qDebug() << "Toolpath displayed:" << name;
    
    return true;
}

int ToolpathManager::displayToolpaths(const std::vector<std::shared_ptr<IntuiCAM::Toolpath::Toolpath>>& toolpaths, 
                                    const QString& baseName)
{
    int displayedCount = 0;
    
    for (size_t i = 0; i < toolpaths.size(); ++i) {
        const auto& toolpath = toolpaths[i];
        if (toolpath) {
            QString name = QString("%1_%2").arg(baseName).arg(i + 1);
            if (displayToolpath(*toolpath, name)) {
                displayedCount++;
            }
        }
    }
    
    return displayedCount;
}

void ToolpathManager::removeToolpath(const QString& name)
{
    if (m_context.IsNull()) {
        return;
    }
    
    if (m_displayedToolpaths.contains(name)) {
        Handle(AIS_Shape) toolpathAIS = m_displayedToolpaths[name];
        
        // Erase from display
        if (!toolpathAIS.IsNull()) {
            m_context->Erase(toolpathAIS, Standard_False);
        }
        
        // Remove from map
        m_displayedToolpaths.remove(name);
        
        m_context->UpdateCurrentViewer();
        
        emit toolpathRemoved(name);
        qDebug() << "Toolpath removed:" << name;
    }
}

void ToolpathManager::clearAllToolpaths()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Get all keys to avoid modifying map during iteration
    QStringList keys = m_displayedToolpaths.keys();
    
    for (const QString& name : keys) {
        Handle(AIS_Shape) toolpathAIS = m_displayedToolpaths[name];
        
        // Erase from display
        if (!toolpathAIS.IsNull()) {
            m_context->Erase(toolpathAIS, Standard_False);
        }
    }
    
    // Clear the map
    m_displayedToolpaths.clear();
    
    // Force view update
    m_context->UpdateCurrentViewer();
    
    emit allToolpathsCleared();
    qDebug() << "All toolpaths cleared";
}

void ToolpathManager::setToolpathVisible(const QString& name, bool visible)
{
    if (m_context.IsNull()) {
        return;
    }
    
    if (m_displayedToolpaths.contains(name)) {
        Handle(AIS_Shape) toolpathAIS = m_displayedToolpaths[name];
        
        if (!toolpathAIS.IsNull()) {
            if (visible) {
                m_context->Display(toolpathAIS, Standard_False);
            } else {
                m_context->Erase(toolpathAIS, Standard_False);
            }
            
            m_context->UpdateCurrentViewer();
        }
    }
}

void ToolpathManager::setDisplaySettings(const ToolpathDisplaySettings& settings)
{
    m_displaySettings = settings;
    
    // Update all toolpath visualizations with new settings
    updateAllToolpathVisualizations();
}

void ToolpathManager::updateAllToolpathVisualizations()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Update display properties for all toolpaths
    for (auto it = m_displayedToolpaths.begin(); it != m_displayedToolpaths.end(); ++it) {
        Handle(AIS_Shape) toolpathAIS = it.value();
        
        if (!toolpathAIS.IsNull()) {
            // Remove from display temporarily
            m_context->Erase(toolpathAIS, Standard_False);
            
            // Update display properties
            setToolpathDisplayProperties(toolpathAIS, false);
            
            // Redisplay
            m_context->Display(toolpathAIS, Standard_False);
        }
    }
    
    m_context->UpdateCurrentViewer();
}

TopoDS_Shape ToolpathManager::createToolpathShape(const IntuiCAM::Toolpath::Toolpath& toolpath)
{
    const std::vector<IntuiCAM::Toolpath::Movement>& movements = toolpath.getMovements();
    
    if (movements.empty()) {
        return TopoDS_Shape();
    }
    
    // Create a compound to hold all toolpath segments
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);
    
    // Track the start of each segment (each segment has same movement type)
    size_t segmentStart = 0;
    IntuiCAM::Toolpath::MovementType currentType = movements[0].type;
    
    // Create segments based on movement type
    for (size_t i = 1; i < movements.size(); ++i) {
        if (movements[i].type != currentType) {
            // End of segment - create shape for this segment
            TopoDS_Shape segmentShape = createToolpathSegment(movements, segmentStart, i - 1);
            if (!segmentShape.IsNull()) {
                builder.Add(compound, segmentShape);
            }
            
            // Start new segment
            segmentStart = i;
            currentType = movements[i].type;
        }
    }
    
    // Add the final segment
    TopoDS_Shape finalSegment = createToolpathSegment(movements, segmentStart, movements.size() - 1);
    if (!finalSegment.IsNull()) {
        builder.Add(compound, finalSegment);
    }
    
    return compound;
}

TopoDS_Shape ToolpathManager::createToolpathSegment(const std::vector<IntuiCAM::Toolpath::Movement>& movements,
                                                  size_t startIdx, size_t endIdx)
{
    if (startIdx > endIdx || endIdx >= movements.size()) {
        return TopoDS_Shape();
    }
    
    // For single point, create a special marker
    if (startIdx == endIdx) {
        const auto& point = movements[startIdx].position;
        gp_Pnt pnt(point.x, point.y, point.z);
        
        // Create a small edge as a placeholder (could be replaced with a more visible marker)
        gp_Pnt pnt2(point.x + 0.1, point.y, point.z);
        BRepBuilderAPI_MakeEdge edge(pnt, pnt2);
        return edge.Shape();
    }
    
    // Create a wire from multiple movements
    BRepBuilderAPI_MakeWire wireBuilder;
    
    for (size_t i = startIdx; i < endIdx; ++i) {
        const auto& point1 = movements[i].position;
        const auto& point2 = movements[i + 1].position;
        
        gp_Pnt pnt1(point1.x, point1.y, point1.z);
        gp_Pnt pnt2(point2.x, point2.y, point2.z);
        
        // Skip if points are too close
        if (pnt1.Distance(pnt2) < 1e-6) {
            continue;
        }
        
        BRepBuilderAPI_MakeEdge edge(pnt1, pnt2);
        if (edge.IsDone()) {
            wireBuilder.Add(edge.Edge());
        }
    }
    
    if (wireBuilder.IsDone()) {
        return wireBuilder.Wire();
    }
    
    return TopoDS_Shape();
}

void ToolpathManager::setToolpathDisplayProperties(Handle(AIS_Shape) aisObject, bool isRapid)
{
    if (aisObject.IsNull()) {
        return;
    }
    
    // Set line color
    Quantity_Color lineColor = isRapid ? 
        Quantity_Color(m_displaySettings.rapidColor.redF(), 
                      m_displaySettings.rapidColor.greenF(), 
                      m_displaySettings.rapidColor.blueF(), 
                      Quantity_TOC_RGB) :
        Quantity_Color(m_displaySettings.cuttingColor.redF(), 
                      m_displaySettings.cuttingColor.greenF(), 
                      m_displaySettings.cuttingColor.blueF(), 
                      Quantity_TOC_RGB);
    
    // Set line width
    double lineWidth = isRapid ? m_displaySettings.rapidLineWidth : m_displaySettings.cuttingLineWidth;
    
    // Line type
    Aspect_TypeOfLine lineType = isRapid ? Aspect_TOL_DASH : Aspect_TOL_SOLID;
    
    // Create aspect
    Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(lineColor, lineType, lineWidth);
    
    // Apply to shape
    aisObject->Attributes()->SetWireAspect(lineAspect);
    
    // No fill for toolpaths
    aisObject->SetDisplayMode(AIS_WireFrame);
}

Quantity_Color ToolpathManager::getMovementColor(const IntuiCAM::Toolpath::Movement& movement)
{
    switch (movement.type) {
        case IntuiCAM::Toolpath::MovementType::Rapid:
            return Quantity_Color(m_displaySettings.rapidColor.redF(), 
                                m_displaySettings.rapidColor.greenF(), 
                                m_displaySettings.rapidColor.blueF(), 
                                Quantity_TOC_RGB);
        
        case IntuiCAM::Toolpath::MovementType::Linear:
        case IntuiCAM::Toolpath::MovementType::CircularCW:
        case IntuiCAM::Toolpath::MovementType::CircularCCW:
            return Quantity_Color(m_displaySettings.cuttingColor.redF(), 
                                m_displaySettings.cuttingColor.greenF(), 
                                m_displaySettings.cuttingColor.blueF(), 
                                Quantity_TOC_RGB);
        
        case IntuiCAM::Toolpath::MovementType::Dwell:
            return Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB); // Yellow for dwell
        
        case IntuiCAM::Toolpath::MovementType::ToolChange:
            return Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB); // Orange for tool change
            
        default:
            return Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB); // Gray for unknown
    }
} 