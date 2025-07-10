#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// OpenCASCADE includes
#include <AIS_InteractiveObject.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>

// IntuiCAM includes
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Toolpath/LatheProfile.h>
#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Forward declarations
class Tool;
class Toolpath;

/**
 * @brief New Toolpath Generation Pipeline following chronological CAM strategy
 *
 * This pipeline follows the exact sequence from the pseudocode:
 * 1. Facing (always first - establish reference surface)
 * 2. Internal features (drilling, boring, roughing, finishing, grooving)
 * 3. External roughing
 * 4. External finishing
 * 5. Chamfering
 * 6. Threading
 * 7. Parting (always last)
 */
class ToolpathGenerationPipeline {
public:
  // Feature detection structures
  struct DetectedFeature {
    std::string type; // "hole", "groove", "chamfer", "thread"
    double depth = 0.0;
    double diameter = 0.0;
    IntuiCAM::Geometry::Point3D coordinates;
    std::map<std::string, double> geometry; // Additional geometry parameters
    std::string tool;                       // Suggested tool type
    bool chamferEdges = false;
  };

  // Input parameters (Section 1 from pseudocode)
  struct PipelineInputs {
    // Extracted from part
    LatheProfile::Profile2D profile2D;

    // Raw material
    double rawMaterialDiameter = 20.0; // mm
    double rawMaterialLength = 50.0;   // mm

    // Datum and dimensions
    double z0 = 50.0;         // provisional datum (raw_material_length)
    double partLength = 40.0; // mm

    // Operation enablement flags
    bool machineInternalFeatures = true;
    bool drilling = true;
    bool internalRoughing = true;
    bool externalRoughing = true;
    bool internalFinishing = true;
    bool externalFinishing = true;
    bool internalGrooving = true;
    bool externalGrooving = true;
    bool chamfering = true;
    bool threading = true;
    bool facing = true;
    bool parting = true;

    // Operation parameters
    double largestDrillSize = 12.0;  // mm - diameters > this are bored
    double facingAllowance = 2.0;    // mm - distance raw-stock → part Z-max
    int internalFinishingPasses = 2; // number of finish passes
    int externalFinishingPasses = 2;
    double partingAllowance = 0.0; // mm

    // Auto-detected features
    std::vector<DetectedFeature> featuresToBeDrilled;
    std::vector<DetectedFeature> internalFeaturesToBeGrooved;
    std::vector<DetectedFeature> externalFeaturesToBeGrooved;
    std::vector<DetectedFeature> featuresToBeChamfered;
    std::vector<DetectedFeature> featuresToBeThreaded;

    // Tools (placeholders for now)
    std::string facingTool = "facing tool";
    std::string internalRoughingTool = "internal roughing tool";
    std::string externalRoughingTool = "external roughing tool";
    std::string internalFinishingTool = "internal finishing tool";
    std::string externalFinishingTool = "external finishing tool";
    std::string partingTool = "parting tool";
  };

  // Pipeline result containing timeline of operations
  struct PipelineResult {
    bool success = false;
    std::string errorMessage;
    std::vector<std::string> warnings;

    // Generated timeline (ordered list of toolpaths)
    std::vector<std::unique_ptr<Toolpath>> timeline;

    // Display objects for visualization
    std::vector<Handle(AIS_InteractiveObject)> toolpathDisplayObjects;
    Handle(AIS_InteractiveObject) profileDisplayObject;

    // Processing metadata
    std::chrono::milliseconds processingTime{0};
    std::string generationTimestamp;

    // Progress callback
    std::function<void(double, const std::string &)> progressCallback;
  };

public:
  ToolpathGenerationPipeline();
  virtual ~ToolpathGenerationPipeline() = default;

  /**
   * @brief Main pipeline execution following pseudocode logic
   * @param inputs Complete input parameters
   * @return Pipeline result with ordered timeline of toolpaths
   */
  PipelineResult executePipeline(const PipelineInputs &inputs);

  /**
   * @brief Extract inputs from part geometry and GUI settings
   * @param partGeometry 3D part to machine
   * @param turningAxis Main turning axis
   * @return Populated input structure
   */
  PipelineInputs extractInputsFromPart(const TopoDS_Shape &partGeometry,
                                       const gp_Ax1 &turningAxis);

  /**
   * @brief Auto-detect features from 2D profile
   * @param profile Profile to analyze
   * @return Detected features for machining
   */
  std::vector<DetectedFeature>
  detectFeatures(const LatheProfile::Profile2D &profile,
                 const TopoDS_Shape &partGeometry = TopoDS_Shape());

  // Cancel ongoing generation
  void cancelGeneration();
  bool isGenerating() const { return m_isGenerating; }

private:
  // STUB FUNCTIONS (Section 2 from pseudocode) - these will be implemented
  // later
  std::vector<std::unique_ptr<Toolpath>>
  facingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                 const IntuiCAM::Geometry::Point3D &startPos,
                 const IntuiCAM::Geometry::Point3D &endPos,
                 const std::string &toolData);

  std::vector<std::unique_ptr<Toolpath>>
  drillingToolpath(double depth, const std::string &toolData);

  std::vector<std::unique_ptr<Toolpath>>
  internalRoughingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                           const std::string &toolData,
                           const LatheProfile::Profile2D &profile);

  std::vector<std::unique_ptr<Toolpath>>
  externalRoughingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                           const std::string &toolData,
                           const LatheProfile::Profile2D &profile);

  std::vector<std::unique_ptr<Toolpath>>
  internalFinishingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                            const std::string &toolData,
                            const LatheProfile::Profile2D &profile);

  std::vector<std::unique_ptr<Toolpath>>
  externalFinishingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                            const std::string &toolData,
                            const LatheProfile::Profile2D &profile);

  std::vector<std::unique_ptr<Toolpath>>
  externalGroovingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                           const std::map<std::string, double> &grooveGeometry,
                           const std::string &toolData, bool chamferEdges);

  std::vector<std::unique_ptr<Toolpath>>
  internalGroovingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                           const std::map<std::string, double> &grooveGeometry,
                           const std::string &toolData, bool chamferEdges);

  std::vector<std::unique_ptr<Toolpath>>
  chamferingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                     const std::map<std::string, double> &chamferGeometry,
                     const std::string &toolData);

  std::vector<std::unique_ptr<Toolpath>>
  threadingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                    const std::map<std::string, double> &threadGeometry,
                    const std::string &toolData);

  std::vector<std::unique_ptr<Toolpath>>
  partingToolpath(const IntuiCAM::Geometry::Point3D &coordinates,
                  const std::string &toolData, bool chamferEdges);

public:
  // Public helper methods for toolpath display
  /// \brief Create display objects for a list of toolpaths.
  /// \param toolpaths          Generated toolpaths.
  /// \param workpieceTransform Transformation from toolpath coordinates to
  ///                            world coordinates (e.g. current workpiece
  ///                            position).
  /// \return Vector of AIS objects ready for display.
  std::vector<Handle(AIS_InteractiveObject)> createToolpathDisplayObjects(
      const std::vector<std::unique_ptr<Toolpath>> &toolpaths,
      const gp_Trsf &workpieceTransform = gp_Trsf());

private:
  // Generation state
  std::atomic<bool> m_isGenerating{false};
  std::atomic<bool> m_cancelRequested{false};

  // IMPROVED: Store actual part geometry for operations
  TopoDS_Shape m_currentPartGeometry;

  // Helper methods
  void reportProgress(double progress, const std::string &status,
                      const PipelineResult &result);

  // IMPROVED: Create Part object from stored geometry
  std::unique_ptr<IntuiCAM::Geometry::OCCTPart> createPartFromGeometry() const;
};

} // namespace Toolpath
} // namespace IntuiCAM