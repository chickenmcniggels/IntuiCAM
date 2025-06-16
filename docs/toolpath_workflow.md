# Updated Toolpath Workflow

This document summarizes the simplified toolpath generation workflow introduced in IntuiCAM.

## Automatic Step Creation

When a STEP file is loaded, the application automatically creates a set of machining steps. These steps are displayed in order at the bottom of the window in the **toolpath timeline**:

1. **Contouring** – automatically performs facing, roughing and finishing of the entire part.
2. **Threading** – lets you choose the faces to be threaded and define the pitch.
3. **Chamfering** – lets you select edges to chamfer and specify the chamfer size.
4. **Parting** – allows you to define how the part should be parted off.

All steps are generated for every uploaded part. You can enable or disable each step individually in the timeline.

The old **Add (+)** and **Generate Toolpaths Automatically** buttons have been removed. Toolpaths are created as soon as you enable an operation in the setup panel.

## Revised Setup Tab Layout

The left side of the setup tab has been simplified:

- The previous tab widget with *Part Setup* and *Machining* tabs has been removed.
- The **upper half** now contains the part setup controls. Here you upload the STEP file, set the orientation and choose the material type and starting diameter.
- The **lower half** contains a new tab view with one tab for each machining step (Contouring, Threading, Chamfering, Parting). Each tab lets you choose tools and adjust parameters relevant to that step.

All operation parameters are edited directly in these tabs. The previous parameter dialog window has been removed. Clicking an entry in the timeline automatically focuses the corresponding tab so you can tweak its settings.

These settings are dedicated to lathe turning and are independent for every step.  The timeline at the bottom always shows the steps in the same order, but you can deactivate any of them.

### Simple vs Advanced Mode

The setup panel now offers a checkbox labeled **Advanced Mode**. When disabled, only the most important parameters are shown for each operation. For example, Contouring displays just the facing allowance and the final surface finish. Threading and Chamfering list the selected faces in tables where you can add or remove entries and adjust individual parameters.

Enabling **Advanced Mode** reveals additional controls such as calculated cutting depth, feed rate and spindle speed. These values are pre‑filled from the material database but can be fine‑tuned manually.

In this mode, Contouring exposes separate **Facing**, **Roughing** and **Finishing** sections. Each section lets you specify spindle speed (RPM), feed rate, surface speed and cutting depth and includes a checkbox for constant surface speed.
Parting offers the same parameters plus a drop-down to choose the retract mode. When Advanced Mode is disabled, you simply enable or disable flood coolant for each operation.
For Chamfering, the edge table now contains a *Chamfer Type* column allowing `Equal Distance`, `Two Distance` or `Distance & Angle` values per edge.
