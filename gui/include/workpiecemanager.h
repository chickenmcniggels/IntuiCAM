#ifndef WORKPIECEMANAGER_H
#define WORKPIECEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <gp_Ax1.hxx>

/**
 * @brief Cylinder detection result structure
 */
struct CylinderInfo {
    gp_Ax1 axis;
    double diameter;
    double estimatedLength;
    QString description;
    
    CylinderInfo(const gp_Ax1& ax, double diam, double len, const QString& desc = "")
        : axis(ax), diameter(diam), estimatedLength(len), description(desc) {}
};

/**
 * @brief Manages workpiece display and geometric analysis
 * 
 * This class handles:
 * - Workpiece loading and display
 * - Cylinder detection in workpieces
 * - Manual axis selection interface
 * - Workpiece material properties
 * - Workpiece positioning and transformation
 */
class WorkpieceManager : public QObject
{
    Q_OBJECT

public:
    explicit WorkpieceManager(QObject *parent = nullptr);
    ~WorkpieceManager();

    /**
     * @brief Initialize with AIS context
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Add a workpiece to the scene
     */
    bool addWorkpiece(const TopoDS_Shape& workpiece);

    /**
     * @brief Detect cylindrical features in a workpiece
     */
    QVector<gp_Ax1> detectCylinders(const TopoDS_Shape& workpiece);

    /**
     * @brief Get detailed information about all detected cylinders
     * @return Vector of CylinderInfo structures with detailed information
     */
    QVector<CylinderInfo> getDetectedCylindersInfo() const { return m_detectedCylinders; }

    /**
     * @brief Manually select which detected cylinder to use as the main axis
     * @param index Index of the cylinder in the detected cylinders list
     * @return True if selection was successful
     */
    bool selectCylinderAxis(int index);

    /**
     * @brief Get the number of detected cylinders
     */
    int getDetectedCylinderCount() const { return m_detectedCylinders.size(); }

    /**
     * @brief Get information about a specific detected cylinder
     * @param index Index of the cylinder
     * @return CylinderInfo structure, or invalid info if index is out of range
     */
    CylinderInfo getCylinderInfo(int index) const;

    /**
     * @brief Set a custom axis manually (not from detected cylinders)
     * @param axis The custom axis to use
     * @param diameter The diameter for this axis
     */
    void setCustomAxis(const gp_Ax1& axis, double diameter);

    /**
     * @brief Clear all workpieces
     */
    void clearWorkpieces();

    /**
     * @brief Get all current workpieces
     */
    QVector<Handle(AIS_Shape)> getWorkpieces() const { return m_workpieces; }

    /**
     * @brief Get the main cylinder axis from the current workpiece
     */
    gp_Ax1 getMainCylinderAxis() const { return m_mainCylinderAxis; }

    /**
     * @brief Get the detected cylinder diameter
     */
    double getDetectedDiameter() const { return m_detectedDiameter; }

    /**
     * @brief Get the selected cylinder index (-1 for automatic/custom)
     * @return Index of currently selected cylinder
     */
    int getSelectedCylinderIndex() const { return m_selectedCylinderIndex; }

    /**
     * @brief Transform workpiece by flipping orientation around a perpendicular axis
     * @param flipped True to flip, false to restore original orientation
     * @return True if transformation was successful
     */
    bool flipWorkpieceOrientation(bool flipped);

    /**
     * @brief Move workpiece along the main cylinder axis
     * @param distance Distance to move in mm (positive = away from chuck)
     * @return True if positioning was successful
     */
    bool positionWorkpieceAlongAxis(double distance);

    /**
     * @brief Get the current workpiece transformation state
     * @return True if workpiece is currently flipped
     */
    bool isWorkpieceFlipped() const { return m_isFlipped; }

    /**
     * @brief Get the current workpiece position offset
     * @return Distance offset from original position in mm
     */
    double getWorkpiecePositionOffset() const { return m_positionOffset; }

signals:
    /**
     * @brief Emitted when a cylinder is detected in a workpiece
     */
    void cylinderDetected(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Emitted when multiple cylinders are detected
     * @param cylinders Vector of detected cylinder information
     */
    void multipleCylindersDetected(const QVector<CylinderInfo>& cylinders);

    /**
     * @brief Emitted when a cylinder axis is manually selected
     * @param index Index of the selected cylinder
     * @param cylinderInfo Information about the selected cylinder
     */
    void cylinderAxisSelected(int index, const CylinderInfo& cylinderInfo);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    QVector<Handle(AIS_Shape)> m_workpieces;
    
    // Analysis results
    gp_Ax1 m_mainCylinderAxis;
    double m_detectedDiameter;
    QVector<CylinderInfo> m_detectedCylinders;
    int m_selectedCylinderIndex;
    
    // Transformation state
    bool m_isFlipped;
    double m_positionOffset;
    
    /**
     * @brief Helper to analyze shape topology for cylinders
     */
    void analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders);

    /**
     * @brief Enhanced cylinder analysis with detailed information
     */
    void performDetailedCylinderAnalysis(const TopoDS_Shape& shape);
    
    /**
     * @brief Set workpiece material properties
     */
    void setWorkpieceMaterial(Handle(AIS_Shape) workpieceAIS);

    /**
     * @brief Estimate cylinder length from workpiece geometry
     */
    double estimateCylinderLength(const TopoDS_Shape& workpiece, const gp_Ax1& axis);

    /**
     * @brief Generate description for a detected cylinder
     */
    QString generateCylinderDescription(const CylinderInfo& info, int index);
};

#endif // WORKPIECEMANAGER_H 