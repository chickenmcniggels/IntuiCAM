#include <IntuiCAM/Toolpath/ToolpathDisplayObject.h>

#include <AIS_Line.hxx>
#include <AIS_Point.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <Precision.hxx>
#include <Geom_Line.hxx>
#include <Geom_Circle.hxx>
#include <Geom_BSplineCurve.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_AspectMarker3d.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <Select3D_SensitivePoint.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

IMPLEMENT_STANDARD_RTTIEXT(ToolpathDisplayObject, AIS_InteractiveObject)
IMPLEMENT_STANDARD_RTTIEXT(ProfileDisplayObject, AIS_InteractiveObject)

// ToolpathDisplayObject Implementation
ToolpathDisplayObject::ToolpathDisplayObject(std::shared_ptr<Toolpath> toolpath,
                                             const VisualizationSettings& settings)
    : AIS_InteractiveObject()
    , toolpath_(toolpath)
    , settings_(settings)
    , isVisible_(true)
    , progress_(1.0)
    , needsUpdate_(true) {
    
    SetDisplayMode(static_cast<Standard_Integer>(DisplayMode::AllMoves));
    SetHilightMode(static_cast<Standard_Integer>(DisplayMode::AllMoves));
}

void ToolpathDisplayObject::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                    const Handle(Prs3d_Presentation)& thePrs,
                                    const Standard_Integer theMode) {
    
    if (!toolpath_ || !isVisible_) {
        return;
    }
    
    thePrs->Clear();
    
    DisplayMode mode = static_cast<DisplayMode>(theMode);
    
    switch (mode) {
        case DisplayMode::Wireframe:
            computeWireframePresentation(thePrs);
            break;
        case DisplayMode::Shaded:
            computeShadedPresentation(thePrs);
            break;
        case DisplayMode::RapidMoves:
        case DisplayMode::FeedMoves:
        case DisplayMode::CuttingMoves:
            computeMoveTypePresentation(thePrs, mode);
            break;
        case DisplayMode::AllMoves:
        default:
            computeWireframePresentation(thePrs);
            break;
    }
}

void ToolpathDisplayObject::ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
                                             const Standard_Integer theMode) {
    
    if (!toolpath_) {
        return;
    }
    
    const auto& moves = toolpath_->getMoves();
    Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
    
    for (size_t i = 0; i < moves.size() && i < static_cast<size_t>(progress_ * moves.size()); ++i) {
        const auto& move = moves[i];
        
        // Apply the same coordinate transformation as in the 2D profile
        // Movements store axial position in `x` and radial position in `z`.
        // Convert to viewer coordinates (X = radius, Z = axial).
        gp_Pnt startPnt(move.startPoint.z, 0.0, move.startPoint.x);  // (radius, 0, axial)
        gp_Pnt endPnt(move.endPoint.z, 0.0, move.endPoint.x);        // (radius, 0, axial)
        
        // Ensure Y=0 to constrain to XZ plane for lathe operations
        startPnt.SetY(0.0);
        endPnt.SetY(0.0);
        
        Handle(Select3D_SensitiveSegment) segment = new Select3D_SensitiveSegment(owner, startPnt, endPnt);
        theSelection->Add(segment);
    }
}

void ToolpathDisplayObject::setToolpath(std::shared_ptr<Toolpath> toolpath) {
    toolpath_ = toolpath;
    needsUpdate_ = true;
    SetToUpdate();
}

void ToolpathDisplayObject::setVisualizationSettings(const VisualizationSettings& settings) {
    settings_ = settings;
    needsUpdate_ = true;
    SetToUpdate();
}

void ToolpathDisplayObject::setVisible(bool visible) {
    isVisible_ = visible;
    SetToUpdate();
}

void ToolpathDisplayObject::setProgress(double progress) {
    progress_ = std::clamp(progress, 0.0, 1.0);
    SetToUpdate();
}

void ToolpathDisplayObject::setColorScheme(ColorScheme scheme) {
    settings_.colorScheme = scheme;
    needsUpdate_ = true;
    SetToUpdate();
}

void ToolpathDisplayObject::setCustomColor(const Quantity_Color& color) {
    // This would set a custom color override
    SetColor(color);
    SetToUpdate();
}

Quantity_Color ToolpathDisplayObject::getColorForMove(const Movement& move, size_t moveIndex) const {
    switch (settings_.colorScheme) {
        case ColorScheme::Default:
            return getDefaultColor(move);
        case ColorScheme::Rainbow:
            return getRainbowColor(static_cast<double>(moveIndex), 0.0, static_cast<double>(toolpath_->getMovements().size()));
        case ColorScheme::DepthBased:
            {
                auto stats = calculateStatistics();
                return getDepthBasedColor(move.position.z, stats.minZ, stats.maxZ);
            }
        case ColorScheme::OperationType:
            return getOperationTypeColor(move);
        default:
            return getDefaultColor(move);
    }
}

ToolpathDisplayObject::DisplayStatistics ToolpathDisplayObject::calculateStatistics() const {
    DisplayStatistics stats;
    
    if (!toolpath_) {
        return stats;
    }
    
    const auto& moves = toolpath_->getMovements();
    stats.totalMoves = moves.size();
    
    if (moves.empty()) {
        return stats;
    }
    
    // Initialize bounding box
    stats.boundingBoxMin = gp_Pnt(moves[0].position.x, moves[0].position.y, moves[0].position.z);
    stats.boundingBoxMax = stats.boundingBoxMin;
    stats.minZ = moves[0].position.z;
    stats.maxZ = moves[0].position.z;
    
    for (size_t moveIndex = 0; moveIndex < moves.size(); ++moveIndex) {
        const auto& move = moves[moveIndex];
        
        // Count move types
        switch (move.type) {
            case MovementType::Rapid:
                stats.rapidMoves++;
                break;
            case MovementType::Linear:
            case MovementType::CircularCW:
            case MovementType::CircularCCW:
                if (move.feedRate > 0) {
                    stats.cuttingMoves++;
                } else {
                    stats.feedMoves++;
                }
                break;
        }
        
        // Calculate lengths
        gp_Pnt start(move.position.x, move.position.y, move.position.z);
        gp_Pnt end = (moveIndex + 1 < moves.size()) ?
            gp_Pnt(moves[moveIndex + 1].position.x,
                   moves[moveIndex + 1].position.y,
                   moves[moveIndex + 1].position.z) : start;
        double length = start.Distance(end);
        
        stats.totalLength += length;
        if (move.type != MovementType::Rapid) {
            stats.cuttingLength += length;
        }
        
        // Update bounding box
        auto updateBoundingBox = [&](const Geometry::Point3D& point) {
            if (point.x < stats.boundingBoxMin.X()) stats.boundingBoxMin.SetX(point.x);
            if (point.y < stats.boundingBoxMin.Y()) stats.boundingBoxMin.SetY(point.y);
            if (point.z < stats.boundingBoxMin.Z()) stats.boundingBoxMin.SetZ(point.z);
            if (point.x > stats.boundingBoxMax.X()) stats.boundingBoxMax.SetX(point.x);
            if (point.y > stats.boundingBoxMax.Y()) stats.boundingBoxMax.SetY(point.y);
            if (point.z > stats.boundingBoxMax.Z()) stats.boundingBoxMax.SetZ(point.z);
            
            if (point.z < stats.minZ) stats.minZ = point.z;
            if (point.z > stats.maxZ) stats.maxZ = point.z;
        };
        
        updateBoundingBox(move.position);
    }
    
    return stats;
}

Handle(ToolpathDisplayObject) ToolpathDisplayObject::create(std::shared_ptr<Toolpath> toolpath,
                                                            const VisualizationSettings& settings) {
    return new ToolpathDisplayObject(toolpath, settings);
}

void ToolpathDisplayObject::highlightMove(size_t moveIndex, bool highlight) {
    auto it = std::find(selectedMoves_.begin(), selectedMoves_.end(), moveIndex);
    
    if (highlight) {
        if (it == selectedMoves_.end()) {
            selectedMoves_.push_back(moveIndex);
        }
    } else {
        if (it != selectedMoves_.end()) {
            selectedMoves_.erase(it);
        }
    }
    
    SetToUpdate();
}

void ToolpathDisplayObject::clearHighlights() {
    selectedMoves_.clear();
    SetToUpdate();
}

void ToolpathDisplayObject::computeWireframePresentation(const Handle(Prs3d_Presentation)& presentation) {
    if (!toolpath_) {
        return;
    }
    
    const auto& moves = toolpath_->getMovements();
    size_t maxMoves = static_cast<size_t>(progress_ * moves.size());
    
    if (maxMoves == 0) {
        return;
    }
    
    // Group moves by type for different visual representation
    std::vector<std::pair<gp_Pnt, gp_Pnt>> rapidMoves, feedMoves, cuttingMoves;
    
    for (size_t i = 1; i < maxMoves; ++i) {
        const auto& prevMove = moves[i-1];
        const auto& currentMove = moves[i];
        
        // COORDINATE SYSTEM TRANSFORMATION FOR LATHE OPERATIONS
        // Movements store axial position in `x` and radial position in `z`.
        // Convert to viewer coordinates where X is radius and Z is axial.
        gp_Pnt startPnt(prevMove.position.z, 0.0, prevMove.position.x);      // (radius, 0, axial)
        gp_Pnt endPnt(currentMove.position.z, 0.0, currentMove.position.x);  // (radius, 0, axial)
        
        // Ensure Y=0 to constrain to XZ plane for lathe operations
        startPnt.SetY(0.0);
        endPnt.SetY(0.0);
        
        // Group by movement type for different visualization
        switch (currentMove.type) {
            case MovementType::Rapid:
                rapidMoves.emplace_back(startPnt, endPnt);
                break;
            case MovementType::Linear:
                if (currentMove.feedRate > 0) {
                    cuttingMoves.emplace_back(startPnt, endPnt);
                } else {
                    feedMoves.emplace_back(startPnt, endPnt);
                }
                break;
            case MovementType::CircularCW:
            case MovementType::CircularCCW:
                cuttingMoves.emplace_back(startPnt, endPnt);
                break;
            default:
                feedMoves.emplace_back(startPnt, endPnt);
                break;
        }
    }
    
    // Draw rapid moves (thin, dashed lines)
    if (!rapidMoves.empty() && settings_.showRapidMoves) {
        Handle(Graphic3d_ArrayOfSegments) rapidArray = new Graphic3d_ArrayOfSegments(
            static_cast<Standard_Integer>(rapidMoves.size() * 2));
        
        for (const auto& move : rapidMoves) {
            rapidArray->AddVertex(move.first);
            rapidArray->AddVertex(move.second);
        }
        
        Handle(Graphic3d_Group) rapidGroup = presentation->NewGroup();
        Handle(Graphic3d_AspectLine3d) rapidAspect = new Graphic3d_AspectLine3d(
            Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB), // Gray
            Aspect_TOL_DASH, 1.0);
        rapidGroup->SetGroupPrimitivesAspect(rapidAspect);
        rapidGroup->AddPrimitiveArray(rapidArray);
    }
    
    // Draw feed moves (medium thickness, solid lines)
    if (!feedMoves.empty()) {
        Handle(Graphic3d_ArrayOfSegments) feedArray = new Graphic3d_ArrayOfSegments(
            static_cast<Standard_Integer>(feedMoves.size() * 2));
        
        for (const auto& move : feedMoves) {
            feedArray->AddVertex(move.first);
            feedArray->AddVertex(move.second);
        }
        
        Handle(Graphic3d_Group) feedGroup = presentation->NewGroup();
        Handle(Graphic3d_AspectLine3d) feedAspect = new Graphic3d_AspectLine3d(
            Quantity_Color(0.0, 0.6, 0.9, Quantity_TOC_RGB), // Blue
            Aspect_TOL_SOLID, settings_.lineWidth);
        feedGroup->SetGroupPrimitivesAspect(feedAspect);
        feedGroup->AddPrimitiveArray(feedArray);
    }
    
    // Draw cutting moves (thick, solid lines)
    if (!cuttingMoves.empty()) {
        Handle(Graphic3d_ArrayOfSegments) cuttingArray = new Graphic3d_ArrayOfSegments(
            static_cast<Standard_Integer>(cuttingMoves.size() * 2));
        
        for (const auto& move : cuttingMoves) {
            cuttingArray->AddVertex(move.first);
            cuttingArray->AddVertex(move.second);
        }
        
        Handle(Graphic3d_Group) cuttingGroup = presentation->NewGroup();
        Handle(Graphic3d_AspectLine3d) cuttingAspect = new Graphic3d_AspectLine3d(
            Quantity_Color(0.9, 0.1, 0.1, Quantity_TOC_RGB), // Red
            Aspect_TOL_SOLID, settings_.lineWidth * 1.5);
        cuttingGroup->SetGroupPrimitivesAspect(cuttingAspect);
        cuttingGroup->AddPrimitiveArray(cuttingArray);
    }
}

void ToolpathDisplayObject::computeShadedPresentation(const Handle(Prs3d_Presentation)& presentation) {
    // For shaded mode, use wireframe with thicker lines
    computeWireframePresentation(presentation);
}

void ToolpathDisplayObject::computeMoveTypePresentation(const Handle(Prs3d_Presentation)& presentation, DisplayMode mode) {
    if (!toolpath_) return;
    
    const auto& moves = toolpath_->getMoves();
    size_t maxMoves = static_cast<size_t>(progress_ * moves.size());
    
    ToolpathMoveType targetType;
    double lineWidth;
    
    switch (mode) {
        case DisplayMode::RapidMoves:
            targetType = ToolpathMoveType::Rapid;
            lineWidth = settings_.rapidLineWidth;
            break;
        case DisplayMode::FeedMoves:
        case DisplayMode::CuttingMoves:
            targetType = ToolpathMoveType::Linear; // Include both linear and arc
            lineWidth = settings_.cutLineWidth;
            break;
        default:
            return;
    }
    
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(static_cast<Standard_Integer>(maxMoves * 2));
    
    for (size_t i = 0; i < maxMoves; ++i) {
        const auto& move = moves[i];
        
        if ((mode == DisplayMode::RapidMoves && move.type != ToolpathMoveType::Rapid) ||
            (mode != DisplayMode::RapidMoves && move.type == ToolpathMoveType::Rapid)) {
            continue;
        }
        
        // Apply the same coordinate transformation as the 2D profile
        // Movements store axial position in `x` and radial position in `z`.
        gp_Pnt startPnt(move.startPoint.z, 0.0, move.startPoint.x);  // (radius, 0, axial)
        gp_Pnt endPnt(move.endPoint.z, 0.0, move.endPoint.x);        // (radius, 0, axial)
        
        // Ensure Y=0 to constrain to XZ plane for lathe operations
        startPnt.SetY(0.0);
        endPnt.SetY(0.0);
        
        Quantity_Color color = getColorForMove(move, i);
        
        segments->AddVertex(startPnt, color);
        segments->AddVertex(endPnt, color);
    }
    
    Handle(Graphic3d_AspectLine3d) lineAspect = new Graphic3d_AspectLine3d();
    lineAspect->SetWidth(lineWidth);
    
    Handle(Graphic3d_Group) group = presentation->NewGroup();
    group->SetPrimitivesAspect(lineAspect);
    group->AddPrimitiveArray(segments);
}

Quantity_Color ToolpathDisplayObject::getDefaultColor(const Movement& move) const {
    switch (move.type) {
        case MovementType::Rapid:
            return Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB); // Gray
        case MovementType::Linear:
            return Quantity_Color(0.0, 0.8, 0.0, Quantity_TOC_RGB); // Green
        case MovementType::CircularCW:
        case MovementType::CircularCCW:
            return Quantity_Color(0.0, 0.0, 0.8, Quantity_TOC_RGB); // Blue
        default:
            return Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB); // Default gray
    }
}

Quantity_Color ToolpathDisplayObject::getRainbowColor(double value, double min, double max) const {
    if (max <= min) return Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB);
    
    double normalized = (value - min) / (max - min);
    normalized = std::clamp(normalized, 0.0, 1.0);
    
    // Convert to HSV and then to RGB for rainbow effect
    double hue = normalized * 240.0; // Blue to red range
    double saturation = 1.0;
    double brightness = 1.0;
    
    // Simple HSV to RGB conversion
    double c = brightness * saturation;
    double x = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    double m = brightness - c;
    
    double r, g, b;
    if (hue < 60) {
        r = c; g = x; b = 0;
    } else if (hue < 120) {
        r = x; g = c; b = 0;
    } else if (hue < 180) {
        r = 0; g = c; b = x;
    } else if (hue < 240) {
        r = 0; g = x; b = c;
    } else {
        r = x; g = 0; b = c;
    }
    
    return Quantity_Color(r + m, g + m, b + m, Quantity_TOC_RGB);
}

Quantity_Color ToolpathDisplayObject::getDepthBasedColor(double z, double minZ, double maxZ) const {
    if (maxZ <= minZ) return Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB);
    
    double normalized = (z - minZ) / (maxZ - minZ);
    normalized = std::clamp(normalized, 0.0, 1.0);
    
    // Blue (deep) to red (shallow)
    return Quantity_Color(normalized, 0.0, 1.0 - normalized, Quantity_TOC_RGB);
}

Quantity_Color ToolpathDisplayObject::getOperationTypeColor(const Movement& move) const {
    // Professional CAM color scheme based on operation types
    switch (move.operationType) {
        case OperationType::Facing:
            return Quantity_Color(0.0, 0.8, 0.2, Quantity_TOC_RGB);     // Bright Green - establishing reference
        case OperationType::ExternalRoughing:
            return Quantity_Color(0.9, 0.1, 0.1, Quantity_TOC_RGB);     // Red - heavy material removal
        case OperationType::InternalRoughing:
            return Quantity_Color(0.7, 0.0, 0.3, Quantity_TOC_RGB);     // Dark Red - internal material removal
        case OperationType::ExternalFinishing:
            return Quantity_Color(0.0, 0.4, 0.9, Quantity_TOC_RGB);     // Blue - precision finishing
        case OperationType::InternalFinishing:
            return Quantity_Color(0.0, 0.6, 0.7, Quantity_TOC_RGB);     // Teal - internal precision finishing
        case OperationType::Drilling:
            return Quantity_Color(0.9, 0.9, 0.0, Quantity_TOC_RGB);     // Yellow - hole making
        case OperationType::Boring:
            return Quantity_Color(0.8, 0.8, 0.2, Quantity_TOC_RGB);     // Olive - hole enlarging
        case OperationType::ExternalGrooving:
            return Quantity_Color(0.9, 0.0, 0.9, Quantity_TOC_RGB);     // Magenta - external groove
        case OperationType::InternalGrooving:
            return Quantity_Color(0.7, 0.0, 0.7, Quantity_TOC_RGB);     // Purple - internal groove
        case OperationType::Chamfering:
            return Quantity_Color(0.0, 0.9, 0.9, Quantity_TOC_RGB);     // Cyan - edge breaking
        case OperationType::Threading:
            return Quantity_Color(0.5, 0.0, 0.9, Quantity_TOC_RGB);     // Purple-Blue - thread cutting
        case OperationType::Parting:
            return Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB);     // Orange - part separation
        case OperationType::Unknown:
        default:
            return getDefaultColor(move);                                // Fall back to move type color
    }
}

TopoDS_Shape ToolpathDisplayObject::createLineShape(const gp_Pnt& start, const gp_Pnt& end) const {
    Handle(Geom_Line) line = new Geom_Line(start, gp_Dir(gp_Vec(start, end)));
    return BRepBuilderAPI_MakeEdge(line, start, end).Shape();
}

TopoDS_Shape ToolpathDisplayObject::createArcShape(const gp_Pnt& start, const gp_Pnt& end, const gp_Pnt& center) const {
    // Create circular arc through start and end points with given center
    gp_Vec startVec(center, start);
    gp_Vec endVec(center, end);
    
    if (startVec.Magnitude() < Precision::Confusion() || endVec.Magnitude() < Precision::Confusion()) {
        return createLineShape(start, end);
    }
    
    double radius = startVec.Magnitude();
    gp_Ax2 axis(center, startVec.Crossed(endVec));
    
    Handle(Geom_Circle) circle = new Geom_Circle(axis, radius);
    return BRepBuilderAPI_MakeEdge(circle, start, end).Shape();
}

void ToolpathDisplayObject::updatePresentation() {
    needsUpdate_ = false;
    SetToUpdate();
}

void ToolpathDisplayObject::invalidateDisplay() {
    needsUpdate_ = true;
    SetToUpdate();
}

// ProfileDisplayObject Implementation
ProfileDisplayObject::ProfileDisplayObject(const LatheProfile::Profile2D& profile,
                                           const ProfileVisualizationSettings& settings)
    : AIS_InteractiveObject()
    , profile_(profile)
    , settings_(settings) {
    
    SetDisplayMode(static_cast<Standard_Integer>(ProfileDisplayMode::Lines));
}

void ProfileDisplayObject::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                   const Handle(Prs3d_Presentation)& thePrs,
                                   const Standard_Integer theMode) {
    
    thePrs->Clear();
    
    ProfileDisplayMode mode = static_cast<ProfileDisplayMode>(theMode);
    
    switch (mode) {
        case ProfileDisplayMode::Points:
            computePointsPresentation(thePrs);
            break;
        case ProfileDisplayMode::Lines:
            computeLinesPresentation(thePrs);
            break;
        case ProfileDisplayMode::Spline:
            computeSplinePresentation(thePrs);
            break;
        case ProfileDisplayMode::Features:
            computeFeaturesPresentation(thePrs);
            break;
    }
}

void ProfileDisplayObject::ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
                                            const Standard_Integer theMode) {
    
    Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
    
    // Iterate over profile segments instead of individual points
    for (const auto& segment : profile_.segments) {
        // Add selection for start point
        gp_Pnt startPnt(segment.start.x, 0.0, segment.start.z);
        Handle(Select3D_SensitivePoint) startSensPoint = new Select3D_SensitivePoint(owner, startPnt);
        theSelection->Add(startSensPoint);
        
        // Add selection for end point
        gp_Pnt endPnt(segment.end.x, 0.0, segment.end.z);
        Handle(Select3D_SensitivePoint) endSensPoint = new Select3D_SensitivePoint(owner, endPnt);
        theSelection->Add(endSensPoint);
        
        // Add selection for the segment itself
        Handle(Select3D_SensitiveSegment) sensSegment = new Select3D_SensitiveSegment(owner, startPnt, endPnt);
        theSelection->Add(sensSegment);
    }
}

TopoDS_Shape ProfileDisplayObject::createProfileWire() const {
    if (profile_.isEmpty()) {
        return TopoDS_Shape();
    }
    
    BRepBuilderAPI_MakeWire wireBuilder;
    
    for (const auto& segment : profile_.segments) {
        gp_Pnt start(segment.start.x, 0.0, segment.start.z);
        gp_Pnt end(segment.end.x, 0.0, segment.end.z);
        
        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(start, end).Edge();
        wireBuilder.Add(edge);
    }
    
    if (wireBuilder.IsDone()) {
        TopoDS_Wire wire = wireBuilder.Wire();
        return wire;
    }
    
    return TopoDS_Shape();
}

void ProfileDisplayObject::setProfile(const LatheProfile::Profile2D& profile) {
    profile_ = profile;
    SetToUpdate();
}

void ProfileDisplayObject::setVisualizationSettings(const ProfileVisualizationSettings& settings) {
    settings_ = settings;
    SetToUpdate();
}

void ProfileDisplayObject::highlightFeature(size_t featureIndex, bool highlight) {
    auto it = std::find(highlightedFeatures_.begin(), highlightedFeatures_.end(), featureIndex);
    
    if (highlight) {
        if (it == highlightedFeatures_.end()) {
            highlightedFeatures_.push_back(featureIndex);
        }
    } else {
        if (it != highlightedFeatures_.end()) {
            highlightedFeatures_.erase(it);
        }
    }
    
    SetToUpdate();
}

void ProfileDisplayObject::clearFeatureHighlights() {
    highlightedFeatures_.clear();
    SetToUpdate();
}

Handle(ProfileDisplayObject) ProfileDisplayObject::create(const LatheProfile::Profile2D& profile,
                                                          const ProfileVisualizationSettings& settings) {
    return new ProfileDisplayObject(profile, settings);
}

void ProfileDisplayObject::computePointsPresentation(const Handle(Prs3d_Presentation)& presentation) {
    if (profile_.isEmpty()) return;

    // Count total points (start and end of each segment)
    size_t totalPoints = profile_.segments.size() * 2;
    Handle(Graphic3d_ArrayOfPoints) points = new Graphic3d_ArrayOfPoints(static_cast<Standard_Integer>(totalPoints));

    for (const auto& segment : profile_.segments) {
        // Add start point
        gp_Pnt startPnt(segment.start.x, 0.0, segment.start.z);
        points->AddVertex(startPnt, settings_.profileColor);
        
        // Add end point
        gp_Pnt endPnt(segment.end.x, 0.0, segment.end.z);
        points->AddVertex(endPnt, settings_.profileColor);
    }
    
    Handle(Graphic3d_AspectMarker3d) markerAspect = new Graphic3d_AspectMarker3d();
    markerAspect->SetScale(settings_.pointSize);
    
    Handle(Graphic3d_Group) group = presentation->NewGroup();
    group->SetPrimitivesAspect(markerAspect);
    group->AddPrimitiveArray(points);
}

void ProfileDisplayObject::computeLinesPresentation(const Handle(Prs3d_Presentation)& presentation) {
    if (profile_.isEmpty()) return;

    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(static_cast<Standard_Integer>(profile_.segments.size() * 2));

    for (const auto& segment : profile_.segments) {
        gp_Pnt start(segment.start.x, 0.0, segment.start.z);
        gp_Pnt end(segment.end.x, 0.0, segment.end.z);
        
        segments->AddVertex(start, settings_.profileColor);
        segments->AddVertex(end, settings_.profileColor);
    }
    
    Handle(Graphic3d_AspectLine3d) lineAspect = new Graphic3d_AspectLine3d();
    lineAspect->SetWidth(settings_.lineWidth);
    
    Handle(Graphic3d_Group) group = presentation->NewGroup();
    group->SetPrimitivesAspect(lineAspect);
    group->AddPrimitiveArray(segments);
    
    // Add points if enabled
    if (settings_.showPoints) {
        computePointsPresentation(presentation);
    }
}

void ProfileDisplayObject::computeSplinePresentation(const Handle(Prs3d_Presentation)& presentation) {
    computeLinesPresentation(presentation);
}

void ProfileDisplayObject::computeFeaturesPresentation(const Handle(Prs3d_Presentation)& presentation) {
    // For now, just use lines presentation
    // TODO: In the future, we could analyze ProfileSegment.isLinear to highlight curved vs linear segments
    computeLinesPresentation(presentation);
    
    // Optional: Add special highlighting for curved segments
    if (settings_.showFeatures) {
        Handle(Graphic3d_ArrayOfPoints) featurePoints = new Graphic3d_ArrayOfPoints(static_cast<Standard_Integer>(profile_.segments.size()));
        
        for (const auto& segment : profile_.segments) {
            if (!segment.isLinear) {
                // Highlight curved segments with special markers
                gp_Pnt midPoint(
                    (segment.start.x + segment.end.x) / 2.0,
                    0.0,
                    (segment.start.z + segment.end.z) / 2.0
                );
                featurePoints->AddVertex(midPoint, settings_.featureColor);
            }
        }
        
        if (featurePoints->VertexNumber() > 0) {
            Handle(Graphic3d_AspectMarker3d) markerAspect = new Graphic3d_AspectMarker3d();
            markerAspect->SetScale(settings_.pointSize * 1.5);
            markerAspect->SetColor(settings_.featureColor);
            
            Handle(Graphic3d_Group) group = presentation->NewGroup();
            group->SetPrimitivesAspect(markerAspect);
            group->AddPrimitiveArray(featurePoints);
        }
    }
}

// Remove deprecated methods that used old ProfileExtractor types
// These are no longer needed with the segment-based approach

// TopoDS_Shape ProfileDisplayObject::createFeatureMarker(const ProfileExtractor::ProfilePoint& point) const {
//     // DEPRECATED - removed because ProfileExtractor::ProfilePoint no longer exists
// }

// Quantity_Color ProfileDisplayObject::getFeatureColor(ProfileExtractor::FeatureType featureType) const {
//     // DEPRECATED - removed because ProfileExtractor::FeatureType no longer exists  
// }

// ToolpathDisplayFactory Implementation
Handle(ToolpathDisplayObject) ToolpathDisplayFactory::createToolpathDisplay(
    std::shared_ptr<Toolpath> toolpath,
    const std::string& operationType,
    const ToolpathDisplayObject::VisualizationSettings& settings) {
    
    ToolpathDisplayObject::VisualizationSettings finalSettings = settings;
    
    // Apply operation-specific defaults
    if (operationType == "roughing") {
        finalSettings = getRoughingVisualization();
    } else if (operationType == "finishing") {
        finalSettings = getFinishingVisualization();
    } else if (operationType == "parting") {
        finalSettings = getPartingVisualization();
    } else if (operationType == "threading") {
        finalSettings = getThreadingVisualization();
    }
    
    return ToolpathDisplayObject::create(toolpath, finalSettings);
}

Handle(ProfileDisplayObject) ToolpathDisplayFactory::createProfileDisplay(
    const LatheProfile::Profile2D& profile,
    const ProfileDisplayObject::ProfileVisualizationSettings& settings) {
    
    return ProfileDisplayObject::create(profile, settings);
}

ToolpathDisplayObject::VisualizationSettings ToolpathDisplayFactory::getRoughingVisualization() {
    ToolpathDisplayObject::VisualizationSettings settings;
    settings.lineWidth = 3.0;
    settings.rapidLineWidth = 1.0;
    settings.cutLineWidth = 4.0;
    settings.colorScheme = ToolpathDisplayObject::ColorScheme::Default;
    settings.showRapidMoves = true;
    settings.showFeedMoves = true;
    return settings;
}

ToolpathDisplayObject::VisualizationSettings ToolpathDisplayFactory::getFinishingVisualization() {
    ToolpathDisplayObject::VisualizationSettings settings;
    settings.lineWidth = 2.0;
    settings.rapidLineWidth = 1.0;
    settings.cutLineWidth = 3.0;
    settings.colorScheme = ToolpathDisplayObject::ColorScheme::DepthBased;
    settings.showRapidMoves = false; // Hide rapids for finishing
    settings.showFeedMoves = true;
    return settings;
}

ToolpathDisplayObject::VisualizationSettings ToolpathDisplayFactory::getPartingVisualization() {
    ToolpathDisplayObject::VisualizationSettings settings;
    settings.lineWidth = 4.0;
    settings.rapidLineWidth = 2.0;
    settings.cutLineWidth = 5.0;
    settings.colorScheme = ToolpathDisplayObject::ColorScheme::Default;
    settings.showStartPoint = true;
    settings.showEndPoint = true;
    return settings;
}

ToolpathDisplayObject::VisualizationSettings ToolpathDisplayFactory::getThreadingVisualization() {
    ToolpathDisplayObject::VisualizationSettings settings;
    settings.lineWidth = 2.0;
    settings.rapidLineWidth = 1.0;
    settings.cutLineWidth = 3.0;
    settings.colorScheme = ToolpathDisplayObject::ColorScheme::Rainbow;
    settings.showRapidMoves = true;
    settings.showFeedMoves = true;
    return settings;
}

ProfileDisplayObject::ProfileVisualizationSettings ToolpathDisplayFactory::getAnalysisProfileVisualization() {
    ProfileDisplayObject::ProfileVisualizationSettings settings;
    settings.pointSize = 2.0;
    settings.lineWidth = 2.0;
    settings.showPoints = true;
    settings.showLines = true;
    settings.showFeatures = true;
    settings.displayMode = ProfileDisplayObject::ProfileDisplayMode::Features;
    return settings;
}

ProfileDisplayObject::ProfileVisualizationSettings ToolpathDisplayFactory::getEditingProfileVisualization() {
    ProfileDisplayObject::ProfileVisualizationSettings settings;
    settings.pointSize = 4.0;
    settings.lineWidth = 2.0;
    settings.showPoints = true;
    settings.showLines = true;
    settings.showFeatures = false;
    settings.displayMode = ProfileDisplayObject::ProfileDisplayMode::Lines;
    return settings;
}

} // namespace Toolpath
} // namespace IntuiCAM 