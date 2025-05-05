#include "IntuiCAMCore/Core.h"

using namespace IntuiCAM::Core;

CAMEngine::CAMEngine() { }
CAMEngine::~CAMEngine() { }

bool CAMEngine::loadSTEP(const std::string& path) {
    // TODO: implement STEP import via OCCT
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

