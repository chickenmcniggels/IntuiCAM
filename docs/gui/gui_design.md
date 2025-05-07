# Details regarding the GUI-Component 
## Proposed Features (Turning)
### Step Viewer
- Allows for selection of to be turned or to be skipped surfaces
- Allows for selection of rotary
- Workpiece is shown inside of raw material is shown held by the chuck

### Manual Path Generation
- User can manually select which feature to turn where
- The following toolpaths are proposed:
  - Outside roughing
  - Outside finishing
  - Inside roughing
  - Inside finishing
  - Drilling
  - Facing
  - Parting
  - Threading
  - (Trepaning - Turning on the face of the part)
  - (Adaptive Roughing)
- The toolpath selection should be stored in a list where the order of the list corresponds to the order of machining operations
- When a toolpath is first set up or selected in the list, a menu opens where the user can adjust parameters

  ### Atomatic Toolpath Generation
  The User can click a button and the following steps are automated.
  - The standard order of operations is automatically added to the toolpath list <br>

    Facing &rarr; Drilling &rarr; Internal Roughing &rarr; Internal Finishing &rarr; External Roughing &rarr; External Finishing &rarr; Parting <br>

    (If the part doesnt have internal features for example, the corresponding steps will be skipped)

  ### Simulation
  A simulation combinded with collision detection will be automatically invoked (Similiar to 3D-Printing Slicers)

  ### Tools
  - This set of Tools will be supported and things will be optimized for them. https://www.paulimot.de/drehmeissel-set/mit_wendeplatte/12mm-

    
  - However Users can add their own tools via a wizard that takes the official ISO Codes and convert them to geometry
    
