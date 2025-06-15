# Details regarding the GUI-Component 
## Proposed Features (Turning)
### Step Viewer
- Allows for selection of to be turned or to be skipped surfaces
- Allows for selection of rotary
- Workpiece is shown inside of raw material is shown held by the chuck

### Automatic Toolpath Timeline
After a STEP file is loaded, IntuiCAM automatically creates a fixed sequence of machining steps. These steps appear at the bottom of the window in the **toolpath timeline**. Each step can be toggled on or off and edited individually.

The standard steps are:
1. **Contouring** – performs facing, roughing and finishing in one operation.
2. **Threading** – user selects faces and pitch.
3. **Chamfering** – user selects edges and chamfer size.
4. **Parting** – defines part‑off strategy.

Selecting a step highlights its tab in the setup panel where all parameters can be edited directly. The separate parameter dialog has been removed.

### Automatic Generation
Toolpaths are created as soon as a STEP file is loaded. The timeline always starts with Contouring followed by Threading, Chamfering and Parting. Users simply review the steps and disable or modify them as needed.

  ### Simulation
  A simulation combinded with collision detection will be automatically invoked (Similiar to 3D-Printing Slicers)

  ### Tools
  - This set of Tools will be supported and things will be optimized for them. https://www.paulimot.de/drehmeissel-set/mit_wendeplatte/12mm-

    
  - However Users can add their own tools via a wizard that takes the official ISO Codes and convert them to geometry
    
