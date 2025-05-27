#ifndef CHUCKMANAGER_H
#define CHUCKMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>

class StepLoader;

/**
 * @brief Manages 3-jaw chuck display and workpiece alignment functionality
 * 
 * This class handles:
 * - Persistent display of the 3-jaw chuck STEP file
 * - Detection of cylindrical features in workpieces
 * - Automatic alignment with standard material diameters
 * - Raw material display with transparency
 */
class ChuckManager : public QObject
{
    Q_OBJECT

public:
    explicit ChuckManager(QObject *parent = nullptr);
    ~ChuckManager();

    // Standard material diameters in mm (common turning stock sizes)
    static const QVector<double> STANDARD_DIAMETERS;

    /**
     * @brief Initialize the chuck manager with AIS context
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Load and display the 3-jaw chuck permanently
     */
    bool loadChuck(const QString& chuckFilePath);

    /**
     * @brief Add a workpiece to the scene
     */
    bool addWorkpiece(const TopoDS_Shape& workpiece);

    /**
     * @brief Detect cylindrical features in a workpiece
     */
    QVector<gp_Ax1> detectCylinders(const TopoDS_Shape& workpiece);

    /**
     * @brief Find the next largest standard diameter for a given diameter
     */
    double getNextStandardDiameter(double diameter);

    /**
     * @brief Align workpiece with chuck and add raw material
     */
    bool alignWorkpieceWithChuck(const TopoDS_Shape& workpiece, const gp_Ax1& cylinderAxis);

    /**
     * @brief Create and display raw material cylinder
     */
    void displayRawMaterial(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Clear all workpieces and raw material (keep chuck)
     */
    void clearWorkpieces();

    /**
     * @brief Set transparency for raw material display
     */
    void setRawMaterialTransparency(double transparency = 0.7);

    /**
     * @brief Get the chuck shape if loaded
     */
    TopoDS_Shape getChuckShape() const { return m_chuckShape; }

    /**
     * @brief Check if chuck is loaded and displayed
     */
    bool isChuckLoaded() const { return !m_chuckShape.IsNull(); }

signals:
    /**
     * @brief Emitted when a cylinder is detected in a workpiece
     */
    void cylinderDetected(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Emitted when workpiece is aligned with raw material
     */
    void workpieceAligned(double rawMaterialDiameter, double length);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    StepLoader* m_stepLoader;
    
    // Chuck related
    TopoDS_Shape m_chuckShape;
    Handle(AIS_Shape) m_chuckAIS;
    
    // Workpiece and raw material
    QVector<Handle(AIS_Shape)> m_workpieces;
    Handle(AIS_Shape) m_rawMaterialAIS;
    TopoDS_Shape m_currentRawMaterial;
    
    // Configuration
    double m_rawMaterialTransparency;
    
    /**
     * @brief Helper to analyze shape topology for cylinders
     */
    void analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders);
    
    /**
     * @brief Create a cylinder shape for raw material
     */
    TopoDS_Shape createCylinder(double diameter, double length, const gp_Ax1& axis);
};

#endif // CHUCKMANAGER_H 