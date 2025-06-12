# CAM Core Engine Overview

## Automatic Turning Workflow
IntuiCAM's initial focus is CNC turning. The core automatically creates four default steps when a new job is generated:

1. **Contouring**
2. **Threading**
3. **Chamfering**
4. **Parting**

These steps appear in the GUI as a timeline. Each can be toggled on or off before toolpath computation. Support for additional operations such as roughing and drilling is planned for future versions.

All turning operations support cutter radius compensation. It can be applied on the CAM side or delegated to the machine controller.

## Future Milling Capabilities
Milling toolpaths are not yet implemented but the architecture allows for future extensions (e.g. adaptive roughing, contour milling, drilling).
