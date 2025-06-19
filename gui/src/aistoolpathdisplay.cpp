#include "aistoolpathdisplay.h"
#include <IntuiCAM/Toolpath/Types.h>
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <Quantity_NameOfColor.hxx>
#include <gp_Pnt.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <cmath>

IMPLEMENT_STANDARD_RTTIEXT(AIS_ToolpathDisplay, AIS_InteractiveObject)

AIS_ToolpathDisplay::AIS_ToolpathDisplay(std::shared_ptr<IntuiCAM::Toolpath::Toolpath> toolpath,
                                        const std::string& operationType)
    : AIS_InteractiveObject()
    , m_toolpath(toolpath)
    , m_operationType(operationType)
    , m_transformation()
    , m_rapidColor(Quantity_NOC_RED)        // Red for rapid moves
    , m_feedColor(Quantity_NOC_GREEN)       // Green for feed moves  
    , m_plungeColor(Quantity_NOC_BLUE)      // Blue for plunge moves
    , m_lineWidth(1.5)
    , m_isVisible(Standard_True)
    , m_needsUpdate(Standard_True)
{
    // Set default properties
    SetHilightMode(0); // Highlight in wireframe mode
    
    // Use operation-specific colors
    m_feedColor = getOperationColor();
    
    // Convert toolpath data to visualization format
    convertToolpathToMoves();
    calculateStats();
    
    // Initialize drawer with appropriate line aspects
    myDrawer->SetWireAspect(new Prs3d_LineAspect(m_feedColor, Aspect_TOL_SOLID, m_lineWidth));
}

void AIS_ToolpathDisplay::SetToolpath(std::shared_ptr<IntuiCAM::Toolpath::Toolpath> toolpath)
{
    m_toolpath = toolpath;
    m_needsUpdate = Standard_True;
    
    // Reconvert toolpath data
    convertToolpathToMoves();
    calculateStats();
    
    // Mark for redraw if object is already displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ToolpathDisplay::SetTransformation(const gp_Trsf& transform)
{
    m_transformation = transform;
    
    // Apply transformation to the AIS object
    SetLocalTransformation(transform);
    
    // Update presentation if needed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ToolpathDisplay::SetOperationType(const std::string& operationType)
{
    m_operationType = operationType;
    m_feedColor = getOperationColor();
    
    // Update wire aspect with new color
    myDrawer->WireAspect()->SetColor(m_feedColor);
    
    // Update presentation if object is displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ToolpathDisplay::SetVisible(Standard_Boolean visible)
{
    m_isVisible = visible;
    
    if (!GetContext().IsNull()) {
        if (visible) {
            GetContext()->Display(this, Standard_False);
        } else {
            GetContext()->Erase(this, Standard_False);
        }
        GetContext()->UpdateCurrentViewer();
    }
}

void AIS_ToolpathDisplay::SetMoveColors(const Quantity_Color& rapidColor,
                                       const Quantity_Color& feedColor,
                                       const Quantity_Color& plungeColor)
{
    m_rapidColor = rapidColor;
    m_feedColor = feedColor;
    m_plungeColor = plungeColor;
    
    // Update wire aspect with new feed color (most common)
    myDrawer->WireAspect()->SetColor(m_feedColor);
    
    // Update presentation if object is displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ToolpathDisplay::SetLineWidth(Standard_Real width)
{
    m_lineWidth = width;
    
    // Update the wire aspect with new width
    myDrawer->WireAspect()->SetWidth(width);
    
    // Update presentation if object is displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

AIS_ToolpathDisplay::ToolpathStats AIS_ToolpathDisplay::GetStats() const
{
    return m_stats;
}

void AIS_ToolpathDisplay::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                 const Handle(Prs3d_Presentation)& thePrs,
                                 const Standard_Integer theMode)
{
    // Only support wireframe mode (mode 0)
    if (theMode != 0) {
        return;
    }
    
    // Skip if no moves or not visible
    if (m_moves.empty() || !m_isVisible) {
        return;
    }
    
    // Create separate geometry for different move types
    Handle(Graphic3d_ArrayOfSegments) rapidSegments = createRapidGeometry();
    Handle(Graphic3d_ArrayOfSegments) feedSegments = createFeedGeometry();
    Handle(Graphic3d_ArrayOfSegments) plungeSegments = createPlungeGeometry();
    
    // Add rapid moves (red, dashed)
    if (!rapidSegments.IsNull() && rapidSegments->VertexNumber() > 0) {
        Handle(Graphic3d_Group) rapidGroup = thePrs->NewGroup();
        Handle(Prs3d_LineAspect) rapidAspect = new Prs3d_LineAspect(
            m_rapidColor, Aspect_TOL_DASH, m_lineWidth);
        rapidGroup->SetGroupPrimitivesAspect(rapidAspect->Aspect());
        rapidGroup->AddPrimitiveArray(rapidSegments);
    }
    
    // Add feed moves (operation color, solid)
    if (!feedSegments.IsNull() && feedSegments->VertexNumber() > 0) {
        Handle(Graphic3d_Group) feedGroup = thePrs->NewGroup();
        Handle(Prs3d_LineAspect) feedAspect = new Prs3d_LineAspect(
            m_feedColor, Aspect_TOL_SOLID, m_lineWidth);
        feedGroup->SetGroupPrimitivesAspect(feedAspect->Aspect());
        feedGroup->AddPrimitiveArray(feedSegments);
    }
    
    // Add plunge moves (blue, dotted)
    if (!plungeSegments.IsNull() && plungeSegments->VertexNumber() > 0) {
        Handle(Graphic3d_Group) plungeGroup = thePrs->NewGroup();
        Handle(Prs3d_LineAspect) plungeAspect = new Prs3d_LineAspect(
            m_plungeColor, Aspect_TOL_DOT, m_lineWidth);
        plungeGroup->SetGroupPrimitivesAspect(plungeAspect->Aspect());
        plungeGroup->AddPrimitiveArray(plungeSegments);
    }
    
    m_needsUpdate = Standard_False;
}

void AIS_ToolpathDisplay::ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                          const Standard_Integer theMode)
{
    // Create selection entities for toolpath segments
    if (theMode == 0 && !m_moves.empty()) {
        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
        
        // Add selection entities for each move segment
        for (const auto& move : m_moves) {
            Handle(Select3D_SensitiveSegment) segment = 
                new Select3D_SensitiveSegment(owner, move.startPoint, move.endPoint);
            theSel->Add(segment);
        }
    }
}

Standard_Boolean AIS_ToolpathDisplay::AcceptDisplayMode(const Standard_Integer theMode) const
{
    // Only support wireframe mode (mode 0)
    return theMode == 0;
}

void AIS_ToolpathDisplay::convertToolpathToMoves()
{
    m_moves.clear();
    
    if (!m_toolpath) {
        return;
    }
    
    // Get moves from the toolpath object
    const auto& toolpathMoves = m_toolpath->getMoves();
    
    if (toolpathMoves.empty()) {
        return;
    }
    
    // Convert consecutive movements to segments for visualization
    gp_Pnt lastPosition(0, 0, 0);
    bool hasLastPosition = false;
    
    for (const auto& move : toolpathMoves) {
        gp_Pnt currentPosition(move.position.x, move.position.y, move.position.z);
        
        if (hasLastPosition) {
            // Convert MovementType to MoveType for display
            IntuiCAM::Toolpath::MoveType displayMoveType;
            switch (move.type) {
                case IntuiCAM::Toolpath::MovementType::Rapid:
                    displayMoveType = IntuiCAM::Toolpath::MoveType::Rapid;
                    break;
                case IntuiCAM::Toolpath::MovementType::Linear:
                    displayMoveType = IntuiCAM::Toolpath::MoveType::Feed;
                    break;
                case IntuiCAM::Toolpath::MovementType::CircularCW:
                    displayMoveType = IntuiCAM::Toolpath::MoveType::CircularCW;
                    break;
                case IntuiCAM::Toolpath::MovementType::CircularCCW:
                    displayMoveType = IntuiCAM::Toolpath::MoveType::CircularCCW;
                    break;
                default:
                    displayMoveType = IntuiCAM::Toolpath::MoveType::Feed;
                    break;
            }
            
            m_moves.emplace_back(lastPosition, currentPosition, displayMoveType, move.feedRate, move.spindleSpeed);
        }
        
        lastPosition = currentPosition;
        hasLastPosition = true;
    }
}

Handle(Graphic3d_ArrayOfSegments) AIS_ToolpathDisplay::createRapidGeometry()
{
    // Count rapid moves
    int rapidCount = 0;
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Rapid) {
            rapidCount++;
        }
    }
    
    if (rapidCount == 0) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }
    
    // Create segments array
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(
        rapidCount * 2,  // vertices (2 per segment)
        rapidCount * 2   // edges (2 indices per segment)
    );
    
    // Add rapid move segments
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Rapid) {
            segments->AddVertex(move.startPoint);
            segments->AddVertex(move.endPoint);
            int vertexIndex = segments->VertexNumber();
            segments->AddEdges(vertexIndex - 1, vertexIndex);
        }
    }
    
    return segments;
}

Handle(Graphic3d_ArrayOfSegments) AIS_ToolpathDisplay::createFeedGeometry()
{
    // Count feed moves
    int feedCount = 0;
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Feed ||
            move.moveType == IntuiCAM::Toolpath::MoveType::Cut) {
            feedCount++;
        }
    }
    
    if (feedCount == 0) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }
    
    // Create segments array
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(
        feedCount * 2,  // vertices (2 per segment)
        feedCount * 2   // edges (2 indices per segment)
    );
    
    // Add feed move segments
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Feed ||
            move.moveType == IntuiCAM::Toolpath::MoveType::Cut) {
            segments->AddVertex(move.startPoint);
            segments->AddVertex(move.endPoint);
            int vertexIndex = segments->VertexNumber();
            segments->AddEdges(vertexIndex - 1, vertexIndex);
        }
    }
    
    return segments;
}

Handle(Graphic3d_ArrayOfSegments) AIS_ToolpathDisplay::createPlungeGeometry()
{
    // Count plunge moves
    int plungeCount = 0;
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Plunge) {
            plungeCount++;
        }
    }
    
    if (plungeCount == 0) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }
    
    // Create segments array
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(
        plungeCount * 2,  // vertices (2 per segment)
        plungeCount * 2   // edges (2 indices per segment)
    );
    
    // Add plunge move segments
    for (const auto& move : m_moves) {
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Plunge) {
            segments->AddVertex(move.startPoint);
            segments->AddVertex(move.endPoint);
            int vertexIndex = segments->VertexNumber();
            segments->AddEdges(vertexIndex - 1, vertexIndex);
        }
    }
    
    return segments;
}

Quantity_Color AIS_ToolpathDisplay::getOperationColor() const
{
    // Color coding by operation type
    if (m_operationType == "facing") {
        return Quantity_NOC_ORANGE;       // Orange for facing
    } else if (m_operationType == "roughing") {
        return Quantity_NOC_YELLOW;       // Yellow for roughing
    } else if (m_operationType == "finishing") {
        return Quantity_NOC_GREEN;        // Green for finishing
    } else if (m_operationType == "contouring") {
        return Quantity_NOC_CYAN;         // Cyan for combined contouring
    } else if (m_operationType == "parting") {
        return Quantity_NOC_MAGENTA;      // Magenta for parting
    } else {
        return Quantity_NOC_WHITE;        // White for unknown types
    }
}

void AIS_ToolpathDisplay::calculateStats()
{
    m_stats = ToolpathStats{};
    
    if (m_moves.empty()) {
        return;
    }
    
    m_stats.totalMoves = static_cast<int>(m_moves.size());
    m_stats.totalLength = 0.0;
    m_stats.estimatedTime = 0.0;
    
    for (const auto& move : m_moves) {
        // Count move types
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Rapid) {
            m_stats.rapidMoves++;
        } else if (move.moveType == IntuiCAM::Toolpath::MoveType::Feed ||
                  move.moveType == IntuiCAM::Toolpath::MoveType::Cut) {
            m_stats.feedMoves++;
        }
        
        // Calculate length
        double segmentLength = move.startPoint.Distance(move.endPoint);
        m_stats.totalLength += segmentLength;
        
        // Estimate time based on move type and feed rate
        if (move.moveType == IntuiCAM::Toolpath::MoveType::Rapid) {
            // Assume rapid rate of 1000 mm/min
            m_stats.estimatedTime += segmentLength / 1000.0;
        } else if (move.feedRate > 0.0) {
            // Use actual feed rate
            m_stats.estimatedTime += segmentLength / move.feedRate;
        } else {
            // Assume default feed rate of 100 mm/min
            m_stats.estimatedTime += segmentLength / 100.0;
        }
    }
} 