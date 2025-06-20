#ifndef AISPROFILEDISPLAY_H
#define AISPROFILEDISPLAY_H

#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager.hxx>
#include <SelectMgr_Selection.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <vector>
#include <IntuiCAM/Toolpath/LatheProfile.h>

DEFINE_STANDARD_HANDLE(AIS_ProfileDisplay, AIS_InteractiveObject)

/**
 * @brief AIS object for displaying 2D lathe profiles in 3D space
 * 
 * This class creates a visual representation of the extracted 2D profile
 * from a turned part. The profile is displayed as a wireframe curve in
 * the XZ plane (lathe coordinate system) and can be positioned to match
 * the part's current location in the workspace.
 */
class AIS_ProfileDisplay : public AIS_InteractiveObject
{
    DEFINE_STANDARD_RTTIEXT(AIS_ProfileDisplay, AIS_InteractiveObject)

public:
    /**
     * @brief Constructor with profile data
     * @param profile 2D profile points (radius, z-coordinate)
     */
    Standard_EXPORT AIS_ProfileDisplay(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile);
    
    /**
     * @brief Virtual destructor
     */
    Standard_EXPORT virtual ~AIS_ProfileDisplay() = default;

    /**
     * @brief Update the profile data
     * @param profile New profile points to display
     */
    Standard_EXPORT void SetProfile(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile);

    /**
     * @brief Set the transformation to position the profile in 3D space
     * @param transform Transformation matrix to apply
     */
    Standard_EXPORT void SetTransformation(const gp_Trsf& transform);

    /**
     * @brief Set the profile color
     * @param color Color for the profile lines
     */
    Standard_EXPORT void SetProfileColor(const Quantity_Color& color);

    /**
     * @brief Set the profile line width
     * @param width Line width in pixels
     */
    Standard_EXPORT void SetLineWidth(Standard_Real width);

    /**
     * @brief Enable or disable profile visibility
     * @param visible True to show profile, false to hide
     */
    Standard_EXPORT void SetVisible(Standard_Boolean visible);

    /**
     * @brief Check if profile is currently visible
     * @return True if visible, false if hidden
     */
    Standard_EXPORT Standard_Boolean IsVisible() const { return m_isVisible; }

protected:
    /**
     * @brief Compute the 3D presentation of the profile
     */
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                       const Handle(Prs3d_Presentation)& thePrs,
                                       const Standard_Integer theMode) override;

    /**
     * @brief Compute selection entities (optional - profiles may not need selection)
     */
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                 const Standard_Integer theMode) override;

    /**
     * @brief Accept display modes (0 = wireframe)
     */
    Standard_EXPORT virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override;

private:
    /**
     * @brief Create the profile geometry as line segments
     */
    Handle(Graphic3d_ArrayOfSegments) createProfileGeometry();

    /**
     * @brief Update the presentation when profile data changes
     */
    void updatePresentation();

    // Member variables
    IntuiCAM::Toolpath::LatheProfile::Profile2D m_profile;  ///< Profile data points
    gp_Trsf m_transformation;                               ///< Positioning transformation
    Quantity_Color m_profileColor;                          ///< Profile line color
    Standard_Real m_lineWidth;                              ///< Line width
    Standard_Boolean m_isVisible;                           ///< Visibility flag
    Standard_Boolean m_needsUpdate;                         ///< Flag for presentation update
};

#endif // AISPROFILEDISPLAY_H 