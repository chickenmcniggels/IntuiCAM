#ifndef ISTEPLOADER_H
#define ISTEPLOADER_H

#include <QString>
#include <TopoDS_Shape.hxx>

/**
 * @brief Interface for STEP file loading functionality
 * 
 * This interface allows components to load STEP files without
 * being tightly coupled to a specific implementation.
 */
class IStepLoader
{
public:
    virtual ~IStepLoader() = default;
    
    /**
     * @brief Load a STEP file and return the shape
     * @param filename Path to the STEP file
     * @return Loaded shape, or null shape if failed
     */
    virtual TopoDS_Shape loadStepFile(const QString& filename) = 0;
    
    /**
     * @brief Get the last error message
     * @return Error message string
     */
    virtual QString getLastError() const = 0;
    
    /**
     * @brief Check if the last operation was successful
     * @return True if valid, false otherwise
     */
    virtual bool isValid() const = 0;
};

#endif // ISTEPLOADER_H 