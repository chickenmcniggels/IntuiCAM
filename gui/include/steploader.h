#ifndef STEPLOADER_H
#define STEPLOADER_H

#include <QString>
#include <TopoDS_Shape.hxx>
#include "isteploader.h"

class StepLoader : public IStepLoader
{
public:
    StepLoader();
    ~StepLoader();
    
    // Load a STEP file and return the shape
    TopoDS_Shape loadStepFile(const QString& filename) override;
    
    // Get the last error message
    QString getLastError() const override { return m_lastError; }
    
    // Check if the last operation was successful
    bool isValid() const override { return m_isValid; }

private:
    QString m_lastError;
    bool m_isValid;
};

#endif // STEPLOADER_H 