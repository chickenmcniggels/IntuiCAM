# Updated Toolpath Workflow

This document summarizes the simplified toolpath generation workflow introduced in IntuiCAM.

## Automatic Step Creation

When a STEP file is loaded, the application automatically creates a set of machining steps. These steps are displayed in order at the bottom of the window in the **toolpath timeline**:

1. **Contouring** – automatically performs facing, roughing and finishing of the entire part.
2. **Threading** – lets you choose the faces to be threaded and define the pitch.
3. **Chamfering** – lets you select edges to chamfer and specify the chamfer size.
4. **Parting** – allows you to define how the part should be parted off.

All steps are generated for every uploaded part. You can enable or disable each step individually in the timeline.

## Revised Setup Tab Layout

The left side of the setup tab has been simplified:

- The previous tab widget with *Part Setup* and *Machining* tabs has been removed.
- The **upper half** now contains the part setup controls. Here you upload the STEP file, set the orientation and choose the material type and starting diameter.
- The **lower half** contains a new tab view with one tab for each machining step (Contouring, Threading, Chamfering, Parting). Each tab lets you choose tools and adjust parameters relevant to that step.

These settings are dedicated to lathe turning and are independent for every step.  The timeline at the bottom always shows the steps in the same order, but you can deactivate any of them.
