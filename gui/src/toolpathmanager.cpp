#include "../include/toolpathmanager.h"
#include "../include/workpiecemanager.h"

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
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Pnt.hxx>
#include <AIS_Shape.hxx>

ToolpathManager::ToolpathManager(QObject *parent)
    : QObject(parent)
    , m_workpieceManager(nullptr)
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

void ToolpathManager::setWorkpieceManager(WorkpieceManager* workpieceManager)
{
    m_workpieceManager = workpieceManager;
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
    
    // Store the original untransformed shape for later transformations
    m_originalToolpathShapes[name] = toolpathShape;
    
    // Apply workpiece transformation to the toolpath shape
    gp_Trsf transformation = getWorkpieceTransformation();
    TopoDS_Shape transformedShape;
    
    // Debug transformation details for toolpath display
    gp_XYZ translation = transformation.TranslationPart();
    qDebug() << "ToolpathManager: Displaying toolpath with transformation:";
    qDebug() << "  - Toolpath name:" << name;
    qDebug() << "  - Translation vector:" << translation.X() << "," << translation.Y() << "," << translation.Z();
    qDebug() << "  - Transformation form:" << transformation.Form();
    
    // Only transform if we have a non-identity transformation
    if (transformation.Form() != gp_Identity) {
        BRepBuilderAPI_Transform transformer(toolpathShape, transformation, Standard_True);
        if (transformer.IsDone()) {
            transformedShape = transformer.Shape();
            qDebug() << "ToolpathManager: Successfully applied transformation to new toolpath";
        } else {
            emit errorOccurred(QString("Failed to transform toolpath '%1'").arg(name));
            transformedShape = toolpathShape; // Use original as fallback
            qDebug() << "ToolpathManager: Failed to transform toolpath, using original shape";
        }
    } else {
        transformedShape = toolpathShape;
        qDebug() << "ToolpathManager: No transformation needed (identity)";
    }
    
    // Create AIS shape for visualization
    Handle(AIS_Shape) toolpathAIS = new AIS_Shape(transformedShape);
    
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
        
        // Remove from maps
        m_displayedToolpaths.remove(name);
        m_originalToolpathShapes.remove(name);
        
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
    
    // Clear the maps
    m_displayedToolpaths.clear();
    m_originalToolpathShapes.clear();
    
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

void ToolpathManager::setAllToolpathsVisible(bool visible)
{
    if (m_context.IsNull()) {
        return;
    }

    for (auto it = m_displayedToolpaths.begin(); it != m_displayedToolpaths.end(); ++it) {
        Handle(AIS_Shape) toolpathAIS = it.value();
        if (!toolpathAIS.IsNull()) {
            if (visible) {
                if (!m_context->IsDisplayed(toolpathAIS)) {
                    m_context->Display(toolpathAIS, Standard_False);
                }
            } else {
                m_context->Erase(toolpathAIS, Standard_False);
            }
        }
    }

    m_context->UpdateCurrentViewer();
}

bool ToolpathManager::areToolpathsVisible() const
{
    if (m_context.IsNull()) {
        return false;
    }

    for (auto it = m_displayedToolpaths.constBegin(); it != m_displayedToolpaths.constEnd(); ++it) {
        Handle(AIS_Shape) ais = it.value();
        if (!ais.IsNull() && m_context->IsDisplayed(ais)) {
            return true;
        }
    }

    return false;
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

void ToolpathManager::applyWorkpieceTransformationToToolpaths()
{
    if (m_context.IsNull()) {
        qDebug() << "ToolpathManager: Cannot apply transformations - context null";
        return;
    }
    
    if (m_displayedToolpaths.isEmpty()) {
        qDebug() << "ToolpathManager: No toolpaths to transform";
        return;
    }
    
    if (!m_workpieceManager) {
        qDebug() << "ToolpathManager: Cannot apply transformations - no workpiece manager set";
        emit errorOccurred("Cannot update toolpaths: Workpiece manager not set");
        return;
    }
    
    qDebug() << "\n==== TOOLPATH TRANSFORMATION UPDATE ====";
    qDebug() << "ToolpathManager: Applying transformations to" << m_displayedToolpaths.size() 
             << "toolpaths and" << m_originalToolpathShapes.size() << "original shapes";
    
    // Get the current workpiece transformation
    gp_Trsf transformation = getWorkpieceTransformation();
    
    if (transformation.Form() == gp_Identity) {
        qDebug() << "ToolpathManager: No transformation needed (identity transformation)";
        return;
    }
    
    // Debug transformation details
    gp_XYZ translation = transformation.TranslationPart();
    double scale = transformation.ScaleFactor();
    qDebug() << "ToolpathManager: Workpiece transformation:";
    qDebug() << "  - Translation vector:" << translation.X() << "," << translation.Y() << "," << translation.Z();
    qDebug() << "  - Scale factor:" << scale;
    qDebug() << "  - Transformation form:" << transformation.Form();
    
    // Get all keys to avoid modifying map during iteration
    QStringList keys = m_displayedToolpaths.keys();
    int successCount = 0;
    int failureCount = 0;
    
    // Process each toolpath
    for (const QString& name : keys) {
        qDebug() << "ToolpathManager: Processing toolpath:" << name;
        
        // Erase the current displayed toolpath
        Handle(AIS_Shape) toolpathAIS = m_displayedToolpaths[name];
        if (!toolpathAIS.IsNull()) {
            m_context->Erase(toolpathAIS, Standard_False);
        }
        
        // Get the original untransformed shape
        if (!m_originalToolpathShapes.contains(name)) {
            qDebug() << "ToolpathManager: ERROR - No original shape found for toolpath:" << name;
            failureCount++;
            continue;
        }
        
        TopoDS_Shape originalShape = m_originalToolpathShapes[name];
        
        // Apply transformation to the original shape
        BRepBuilderAPI_Transform transformer(originalShape, transformation, Standard_True);
        if (transformer.IsDone()) {
            // Create new AIS shape with transformed geometry
            Handle(AIS_Shape) newToolpathAIS = new AIS_Shape(transformer.Shape());
            
            // Set display properties
            setToolpathDisplayProperties(newToolpathAIS, false);
            
            // Update in map and display
            m_displayedToolpaths[name] = newToolpathAIS;
            m_context->Display(newToolpathAIS, Standard_False);
            
            qDebug() << "ToolpathManager: Successfully transformed toolpath:" << name;
            successCount++;
        } else {
            qDebug() << "ToolpathManager: Failed to transform toolpath:" << name;
            failureCount++;
            emit errorOccurred(QString("Failed to transform toolpath: %1").arg(name));
        }
    }
    
    m_context->UpdateCurrentViewer();
    qDebug() << "ToolpathManager: Transformation complete -" << successCount << "succeeded," 
             << failureCount << "failed";
    qDebug() << "==== END TOOLPATH TRANSFORMATION ====\n";
}

gp_Trsf ToolpathManager::getWorkpieceTransformation() const
{
    // Return identity transformation if workpiece manager not set
    if (!m_workpieceManager) {
        qDebug() << "ToolpathManager: No workpiece manager set, returning identity transformation";
        return gp_Trsf(); // Identity transformation
    }
    
    // Get transformation from workpiece manager
    gp_Trsf transform = m_workpieceManager->getCurrentTransformation();
    
    // Debug transformation details
    gp_XYZ translation = transform.TranslationPart();
    qDebug() << "ToolpathManager: Current workpiece transformation:";
    qDebug() << "  - Translation vector:" << translation.X() << "," << translation.Y() << "," << translation.Z();
    qDebug() << "  - Transformation form:" << transform.Form();
    qDebug() << "  - Workpiece position offset:" << m_workpieceManager->getWorkpiecePositionOffset() << "mm";
    qDebug() << "  - Workpiece flipped:" << (m_workpieceManager->isWorkpieceFlipped() ? "Yes" : "No");
    qDebug() << "  - Has axis alignment:" << (m_workpieceManager->hasAxisAlignmentTransformation() ? "Yes" : "No");
    
    return transform;
}

// === New method: displayLatheProfile =======================================
bool ToolpathManager::displayLatheProfile(const std::vector<IntuiCAM::Geometry::Point2D>& profile, const QString& name)
{
    if (m_context.IsNull()) {
        emit errorOccurred("Cannot display profile: AIS context not initialized");
        return false;
    }
    
    if (profile.empty()) {
        emit errorOccurred("Cannot display empty profile");
        return false;
    }
    
    // Remove existing profile with the same name if it exists
    removeProfile(name);
    
    try {
        // Create a compound shape to hold all profile edges
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        
        // Convert 2D profile points to 3D (assuming Y=0 for turning operations)
        std::vector<gp_Pnt> points3D;
        for (const auto& point : profile) {
            // For lathe profiles: x = radius, z = z-position
            points3D.emplace_back(point.x, 0.0, point.z);
        }
        
        // Create edges connecting consecutive points
        for (size_t i = 1; i < points3D.size(); ++i) {
            const gp_Pnt& p1 = points3D[i-1];
            const gp_Pnt& p2 = points3D[i];
            
            // Skip degenerate edges
            if (p1.Distance(p2) < 1e-6) {
                continue;
            }
            
            BRepBuilderAPI_MakeEdge edgeBuilder(p1, p2);
            if (edgeBuilder.IsDone()) {
                builder.Add(compound, edgeBuilder.Edge());
            }
        }
        
        // Apply workpiece transformation if available
        gp_Trsf transformation = getWorkpieceTransformation();
        TopoDS_Shape transformedShape = compound;
        
        if (transformation.Form() != gp_Identity) {
            BRepBuilderAPI_Transform transformer(compound, transformation, Standard_True);
            if (transformer.IsDone()) {
                transformedShape = transformer.Shape();
            }
        }
        
        // Create AIS shape for visualization
        Handle(AIS_Shape) profileAIS = new AIS_Shape(transformedShape);
        
        // Set profile display properties (different from toolpaths)
        profileAIS->SetColor(Quantity_NOC_YELLOW);
        profileAIS->SetWidth(2.0);
        profileAIS->SetTransparency(0.3);
        
        // Display the profile
        m_context->Display(profileAIS, Standard_False);
        
        // Store the profile AIS object for later removal
        m_displayedProfiles[name] = profileAIS;
        
        // Force view update
        m_context->UpdateCurrentViewer();
        
        qDebug() << "Profile displayed:" << name << "with" << profile.size() << "points";
        return true;
        
    } catch (const Standard_Failure& e) {
        QString errorMsg = QString("OpenCASCADE error displaying profile: %1").arg(e.GetMessageString());
        emit errorOccurred(errorMsg);
        return false;
    } catch (const std::exception& e) {
        QString errorMsg = QString("Error displaying profile: %1").arg(e.what());
        emit errorOccurred(errorMsg);
        return false;
    }
}

void ToolpathManager::removeProfile(const QString& name)
{
    if (m_context.IsNull()) {
        return;
    }
    
    if (m_displayedProfiles.contains(name)) {
        Handle(AIS_Shape) profileAIS = m_displayedProfiles[name];
        
        // Erase from display
        if (!profileAIS.IsNull()) {
            m_context->Erase(profileAIS, Standard_False);
        }
        
        // Remove from map
        m_displayedProfiles.remove(name);
        
        m_context->UpdateCurrentViewer();
        
        qDebug() << "Profile removed:" << name;
    }
} 