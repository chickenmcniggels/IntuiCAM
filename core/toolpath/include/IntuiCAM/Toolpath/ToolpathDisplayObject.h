#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Toolpath/ProfileExtractor.h>
#include <IntuiCAM/Geometry/Types.h>

#include <AIS_InteractiveObject.hxx>
#include <AIS_ColoredShape.hxx>
#include <Prs3d_Presentation.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <memory>
#include <vector>
#include <string>

// Forward declarations
namespace IntuiCAM {
namespace Toolpath {
    class ProfileExtractor;
    using ToolpathMoveType = MovementType; // Alias for compatibility
}
}

namespace IntuiCAM {
namespace Toolpath {

class ToolpathDisplayObject : public AIS_InteractiveObject {
    DEFINE_STANDARD_RTTIEXT(ToolpathDisplayObject, AIS_InteractiveObject)

public:
    enum class DisplayMode {
        Wireframe = 0,
        Shaded = 1,
        RapidMoves = 2,
        FeedMoves = 3,
        CuttingMoves = 4,
        AllMoves = 5
    };

    enum class ColorScheme {
        Default,        // Standard CAM colors
        Rainbow,        // Color by speed/feed
        DepthBased,    // Color by Z depth
        OperationType, // Color by operation type
        Tool,          // Color by tool
        Material       // Color by material removal
    };

    struct VisualizationSettings {
        double lineWidth = 2.0;
        double rapidLineWidth = 1.0;
        double cutLineWidth = 3.0;
        bool showRapidMoves = true;
        bool showFeedMoves = true;
        bool showToolPath = true;
        bool showStartPoint = true;
        bool showEndPoint = true;
        bool animateProgress = false;
        double animationSpeed = 1.0;
        ColorScheme colorScheme = ColorScheme::Default;
        double transparency = 0.0;
    };

    // Constructor
    explicit ToolpathDisplayObject(std::shared_ptr<Toolpath> toolpath,
                                   const VisualizationSettings& settings = VisualizationSettings{});

    // Standard AIS methods
    void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                 const Handle(Prs3d_Presentation)& thePrs,
                 const Standard_Integer theMode = 0) override;

    void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
                          const Standard_Integer theMode) override;

    // Toolpath specific methods
    void setToolpath(std::shared_ptr<Toolpath> toolpath);
    std::shared_ptr<Toolpath> getToolpath() const { return toolpath_; }

    void setVisualizationSettings(const VisualizationSettings& settings);
    const VisualizationSettings& getVisualizationSettings() const { return settings_; }

    // Display control
    void setVisible(bool visible);
    bool isVisible() const { return isVisible_; }

    void setProgress(double progress); // 0.0 to 1.0 for animation
    double getProgress() const { return progress_; }

    // Coordinate transformation
    void setTransform(const gp_Trsf& trsf) { transform_ = trsf; }
    const gp_Trsf& getTransform() const { return transform_; }

    // Color management
    void setColorScheme(ColorScheme scheme);
    void setCustomColor(const Quantity_Color& color);
    Quantity_Color getColorForMove(const Movement& move, size_t moveIndex) const;

    // Statistics
    struct DisplayStatistics {
        size_t totalMoves = 0;
        size_t rapidMoves = 0;
        size_t feedMoves = 0;
        size_t cuttingMoves = 0;
        double totalLength = 0.0;
        double cuttingLength = 0.0;
        double minZ = 0.0;
        double maxZ = 0.0;
        gp_Pnt boundingBoxMin;
        gp_Pnt boundingBoxMax;
    };

    DisplayStatistics calculateStatistics() const;

    // Utility methods
    static Handle(ToolpathDisplayObject) create(std::shared_ptr<Toolpath> toolpath,
                                                const VisualizationSettings& settings = VisualizationSettings{});

    // Selection and highlighting
    void highlightMove(size_t moveIndex, bool highlight = true);
    void clearHighlights();
    std::vector<size_t> getSelectedMoves() const { return selectedMoves_; }

private:
    // Core data
    std::shared_ptr<Toolpath> toolpath_;
    VisualizationSettings settings_;
    bool isVisible_;
    double progress_; // For animation
    
    // Display state
    std::vector<size_t> selectedMoves_;
    bool needsUpdate_;

    // Optional transformation from work coordinates to global viewer coordinates
    gp_Trsf transform_;
    
    // Computed geometry
    std::vector<Handle(AIS_InteractiveObject)> moveObjects_;
    Handle(AIS_InteractiveObject) startPointMarker_;
    Handle(AIS_InteractiveObject) endPointMarker_;
    
    // Internal methods
    void computeWireframePresentation(const Handle(Prs3d_Presentation)& presentation);
    void computeShadedPresentation(const Handle(Prs3d_Presentation)& presentation);
    void computeMoveTypePresentation(const Handle(Prs3d_Presentation)& presentation, DisplayMode mode);
    
    void createMoveGeometry();
    Handle(AIS_InteractiveObject) createMoveObject(const Movement& move, size_t moveIndex);
    Handle(AIS_InteractiveObject) createPointMarker(const gp_Pnt& point, const Quantity_Color& color);
    
    // Color calculation
    Quantity_Color getDefaultColor(const Movement& move) const;
    Quantity_Color getRainbowColor(double value, double min, double max) const;
    Quantity_Color getDepthBasedColor(double z, double minZ, double maxZ) const;
    Quantity_Color getOperationTypeColor(const Movement& move) const;
    
    // Geometry utilities
    TopoDS_Shape createLineShape(const gp_Pnt& start, const gp_Pnt& end) const;
    TopoDS_Shape createArcShape(const gp_Pnt& start, const gp_Pnt& end, const gp_Pnt& center) const;
    
    void updatePresentation();
    void invalidateDisplay();
};

// Profile Display Object for 2D profile visualization
class ProfileDisplayObject : public AIS_InteractiveObject {
    DEFINE_STANDARD_RTTIEXT(ProfileDisplayObject, AIS_InteractiveObject)

public:
    enum class ProfileDisplayMode {
        Points = 0,
        Lines = 1,
        Spline = 2,
        Features = 3
    };

    struct ProfileVisualizationSettings {
        double pointSize = 3.0;
        double lineWidth = 2.0;
        bool showPoints = true;
        bool showLines = true;
        bool showFeatures = true;
        bool showDimensions = false;
        ProfileDisplayMode displayMode = ProfileDisplayMode::Lines;
        Quantity_Color profileColor = Quantity_Color(0.2, 0.7, 0.9, Quantity_TOC_RGB);
        Quantity_Color featureColor = Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB);
        double transparency = 0.1;
    };

    // Constructor
    explicit ProfileDisplayObject(const LatheProfile::Profile2D& profile,
                                  const ProfileVisualizationSettings& settings = ProfileVisualizationSettings{});

    // Standard AIS methods
    void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                 const Handle(Prs3d_Presentation)& thePrs,
                 const Standard_Integer theMode = 0) override;

    void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
                          const Standard_Integer theMode) override;

    // Profile specific methods
    void setProfile(const LatheProfile::Profile2D& profile);
    const LatheProfile::Profile2D& getProfile() const { return profile_; }

    void setVisualizationSettings(const ProfileVisualizationSettings& settings);
    const ProfileVisualizationSettings& getVisualizationSettings() const { return settings_; }

    // Feature highlighting
    void highlightFeature(size_t featureIndex, bool highlight = true);
    void clearFeatureHighlights();

    // Utility methods
    static Handle(ProfileDisplayObject) create(const LatheProfile::Profile2D& profile,
                                               const ProfileVisualizationSettings& settings = ProfileVisualizationSettings{});

private:
    LatheProfile::Profile2D profile_;
    ProfileVisualizationSettings settings_;
    std::vector<size_t> highlightedFeatures_;
    
    void computePointsPresentation(const Handle(Prs3d_Presentation)& presentation);
    void computeLinesPresentation(const Handle(Prs3d_Presentation)& presentation);
    void computeSplinePresentation(const Handle(Prs3d_Presentation)& presentation);
    void computeFeaturesPresentation(const Handle(Prs3d_Presentation)& presentation);
    
    TopoDS_Shape createProfileWire() const;
    TopoDS_Shape createFeatureMarker(const ProfileExtractor::ProfilePoint& point) const;
    Quantity_Color getFeatureColor(ProfileExtractor::FeatureType featureType) const;
};

// Factory class for creating display objects
class ToolpathDisplayFactory {
public:
    static Handle(ToolpathDisplayObject) createToolpathDisplay(
        std::shared_ptr<Toolpath> toolpath,
        const std::string& operationType = "",
        const ToolpathDisplayObject::VisualizationSettings& settings = ToolpathDisplayObject::VisualizationSettings{});

    static Handle(ProfileDisplayObject) createProfileDisplay(
        const LatheProfile::Profile2D& profile,
        const ProfileDisplayObject::ProfileVisualizationSettings& settings = ProfileDisplayObject::ProfileVisualizationSettings{});

    // Predefined visualization configurations
    static ToolpathDisplayObject::VisualizationSettings getRoughingVisualization();
    static ToolpathDisplayObject::VisualizationSettings getFinishingVisualization();
    static ToolpathDisplayObject::VisualizationSettings getPartingVisualization();
    static ToolpathDisplayObject::VisualizationSettings getThreadingVisualization();

    static ProfileDisplayObject::ProfileVisualizationSettings getAnalysisProfileVisualization();
    static ProfileDisplayObject::ProfileVisualizationSettings getEditingProfileVisualization();
};

} // namespace Toolpath
} // namespace IntuiCAM 