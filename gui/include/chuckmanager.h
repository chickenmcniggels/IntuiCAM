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

namespace IntuiCAM { namespace Geometry { class IStepLoader; } }
using IStepLoader = IntuiCAM::Geometry::IStepLoader;

/**
 * @brief Manages 3-jaw chuck display functionality
 * 
 * This class handles only chuck-specific functionality:
 * - Persistent display of the 3-jaw chuck STEP file
 * - Chuck material properties and positioning
 * - Chuck-related configuration and status
 * - Chuck centerline axis detection and alignment
 */
class ChuckManager : public QObject
{
    Q_OBJECT

public:
    explicit ChuckManager(QObject *parent = nullptr);
    ~ChuckManager();

    /**
     * @brief Initialize the chuck manager with AIS context and STEP loader
     */
    void initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader);

    /**
     * @brief Load and display the 3-jaw chuck permanently
     */
    bool loadChuck(const QString& chuckFilePath);

    /**
     * @brief Clear chuck display
     */
    void clearChuck();

    /**
     * @brief Get the chuck shape if loaded
     */
    TopoDS_Shape getChuckShape() const { return m_chuckShape; }

    /**
     * @brief Check if chuck is loaded and displayed
     */
    bool isChuckLoaded() const { return !m_chuckShape.IsNull(); }

    /**
     * @brief Get the chuck centerline axis for alignment
     * @return The main axis of the chuck (typically Z-axis through origin)
     */
    gp_Ax1 getChuckCenterlineAxis() const;

    /**
     * @brief Detect and analyze the chuck geometry to find its centerline
     * @return True if centerline was successfully detected
     */
    bool detectChuckCenterline();

    /**
     * @brief Set a custom chuck centerline axis (for manual override)
     * @param axis The custom axis to use as chuck centerline
     */
    void setCustomChuckCenterline(const gp_Ax1& axis);

    /**
     * @brief Check if chuck centerline has been detected or set
     */
    bool hasValidCenterline() const { return m_centerlineDetected; }

    /**
     * @brief Verify that the chuck is properly configured as non-selectable
     * @return True if chuck is non-selectable and non-highlightable
     */
    bool isChuckNonSelectable() const;

    /**
     * @brief Redisplay the chuck (used after clearing the context)
     */
    void redisplayChuck();

    /**
     * @brief Show or hide the chuck without deleting it
     */
    void setChuckVisible(bool visible);

    /**
     * @brief Check if chuck is currently visible
     */
    bool isChuckVisible() const;

    /**
     * @brief Get the current chuck AIS object
     * @return Current chuck AIS object, or null if not loaded
     */
    Handle(AIS_Shape) getChuckAIS() const { return m_chuckAIS; }

signals:
    /**
     * @brief Emitted when chuck is successfully loaded
     */
    void chuckLoaded();

    /**
     * @brief Emitted when chuck centerline is detected
     * @param axis The detected centerline axis
     */
    void chuckCenterlineDetected(const gp_Ax1& axis);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    IStepLoader* m_stepLoader;
    
    // Chuck related
    TopoDS_Shape m_chuckShape;
    Handle(AIS_Shape) m_chuckAIS;
    
    // Chuck centerline alignment
    gp_Ax1 m_chuckCenterlineAxis;
    bool m_centerlineDetected;
    bool m_isVisible { true };
    
    /**
     * @brief Set chuck material properties
     */
    void setChuckMaterial(Handle(AIS_Shape) chuckAIS);

    /**
     * @brief Analyze chuck geometry to find the main rotational axis
     */
    void analyzeChuckGeometry();
};

#endif // CHUCKMANAGER_H 