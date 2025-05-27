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
 * @brief Manages workpiece display and geometric analysis
 * 
 * This class handles:
 * - Workpiece loading and display
 * - Cylinder detection in workpieces
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

signals:
    /**
     * @brief Emitted when a cylinder is detected in a workpiece
     */
    void cylinderDetected(double diameter, double length, const gp_Ax1& axis);

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
    
    /**
     * @brief Helper to analyze shape topology for cylinders
     */
    void analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders);
    
    /**
     * @brief Set workpiece material properties
     */
    void setWorkpieceMaterial(Handle(AIS_Shape) workpieceAIS);
};

#endif // WORKPIECEMANAGER_H 