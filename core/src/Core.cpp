#include "IntuiCAMCore/Core.h"

using namespace IntuiCAM::Core;

CAMEngine::CAMEngine() { }
CAMEngine::~CAMEngine() { }

#include "IntuiCAMCore/Core.h"
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <IFSelect_ReturnStatus.hxx>

using namespace IntuiCAM::Core;

CAMEngine::CAMEngine() { }
CAMEngine::~CAMEngine() { }

bool CAMEngine::loadSTEP(const std::string& path) {
    // 1) Create a STEP reader
    STEPControl_Reader reader;
    // 2) Read the file into the reader; IFSelect_RetDone indicates success
    IFSelect_ReturnStatus stat = reader.ReadFile(path.c_str());
    if (stat != IFSelect_RetDone) {
        return false;
    }
    // 3) Transfer all root entities into OCCT TopoDS_Shapes
    reader.TransferRoots();
    // 4) Retrieve the combined shape (might be a compound if multiple parts)
    TopoDS_Shape shape = reader.OneShape();
    // 5) Store it (e.g. for later visualization or processing)
    myShape = shape;
    return true;
}


std::vector<std::string> CAMEngine::computeToolpaths() {
    // TODO: generate real toolpaths
    return { "G0 X0 Z0", "G1 X10 Z-5" };
}

bool CAMEngine::exportGCode(const std::string& outPath) {
    // TODO: write toolpaths to file
    return true;
}

