#include "aisprofiledisplay.h"
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <Quantity_NameOfColor.hxx>
#include <gp_Pnt.hxx>

IMPLEMENT_STANDARD_RTTIEXT(AIS_ProfileDisplay, AIS_InteractiveObject)

AIS_ProfileDisplay::AIS_ProfileDisplay(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile)
    : AIS_InteractiveObject()
    , m_profile(profile)
    , m_transformation()
    , m_profileColor(Quantity_NOC_BLUE1)
    , m_lineWidth(2.0)
    , m_isVisible(Standard_True)
    , m_needsUpdate(Standard_True)
{
    // Set default properties
    SetHilightMode(0); // Highlight in wireframe mode
    
    // Initialize drawer with appropriate line aspect
    myDrawer->SetWireAspect(new Prs3d_LineAspect(m_profileColor, Aspect_TOL_SOLID, m_lineWidth));
}

void AIS_ProfileDisplay::SetProfile(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile)
{
    m_profile = profile;
    m_needsUpdate = Standard_True;
    
    // Mark for redraw if object is already displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ProfileDisplay::SetTransformation(const gp_Trsf& transform)
{
    m_transformation = transform;
    
    // Apply transformation to the AIS object
    SetLocalTransformation(transform);
    
    // Update presentation if needed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ProfileDisplay::SetProfileColor(const Quantity_Color& color)
{
    m_profileColor = color;
    
    // Update the wire aspect with new color
    myDrawer->WireAspect()->SetColor(color);
    
    // Update presentation if object is displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ProfileDisplay::SetLineWidth(Standard_Real width)
{
    m_lineWidth = width;
    
    // Update the wire aspect with new width
    myDrawer->WireAspect()->SetWidth(width);
    
    // Update presentation if object is displayed
    if (!GetContext().IsNull()) {
        GetContext()->Redisplay(this, Standard_False);
    }
}

void AIS_ProfileDisplay::SetVisible(Standard_Boolean visible)
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

void AIS_ProfileDisplay::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                const Handle(Prs3d_Presentation)& thePrs,
                                const Standard_Integer theMode)
{
    // Only support wireframe mode (mode 0)
    if (theMode != 0) {
        return;
    }
    
    // Skip if profile is empty or not visible
    if (m_profile.empty() || !m_isVisible) {
        return;
    }
    
    // Create the profile geometry
    Handle(Graphic3d_ArrayOfSegments) profileSegments = createProfileGeometry();
    
    if (profileSegments.IsNull() || profileSegments->VertexNumber() < 2) {
        return;
    }
    
    // Create a group for the profile
    Handle(Graphic3d_Group) profileGroup = thePrs->NewGroup();
    
    // Set the line aspect for the group
    profileGroup->SetGroupPrimitivesAspect(myDrawer->WireAspect()->Aspect());
    
    // Add the profile segments to the group
    profileGroup->AddPrimitiveArray(profileSegments);
    
    m_needsUpdate = Standard_False;
}

void AIS_ProfileDisplay::ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                         const Standard_Integer theMode)
{
    // For now, profiles are display-only and don't support selection
    // This could be extended later if needed for interactive profile editing
}

Standard_Boolean AIS_ProfileDisplay::AcceptDisplayMode(const Standard_Integer theMode) const
{
    // Only support wireframe mode (mode 0)
    return theMode == 0;
}

Handle(Graphic3d_ArrayOfSegments) AIS_ProfileDisplay::createProfileGeometry()
{
    if (m_profile.size() < 2) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }
    
    // Create array for line segments - need (n-1) segments for n points
    Standard_Integer numSegments = static_cast<Standard_Integer>(m_profile.size() - 1);
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(
        static_cast<Standard_Integer>(m_profile.size()), // vertices
        numSegments * 2                                  // edges (2 indices per segment)
    );
    
    // Add vertices (convert from 2D profile to 3D points in XZ plane)
    // Profile points are (radius, z), convert to (x=radius, y=0, z=z)
    for (const auto& point : m_profile) {
        gp_Pnt vertex(point.x, 0.0, point.z);
        segments->AddVertex(vertex);
    }
    
    // Add line segments connecting consecutive points
    for (Standard_Integer i = 1; i <= numSegments; ++i) {
        segments->AddEdges(i, i + 1);
    }
    
    return segments;
}

void AIS_ProfileDisplay::updatePresentation()
{
    if (!GetContext().IsNull() && m_needsUpdate) {
        GetContext()->Redisplay(this, Standard_False);
        m_needsUpdate = Standard_False;
    }
} 