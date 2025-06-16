#include "workpiecemanager.h"

#include <QDebug>
#include <cmath>
#include <limits>
#include <algorithm>

// OpenCASCADE includes
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Ax1.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Circ.hxx>

WorkpieceManager::WorkpieceManager(QObject *parent)
    : QObject(parent)
    , m_detectedDiameter(0.0)
    , m_selectedCylinderIndex(-1)
    , m_isFlipped(false)
    , m_positionOffset(0.0)
    , m_hasAxisAlignment(false)
{
    // Initialize default main cylinder axis
    m_mainCylinderAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    // Initialize axis alignment transform as identity (default constructor already creates identity)
    // m_axisAlignmentTransform is already identity by default
}

WorkpieceManager::~WorkpieceManager()
{
}

void WorkpieceManager::initialize(Handle(AIS_InteractiveContext) context)
{
    m_context = context;
    qDebug() << "WorkpieceManager initialized with AIS context";
}

bool WorkpieceManager::addWorkpiece(const TopoDS_Shape& workpiece)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
        return false;
    }
    
    if (workpiece.IsNull()) {
        emit errorOccurred("Invalid workpiece shape provided");
        return false;
    }
    
    // Create AIS shape for the workpiece
    Handle(AIS_Shape) workpieceAIS = new AIS_Shape(workpiece);
    
    // Set workpiece material properties
    setWorkpieceMaterial(workpieceAIS);
    
    // Display the workpiece only if currently visible
    if (m_visible) {
        m_context->Display(workpieceAIS, AIS_Shaded, 0, false);
    }
    
    // Store the workpiece
    m_workpieces.append(workpieceAIS);
    
    qDebug() << "Workpiece added and displayed successfully";
    return true;
}

QVector<gp_Ax1> WorkpieceManager::detectCylinders(const TopoDS_Shape& workpiece)
{
    QVector<gp_Ax1> cylinders;
    
    if (workpiece.IsNull()) {
        return cylinders;
    }
    
    // Reset previous analysis results
    m_detectedDiameter = 0.0;
    m_detectedCylinders.clear();
    m_selectedCylinderIndex = -1;
    
    // Perform detailed cylinder analysis
    performDetailedCylinderAnalysis(workpiece);
    
    // Extract axes for backward compatibility
    for (const CylinderInfo& info : m_detectedCylinders) {
        cylinders.append(info.axis);
    }
    
    // If multiple cylinders detected, emit signal for manual selection
    if (m_detectedCylinders.size() > 1) {
        emit multipleCylindersDetected(m_detectedCylinders);
        qDebug() << "WorkpieceManager: Multiple cylinders detected (" << m_detectedCylinders.size() << "), manual selection available";
    }
    
    // Auto-select the largest cylinder if any detected
    if (!m_detectedCylinders.isEmpty()) {
        selectCylinderAxis(0); // The cylinders are sorted by diameter (largest first)
    }
    
    return cylinders;
}

bool WorkpieceManager::selectCylinderAxis(int index)
{
    if (index < 0 || index >= m_detectedCylinders.size()) {
        emit errorOccurred(QString("Invalid cylinder index: %1").arg(index));
        return false;
    }
    
    const CylinderInfo& selectedCylinder = m_detectedCylinders[index];
    
    m_mainCylinderAxis = selectedCylinder.axis;
    m_detectedDiameter = selectedCylinder.diameter;
    m_selectedCylinderIndex = index;
    
    emit cylinderAxisSelected(index, selectedCylinder);
    emit cylinderDetected(selectedCylinder.diameter, selectedCylinder.estimatedLength, selectedCylinder.axis);
    
    qDebug() << "WorkpieceManager: Selected cylinder" << index << "- Diameter:" << selectedCylinder.diameter << "mm";
    return true;
}

CylinderInfo WorkpieceManager::getCylinderInfo(int index) const
{
    if (index >= 0 && index < m_detectedCylinders.size()) {
        return m_detectedCylinders[index];
    }
    
    // Return invalid info
    return CylinderInfo(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0.0, 0.0, "Invalid");
}

void WorkpieceManager::setCustomAxis(const gp_Ax1& axis, double diameter)
{
    m_mainCylinderAxis = axis;
    m_detectedDiameter = diameter;
    m_selectedCylinderIndex = -1; // Indicate custom axis
    
    // Create a custom cylinder info
    CylinderInfo customInfo(axis, diameter, 100.0, "Custom Axis");
    
    emit cylinderAxisSelected(-1, customInfo);
    emit cylinderDetected(diameter, 100.0, axis);
    
    qDebug() << "WorkpieceManager: Custom axis set - Diameter:" << diameter << "mm";
}

void WorkpieceManager::performDetailedCylinderAnalysis(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return;
    }
    
    try {
        TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
        QVector<CylinderInfo> tempCylinders;
        
        for (; faceExplorer.More(); faceExplorer.Next()) {
            TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                gp_Cylinder cylinder = surface.Cylinder();
                gp_Ax1 axis = cylinder.Axis();
                double radius = cylinder.Radius();
                double diameter = 2.0 * radius;
                
                // Only consider cylinders with reasonable diameters (> 5mm, < 500mm)
                if (diameter > 5.0 && diameter < 500.0) {
                    double estimatedLength = estimateCylinderLength(shape, axis);
                    
                    CylinderInfo info(axis, diameter, estimatedLength);
                    tempCylinders.append(info);
                    
                    qDebug() << "WorkpieceManager: Detected cylinder - Diameter:" << diameter << "mm, Length:" << estimatedLength << "mm";
                }
            }
        }
        
        // Sort cylinders by diameter (largest first)
        std::sort(tempCylinders.begin(), tempCylinders.end(), 
                  [](const CylinderInfo& a, const CylinderInfo& b) {
                      return a.diameter > b.diameter;
                  });
        
        // Generate descriptions and store
        for (int i = 0; i < tempCylinders.size(); ++i) {
            CylinderInfo& info = tempCylinders[i];
            info.description = generateCylinderDescription(info, i);
            m_detectedCylinders.append(info);
        }
        
        qDebug() << "WorkpieceManager: Detailed analysis complete - Found" << m_detectedCylinders.size() << "cylinders";
        
    } catch (const std::exception& e) {
        qDebug() << "WorkpieceManager: Error in detailed cylinder analysis:" << e.what();
        emit errorOccurred(QString("Cylinder analysis failed: %1").arg(e.what()));
    }
}

double WorkpieceManager::estimateCylinderLength(const TopoDS_Shape& workpiece, const gp_Ax1& axis)
{
    try {
        // Get bounding box of the workpiece
        Bnd_Box bbox;
        BRepBndLib::Add(workpiece, bbox);
        
        if (bbox.IsVoid()) {
            return 100.0; // Default length
        }
        
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Calculate the bounds along the cylinder axis
        gp_Vec axisDir = axis.Direction();
        gp_Pnt axisLoc = axis.Location();
        
        // Project bounding box corners onto the axis to find extent
        double minProjection = std::numeric_limits<double>::max();
        double maxProjection = std::numeric_limits<double>::lowest();
        
        // Check all 8 corners of the bounding box
        gp_Pnt corners[8] = {
            gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymin, zmin),
            gp_Pnt(xmin, ymax, zmin), gp_Pnt(xmax, ymax, zmin),
            gp_Pnt(xmin, ymin, zmax), gp_Pnt(xmax, ymin, zmax),
            gp_Pnt(xmin, ymax, zmax), gp_Pnt(xmax, ymax, zmax)
        };
        
        for (int i = 0; i < 8; i++) {
            gp_Vec toCorner(axisLoc, corners[i]);
            double projection = toCorner.Dot(axisDir);
            minProjection = std::min(minProjection, projection);
            maxProjection = std::max(maxProjection, projection);
        }
        
        double length = maxProjection - minProjection;
        return std::max(length, 10.0); // Minimum 10mm length
        
    } catch (const std::exception& e) {
        qDebug() << "WorkpieceManager: Error estimating cylinder length:" << e.what();
        return 100.0; // Default fallback
    }
}

QString WorkpieceManager::generateCylinderDescription(const CylinderInfo& info, int index)
{
    QString desc = QString("Cylinder %1: Ø%2mm × %3mm")
                   .arg(index + 1)
                   .arg(QString::number(info.diameter, 'f', 1))
                   .arg(QString::number(info.estimatedLength, 'f', 1));
    
    if (index == 0) {
        desc += " (Largest)";
    }
    
    return desc;
}

void WorkpieceManager::analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders)
{
    // This method is kept for backward compatibility
    // The actual analysis is now done in performDetailedCylinderAnalysis
    TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
    
    double largestDiameter = 0.0;
    
    for (; faceExplorer.More(); faceExplorer.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
        BRepAdaptor_Surface surface(face);
        
        if (surface.GetType() == GeomAbs_Cylinder) {
            gp_Cylinder cylinder = surface.Cylinder();
            gp_Ax1 axis = cylinder.Axis();
            double radius = cylinder.Radius();
            double diameter = 2.0 * radius;
            
            // Only consider cylinders with reasonable diameters (> 5mm, < 500mm)
            if (diameter > 5.0 && diameter < 500.0) {
                cylinders.append(axis);
                
                // Track the largest diameter for main cylinder identification
                if (diameter > largestDiameter) {
                    largestDiameter = diameter;
                    m_detectedDiameter = diameter;
                }
                
                emit cylinderDetected(diameter, 100.0, axis); // Default length estimate
                qDebug() << "Detected cylinder: diameter =" << diameter << "mm";
            }
        }
    }
}

void WorkpieceManager::clearWorkpieces()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Remove all workpieces
    for (Handle(AIS_Shape) workpiece : m_workpieces) {
        if (!workpiece.IsNull()) {
            m_context->Remove(workpiece, false);
        }
    }
    m_workpieces.clear();
    
    // Reset analysis results
    m_detectedDiameter = 0.0;
    m_mainCylinderAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    m_detectedCylinders.clear();
    m_selectedCylinderIndex = -1;
    
    // Reset transformation state
    m_isFlipped = false;
    m_positionOffset = 0.0;
    
    // Reset axis alignment state
    m_hasAxisAlignment = false;
    m_axisAlignmentTransform = gp_Trsf(); // Reset to identity

    qDebug() << "All workpieces cleared";

    m_context->UpdateCurrentViewer();
}

void WorkpieceManager::setWorkpiecesVisible(bool visible)
{
    if (m_context.IsNull()) {
        return;
    }

    m_visible = visible;

    for (Handle(AIS_Shape) workpiece : m_workpieces) {
        if (!workpiece.IsNull()) {
            if (visible) {
                if (!m_context->IsDisplayed(workpiece)) {
                    m_context->Display(workpiece, AIS_Shaded, 0, Standard_False);
                } else {
                    m_context->Redisplay(workpiece, Standard_False);
                }
            } else {
                m_context->Erase(workpiece, Standard_False);
            }
        }
    }

    m_context->UpdateCurrentViewer();
}

bool WorkpieceManager::areWorkpiecesVisible() const
{
    if (m_context.IsNull()) {
        return false;
    }

    for (Handle(AIS_Shape) workpiece : m_workpieces) {
        if (!workpiece.IsNull() && m_context->IsDisplayed(workpiece)) {
            return true;
        }
    }

    return false;
}

void WorkpieceManager::setWorkpieceMaterial(Handle(AIS_Shape) workpieceAIS)
{
    if (workpieceAIS.IsNull()) {
        return;
    }
    
    // Set workpiece material (aluminum-like)
    Graphic3d_MaterialAspect workpieceMaterial(Graphic3d_NOM_ALUMINIUM);
    workpieceMaterial.SetColor(Quantity_Color(0.8, 0.8, 0.9, Quantity_TOC_RGB));
    workpieceAIS->SetMaterial(workpieceMaterial);
}

bool WorkpieceManager::flipWorkpieceOrientation(bool flipped)
{
    if (m_context.IsNull() || m_workpieces.isEmpty()) {
        emit errorOccurred("No workpieces available for transformation");
        return false;
    }

    try {
        // Only apply transformation if state actually changes
        if (m_isFlipped == flipped) {
            return true; // Already in desired state
        }

        // Update flip state
        m_isFlipped = flipped;
        
        // Get the complete current transformation (position + flip)
        gp_Trsf newTransformation = getCurrentTransformation();

        // Apply complete transformation to all workpieces
        for (Handle(AIS_Shape) workpiece : m_workpieces) {
            if (!workpiece.IsNull()) {
                workpiece->SetLocalTransformation(newTransformation);
                if (m_visible) {
                    if (!m_context->IsDisplayed(workpiece)) {
                        m_context->Display(workpiece, AIS_Shaded, 0, Standard_False);
                    } else {
                        m_context->Redisplay(workpiece, false);
                    }
                }
            }
        }
        
        // Use a single efficient update
        m_context->UpdateCurrentViewer();
        
        // Notify that workpiece transformation has changed
        emit workpieceTransformed();
        
        qDebug() << "WorkpieceManager: Workpiece orientation" << (flipped ? "flipped" : "restored") 
                 << "with position offset" << m_positionOffset << "mm";
        return true;

    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to flip workpiece orientation: %1").arg(e.what()));
        return false;
    }
}

gp_Trsf WorkpieceManager::getCurrentTransformation() const
{
    gp_Trsf transform; // Default constructor creates identity
    
    // Debug current state
    qDebug() << "WorkpieceManager: Building current transformation:";
    qDebug() << "  - Position offset:" << m_positionOffset << "mm";
    qDebug() << "  - Flipped:" << (m_isFlipped ? "Yes" : "No");
    qDebug() << "  - Has axis alignment:" << (m_hasAxisAlignment ? "Yes" : "No");
    
    // Step 1: Apply axis alignment transformation first (if present)
    // This aligns the workpiece axis with the Z-axis
    if (m_hasAxisAlignment) {
        transform = m_axisAlignmentTransform * transform;
        qDebug() << "  - Applied axis alignment transformation";
    }
    
    // Step 2: Apply flip transformation (around the now-aligned axis)
    if (m_isFlipped) {
        // Flip around Y-axis at the origin (since axis is now aligned with Z)
        gp_Ax1 rotationAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
        gp_Trsf flipTransform;
        flipTransform.SetRotation(rotationAxis, M_PI); // 180 degrees
        transform = flipTransform * transform;
        qDebug() << "  - Applied flip transformation (180° around Y axis)";
    }
    
    // Step 3: Apply global position offset (always in Z+ direction for chuck distance)
    if (std::abs(m_positionOffset) > 1e-6) {
        gp_Vec globalTranslation(0, 0, m_positionOffset);  // Global Z+ direction
        gp_Trsf translationTransform;
        translationTransform.SetTranslation(globalTranslation);
        transform = translationTransform * transform;
        qDebug() << "  - Applied position offset:" << m_positionOffset << "mm in Z+ direction";
    }
    
    // Debug the resulting transformation
    gp_XYZ translation = transform.TranslationPart();
    qDebug() << "  - Final transformation - Translation:" << translation.X() << "," << translation.Y() << "," << translation.Z();
    qDebug() << "  - Final transformation - Form:" << transform.Form();
    
    return transform;
}

// === Bounding-box helper implementations ===

double WorkpieceManager::getLocalMinZ(const TopoDS_Shape& shape) const
{
    if (shape.IsNull()) {
        return 0.0;
    }

    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);
    if (bbox.IsVoid()) {
        return 0.0;
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return zmin;
}

/**
 * The currentMinZ() method evaluates the minimum global Z coordinate of the loaded
 * workpiece(s) **after** applying the currently active transformation stack
 * (axis-alignment, flip and translation).  This is required so that we can move
 * the part such that its minimum Z coincides with the requested distance-to-chuck
 * irrespective of its previous position or orientation.
 */
double WorkpieceManager::currentMinZ() const
{
    if (m_workpieces.isEmpty()) {
        return 0.0;
    }

    gp_Trsf transform = getCurrentTransformation();

    double globalMinZ = std::numeric_limits<double>::max();

    for (const Handle(AIS_Shape)& aisShape : m_workpieces) {
        if (aisShape.IsNull()) {
            continue;
        }
        const TopoDS_Shape& shp = aisShape->Shape();
        Bnd_Box bbox;
        BRepBndLib::Add(shp, bbox);
        if (bbox.IsVoid()) {
            continue;
        }
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

        gp_Pnt corners[8] = {
            gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymin, zmin),
            gp_Pnt(xmin, ymax, zmin), gp_Pnt(xmax, ymax, zmin),
            gp_Pnt(xmin, ymin, zmax), gp_Pnt(xmax, ymin, zmax),
            gp_Pnt(xmin, ymax, zmax), gp_Pnt(xmax, ymax, zmax)
        };

        for (const gp_Pnt& corner : corners) {
            gp_Pnt c = corner;
            c.Transform(transform);
            globalMinZ = std::min(globalMinZ, c.Z());
        }
    }

    if (globalMinZ == std::numeric_limits<double>::max()) {
        return 0.0;
    }
    return globalMinZ;
}

bool WorkpieceManager::positionWorkpieceAlongAxis(double distance)
{
    if (m_context.IsNull() || m_workpieces.isEmpty()) {
        emit errorOccurred("No workpieces available for positioning");
        return false;
    }

    try {
        // Compute current minimum Z of the geometry (after current transform)
        double currentMin = currentMinZ();
        double delta = distance - currentMin; // positive if we need to move part further away from chuck

        // Accumulate into the global position offset so that subsequent calls are relative-aware
        m_positionOffset += delta;

        // Build updated transformation stack with new position
        gp_Trsf newTransformation = getCurrentTransformation();

        // Apply to all workpieces
        for (Handle(AIS_Shape) workpiece : m_workpieces) {
            if (!workpiece.IsNull()) {
                workpiece->SetLocalTransformation(newTransformation);
                if (m_visible) {
                    if (!m_context->IsDisplayed(workpiece)) {
                        m_context->Display(workpiece, AIS_Shaded, 0, Standard_False);
                    } else {
                        m_context->Redisplay(workpiece, false);
                    }
                }
            }
        }

        m_context->UpdateCurrentViewer();

        // Notify listeners
        emit workpieceTransformed();

        qDebug() << "WorkpieceManager: Re-positioned workpiece so that min-Z ==" << distance
                 << "mm (Δ=" << delta << "mm, accumulated offset=" << m_positionOffset << "mm)";
        return true;

    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to position workpiece: %1").arg(e.what()));
        return false;
    }
}

bool WorkpieceManager::setAxisAlignmentTransformation(const gp_Trsf& transform)
{
    if (m_context.IsNull() || m_workpieces.isEmpty()) {
        emit errorOccurred("No workpieces available for axis alignment transformation");
        return false;
    }

    try {
        // Store the axis alignment transformation
        m_axisAlignmentTransform = transform;
        m_hasAxisAlignment = true;
        
        // Apply the complete transformation (alignment + flip + position)
        gp_Trsf completeTransform = getCurrentTransformation();

        // Apply to all workpieces
        for (Handle(AIS_Shape) workpiece : m_workpieces) {
            if (!workpiece.IsNull()) {
                workpiece->SetLocalTransformation(completeTransform);
                if (m_visible) {
                    if (!m_context->IsDisplayed(workpiece)) {
                        m_context->Display(workpiece, AIS_Shaded, 0, Standard_False);
                    } else {
                        m_context->Redisplay(workpiece, false);
                    }
                }
            }
        }
        
        m_context->UpdateCurrentViewer();
        
        // Notify that workpiece transformation has changed
        emit workpieceTransformed();
        
        qDebug() << "WorkpieceManager: Axis alignment transformation applied successfully";
        return true;

    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to apply axis alignment transformation: %1").arg(e.what()));
        return false;
    }
}

TopoDS_Shape WorkpieceManager::getWorkpieceShape() const
{
    if (!m_workpieces.isEmpty()) {
        // Return the shape of the first workpiece
        Handle(AIS_Shape) aisShape = m_workpieces.first();
        if (!aisShape.IsNull()) {
            return aisShape->Shape();
        }
    }
    
    // Return null shape if no workpiece or invalid shape
    return TopoDS_Shape();
}

double WorkpieceManager::getLargestCircularEdgeDiameter(const TopoDS_Shape& workpiece) const
{
    if (workpiece.IsNull()) {
        return 0.0;
    }

    double maxDiameter = 0.0;

    try {
        for (TopExp_Explorer exp(workpiece, TopAbs_EDGE); exp.More(); exp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
            BRepAdaptor_Curve curve(edge);

            if (curve.GetType() == GeomAbs_Circle) {
                gp_Circ circ = curve.Circle();
                double diameter = circ.Radius() * 2.0;
                if (diameter > maxDiameter) {
                    maxDiameter = diameter;
                }
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "WorkpieceManager: Error detecting circular edges:" << e.what();
    }

    return maxDiameter;
}
