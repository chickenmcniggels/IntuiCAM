#include "io/StepLoader.h"

// OpenCASCADE includes
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <Message_ProgressRange.hxx>

namespace IntuiCAM {
namespace IO {

StepLoader::StepLoader() {
}

StepLoader::~StepLoader() {
}

bool StepLoader::loadFile(const std::string& filename) {
    try {
        STEPControl_Reader reader;
        
        // Configure reader
        reader.SetNameMode(true);
        
        // Read the STEP file
        Message_ProgressRange progressRange;
        IFSelect_ReturnStatus status = reader.ReadFile(filename.c_str());
        
        if (status != IFSelect_RetDone) {
            return false;
        }
        
        // Transfer shapes
        reader.TransferRoots(progressRange);
        
        // Create a compound to store all shapes
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        
        // Get all shapes from the reader
        for (int i = 1; i <= reader.NbShapes(); i++) {
            const TopoDS_Shape& shape = reader.Shape(i);
            builder.Add(compound, shape);
        }
        
        // Store the compound as our model
        m_model = compound;
        return true;
    } catch (...) {
        return false;
    }
}

TopoDS_Shape StepLoader::getModel() const {
    return m_model;
}

} // namespace IO
} // namespace IntuiCAM 