#ifndef AISTOOLPATHDISPLAY_H
#define AISTOOLPATHDISPLAY_H

#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager.hxx>
#include <SelectMgr_Selection.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <vector>
#include <memory>

// Forward declarations for IntuiCAM types
namespace IntuiCAM {
namespace Toolpath {
    class Toolpath;
    enum class MoveType;
}
}

DEFINE_STANDARD_HANDLE(AIS_ToolpathDisplay, AIS_InteractiveObject)

/**
 * @brief AIS object for displaying CNC toolpaths in 3D space
 * 
 * This class creates a visual representation of generated toolpaths with
 * color coding for different move types (rapid, feed, etc.) and proper
 * positioning to match the part and chuck setup in the workspace.
 */
class AIS_ToolpathDisplay : public AIS_InteractiveObject
{
    DEFINE_STANDARD_RTTIEXT(AIS_ToolpathDisplay, AIS_InteractiveObject)

public:
    /**
     * @brief Toolpath move data for visualization
     */
    struct ToolpathMove {
        gp_Pnt startPoint;
        gp_Pnt endPoint;
        IntuiCAM::Toolpath::MoveType moveType;
        double feedRate;        ///< Feed rate for this move
        double spindleSpeed;    ///< Spindle speed for this move
        
        ToolpathMove(const gp_Pnt& start, const gp_Pnt& end, 
                    IntuiCAM::Toolpath::MoveType type,
                    double feed = 0.0, double spindle = 0.0)
            : startPoint(start), endPoint(end), moveType(type)
            , feedRate(feed), spindleSpeed(spindle) {}
    };

    /**
     * @brief Constructor with toolpath data
     * @param toolpath Toolpath object containing moves and metadata
     * @param operationType Type of operation (facing, roughing, finishing, etc.)
     */
    Standard_EXPORT AIS_ToolpathDisplay(std::shared_ptr<IntuiCAM::Toolpath::Toolpath> toolpath,
                                       const std::string& operationType);
    
    /**
     * @brief Virtual destructor
     */
    Standard_EXPORT virtual ~AIS_ToolpathDisplay() = default;

    /**
     * @brief Update the toolpath data
     * @param toolpath New toolpath to display
     */
    Standard_EXPORT void SetToolpath(std::shared_ptr<IntuiCAM::Toolpath::Toolpath> toolpath);

    /**
     * @brief Set the transformation to position the toolpath in 3D space
     * @param transform Transformation matrix to apply
     */
    Standard_EXPORT void SetTransformation(const gp_Trsf& transform);

    /**
     * @brief Set the operation type for appropriate color coding
     * @param operationType Type of operation (affects colors)
     */
    Standard_EXPORT void SetOperationType(const std::string& operationType);

    /**
     * @brief Enable or disable toolpath visibility
     * @param visible True to show toolpath, false to hide
     */
    Standard_EXPORT void SetVisible(Standard_Boolean visible);

    /**
     * @brief Check if toolpath is currently visible
     * @return True if visible, false if hidden
     */
    Standard_EXPORT Standard_Boolean IsVisible() const { return m_isVisible; }

    /**
     * @brief Set custom colors for different move types
     * @param rapidColor Color for rapid moves
     * @param feedColor Color for feed moves
     * @param plungeColor Color for plunge moves
     */
    Standard_EXPORT void SetMoveColors(const Quantity_Color& rapidColor,
                                      const Quantity_Color& feedColor,
                                      const Quantity_Color& plungeColor);

    /**
     * @brief Set line width for toolpath display
     * @param width Line width in pixels
     */
    Standard_EXPORT void SetLineWidth(Standard_Real width);

    /**
     * @brief Get the operation type
     * @return Current operation type string
     */
    Standard_EXPORT const std::string& GetOperationType() const { return m_operationType; }

    /**
     * @brief Get toolpath statistics for display
     */
    struct ToolpathStats {
        int totalMoves;
        int rapidMoves;
        int feedMoves;
        double totalLength;     ///< Total toolpath length (mm)
        double estimatedTime;   ///< Estimated machining time (minutes)
    };
    Standard_EXPORT ToolpathStats GetStats() const;

protected:
    /**
     * @brief Compute the 3D presentation of the toolpath
     */
    Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                       const Handle(Prs3d_Presentation)& thePrs,
                                       const Standard_Integer theMode) override;

    /**
     * @brief Compute selection entities for toolpath selection
     */
    Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                 const Standard_Integer theMode) override;

    /**
     * @brief Accept display modes (0 = wireframe)
     */
    Standard_EXPORT virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override;

private:
    /**
     * @brief Convert toolpath data to visualization moves
     */
    void convertToolpathToMoves();

    /**
     * @brief Create geometry for rapid moves
     */
    Handle(Graphic3d_ArrayOfSegments) createRapidGeometry();

    /**
     * @brief Create geometry for feed moves
     */
    Handle(Graphic3d_ArrayOfSegments) createFeedGeometry();

    /**
     * @brief Create geometry for plunge moves
     */
    Handle(Graphic3d_ArrayOfSegments) createPlungeGeometry();

    /**
     * @brief Get color for operation type
     */
    Quantity_Color getOperationColor() const;

    /**
     * @brief Calculate toolpath statistics
     */
    void calculateStats();

    // Member variables
    std::shared_ptr<IntuiCAM::Toolpath::Toolpath> m_toolpath;   ///< Source toolpath data
    std::vector<ToolpathMove> m_moves;                          ///< Converted move data
    std::string m_operationType;                                ///< Operation type (facing, roughing, etc.)
    gp_Trsf m_transformation;                                   ///< Positioning transformation
    
    // Visual properties
    Quantity_Color m_rapidColor;        ///< Color for rapid moves
    Quantity_Color m_feedColor;         ///< Color for feed moves
    Quantity_Color m_plungeColor;       ///< Color for plunge moves
    Standard_Real m_lineWidth;          ///< Line width
    Standard_Boolean m_isVisible;       ///< Visibility flag
    Standard_Boolean m_needsUpdate;     ///< Flag for presentation update
    
    // Statistics
    ToolpathStats m_stats;
};

#endif // AISTOOLPATHDISPLAY_H 