#ifndef TOOLPATHMANAGER_H
#define TOOLPATHMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QColor>

// OpenCASCADE includes
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Builder.hxx>
#include <gp_Pnt.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Quantity_Color.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <gp_Trsf.hxx>

// Core includes
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>

// Forward declarations
class WorkpieceManager;

/**
 * @brief Manager for displaying toolpaths in the 3D view
 * 
 * This class handles:
 * - Conversion of Toolpath objects to OpenCASCADE visualization objects
 * - Different display styles for different movement types (rapid, cutting, etc.)
 * - Toolpath visibility and color management
 */
class ToolpathManager : public QObject
{
    Q_OBJECT

public:
    enum class DisplayMode {
        All,            // Show all toolpath types
        CuttingOnly,    // Show only cutting movements
        RapidOnly       // Show only rapid movements
    };

    enum class MovementStyleMode {
        Standard,       // Standard visualization style
        ColorCoded,     // Color-coded by operation type
        Animated        // Animated toolpath (if supported)
    };

    struct ToolpathDisplaySettings {
        QColor rapidColor = QColor(255, 0, 0);        // Red
        QColor cuttingColor = QColor(0, 128, 255);    // Blue
        double rapidLineWidth = 1.0;
        double cuttingLineWidth = 2.0;
        bool showPoints = true;
        double pointSize = 2.0;
        DisplayMode displayMode = DisplayMode::All;
        MovementStyleMode styleMode = MovementStyleMode::ColorCoded;
    };

public:
    explicit ToolpathManager(QObject *parent = nullptr);
    ~ToolpathManager();

    /**
     * @brief Initialize with AIS context
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Set the workpiece manager to get transformations from
     */
    void setWorkpieceManager(WorkpieceManager* workpieceManager);

    /**
     * @brief Display a toolpath in the 3D view
     * @param toolpath The toolpath to display
     * @param name A unique identifier for this toolpath
     * @return True if displayed successfully
     */
    bool displayToolpath(const IntuiCAM::Toolpath::Toolpath& toolpath, const QString& name);

    /**
     * @brief Display multiple toolpaths
     * @param toolpaths Vector of toolpaths to display
     * @param baseName Base name for the toolpaths, will be appended with indices
     * @return Number of successfully displayed toolpaths
     */
    int displayToolpaths(const std::vector<std::shared_ptr<IntuiCAM::Toolpath::Toolpath>>& toolpaths, 
                         const QString& baseName);

    /**
     * @brief Remove a displayed toolpath
     * @param name The unique identifier for the toolpath to remove
     */
    void removeToolpath(const QString& name);

    /**
     * @brief Clear all displayed toolpaths
     */
    void clearAllToolpaths();

    /**
     * @brief Hide/show a specific toolpath
     * @param name The unique identifier for the toolpath
     * @param visible Whether the toolpath should be visible
     */
    void setToolpathVisible(const QString& name, bool visible);

    /**
     * @brief Set display settings for toolpaths
     */
    void setDisplaySettings(const ToolpathDisplaySettings& settings);

    /**
     * @brief Get current display settings
     */
    const ToolpathDisplaySettings& getDisplaySettings() const { return m_displaySettings; }

    /**
     * @brief Update all toolpath visualizations with current settings
     */
    void updateAllToolpathVisualizations();

    /**
     * @brief Apply current workpiece transformation to all toolpaths
     * This ensures toolpaths are displayed in the correct position
     * relative to the transformed workpiece
     */
    void applyWorkpieceTransformationToToolpaths();

    // Display a 2-D lathe profile (radius,Z) as a wireframe overlay.
    bool displayLatheProfile(const std::vector<IntuiCAM::Geometry::Point2D>& profile,
                             const QString& name);

signals:
    /**
     * @brief Emitted when a toolpath is displayed
     */
    void toolpathDisplayed(const QString& name);

    /**
     * @brief Emitted when a toolpath is removed
     */
    void toolpathRemoved(const QString& name);

    /**
     * @brief Emitted when all toolpaths are cleared
     */
    void allToolpathsCleared();

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& message);

private:
    /**
     * @brief Convert toolpath to visualization shape
     * @param toolpath The toolpath to convert
     * @return The shape for visualization
     */
    TopoDS_Shape createToolpathShape(const IntuiCAM::Toolpath::Toolpath& toolpath);

    /**
     * @brief Create a wire representing a toolpath segment
     * @param movements The movements to include in the wire
     * @param startIdx The starting movement index
     * @param endIdx The ending movement index (inclusive)
     * @return A wire shape for visualization
     */
    TopoDS_Shape createToolpathSegment(const std::vector<IntuiCAM::Toolpath::Movement>& movements,
                                      size_t startIdx, size_t endIdx);

    /**
     * @brief Set display properties for a toolpath AIS object
     * @param aisObject The AIS object to configure
     * @param isRapid Whether this is a rapid movement
     */
    void setToolpathDisplayProperties(Handle(AIS_Shape) aisObject, bool isRapid);

    /**
     * @brief Get the appropriate display color for a movement
     * @param movement The movement to get a color for
     * @return The Quantity_Color to use
     */
    Quantity_Color getMovementColor(const IntuiCAM::Toolpath::Movement& movement);

    /**
     * @brief Get the current workpiece transformation
     * @return The transformation to apply to toolpaths
     */
    gp_Trsf getWorkpieceTransformation() const;

private:
    Handle(AIS_InteractiveContext) m_context;
    QMap<QString, Handle(AIS_Shape)> m_displayedToolpaths;
    // Store original untransformed toolpath shapes to avoid accumulating transformations
    QMap<QString, TopoDS_Shape> m_originalToolpathShapes;
    ToolpathDisplaySettings m_displaySettings;
    WorkpieceManager* m_workpieceManager;
};

#endif // TOOLPATHMANAGER_H 