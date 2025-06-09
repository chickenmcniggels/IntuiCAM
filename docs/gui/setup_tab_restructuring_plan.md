# IntuiCAM Setup Tab Restructuring Plan

## Overview
Restructure the setup tab layout based on Orca Slicer/Bambu Studio's intuitive design, focusing on lathe-specific CAM settings and automatic toolpath generation.

## Phase 1: Analyze Current Layout and Design New Structure
**Estimated Complexity: Medium**  
**Recommended Agent Calls: 1-2**

### Tasks:
1. **Layout Analysis**
   - Document current setup tab structure (3-panel design: Project Tree | Part Loading Panel | Properties vs 3D Viewer)
   - Identify components to remove/modify/keep
   - Map Orca Slicer layout patterns to lathe CAM workflow

2. **Design New Layout Structure**
   - Create wireframe for new layout inspired by Orca Slicer
   - Plan left sidebar for settings panels
   - Plan right side for 3D viewport and operation controls
   - Design automatic toolpath generation workflow

## Phase 2: Create New Setup Configuration Panel
**Estimated Complexity: High**  
**Recommended Agent Calls: 2-3**

### Tasks:
1. **Create New Configuration Widget**
   - Replace current `PartLoadingPanel` with comprehensive `SetupConfigurationPanel`
   - Design collapsible sections similar to Orca Slicer's parameter groups

2. **Implement Core Settings Groups:**
   - **Part Setup**: STEP file loading, axis selection, positioning
   - **Material Settings**: Raw material diameter, material type selection, surface finish requirements
   - **Machining Parameters**: Facing allowance, stock allowance, safety margins
   - **Operations Control**: Individual operation toggles (facing, roughing, finishing, parting)
   - **Quality Settings**: Surface finish specifications, precision levels

3. **Add Operation Controls**
   - Toggle switches for each operation type
   - Basic parameter preview/quick adjustment
   - Visual indicators for enabled/disabled operations

## Phase 3: Implement Automatic Toolpath Generation
**Estimated Complexity: High**  
**Recommended Agent Calls: 2-3**

### Tasks:
1. **Create ToolpathGenerationController**
   - Implement automatic operation sequence logic
   - Part analysis for internal/external features
   - Intelligent operation selection based on geometry

2. **Add Generation Button and Progress UI**
   - Prominent "Generate Toolpaths" button
   - Progress indicator for generation process
   - Result summary and validation feedback

3. **Implement Operation List Management**
   - Dynamic operation list showing selected operations
   - Drag-and-drop reordering capability
   - Individual operation parameter access

## Phase 4: Create Operation Parameter Dialogs
**Estimated Complexity: Medium**  
**Recommended Agent Calls: 2**

### Tasks:
1. **Design Parameter Dialog System**
   - Modal dialogs for detailed operation parameters
   - Context-aware parameter sets based on part geometry
   - Real-time parameter validation and feedback

2. **Implement Operation-Specific Dialogs:**
   - Facing: stepover, feed rate, spindle speed
   - Roughing: depth of cut, stock allowance, feed rate
   - Finishing: surface finish, final dimensions, cutting speed
   - Parting: parting position, feed rate, safety settings

## Phase 5: Integrate Material and Tool Management
**Estimated Complexity: Medium**  
**Recommended Agent Calls: 1-2**

### Tasks:
1. **Material Selection System**
   - Dropdown/combobox for common materials (aluminum, steel, brass, etc.)
   - Material property integration with cutting parameters
   - Custom material definition capability

2. **Tool Integration Preview**
   - Display selected/recommended tools for operations
   - Tool capability validation against operations
   - Quick tool parameter access

## Phase 6: Layout Integration and Testing
**Estimated Complexity: Medium**  
**Recommended Agent Calls: 1-2**

### Tasks:
1. **Layout Assembly**
   - Integrate new configuration panel into setup tab
   - Ensure proper splitter sizing and responsiveness
   - Maintain 3D viewer functionality and positioning

2. **Connection and Signal Handling**
   - Connect new UI elements to existing WorkspaceController
   - Update signal/slot connections for new workflow
   - Ensure backward compatibility with existing systems

3. **Testing and Refinement**
   - Test automatic toolpath generation workflow
   - Validate parameter passing to core toolpath system
   - Ensure UI responsiveness and usability

## Phase 7: UI Polish and Advanced Features
**Estimated Complexity: Low-Medium**  
**Recommended Agent Calls: 1**

### Tasks:
1. **Visual Enhancements**
   - Apply consistent styling inspired by Orca Slicer
   - Add icons and visual indicators
   - Implement collapsible sections with animations

2. **User Experience Improvements**
   - Add tooltips and help text
   - Implement smart defaults based on part analysis
   - Add keyboard shortcuts for common operations

## Files to Modify/Create

### New Files:
- `gui/include/setupconfigurationpanel.h`
- `gui/src/setupconfigurationpanel.cpp`
- `gui/include/toolpathgenerationcontroller.h`
- `gui/src/toolpathgenerationcontroller.cpp`
- `gui/include/operationparameterdialog.h`
- `gui/src/operationparameterdialog.cpp`

### Modified Files:
- `gui/src/MainWindow.cpp` (createSetupTab method)
- `gui/include/mainwindow.h` (member variables and signals)
- `gui/src/workspacecontroller.cpp` (integration with new UI)

## Key Features to Implement

### Removed Features (Not Needed):
- Complex project tree (simplified to essential items)
- Detailed properties panel (integrated into configuration panel)
- Manual distance sliders (replaced with intelligent positioning)

### New Features (CAM-Specific):
- **Surface Finish Selection**: Ra values, machining strategies
- **Facing Allowance Control**: Stock for facing operations
- **Operation Toggle Switches**: Enable/disable operations individually
- **Material Database**: Common materials with cutting parameters
- **Automatic Tool Selection**: Based on operations and material
- **Intelligent Defaults**: Part analysis driving parameter suggestions
- **Operation Sequencing**: Visual operation order with customization
- **Quick Parameter Access**: Hover/click for operation details

## Success Criteria
1. Intuitive layout similar to Orca Slicer's organization
2. One-click automatic toolpath generation with intelligent defaults
3. Easy access to individual operation parameters
4. Clear visual feedback on enabled operations and settings
5. Seamless integration with existing 3D visualization
6. Professional CAM workflow that reduces setup time while maintaining control

## Implementation Progress Tracking

### ✅ Completed:
- Plan creation and documentation
- **Phase 1: Layout Analysis and Design** - COMPLETED
  - ✅ Analyzed current 3-panel layout structure
  - ✅ Designed new Orca Slicer-inspired layout
  - ✅ Created SetupConfigurationPanel header file
  - ✅ Implemented comprehensive SetupConfigurationPanel.cpp
  - ✅ Updated MainWindow.h with new components and signal handlers
  - ✅ Replaced createSetupTab() with new Orca Slicer-inspired layout
  - ✅ Added signal connections for new workflow
  - ✅ Implemented new slot handlers for configuration changes

- **Phase 2: Core Setup Configuration Panel** - COMPLETED
  - ✅ Created comprehensive SetupConfigurationPanel class
  - ✅ Implemented Part Setup group (STEP loading, axis selection, positioning)
  - ✅ Implemented Material Settings group (material type, raw dimensions, properties)
  - ✅ Implemented Machining Parameters group (allowances, safety margins)
  - ✅ Implemented Operations Control group (enable/disable operations, parameters access)
  - ✅ Implemented Quality Settings group (surface finish, tolerance)
  - ✅ Implemented Automatic Generation Controls (prominent button, progress tracking)
  - ✅ Added modern styling inspired by Orca Slicer with color-coded sections

### 🔄 In Progress:
- **Phase 3: Automatic Toolpath Generation** - PARTIALLY COMPLETED
  - ✅ Basic automatic generation workflow implemented
  - ✅ Operation list management and status tracking
  - ⏳ Integration with core toolpath generation system (placeholder)
  - ⏳ Progress feedback and result validation

### ⏳ Pending:
- Phase 4: Create Operation Parameter Dialogs
- Phase 5: Integrate Material and Tool Management
- Phase 6: Layout Integration and Testing
- Phase 7: UI Polish and Advanced Features

---

*This plan can be executed in phases, with each phase building upon the previous one. The modular approach allows for incremental development and testing while maintaining system stability.* 