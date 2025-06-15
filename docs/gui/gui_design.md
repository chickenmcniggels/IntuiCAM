# Details regarding the GUI-Component 
## Proposed Features (Turning)
### Step Viewer
- Allows for selection of to be turned or to be skipped surfaces
- Allows for selection of rotary
- Workpiece is shown inside of raw material is shown held by the chuck

### Automatic Step Generation
Every imported part now receives a predefined list of turning steps automatically.
These steps appear in a timeline at the bottom of the window in the following order:

1. **Contouring** – combines facing, roughing and finishing.
2. **Threading** – user selects the faces to thread and defines the pitch.
3. **Chamfering** – edges can be chosen and a chamfer size assigned.
4. **Parting** – optional cutoff of the workpiece.

Each step has a checkbox in the timeline to enable or disable it. When checked,
the toolpath preview is generated live. The previous `+` button and the
*Generate Toolpaths Automatically* action have been removed, as the default
steps are created for every part.

### Parameter Editing
The left side of the setup tab is divided into two areas:

* **Part Setup** – upload the STEP file, set orientation and choose the stock diameter.
* **Step Parameters** – a tab view with one tab per machining step. Each tab allows
  tool selection and configuration of parameters such as threading surfaces or
  chamfer size.

  ### Simulation
  A simulation combinded with collision detection will be automatically invoked (Similiar to 3D-Printing Slicers)

  ### Tools
  - This set of Tools will be supported and things will be optimized for them. https://www.paulimot.de/drehmeissel-set/mit_wendeplatte/12mm-

    
  - However Users can add their own tools via a wizard that takes the official ISO Codes and convert them to geometry
    
