#include "../core/include/IntuiCAMCore/Core.h"
#include <cassert>

int main() {
    IntuiCAM::Core::CAMEngine engine;
    assert(engine.loadSTEP("dummy.step"));
    auto paths = engine.computeToolpaths();
    assert(!paths.empty());
    assert(engine.exportGCode("dummy.gcode"));
    return 0;
}
