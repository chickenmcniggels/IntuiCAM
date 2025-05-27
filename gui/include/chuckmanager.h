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

class IStepLoader;

/**
 * @brief Manages 3-jaw chuck display functionality
 * 
 * This class handles only chuck-specific functionality:
 * - Persistent display of the 3-jaw chuck STEP file
 * - Chuck material properties and positioning
 * - Chuck-related configuration and status
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

signals:
    /**
     * @brief Emitted when chuck is successfully loaded
     */
    void chuckLoaded();

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
    
    /**
     * @brief Set chuck material properties
     */
    void setChuckMaterial(Handle(AIS_Shape) chuckAIS);
};

#endif // CHUCKMANAGER_H 