# IntuiCAM Toolpath Generation Pipeline - Implementation Plan

## Overview
This document outlines the comprehensive plan for implementing a robust toolpath generation pipeline in IntuiCAM. The pipeline will handle multiple CNC turning operations with proper 2D profile extraction, parameter management, visualization, and debugging capabilities.

## Project Scope
- **Core Operations**: Contouring (Facing + Roughing + Finishing), Threading, Chamfering, Parting
- **Architecture**: 2D profile agnostic design with modular pipeline architecture
- **Visualization**: Real-time 3D display with visibility controls
- **Parameters**: Advanced configuration management with validation
- **Quality**: Safe code with comprehensive debugging and error handling

---

## Phase 1: Architecture Design & Core Infrastructure ❌ TODO

### 1.1 Pipeline Architecture Design
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: None  

#### Tasks:
- [ ] Design `ToolpathPipeline` core class using Command pattern
- [ ] Implement `IToolpathOperation` interface with Strategy pattern
- [ ] Create `Profile2DExtractor` with Factory pattern for different geometries
- [ ] Design `ParameterManager` with Observer pattern for configuration changes
- [ ] Implement `ToolpathVisualizer` with Visitor pattern for different toolpath types

#### Design Patterns:
- **Pipeline Pattern**: For sequential operation processing
- **Strategy Pattern**: For different operation algorithms
- **Factory Pattern**: For creating operation-specific extractors
- **Observer Pattern**: For parameter change notifications
- **Command Pattern**: For undoable operations
- **Visitor Pattern**: For toolpath visualization

#### Key Classes:
```cpp
namespace IntuiCAM::Toolpath {
    class ToolpathPipeline;
    class IToolpathOperation;
    class Profile2DExtractor;
    class ParameterManager;
    class ToolpathVisualizer;
    class OperationContext;
}
```

### 1.2 Data Structures Design
**Status**: ❌ Not Started  
**Estimated Time**: 1-2 days  
**Dependencies**: 1.1  

#### Tasks:
- [ ] Design `Toolpath2D` structure for 2D profile representation
- [ ] Create `ToolpathSegment` for individual toolpath components
- [ ] Implement `OperationParameters` hierarchy for different operations
- [ ] Design `VisualizationState` for visibility management
- [ ] Create `PipelineResult` for operation outcomes

#### Data Structures:
```cpp
struct Toolpath2D {
    std::vector<gp_Pnt2d> profile_points;
    TopoDS_Wire profile_wire;
    std::vector<ToolpathSegment> segments;
    OperationType operation_type;
    BoundingBox2D bounds;
};

struct ToolpathSegment {
    std::vector<gp_Pnt> points;
    MotionType motion_type; // Rapid, Feed, Arc
    double feed_rate;
    SpindleState spindle_state;
};
```

### 1.3 Core Interface Definition
**Status**: ❌ Not Started  
**Estimated Time**: 1 day  
**Dependencies**: 1.1, 1.2  

#### Tasks:
- [ ] Define `IToolpathOperation` interface
- [ ] Create `IProfile2DExtractor` interface
- [ ] Design `IParameterValidator` interface
- [ ] Implement `IToolpathVisualizer` interface
- [ ] Create `IPipelineLogger` for debugging

---

## Phase 2: 2D Profile Extraction System ❌ TODO

### 2.1 Profile Extraction Engine
**Status**: ❌ Not Started  
**Estimated Time**: 3-4 days  
**Dependencies**: Phase 1  

#### Tasks:
- [ ] Implement `STEPProfile2DExtractor` using OpenCASCADE B-Rep analysis
- [ ] Create `LatheProfileAnalyzer` for turning-specific geometry
- [ ] Develop `ProfileSimplification` algorithm for optimal toolpaths
- [ ] Implement `ProfileValidation` for geometric consistency
- [ ] Add `ProfileCaching` for performance optimization

#### Key Features:
- B-Rep face analysis for revolution profiles
- Automatic axis detection and alignment
- Profile simplification with tolerance control
- Validation for manufacturing feasibility
- Caching for repeated operations

### 2.2 Geometry Processing
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: 2.1  

#### Tasks:
- [ ] Implement contour extraction from STEP faces
- [ ] Create edge classification (external, internal, transition)
- [ ] Develop feature recognition (grooves, chamfers, radii)
- [ ] Implement stock material boundary detection
- [ ] Add geometric constraint validation

---

## Phase 3: Operation Implementation ❌ TODO

### 3.1 Contouring Operation System
**Status**: ❌ Not Started  
**Estimated Time**: 4-5 days  
**Dependencies**: Phase 2  

#### Tasks:
- [ ] Enhance `ContouringOperation` with 2D profile integration
- [ ] Implement multi-pass facing with configurable step-over
- [ ] Create adaptive roughing cycle with material detection
- [ ] Develop finishing pass generation with stock allowance
- [ ] Add tool collision detection and avoidance

#### Features:
- Configurable number of facing passes
- Stock-aware roughing with optimal tool paths
- Finishing allowance management
- Automatic tool selection validation
- Collision detection with chuck and fixtures

### 3.2 Threading Operation System
**Status**: ❌ Not Started  
**Estimated Time**: 3-4 days  
**Dependencies**: Phase 2  

#### Tasks:
- [ ] Enhance `ThreadingOperation` with face-based threading
- [ ] Implement thread pitch calculation and validation
- [ ] Create multi-start thread support
- [ ] Develop thread relief and chamfer integration
- [ ] Add thread form validation against tool geometry

### 3.3 Chamfering Operation System
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: Phase 2  

#### Tasks:
- [ ] Create `ChamferingOperation` for edge-based chamfering
- [ ] Implement chamfer angle and depth calculation
- [ ] Add automatic edge detection and selection
- [ ] Create tool path optimization for multiple chamfers
- [ ] Implement chamfer quality validation

### 3.4 Parting Operation System
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: Phase 2  

#### Tasks:
- [ ] Enhance `PartingOperation` with profile-aware parting
- [ ] Implement part-off position optimization
- [ ] Create stock support and clamping consideration
- [ ] Add part catch and handling safety features
- [ ] Implement parting tool validation

---

## Phase 4: Parameter Management System ❌ TODO

### 4.1 Advanced Parameter Configuration
**Status**: ❌ Not Started  
**Estimated Time**: 3-4 days  
**Dependencies**: Phase 3  

#### Tasks:
- [ ] Extend GUI `OperationParameterDialog` with advanced mode
- [ ] Create parameter validation with real-time feedback
- [ ] Implement parameter templates and presets
- [ ] Add parameter dependency management
- [ ] Create parameter export/import functionality

#### New Parameters to Add:
```cpp
// Contouring Parameters
struct ContouringParams {
    int facing_passes = 1;
    double facing_step_over = 2.0; // mm
    double roughing_step_down = 1.0; // mm
    double finishing_allowance = 0.2; // mm
    int finishing_passes = 1;
    bool optimize_tool_path = true;
};

// Threading Parameters
struct ThreadingParams {
    double thread_pitch = 1.5; // mm
    int thread_starts = 1;
    double thread_depth = 1.0; // mm
    bool add_thread_relief = true;
    double relief_width = 2.0; // mm
};
```

### 4.2 Parameter Validation System
**Status**: ❌ Not Started  
**Estimated Time**: 2 days  
**Dependencies**: 4.1  

#### Tasks:
- [ ] Create parameter range validation
- [ ] Implement cross-parameter consistency checks
- [ ] Add material-based parameter suggestions
- [ ] Create tool compatibility validation
- [ ] Implement safety parameter enforcement

---

## Phase 5: Visualization & UI Integration ❌ TODO

### 5.1 Toolpath Visualization
**Status**: ❌ Not Started  
**Estimated Time**: 4-5 days  
**Dependencies**: Phase 3  

#### Tasks:
- [ ] Integrate toolpath display with `OpenGL3DWidget`
- [ ] Implement different visualization modes (wireframe, solid, rapid/feed)
- [ ] Create 2D profile overlay visualization
- [ ] Add toolpath animation and simulation preview
- [ ] Implement color-coded operation visualization

#### Visualization Features:
- Real-time toolpath generation preview
- Color coding by operation type
- Feed rate visualization
- Tool position animation
- Material removal simulation

### 5.2 Visibility Control System
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: 5.1  

#### Tasks:
- [ ] Add visibility menu to main window
- [ ] Implement per-operation visibility toggle
- [ ] Create 2D profile visibility control
- [ ] Add tool visualization toggle
- [ ] Implement chuck and fixture visibility

#### UI Controls:
```cpp
// New menu items in View menu
- Show/Hide Toolpaths
  - Show Facing Operations
  - Show Roughing Operations  
  - Show Finishing Operations
  - Show Threading Operations
  - Show Chamfering Operations
  - Show Parting Operations
- Show/Hide 2D Profile
- Show/Hide Tool Positions
- Show/Hide Material Removal
```

---

## Phase 6: Pipeline Integration & Testing ❌ TODO

### 6.1 Pipeline Orchestration
**Status**: ❌ Not Started  
**Estimated Time**: 3-4 days  
**Dependencies**: Phase 5  

#### Tasks:
- [ ] Integrate pipeline with `WorkspaceController`
- [ ] Implement operation sequencing and dependencies
- [ ] Create pipeline state management
- [ ] Add progress tracking and user feedback
- [ ] Implement pipeline cancellation and cleanup

### 6.2 Error Handling & Debugging
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: 6.1  

#### Tasks:
- [ ] Implement comprehensive error handling
- [ ] Create detailed logging system
- [ ] Add operation validation and warnings
- [ ] Implement graceful degradation
- [ ] Create debugging visualization tools

### 6.3 Performance Optimization
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: 6.1  

#### Tasks:
- [ ] Implement background processing for heavy operations
- [ ] Add progress indicators for long-running tasks
- [ ] Optimize memory usage for large toolpaths
- [ ] Implement toolpath caching and reuse
- [ ] Add performance monitoring and metrics

---

## Phase 7: Testing & Validation ❌ TODO

### 7.1 Unit Testing
**Status**: ❌ Not Started  
**Estimated Time**: 3-4 days  
**Dependencies**: Phase 6  

#### Tasks:
- [ ] Create tests for 2D profile extraction
- [ ] Test all operation implementations
- [ ] Validate parameter management system
- [ ] Test visualization components
- [ ] Create pipeline integration tests

### 7.2 Integration Testing
**Status**: ❌ Not Started  
**Estimated Time**: 2-3 days  
**Dependencies**: 7.1  

#### Tasks:
- [ ] Test complete workflows with sample parts
- [ ] Validate G-code generation integration
- [ ] Test UI responsiveness under load
- [ ] Validate memory management
- [ ] Test error recovery scenarios

---

## Technical Specifications

### Design Patterns Used
1. **Pipeline Pattern**: Sequential operation processing
2. **Strategy Pattern**: Pluggable operation algorithms
3. **Factory Pattern**: Operation and extractor creation
4. **Observer Pattern**: Parameter change notifications
5. **Command Pattern**: Undoable operations
6. **Visitor Pattern**: Toolpath visualization
7. **Singleton Pattern**: Global configuration management

### Data Flow Architecture
```
STEP File → Profile2DExtractor → OperationPipeline → ToolpathGenerator → Visualizer
     ↓              ↓                     ↓               ↓              ↓
   B-Rep        2D Profile          Parameters      Toolpaths      3D Display
 Analysis      Extraction           Validation      Generation     + Controls
```

### Performance Requirements
- Profile extraction: < 2 seconds for typical parts
- Toolpath generation: < 5 seconds per operation
- Visualization update: < 100ms for interactive feedback
- Memory usage: < 500MB for complex parts
- UI responsiveness: Always maintain 60fps during generation

### Safety & Validation
- Parameter range validation with warnings
- Tool collision detection
- Material removal validation
- Chuck clearance verification
- Feed rate safety limits
- Spindle speed validation

---

## Progress Tracking

### Completed Phases: 0/7
- [ ] Phase 1: Architecture Design & Core Infrastructure
- [ ] Phase 2: 2D Profile Extraction System  
- [ ] Phase 3: Operation Implementation
- [ ] Phase 4: Parameter Management System
- [ ] Phase 5: Visualization & UI Integration
- [ ] Phase 6: Pipeline Integration & Testing
- [ ] Phase 7: Testing & Validation

### Estimated Timeline
- **Total Estimated Time**: 25-35 development days
- **Critical Path**: Profile Extraction → Operation Implementation → Visualization
- **Risk Areas**: OpenCASCADE B-Rep complexity, Performance optimization
- **Milestones**: 
  - Week 2: Core architecture complete
  - Week 4: Profile extraction working
  - Week 6: Basic operations implemented
  - Week 8: Full pipeline integrated

### Key Deliverables
1. **Core Pipeline Infrastructure** (Phase 1-2)
2. **Operation Implementations** (Phase 3)
3. **Enhanced Parameter Management** (Phase 4)
4. **Integrated Visualization** (Phase 5)
5. **Production-Ready System** (Phase 6-7)

---

## Notes & Considerations

### Context7 Integration
- Use Qt 6 best practices for signals/slots
- Follow Modern C++ patterns (C++17/20)
- Leverage OpenCASCADE efficiently
- Maintain high test coverage

### Future Enhancements
- Multi-threading for parallel operations
- Advanced collision detection
- Adaptive toolpath optimization
- Machine-specific post-processing
- Real-time material removal simulation

### Risk Mitigation
- Incremental implementation with testing
- Fallback options for complex geometries
- Performance monitoring throughout
- Regular integration testing
- User feedback incorporation

---

**Last Updated**: Created  
**Next Review**: After Phase 1 completion  
**Status**: Planning Phase 