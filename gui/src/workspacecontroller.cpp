#define _USE_MATH_DEFINES
#include "workspacecontroller.h"
#include "chuckmanager.h"
#include "workpiecemanager.h"
#include "rawmaterialmanager.h"
#include <IntuiCAM/Geometry/IStepLoader.h>
#include <IntuiCAM/Toolpath/ToolpathGenerationPipeline.h>
#include <IntuiCAM/Toolpath/ToolpathDisplayObject.h>

#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenCASCADE includes
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Circ.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Precision.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

WorkspaceController::WorkspaceController(QObject *parent)
    : QObject(parent)
    , m_chuckManager(nullptr)
    , m_workpieceManager(nullptr)
    , m_rawMaterialManager(nullptr)
    , m_coordinateManager(nullptr)
    , m_stepLoader(nullptr)
    , m_initialized(false)
    , m_profileVisible(true)  // Initialize profile visibility to true
{
    // Create component managers
    m_chuckManager = new ChuckManager(this);
    m_workpieceManager = new WorkpieceManager(this);
    m_rawMaterialManager = new RawMaterialManager(this);
    m_coordinateManager = new WorkspaceCoordinateManager(this);
    
    // Set up signal connections
    setupManagerConnections();
    
    qDebug() << "WorkspaceController created with all component managers";
}

WorkspaceController::~WorkspaceController()
{
    // Qt handles cleanup of child objects automatically
}

void WorkspaceController::initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader)
{
    if (context.IsNull() || !stepLoader) {
        qDebug() << "WorkspaceController: Invalid context or stepLoader provided";
        emit errorOccurred("WorkspaceController", "Invalid initialization parameters");
        return;
    }
    
    m_context = context;
    m_stepLoader = stepLoader;
    
    // Initialize all component managers
    m_chuckManager->initialize(context, stepLoader);
    m_workpieceManager->initialize(context);
    m_rawMaterialManager->initialize(context);
    
    m_initialized = true;
    qDebug() << "WorkspaceController initialized successfully";
}

bool WorkspaceController::initializeChuck(const QString& chuckFilePath)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    qDebug() << "WorkspaceController: Initializing chuck from" << chuckFilePath;
    
    bool success = m_chuckManager->loadChuck(chuckFilePath);
    if (success) {
        emit chuckInitialized();
        qDebug() << "WorkspaceController: Chuck initialization completed successfully";
    } else {
        qDebug() << "WorkspaceController: Chuck initialization failed";
    }
    
    return success;
}

bool WorkspaceController::addWorkpiece(const TopoDS_Shape& workpiece)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    if (workpiece.IsNull()) {
        emit errorOccurred("WorkspaceController", "Invalid workpiece shape provided");
        return false;
    }
    
    qDebug() << "WorkspaceController: Processing workpiece workflow";
    
    try {
        executeWorkpieceWorkflow(workpiece);
        return true;
    } catch (const std::exception& e) {
        QString errorMsg = QString("Workpiece workflow failed: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

void WorkspaceController::clearWorkpieces()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "WorkspaceController: Clearing workpieces";
    
    // Clear profile display
    clearProfileDisplay();
    m_extractedProfile = IntuiCAM::Toolpath::LatheProfile::Profile2D();
    
    m_workpieceManager->clearWorkpieces();
    m_rawMaterialManager->clearRawMaterial();
    
    // Clear stored workpiece
    m_currentWorkpiece = TopoDS_Shape();
    
    qDebug() << "WorkspaceController: Workpieces cleared";
}

void WorkspaceController::clearWorkspace()
{
    if (!m_initialized) {
        return;
    }
    
    qDebug() << "WorkspaceController: Clearing entire workspace";
    
    // Clear profile display
    clearProfileDisplay();
    m_extractedProfile = IntuiCAM::Toolpath::LatheProfile::Profile2D();
    
    m_chuckManager->clearChuck();
    m_workpieceManager->clearWorkpieces();
    m_rawMaterialManager->clearRawMaterial();
    
    // Clear stored workpiece
    m_currentWorkpiece = TopoDS_Shape();
    
    emit workspaceCleared();
    qDebug() << "WorkspaceController: Workspace cleared completely";
}

bool WorkspaceController::isInitialized() const
{
    return m_initialized;
}

bool WorkspaceController::isChuckLoaded() const
{
    return m_initialized && m_chuckManager && m_chuckManager->isChuckLoaded();
}

void WorkspaceController::setupManagerConnections()
{
    // Connect error signals from all managers
    connect(m_chuckManager, &ChuckManager::errorOccurred,
            this, &WorkspaceController::handleChuckError);
    
    connect(m_workpieceManager, &WorkpieceManager::errorOccurred,
            this, &WorkspaceController::handleWorkpieceError);
    
    connect(m_rawMaterialManager, &RawMaterialManager::errorOccurred,
            this, &WorkspaceController::handleRawMaterialError);
    
    // Connect cylinder detection for automated workflow
    connect(m_workpieceManager, &WorkpieceManager::cylinderDetected,
            this, &WorkspaceController::handleCylinderDetected);
    
    // Connect chuck centerline detection
    connect(m_chuckManager, &ChuckManager::chuckCenterlineDetected,
            this, &WorkspaceController::handleChuckCenterlineDetected);
    
    // Connect multiple cylinders detection for manual selection
    connect(m_workpieceManager, &WorkpieceManager::multipleCylindersDetected,
            this, &WorkspaceController::handleMultipleCylindersDetected);
    
    // Connect manual cylinder axis selection
    connect(m_workpieceManager, &WorkpieceManager::cylinderAxisSelected,
            this, &WorkspaceController::handleCylinderAxisSelected);
    
    qDebug() << "WorkspaceController: Manager signal connections established";
}

void WorkspaceController::executeWorkpieceWorkflow(const TopoDS_Shape& workpiece)
{
    // Store the original workpiece shape for later re-processing
    m_currentWorkpiece = workpiece;
    
    // Step 1: Add workpiece to scene
    bool workpieceAdded = m_workpieceManager->addWorkpiece(workpiece);
    if (!workpieceAdded) {
        emit errorOccurred("WorkspaceController", "Failed to add workpiece to scene");
        return;
    }
    
    // Step 2: Analyze workpiece geometry for cylinders
    QVector<gp_Ax1> cylinders = m_workpieceManager->detectCylinders(workpiece);
    if (cylinders.isEmpty()) {
        qDebug() << "WorkspaceController: No suitable cylinders detected in workpiece";
        // Still consider this a success - workpiece is displayed
        return;
    }
    
    // Step 3: Get the main cylinder information
    gp_Ax1 mainAxis = m_workpieceManager->getMainCylinderAxis();
    double detectedDiameter = m_workpieceManager->getDetectedDiameter();
    
    if (detectedDiameter <= 0.0) {
        qDebug() << "WorkspaceController: Invalid diameter detected";
        return;
    }
    
    // Step 4: Automatically align the detected axis with the Z-axis
    gp_Trsf axisTransform = createAxisAlignmentTransformation(mainAxis);
    m_workpieceManager->setAxisAlignmentTransformation(axisTransform);

    // Update internal axis to reflect the new orientation (now Z-axis)
    gp_Ax1 alignedAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    m_workpieceManager->setCustomAxis(alignedAxis, detectedDiameter);

    // Align raw material axis with chuck centerline if available
    gp_Ax1 alignmentAxis = alignedAxis;
    if (m_chuckManager->hasValidCenterline()) {
        alignmentAxis = alignWorkpieceWithChuckCenterline(alignedAxis);
        qDebug() << "WorkspaceController: Workpiece aligned with chuck centerline";
    }
    
    // Step 5: Position workpiece at requested distance-to-chuck (snap min-Z)
    m_workpieceManager->positionWorkpieceAlongAxis(m_lastDistanceToChuck);
    
    // Step 6: Determine raw material diameter from full circular features
    double edgeDiameter = m_workpieceManager->getLargestCircularEdgeDiameter(workpiece);
    double rawMaterialDiameter = (edgeDiameter > 0.0 ? edgeDiameter : detectedDiameter) + 4.0;
    
    // Step 7: Create and display raw material that encompasses the workpiece
    m_rawMaterialManager->displayRawMaterialForWorkpiece(rawMaterialDiameter, workpiece, alignmentAxis);
    
    // Step 8: Initialize work coordinate system at raw material end
    initializeWorkCoordinateSystem(alignmentAxis);
    
    // Step 9: Emit workflow completion signal
    emit workpieceWorkflowCompleted(detectedDiameter, rawMaterialDiameter);
    
    qDebug() << "WorkspaceController: Workpiece workflow completed successfully"
             << "- Detected diameter:" << detectedDiameter << "mm"
             << "- Raw material diameter:" << rawMaterialDiameter << "mm";

    // Extract and display profile
    extractAndDisplayProfile();
}

gp_Ax1 WorkspaceController::alignWorkpieceWithChuckCenterline(const gp_Ax1& workpieceAxis)
{
    if (!m_chuckManager->hasValidCenterline()) {
        qDebug() << "WorkspaceController: No valid chuck centerline for alignment";
        return workpieceAxis;
    }
    
    try {
        gp_Ax1 chuckCenterline = m_chuckManager->getChuckCenterlineAxis();
        
        // For now, we align the workpiece axis direction with the chuck centerline direction
        // while preserving the workpiece axis location
        gp_Ax1 alignedAxis(workpieceAxis.Location(), chuckCenterline.Direction());
        
        qDebug() << "WorkspaceController: Workpiece axis aligned with chuck centerline";
        return alignedAxis;
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Error aligning workpiece with chuck centerline:" << e.what();
        return workpieceAxis;
    }
}

bool WorkspaceController::selectWorkpieceCylinderAxis(int cylinderIndex)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    bool success = m_workpieceManager->selectCylinderAxis(cylinderIndex);
    if (success) {
        // Recalculate raw material with the newly selected axis
        bool rawMaterialSuccess = recalculateRawMaterial();
        
        if (rawMaterialSuccess) {
            // Get selected cylinder info for workflow completion signal
            CylinderInfo selectedCylinder = m_workpieceManager->getCylinderInfo(cylinderIndex);
            double rawMaterialDiameter = m_rawMaterialManager->getCurrentDiameter();
            
            emit workpieceWorkflowCompleted(selectedCylinder.diameter, rawMaterialDiameter);
            qDebug() << "WorkspaceController: Cylinder axis" << cylinderIndex << "selected and raw material recalculated";
        } else {
            qDebug() << "WorkspaceController: Cylinder axis selected but raw material recalculation failed";
        }
        
        return rawMaterialSuccess;
    }
    
    return false;
}

QVector<CylinderInfo> WorkspaceController::getDetectedCylinders() const
{
    if (m_workpieceManager) {
        return m_workpieceManager->getDetectedCylindersInfo();
    }
    return QVector<CylinderInfo>();
}

bool WorkspaceController::hasChuckCenterline() const
{
    return m_chuckManager && m_chuckManager->hasValidCenterline();
}

gp_Ax1 WorkspaceController::getChuckCenterlineAxis() const
{
    if (m_chuckManager && m_chuckManager->hasValidCenterline()) {
        return m_chuckManager->getChuckCenterlineAxis();
    }
    return gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Default axis
}

void WorkspaceController::handleChuckCenterlineDetected(const gp_Ax1& axis)
{
    emit chuckCenterlineDetected(axis);
    qDebug() << "WorkspaceController: Chuck centerline detected and forwarded to UI";
}

void WorkspaceController::handleMultipleCylindersDetected(const QVector<CylinderInfo>& cylinders)
{
    emit multipleCylindersDetected(cylinders);
    qDebug() << "WorkspaceController: Multiple cylinders detected (" << cylinders.size() << "), manual selection available";
}

void WorkspaceController::handleCylinderAxisSelected(int index, const CylinderInfo& cylinderInfo)
{
    emit cylinderAxisSelected(index, cylinderInfo);
    qDebug() << "WorkspaceController: Cylinder axis" << index << "selected:" << cylinderInfo.description;
}

void WorkspaceController::handleChuckError(const QString& message)
{
    emit errorOccurred("ChuckManager", message);
}

void WorkspaceController::handleWorkpieceError(const QString& message)
{
    emit errorOccurred("WorkpieceManager", message);
}

void WorkspaceController::handleRawMaterialError(const QString& message)
{
    emit errorOccurred("RawMaterialManager", message);
}

void WorkspaceController::handleCylinderDetected(double diameter, double length, const gp_Ax1& axis)
{
    Q_UNUSED(axis) // Axis information is handled in the main workflow
    qDebug() << "WorkspaceController: Cylinder detected - diameter:" << diameter 
             << "mm, estimated length:" << length << "mm";
}

bool WorkspaceController::updateRawMaterialDiameter(double diameter)
{
    qDebug() << "WorkspaceController: updateRawMaterialDiameter called with diameter:" << diameter << "mm";
    
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    if (diameter <= 0.0) {
        emit errorOccurred("WorkspaceController", "Invalid diameter specified");
        return false;
    }

    // Get current workpiece and axis information
    QVector<Handle(AIS_Shape)> workpieces = m_workpieceManager->getWorkpieces();
    if (workpieces.isEmpty()) {
        emit errorOccurred("WorkspaceController", "No workpiece loaded");
        return false;
    }

    try {
        // Use centralized recalculation method
        bool success = recalculateRawMaterial(diameter);
        
        if (success) {
            qDebug() << "WorkspaceController: Raw material diameter successfully updated to" << diameter << "mm";
        } else {
            qDebug() << "WorkspaceController: Failed to update raw material diameter to" << diameter << "mm";
        }
        
        return success;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to update raw material diameter: %1").arg(e.what());
        qDebug() << errorMsg;
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::updateDistanceToChuck(double distance)
{
    if (!m_initialized || !m_workpieceManager) {
        emit errorOccurred("WorkspaceController", "Cannot update chuck distance - workspace not initialized");
        return false;
    }

    qDebug() << "WorkspaceController: Updating distance to chuck:" << distance << "mm";

    try {
        // Update position in workpiece manager
        bool success = m_workpieceManager->positionWorkpieceAlongAxis(distance);
        
        if (success) {
            // Store the latest distance for future automatic re-application (e.g., after flip)
            m_lastDistanceToChuck = distance;
            qDebug() << "WorkspaceController: Workpiece positioned at" << distance << "mm from chuck";
            
            // If raw material is loaded, update it to match workpiece position
            if (m_rawMaterialManager && m_rawMaterialManager->isRawMaterialDisplayed()) {
                recalculateRawMaterial(-1.0); // Use current diameter
                qDebug() << "WorkspaceController: Recalculated raw material for new position";
            }
            
            // Make sure toolpaths are properly transformed
            qDebug() << "WorkspaceController: Emitting workpiecePositionChanged signal for toolpath updates";
            emit workpiecePositionChanged(distance);
            
            return true;
        } else {
            emit errorOccurred("WorkspaceController", "Failed to position workpiece");
            return false;
        }
    } catch (const std::exception& e) {
        emit errorOccurred("WorkspaceController", QString("Exception while updating chuck distance: %1").arg(e.what()));
        return false;
    }
}

bool WorkspaceController::flipWorkpieceOrientation(bool flipped)
{
    qDebug() << "WorkspaceController: flipWorkpieceOrientation called with flipped:" << flipped;
    
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    QVector<Handle(AIS_Shape)> workpieces = m_workpieceManager->getWorkpieces();
    if (workpieces.isEmpty()) {
        emit errorOccurred("WorkspaceController", "No workpiece loaded");
        return false;
    }

    try {
        // Use WorkpieceManager's transformation capabilities
        bool flipSuccess = m_workpieceManager->flipWorkpieceOrientation(flipped);
        
        if (flipSuccess) {
            // Re-snap workpiece to stored distance-to-chuck after flip
            m_workpieceManager->positionWorkpieceAlongAxis(m_lastDistanceToChuck);
            
            qDebug() << "WorkspaceController: Workpiece orientation" << (flipped ? "flipped" : "restored") << "successfully, now recalculating raw material";
            // Recalculate raw material to match the new workpiece orientation
            bool rawMaterialSuccess = recalculateRawMaterial();
            
            // Update profile display after orientation change
            updateProfileDisplay();
            
            if (rawMaterialSuccess) {
                qDebug() << "WorkspaceController: Workpiece orientation" << (flipped ? "flipped" : "restored") << "and raw material updated successfully";
            } else {
                qDebug() << "WorkspaceController: Orientation updated but raw material recalculation failed";
            }
            
            return rawMaterialSuccess;
        } else {
            qDebug() << "WorkspaceController: Failed to" << (flipped ? "flip" : "restore") << "workpiece orientation";
        }
        
        return false;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to flip workpiece orientation: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::applyPartLoadingSettings(double distance, double diameter, bool flipped)
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }

    bool success = true;
    
    // Apply orientation flip
    success &= flipWorkpieceOrientation(flipped);
    
    // Apply distance positioning
    success &= updateDistanceToChuck(distance);
    
    // Apply raw material diameter
    success &= updateRawMaterialDiameter(diameter);
    
    // Update profile display after all transformations
    if (success) {
        updateProfileDisplay();
    }
    
    return success;
}

bool WorkspaceController::processManualAxisSelection(const TopoDS_Shape& selectedShape, const gp_Pnt& clickPoint)
{
    if (!m_initialized || selectedShape.IsNull()) {
        emit errorOccurred("WorkspaceController", "Invalid selection for axis extraction");
        return false;
    }

    try {
        gp_Ax1 extractedAxis;
        double extractedDiameter = 0.0;
        bool axisFound = false;

        // Try to extract cylindrical axis from the selected shape
        if (selectedShape.ShapeType() == TopAbs_FACE) {
            // Analyze cylindrical face
            TopoDS_Face face = TopoDS::Face(selectedShape);
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                gp_Cylinder cylinder = surface.Cylinder();
                extractedAxis = cylinder.Axis();
                extractedDiameter = cylinder.Radius() * 2.0;
                axisFound = true;
                qDebug() << "WorkspaceController: Extracted axis from cylindrical face - Diameter:" << extractedDiameter << "mm";
            }
        } else if (selectedShape.ShapeType() == TopAbs_EDGE) {
            // Analyze circular edge
            TopoDS_Edge edge = TopoDS::Edge(selectedShape);
            BRepAdaptor_Curve curve(edge);
            
            if (curve.GetType() == GeomAbs_Circle) {
                gp_Circ circle = curve.Circle();
                extractedAxis = circle.Axis();
                extractedDiameter = circle.Radius() * 2.0;
                axisFound = true;
                qDebug() << "WorkspaceController: Extracted axis from circular edge - Diameter:" << extractedDiameter << "mm";
            }
        }

        if (!axisFound) {
            emit errorOccurred("WorkspaceController", "Selected geometry is not cylindrical or circular. Please select a cylindrical face or circular edge.");
            return false;
        }

        // Create transformation to align the extracted axis with the Z-axis
        gp_Trsf alignmentTransform = createAxisAlignmentTransformation(extractedAxis);
        
        // Apply the transformation through WorkpieceManager for robust handling
        if (!m_workpieceManager->setAxisAlignmentTransformation(alignmentTransform)) {
            emit errorOccurred("WorkspaceController", "Failed to apply axis alignment transformation");
            return false;
        }

        // Update the workpiece manager with the new custom axis (now aligned with Z)
        gp_Ax1 alignedAxis(extractedAxis.Location(), gp_Dir(0, 0, 1));
        m_workpieceManager->setCustomAxis(alignedAxis, extractedDiameter);

        // Recalculate raw material with the new alignment
        bool rawMaterialSuccess = recalculateRawMaterial();
        
        if (rawMaterialSuccess) {
            // Create CylinderInfo for the manually selected axis
            CylinderInfo manualAxisInfo(alignedAxis, extractedDiameter, 100.0, "Manual Selection");
            
            // Emit signals for UI updates
            emit manualAxisSelected(extractedDiameter, alignedAxis);
            emit cylinderAxisSelected(-1, manualAxisInfo); // Use -1 to indicate manual selection
            emit workpieceWorkflowCompleted(extractedDiameter, m_rawMaterialManager->getCurrentDiameter());
            
            qDebug() << "WorkspaceController: Manual axis selection completed successfully";
            return true;
        } else {
            qDebug() << "WorkspaceController: Manual axis selection succeeded but raw material recalculation failed";
            return false;
        }

    } catch (const std::exception& e) {
        QString errorMsg = QString("Error processing manual axis selection: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

gp_Trsf WorkspaceController::createAxisAlignmentTransformation(const gp_Ax1& sourceAxis)
{
    gp_Trsf transform;
    
    try {
        // Target axis is Z-axis through origin
        gp_Ax1 targetAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        
        // Get the direction vectors
        gp_Dir sourceDir = sourceAxis.Direction();
        gp_Dir targetDir = targetAxis.Direction();
        
        // Check if axes are already aligned
        if (sourceDir.IsEqual(targetDir, Precision::Angular())) {
            // Only need translation to move to origin
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                transform.SetTranslation(translation);
            }
            return transform;
        }
        
        // Check if axes are opposite
        if (sourceDir.IsOpposite(targetDir, Precision::Angular())) {
            // Rotate 180 degrees around Y-axis and translate
            gp_Ax1 rotationAxis(sourceAxis.Location(), gp_Dir(0, 1, 0));
            transform.SetRotation(rotationAxis, M_PI);
            
            // Then translate to origin
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                gp_Trsf translationTransform;
                translationTransform.SetTranslation(translation);
                transform = translationTransform * transform;
            }
            return transform;
        }
        
        // Calculate rotation axis as cross product of source and target directions
        gp_Vec sourceVec(sourceDir);
        gp_Vec targetVec(targetDir);
        gp_Vec rotationVec = sourceVec.Crossed(targetVec);
        
        if (rotationVec.Magnitude() < Precision::Confusion()) {
            // Vectors are parallel, no rotation needed, just translation
            gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
            if (translation.Magnitude() > Precision::Confusion()) {
                transform.SetTranslation(translation);
            }
            return transform;
        }
        
        // Normalize rotation vector and calculate angle
        gp_Dir rotationDir(rotationVec);
        double angle = sourceVec.Angle(targetVec);
        
        // Create rotation around the calculated axis through the source axis location
        gp_Ax1 rotationAxis(sourceAxis.Location(), rotationDir);
        transform.SetRotation(rotationAxis, angle);
        
        // Apply translation to move to target location
        gp_Vec translation = gp_Vec(sourceAxis.Location(), targetAxis.Location());
        if (translation.Magnitude() > Precision::Confusion()) {
            gp_Trsf translationTransform;
            translationTransform.SetTranslation(translation);
            transform = translationTransform * transform;
        }
        
        qDebug() << "WorkspaceController: Created axis alignment transformation - Rotation angle:" 
                 << (angle * 180.0 / M_PI) << "degrees";
        
        return transform;
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Error creating axis alignment transformation:" << e.what();
        return gp_Trsf(); // Return identity transformation
    }
}

bool WorkspaceController::reprocessCurrentWorkpiece()
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        emit errorOccurred("WorkspaceController", "No workpiece available for reprocessing");
        return false;
    }

    try {
        // Clear current workpieces and raw material
        m_workpieceManager->clearWorkpieces();
        m_rawMaterialManager->clearRawMaterial();
        
        // Re-execute the complete workflow
        executeWorkpieceWorkflow(m_currentWorkpiece);
        
        qDebug() << "WorkspaceController: Workpiece reprocessed successfully";
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to reprocess workpiece: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

bool WorkspaceController::recalculateRawMaterial(double diameter)
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        qDebug() << "WorkspaceController: Cannot recalculate raw material - not initialized or no workpiece";
        return false;
    }

    try {
        // Get current settings - use Z-aligned axis if axis alignment is active
        gp_Ax1 currentAxis;
        if (m_workpieceManager->hasAxisAlignmentTransformation()) {
            // After manual axis selection, always use Z-axis for raw material
            currentAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
            qDebug() << "WorkspaceController: Using Z-aligned axis for raw material (manual selection active)";
        } else {
            // Use the detected/selected axis from workpiece manager
            currentAxis = m_workpieceManager->getMainCylinderAxis();
            qDebug() << "WorkspaceController: Using workpiece manager axis for raw material";
        }
        
        double currentDiameter = (diameter > 0.0) ? diameter : m_rawMaterialManager->getCurrentDiameter();
        
        // Ensure we have a valid diameter
        if (currentDiameter <= 0.0) {
            currentDiameter = m_rawMaterialManager->getNextStandardDiameter(
                m_workpieceManager->getDetectedDiameter());
        }
        
        // Get the current transformation from the workpiece manager
        gp_Trsf currentTransform = m_workpieceManager->getCurrentTransformation();
        
        // Debug transformation details for troubleshooting
        gp_XYZ translation = currentTransform.TranslationPart();
        qDebug() << "WorkspaceController: Complete transformation - Translation:" 
                 << translation.X() << "," << translation.Y() << "," << translation.Z();
        qDebug() << "WorkspaceController: Axis alignment active:" << m_workpieceManager->hasAxisAlignmentTransformation();
        qDebug() << "WorkspaceController: Workpiece flipped:" << m_workpieceManager->isWorkpieceFlipped();
        qDebug() << "WorkspaceController: Position offset:" << m_workpieceManager->getWorkpiecePositionOffset() << "mm";
        
        // Apply chuck alignment if available
        gp_Ax1 alignmentAxis = currentAxis;
        if (m_chuckManager->hasValidCenterline()) {
            alignmentAxis = alignWorkpieceWithChuckCenterline(currentAxis);
        }
        
        // Clear and regenerate raw material using transform-aware method
        m_rawMaterialManager->clearRawMaterial();
        qDebug() << "WorkspaceController: Recalculating raw material with diameter:" << currentDiameter << "mm";
        m_rawMaterialManager->displayRawMaterialForWorkpieceWithTransform(
            currentDiameter, m_currentWorkpiece, alignmentAxis, currentTransform);
        
        // Update work coordinate system with new raw material positioning
        initializeWorkCoordinateSystem(alignmentAxis);
        
        // The raw material manager already calls UpdateCurrentViewer, but ensure it's called
        if (!m_context.IsNull()) {
            m_context->UpdateCurrentViewer();
        }
        
        qDebug() << "WorkspaceController: Raw material recalculated successfully"
                 << "- Diameter:" << currentDiameter << "mm";
        
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to recalculate raw material: %1").arg(e.what());
        qDebug() << errorMsg;
        emit errorOccurred("WorkspaceController", errorMsg);
        return false;
    }
}

double WorkspaceController::getAutoRawMaterialDiameter() const
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        return 0.0;
    }

    double largestCircle = m_workpieceManager->getLargestCircularEdgeDiameter(m_currentWorkpiece);
    if (largestCircle <= 0.0) {
        largestCircle = m_workpieceManager->getDetectedDiameter();
    }

    if (largestCircle <= 0.0) {
        return 0.0;
    }

    return largestCircle + 4.0;
}

bool WorkspaceController::hasPartShape() const
{
    // Check if the workpiece manager exists and has a part
    return m_workpieceManager && m_workpieceManager->hasWorkpiece();
}

TopoDS_Shape WorkspaceController::getPartShape() const
{
    // If we have a workpiece manager and it has a part, return it
    if (m_workpieceManager && m_workpieceManager->hasWorkpiece()) {
        return m_workpieceManager->getWorkpieceShape();
    }
    
    // Otherwise return a null shape
    return TopoDS_Shape();
}

// ================================================================
//  Redisplay helpers
// ================================================================

void WorkspaceController::redisplayAll()
{
    if (!m_initialized || m_context.IsNull()) {
        return;
    }

    // Chuck
    if (m_chuckManager && m_chuckManager->isChuckLoaded()) {
        m_chuckManager->redisplayChuck();
    }

    // Workpieces
    if (m_workpieceManager) {
        QVector<Handle(AIS_Shape)> wp = m_workpieceManager->getWorkpieces();
        gp_Trsf trsf = m_workpieceManager->getCurrentTransformation();
        for (const Handle(AIS_Shape)& ais : wp) {
            if (!ais.IsNull()) {
                ais->SetLocalTransformation(trsf);
                m_context->Display(ais, Standard_False);
            }
        }
    }

    // Raw material
    if (m_rawMaterialManager && m_rawMaterialManager->isRawMaterialDisplayed()) {
        Handle(AIS_Shape) rmAIS = m_rawMaterialManager->getCurrentRawMaterialAIS();
        if (!rmAIS.IsNull()) {
            m_context->Display(rmAIS, Standard_False);
        }
    }

    // Force viewer redraw
    m_context->UpdateCurrentViewer();
}

bool WorkspaceController::generateToolpaths()
{
    if (!m_initialized) {
        emit errorOccurred("WorkspaceController", "Workspace not initialized");
        return false;
    }
    
    if (!hasPartShape()) {
        emit errorOccurred("WorkspaceController", "No part loaded - cannot generate toolpaths");
        return false;
    }
    
    try {
        qDebug() << "WorkspaceController: Starting toolpath generation";
        
        // Get the current part shape
        TopoDS_Shape partShape = getPartShape();
        
        // Create the toolpath generation pipeline
        auto pipeline = std::make_unique<IntuiCAM::Toolpath::ToolpathGenerationPipeline>();
        
        // Get turning axis from workspace (chuck centerline) or default
        gp_Ax1 turningAxis;
        if (hasChuckCenterline()) {
            turningAxis = getChuckCenterlineAxis();
        } else {
            turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Default Z-axis
        }
        
        // Extract inputs from part using the new pipeline interface
        auto inputs = pipeline->extractInputsFromPart(partShape, turningAxis);
        
        // Set default parameters for workspace controller generated toolpaths
        inputs.rawMaterialDiameter = getAutoRawMaterialDiameter();
        inputs.rawMaterialLength = 100.0; // Default length
        inputs.partLength = 80.0; // Default part length
        inputs.z0 = inputs.rawMaterialLength;
        inputs.facingAllowance = 2.0;
        inputs.largestDrillSize = 12.0;
        inputs.internalFinishingPasses = 2;
        inputs.externalFinishingPasses = 2;
        inputs.partingAllowance = 0.0;
        
        // Enable basic operations
        inputs.facing = true;
        inputs.externalRoughing = true;
        inputs.externalFinishing = true;
        inputs.parting = true;
        inputs.drilling = false; // Disabled by default in workspace controller
        inputs.machineInternalFeatures = false;
        inputs.internalRoughing = false;
        inputs.internalFinishing = false;
        inputs.internalGrooving = false;
        inputs.externalGrooving = false;
        inputs.chamfering = false;
        inputs.threading = false;
        
        qDebug() << "WorkspaceController: Executing toolpath generation pipeline";
        auto result = pipeline->executePipeline(inputs);
        
        if (!result.success) {
            QString errorMsg = QString("Toolpath generation failed: %1")
                               .arg(QString::fromStdString(result.errorMessage));
            emit errorOccurred("WorkspaceController", errorMsg);
            return false;
        }
        
        qDebug() << "WorkspaceController: Toolpath generation successful";
        qDebug() << "  - Generated" << result.timeline.size() << "toolpaths";
        qDebug() << "  - Processing time:" << result.processingTime.count() << "ms";
        
        // Apply work coordinate system transformations if initialized
        if (m_coordinateManager && m_coordinateManager->isInitialized()) {
            qDebug() << "WorkspaceController: Applying work coordinate transformations to toolpaths";
            
            // Transform toolpath display objects from work coordinates to global coordinates
            for (size_t i = 0; i < result.toolpathDisplayObjects.size(); ++i) {
                const auto& displayObj = result.toolpathDisplayObjects[i];
                if (!displayObj.IsNull()) {
                    // Get work-to-global transformation matrix
                    const auto& transform = m_coordinateManager->getWorkToGlobalMatrix();
                    
                    // Convert to OpenCASCADE transformation
                    gp_Trsf occTransform;
                    gp_Mat rotation(transform.data[0], transform.data[4], transform.data[8],
                                   transform.data[1], transform.data[5], transform.data[9],
                                   transform.data[2], transform.data[6], transform.data[10]);
                    gp_Vec translation(transform.data[12], transform.data[13], transform.data[14]);
                    
                    occTransform.SetValues(rotation(1,1), rotation(1,2), rotation(1,3), translation.X(),
                                          rotation(2,1), rotation(2,2), rotation(2,3), translation.Y(),
                                          rotation(3,1), rotation(3,2), rotation(3,3), translation.Z());
                    
                    displayObj->SetLocalTransformation(occTransform);
                }
            }
        }
        
        // Display the generated toolpaths in the 3D viewer
        for (size_t i = 0; i < result.toolpathDisplayObjects.size(); ++i) {
            const auto& displayObj = result.toolpathDisplayObjects[i];
            if (!displayObj.IsNull()) {
                m_context->Display(displayObj, Standard_False);
                qDebug() << "  - Displayed toolpath" << i;
            }
        }
        
        // Display the 2D profile if available
        if (!result.profileDisplayObject.IsNull()) {
            m_context->Display(result.profileDisplayObject, Standard_False);
            qDebug() << "  - Displayed 2D profile";
        }
        
        // Update the viewer
        m_context->UpdateCurrentViewer();
        
        qDebug() << "WorkspaceController: Toolpath generation and display completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Toolpath generation failed with exception: %1").arg(e.what());
        emit errorOccurred("WorkspaceController", errorMsg);
        qDebug() << errorMsg;
        return false;
    } catch (...) {
        QString errorMsg = "Toolpath generation failed with unknown error";
        emit errorOccurred("WorkspaceController", errorMsg);
        qDebug() << errorMsg;
        return false;
    }
}

// ================================================================
//  WorkspaceCoordinateManager Implementation
// ================================================================

WorkspaceCoordinateManager::WorkspaceCoordinateManager(QObject* parent)
    : QObject(parent)
    , initialized_(false) {
}

void WorkspaceCoordinateManager::initializeWorkCoordinates(const IntuiCAM::Geometry::Point3D& rawMaterialEnd, 
                                                         const IntuiCAM::Geometry::Vector3D& spindleAxis) {
    workCoordinateSystem_.setFromLatheMaterial(rawMaterialEnd, spindleAxis);
    initialized_ = true;
    
    qDebug() << "WorkspaceCoordinateManager: Work coordinate system initialized";
    qDebug() << "  - Origin at: (" << rawMaterialEnd.x << "," << rawMaterialEnd.y << "," << rawMaterialEnd.z << ")";
    qDebug() << "  - Spindle axis: (" << spindleAxis.x << "," << spindleAxis.y << "," << spindleAxis.z << ")";
    
    emit workCoordinateSystemChanged();
}

const IntuiCAM::Geometry::WorkCoordinateSystem& WorkspaceCoordinateManager::getWorkCoordinateSystem() const {
    return workCoordinateSystem_;
}

IntuiCAM::Geometry::Point3D WorkspaceCoordinateManager::globalToWork(const IntuiCAM::Geometry::Point3D& globalPoint) const {
    if (!initialized_) return globalPoint;
    return workCoordinateSystem_.fromGlobal(globalPoint);
}

IntuiCAM::Geometry::Point3D WorkspaceCoordinateManager::workToGlobal(const IntuiCAM::Geometry::Point3D& workPoint) const {
    if (!initialized_) return workPoint;
    return workCoordinateSystem_.toGlobal(workPoint);
}

IntuiCAM::Geometry::Point2D WorkspaceCoordinateManager::globalToLathe(const IntuiCAM::Geometry::Point3D& globalPoint) const {
    if (!initialized_) return IntuiCAM::Geometry::Point2D(0, 0);
    return workCoordinateSystem_.globalToLathe(globalPoint);
}

IntuiCAM::Geometry::Point3D WorkspaceCoordinateManager::latheToGlobal(const IntuiCAM::Geometry::Point2D& lathePoint) const {
    if (!initialized_) return IntuiCAM::Geometry::Point3D(lathePoint.x, 0, lathePoint.z);
    return workCoordinateSystem_.latheToGlobal(lathePoint);
}

void WorkspaceCoordinateManager::updateWorkOrigin(const IntuiCAM::Geometry::Point3D& newOrigin) {
    workCoordinateSystem_.setOrigin(newOrigin);
    
    if (initialized_) {
        qDebug() << "WorkspaceCoordinateManager: Work origin updated to: (" 
                 << newOrigin.x << "," << newOrigin.y << "," << newOrigin.z << ")";
        emit workCoordinateSystemChanged();
    }
}

const IntuiCAM::Geometry::Matrix4x4& WorkspaceCoordinateManager::getWorkToGlobalMatrix() const {
    return workCoordinateSystem_.getToGlobalMatrix();
}

const IntuiCAM::Geometry::Matrix4x4& WorkspaceCoordinateManager::getGlobalToWorkMatrix() const {
    return workCoordinateSystem_.getFromGlobalMatrix();
}

void WorkspaceController::initializeWorkCoordinateSystem(const gp_Ax1& axis) {
    if (!m_coordinateManager || !m_rawMaterialManager || m_currentWorkpiece.IsNull()) {
        qDebug() << "WorkspaceController: Cannot initialize work coordinate system - missing components or workpiece";
        return;
    }
    
    try {
        // Get the raw material shape to determine its end position
        TopoDS_Shape rawMaterial = m_rawMaterialManager->getCurrentRawMaterial();
        if (rawMaterial.IsNull()) {
            qDebug() << "WorkspaceController: No raw material available for work coordinate system";
            return;
        }
        
        // Based on the raw material positioning logic:
        // - Chuck face is at Z=0 
        // - Raw material extends 50mm into chuck (Z=-50 to Z=0)
        // - Raw material extends into positive Z direction for the workpiece
        // - Work origin should be at the END of the raw material (the face to be machined)
        
        // Get the current workpiece transformation to calculate proper bounds
        gp_Trsf currentTransform = m_workpieceManager->getCurrentTransformation();
        
        // Apply transformation to workpiece for calculation
        TopoDS_Shape transformedWorkpiece = m_currentWorkpiece;
        if (currentTransform.Form() != gp_Identity) {
            BRepBuilderAPI_Transform transformer(m_currentWorkpiece, currentTransform);
            transformedWorkpiece = transformer.Shape();
        }
        
        // Calculate workpiece bounds along the axis
        Bnd_Box bbox;
        BRepBndLib::Add(transformedWorkpiece, bbox);
        
        if (!bbox.IsVoid()) {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            gp_Dir axisDir = axis.Direction();
            gp_Pnt axisLoc = axis.Location();
            
            // Project bounding box corners to find extent along axis
            double maxProjection = std::numeric_limits<double>::lowest();
            
            gp_Pnt corners[8] = {
                gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymin, zmin),
                gp_Pnt(xmin, ymax, zmin), gp_Pnt(xmax, ymax, zmin),
                gp_Pnt(xmin, ymin, zmax), gp_Pnt(xmax, ymin, zmax),
                gp_Pnt(xmin, ymax, zmax), gp_Pnt(xmax, ymax, zmax)
            };
            
            for (int i = 0; i < 8; i++) {
                gp_Vec toCorner(axisLoc, corners[i]);
                double projection = toCorner.Dot(axisDir);
                maxProjection = std::max(maxProjection, projection);
            }
            
            // Calculate raw material end position
            // Raw material extends beyond the workpiece with facing allowance
            double facingAllowance = 10.0; // From raw material manager default
            double rawMaterialEnd = maxProjection + facingAllowance;
            
            // Ensure minimum extension past chuck face (Z=0)
            rawMaterialEnd = std::max(rawMaterialEnd, 20.0);
            
            // Calculate the actual end position in global coordinates
            gp_Pnt workOriginGlobal = axisLoc.Translated(gp_Vec(axisDir) * rawMaterialEnd);
            
            // Convert to our geometry types
            IntuiCAM::Geometry::Point3D workOrigin(workOriginGlobal.X(), workOriginGlobal.Y(), workOriginGlobal.Z());
            IntuiCAM::Geometry::Vector3D spindleAxis(axisDir.X(), axisDir.Y(), axisDir.Z());
            
            // Initialize the work coordinate system
            m_coordinateManager->initializeWorkCoordinates(workOrigin, spindleAxis);
            
            qDebug() << "WorkspaceController: Work coordinate system initialized";
            qDebug() << "  - Work origin (raw material end): (" << workOrigin.x << "," << workOrigin.y << "," << workOrigin.z << ")";
            qDebug() << "  - Spindle axis: (" << spindleAxis.x << "," << spindleAxis.y << "," << spindleAxis.z << ")";
            qDebug() << "  - Raw material end at:" << rawMaterialEnd << "mm along axis";
            
        } else {
            qDebug() << "WorkspaceController: Invalid workpiece bounds - using default work coordinate system";
            
            // Fallback: position work origin 70mm along positive axis direction (50mm chuck + 20mm minimum)
            gp_Pnt workOriginGlobal = axis.Location().Translated(gp_Vec(axis.Direction()) * 70.0);
            IntuiCAM::Geometry::Point3D workOrigin(workOriginGlobal.X(), workOriginGlobal.Y(), workOriginGlobal.Z());
            IntuiCAM::Geometry::Vector3D spindleAxis(axis.Direction().X(), axis.Direction().Y(), axis.Direction().Z());
            
            m_coordinateManager->initializeWorkCoordinates(workOrigin, spindleAxis);
        }
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Error initializing work coordinate system:" << e.what();
    }
}

bool WorkspaceController::extractAndDisplayProfile()
{
    if (!m_initialized || m_currentWorkpiece.IsNull()) {
        qDebug() << "WorkspaceController: Cannot extract profile - workspace not initialized or no workpiece";
        return false;
    }

    try {
        // Clear existing profile display
        clearProfileDisplay();

        // Get the current transformation from workpiece manager
        gp_Trsf workpieceTrsf = m_workpieceManager->getCurrentTransformation();
        
        // Apply transformation to get the workpiece in world coordinates
        BRepBuilderAPI_Transform transformer(m_currentWorkpiece, workpieceTrsf);
        TopoDS_Shape transformedWorkpiece = transformer.Shape();

        // Set up profile extraction parameters
        IntuiCAM::Toolpath::ProfileExtractor::ExtractionParameters params;
        
        // Use the work coordinate system axis for profile extraction (should be Z-axis aligned)
        if (m_coordinateManager && m_coordinateManager->isInitialized()) {
            // Extract relative to the work coordinate system - the turning axis is always Z in work coordinates
            const auto& workCS = m_coordinateManager->getWorkCoordinateSystem();
            gp_Pnt workOrigin(workCS.getToGlobalMatrix().data[12], 
                            workCS.getToGlobalMatrix().data[13], 
                            workCS.getToGlobalMatrix().data[14]);
            gp_Dir workZAxis(workCS.getToGlobalMatrix().data[8], 
                           workCS.getToGlobalMatrix().data[9], 
                           workCS.getToGlobalMatrix().data[10]);
            params.turningAxis = gp_Ax1(workOrigin, workZAxis);
            qDebug() << "WorkspaceController: Using work coordinate system axis for profile extraction";
        } else if (m_chuckManager->isChuckLoaded() && m_chuckManager->hasValidCenterline()) {
            params.turningAxis = m_chuckManager->getChuckCenterlineAxis();
            qDebug() << "WorkspaceController: Using chuck centerline for profile extraction";
        } else {
            // Fallback to Z-axis if no chuck centerline or work coordinate system
            params.turningAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
            qDebug() << "WorkspaceController: Using fallback Z-axis for profile extraction";
        }

        // Configure extraction parameters for lathe operations
        params.profileTolerance = 0.01;        // 0.01mm tolerance
        params.projectionTolerance = 0.001;    // 0.001mm projection tolerance
        params.profileSections = 200;          // High resolution for accurate profile
        params.includeInternalFeatures = true; // Include grooves and bores
        params.autoDetectFeatures = true;      // Automatic feature detection
        params.optimizeProfile = true;         // Optimize for smoothness
        params.minFeatureSize = 0.1;           // 0.1mm minimum feature size

        qDebug() << "WorkspaceController: Extracting profile with" << params.profileSections << "sections";

        // Extract the profile
        m_extractedProfile = IntuiCAM::Toolpath::ProfileExtractor::extractProfile(transformedWorkpiece, params);

        if (!m_extractedProfile.isEmpty()) {
            qDebug() << "WorkspaceController: Profile extracted successfully with" 
                     << m_extractedProfile.getTotalPointCount() << "total points";
            
            // Create profile display object
            m_profileDisplayObject = createProfileDisplayObject(m_extractedProfile);
            
            if (!m_profileDisplayObject.IsNull() && m_profileVisible) {
                // Display the profile in the context
                m_context->Display(m_profileDisplayObject, Standard_False);
                m_context->UpdateCurrentViewer();
                qDebug() << "WorkspaceController: Profile displayed successfully";
            }
            
            return true;
        } else {
            qDebug() << "WorkspaceController: Profile extraction returned empty result";
            return false;
        }
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Profile extraction failed:" << e.what();
        emit errorOccurred("WorkspaceController", QString("Profile extraction failed: %1").arg(e.what()));
        return false;
    }
}

void WorkspaceController::setProfileVisible(bool visible)
{
    m_profileVisible = visible;
    
    if (!m_profileDisplayObject.IsNull() && m_context) {
        if (visible) {
            m_context->Display(m_profileDisplayObject, Standard_False);
        } else {
            m_context->Erase(m_profileDisplayObject, Standard_False);
        }
        m_context->UpdateCurrentViewer();
        
        qDebug() << "WorkspaceController: Profile visibility set to" << (visible ? "visible" : "hidden");
    }
}

bool WorkspaceController::isProfileVisible() const
{
    return m_profileVisible;
}

IntuiCAM::Toolpath::LatheProfile::Profile2D WorkspaceController::getExtractedProfile() const
{
    return m_extractedProfile;
}

Handle(AIS_InteractiveObject) WorkspaceController::createProfileDisplayObject(const IntuiCAM::Toolpath::LatheProfile::Profile2D& profile)
{
    if (profile.isEmpty()) {
        qDebug() << "WorkspaceController: Cannot create display object for empty profile";
        return Handle(AIS_InteractiveObject)();
    }

    try {
        // Create a compound shape to hold all profile curves
        TopoDS_Compound profileCompound;
        BRep_Builder builder;
        builder.MakeCompound(profileCompound);

        // PROFILE DISPLAY COORDINATE SYSTEM
        // Profile extraction happens on the already-transformed workpiece in global coordinates
        // The extracted Point2D(radius, axial) are relative to the extraction axis in global space
        // We need to convert these to 3D points and apply the same transformation as the workpiece

        // Get the extraction axis for reconstruction
        gp_Ax1 extractionAxis;
        if (m_coordinateManager && m_coordinateManager->isInitialized()) {
            const auto& workCS = m_coordinateManager->getWorkCoordinateSystem();
            gp_Pnt workOrigin(workCS.getToGlobalMatrix().data[12], 
                            workCS.getToGlobalMatrix().data[13], 
                            workCS.getToGlobalMatrix().data[14]);
            gp_Dir workZAxis(workCS.getToGlobalMatrix().data[8], 
                           workCS.getToGlobalMatrix().data[9], 
                           workCS.getToGlobalMatrix().data[10]);
            extractionAxis = gp_Ax1(workOrigin, workZAxis);
        } else if (m_chuckManager->isChuckLoaded() && m_chuckManager->hasValidCenterline()) {
            extractionAxis = m_chuckManager->getChuckCenterlineAxis();
        } else {
            extractionAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        }

        // External profile
        if (!profile.externalProfile.points.empty()) {
            std::vector<gp_Pnt> externalPoints3D;
            for (const auto& point2D : profile.externalProfile.points) {
                // Point2D: x = radius, z = axial position relative to extraction axis
                // Reconstruct the 3D point relative to the extraction axis
                gp_Pnt axisPoint = extractionAxis.Location().Translated(
                    gp_Vec(extractionAxis.Direction()) * point2D.z);
                
                // For XZ plane display, offset the point by radius in the X direction
                // This assumes the extraction axis is aligned with Z and we want radius in X
                gp_Vec radialOffset;
                if (extractionAxis.Direction().IsEqual(gp_Dir(0, 0, 1), Precision::Angular())) {
                    // Z-axis aligned: radius goes in X direction
                    radialOffset = gp_Vec(point2D.x, 0.0, 0.0);
                } else {
                    // General case: find perpendicular direction in XZ plane
                    gp_Dir axisDir = extractionAxis.Direction();
                    gp_Vec perpendicular(axisDir.Y(), -axisDir.X(), 0.0);
                    if (perpendicular.Magnitude() < Precision::Confusion()) {
                        perpendicular = gp_Vec(1.0, 0.0, 0.0);
                    }
                    perpendicular.Normalize();
                    radialOffset = perpendicular * point2D.x;
                }
                
                gp_Pnt point3D = axisPoint.Translated(radialOffset);
                externalPoints3D.push_back(point3D);
            }

            // Create polyline for external profile
            if (externalPoints3D.size() >= 2) {
                for (size_t i = 0; i < externalPoints3D.size() - 1; ++i) {
                    BRepBuilderAPI_MakeEdge edgeBuilder(externalPoints3D[i], externalPoints3D[i + 1]);
                    if (edgeBuilder.IsDone()) {
                        builder.Add(profileCompound, edgeBuilder.Edge());
                    }
                }
            }
        }

        // Internal profile (if any)
        if (!profile.internalProfile.points.empty()) {
            std::vector<gp_Pnt> internalPoints3D;
            for (const auto& point2D : profile.internalProfile.points) {
                // Point2D: x = radius, z = axial position relative to extraction axis
                // Reconstruct the 3D point relative to the extraction axis
                gp_Pnt axisPoint = extractionAxis.Location().Translated(
                    gp_Vec(extractionAxis.Direction()) * point2D.z);
                
                // For XZ plane display, offset the point by radius in the X direction
                // This assumes the extraction axis is aligned with Z and we want radius in X
                gp_Vec radialOffset;
                if (extractionAxis.Direction().IsEqual(gp_Dir(0, 0, 1), Precision::Angular())) {
                    // Z-axis aligned: radius goes in X direction
                    radialOffset = gp_Vec(point2D.x, 0.0, 0.0);
                } else {
                    // General case: find perpendicular direction in XZ plane
                    gp_Dir axisDir = extractionAxis.Direction();
                    gp_Vec perpendicular(axisDir.Y(), -axisDir.X(), 0.0);
                    if (perpendicular.Magnitude() < Precision::Confusion()) {
                        perpendicular = gp_Vec(1.0, 0.0, 0.0);
                    }
                    perpendicular.Normalize();
                    radialOffset = perpendicular * point2D.x;
                }
                
                gp_Pnt point3D = axisPoint.Translated(radialOffset);
                internalPoints3D.push_back(point3D);
            }

            // Create polyline for internal profile
            if (internalPoints3D.size() >= 2) {
                for (size_t i = 0; i < internalPoints3D.size() - 1; ++i) {
                    BRepBuilderAPI_MakeEdge edgeBuilder(internalPoints3D[i], internalPoints3D[i + 1]);
                    if (edgeBuilder.IsDone()) {
                        builder.Add(profileCompound, edgeBuilder.Edge());
                    }
                }
            }
        }

        // Create AIS shape for the profile
        Handle(AIS_Shape) profileShape = new AIS_Shape(profileCompound);
        
        // Set profile display properties
        profileShape->SetColor(Quantity_NOC_RED);  // Red color for profile
        profileShape->SetWidth(3.0);               // Thicker line for visibility
        profileShape->SetDisplayMode(AIS_WireFrame);
        profileShape->SetTransparency(0.0);
        
        qDebug() << "WorkspaceController: Profile display object created successfully with coordinate transformation";
        return profileShape;
        
    } catch (const std::exception& e) {
        qDebug() << "WorkspaceController: Failed to create profile display object:" << e.what();
        return Handle(AIS_InteractiveObject)();
    }
}

void WorkspaceController::updateProfileDisplay()
{
    if (!m_extractedProfile.isEmpty()) {
        // Re-extract and display profile with current transformation
        extractAndDisplayProfile();
    }
}

void WorkspaceController::clearProfileDisplay()
{
    if (!m_profileDisplayObject.IsNull() && m_context) {
        m_context->Erase(m_profileDisplayObject, Standard_False);
        m_context->UpdateCurrentViewer();
        m_profileDisplayObject.Nullify();
        qDebug() << "WorkspaceController: Profile display cleared";
    }
}

 