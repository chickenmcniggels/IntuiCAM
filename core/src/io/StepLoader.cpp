#include "./../include/IntuiCAMCore/io/StepLoader.h"
#include <STEPControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>

using namespace IntuiCAM::Core::IO;

bool StepLoader::loadStep(const std::string& filePath, TopoDS_Shape& outShape) {
    STEPControl_Reader reader;
    if (IFSelect_ReturnStatus status = reader.ReadFile(filePath.c_str()); status != IFSelect_RetDone) {
        return false;  // Failed to read the file (file not found or invalid format)
    }

    // Transfer all roots from the STEP file into OpenCASCADE shapes
    if (reader.TransferRoots() <= 0) {
        return false;  // No transferable roots found or translation failed
    }

    // Collect the results into one shape (could be a compound of multiple parts)
    outShape = reader.OneShape();
    return !outShape.IsNull();
}
