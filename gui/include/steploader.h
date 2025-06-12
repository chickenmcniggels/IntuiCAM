#ifndef STEPLOADER_H
#define STEPLOADER_H

#include <QString>
#include <TopoDS_Shape.hxx>
#include <string>
#include <IntuiCAM/Geometry/IStepLoader.h>

class StepLoader : public IntuiCAM::Geometry::IStepLoader
{
public:
    StepLoader();
    ~StepLoader();
    
    // Load a STEP file and return the shape
    TopoDS_Shape loadStepFile(const std::string& filename) override;
    
    // Get the last error message
    std::string getLastError() const override { return m_lastError; }
    
    // Check if the last operation was successful
    bool isValid() const override { return m_isValid; }

private:
    std::string m_lastError;
    bool m_isValid;
};

#endif // STEPLOADER_H 