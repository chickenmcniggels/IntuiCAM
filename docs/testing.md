# Testing Guide

This document details the testing approach for IntuiCAM, covering unit testing, component testing, integration testing, and GUI testing. It provides guidelines, examples, and best practices aligned with Qt and OpenCASCADE recommendations.

---

## 1. Testing Philosophy

IntuiCAM follows a comprehensive testing strategy to ensure reliability and maintainability:

* **Test-Driven Development (TDD)**: Write tests before implementing functionality when possible
* **Continuous Testing**: Run tests on every commit through CI
* **Layered Testing**: Cover all layers from individual functions to complete workflows
* **Clear Failures**: Tests should clearly indicate what failed and why

---

## 2. Test Organization

### 2.1 Test Directory Structure

```
IntuiCAM/
├── core/
│   ├── common/
│   │   ├── src/
│   │   └── tests/                   # Unit tests for common module
│   │       ├── test_types.cpp
│   │       └── ...
│   ├── geometry/
│   │   ├── src/
│   │   └── tests/                   # Unit tests for geometry module
│   │       ├── test_intersection.cpp
│   │       └── ...
│   └── ...
├── gui/
│   ├── src/
│   └── tests/                       # GUI component tests
│       ├── test_viewport.cpp
│       └── ...
└── tests/                           # Integration and system tests
    ├── test_workflow_turning.cpp
    └── ...
```

### 2.2 CMake Integration

Each test directory contains a `CMakeLists.txt` file that registers tests with CTest:

```cmake
# Example core/geometry/tests/CMakeLists.txt
add_executable(test_geometry 
  test_intersection.cpp
  test_transformation.cpp
)

target_link_libraries(test_geometry 
  PRIVATE 
  IntuiCAMCore 
  GTest::gtest_main
)

add_test(NAME GeometryTests COMMAND test_geometry)
```

---

## 3. Core Library Testing

### 3.1 Unit Testing with Google Test

IntuiCAM uses [Google Test](https://github.com/google/googletest) for unit testing the core C++ libraries.

#### Basic Test Structure

```cpp
#include <gtest/gtest.h>
#include <IntuiCAM/Geometry/Intersection.h>

// Test suite for the Intersection module
TEST(IntersectionTest, LineLineIntersection) {
  // Arrange
  Line line1(Point(0, 0, 0), Vector(1, 0, 0));
  Line line2(Point(0.5, -1, 0), Vector(0, 1, 0));
  
  // Act
  auto result = calculate_line_line_intersection(line1, line2);
  
  // Assert
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->x, 0.5, 1e-6);
  EXPECT_NEAR(result->y, 0, 1e-6);
  EXPECT_NEAR(result->z, 0, 1e-6);
}

// Test with parameterized inputs
TEST(IntersectionTest, ParallelLinesNoIntersection) {
  // Arrange
  Line line1(Point(0, 0, 0), Vector(1, 0, 0));
  Line line2(Point(0, 1, 0), Vector(1, 0, 0));
  
  // Act
  auto result = calculate_line_line_intersection(line1, line2);
  
  // Assert
  ASSERT_FALSE(result.has_value());
}
```

#### Test Fixtures

For tests that share setup code, use test fixtures:

```cpp
class GeometryTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Common setup code
    cube = create_test_cube();
    sphere = create_test_sphere();
  }
  
  void TearDown() override {
    // Common cleanup code
  }
  
  Solid cube;
  Solid sphere;
};

TEST_F(GeometryTest, SolidIntersection) {
  // Use cube and sphere from the fixture
  auto result = calculate_solid_intersection(cube, sphere);
  EXPECT_TRUE(result.is_valid());
}
```

### 3.2 Testing with OpenCASCADE

For tests involving OpenCASCADE, follow these guidelines:

#### Initialize OpenCASCADE Environment

```cpp
#include <gtest/gtest.h>
#include <Standard_Version.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <IntuiCAM/Geometry/OCCTWrapper.h>

class OCCTTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    // Initialize OCCT environment once for all tests
    initialize_occt_environment();
  }
  
  static void TearDownTestSuite() {
    // Cleanup OCCT environment
    cleanup_occt_environment();
  }
};

TEST_F(OCCTTest, BoxCreation) {
  // Create a box using OCCT
  BRepPrimAPI_MakeBox box_maker(10.0, 20.0, 30.0);
  TopoDS_Shape occt_box = box_maker.Shape();
  
  // Convert to IntuiCAM type
  auto intuicam_box = convert_occt_to_intuicam_solid(occt_box);
  
  // Verify properties
  EXPECT_NEAR(intuicam_box.volume(), 6000.0, 1e-6);
}
```

#### Test Data for CAD Operations

For tests requiring CAD data:

1. Use programmatically generated simple shapes when possible
2. For complex shapes, include small STEP files in the test data directory
3. Verify expected properties rather than exact geometry

```cpp
TEST_F(ImportTest, StepImport) {
  // Load a test STEP file
  std::string test_file = TEST_DATA_DIR "/simple_part.step";
  auto result = import_step_file(test_file);
  
  // Verify the import was successful
  ASSERT_TRUE(result.is_valid());
  
  // Check basic properties instead of exact geometry
  EXPECT_EQ(result.face_count(), 6);
  EXPECT_EQ(result.edge_count(), 12);
  EXPECT_NEAR(result.volume(), 1000.0, 1e-6);
}
```

---

## 4. GUI Testing

### 4.1 Qt Test Framework

IntuiCAM uses the Qt Test framework for testing GUI components.

#### Basic Structure for Qt Tests

```cpp
#include <QtTest/QtTest>
#include "viewport_widget.h"

class TestViewport : public QObject {
  Q_OBJECT
  
private slots:
  // Called before each test function
  void initTestCase() {
    // Global initialization
  }
  
  void init() {
    // Per-test initialization
    viewport = new ViewportWidget();
  }
  
  void cleanup() {
    // Per-test cleanup
    delete viewport;
    viewport = nullptr;
  }
  
  void cleanupTestCase() {
    // Global cleanup
  }
  
  // Test functions
  void testInitialState() {
    QVERIFY(viewport->cameraPosition() == QVector3D(0, 0, 10));
    QVERIFY(viewport->isOrthographic() == false);
  }
  
  void testZoom() {
    // Simulate user zoom
    QTest::keyClick(viewport, Qt::Key_Plus, Qt::ControlModifier);
    
    // Verify zoom changed
    QVERIFY(viewport->zoomFactor() > 1.0);
  }
  
private:
  ViewportWidget *viewport;
};

QTEST_MAIN(TestViewport)
#include "test_viewport.moc"
```

### 4.2 Testing Qt Widgets and User Interactions

For testing user interactions:

```cpp
void TestToolControls::testToolParameterChange() {
  // Create the widget
  ToolControlPanel panel;
  
  // Find the diameter spinbox
  QDoubleSpinBox *diameterSpinBox = panel.findChild<QDoubleSpinBox*>("diameterSpinBox");
  QVERIFY(diameterSpinBox != nullptr);
  
  // Connect to the signal to verify it's emitted
  QSignalSpy spy(panel, SIGNAL(toolParameterChanged(QString, double)));
  
  // Change the value
  diameterSpinBox->setValue(12.5);
  
  // Verify the signal was emitted with correct parameters
  QCOMPARE(spy.count(), 1);
  QList<QVariant> arguments = spy.takeFirst();
  QCOMPARE(arguments.at(0).toString(), "diameter");
  QCOMPARE(arguments.at(1).toDouble(), 12.5);
}
```

### 4.3 Testing with OpenCASCADE Visualization

For testing integration with OpenCASCADE visualization:

```cpp
void TestOCCViewer::testShapeDisplay() {
  // Create the OCCT viewer widget
  OCCTViewer viewer;
  
  // Create a simple shape
  TopoDS_Shape box = BRepPrimAPI_MakeBox(10, 10, 10).Shape();
  
  // Display the shape
  AIS_Shape *ais_shape = new AIS_Shape(box);
  viewer.context()->Display(ais_shape, Standard_True);
  
  // Verify the shape is displayed
  QVERIFY(viewer.context()->NbDisplayed() == 1);
  
  // Verify camera shows the full shape
  viewer.fitAll();
  QVERIFY(!viewer.isCameraClipping());
}
```

### 4.4 Testing with Mock Data

Use mocks for external dependencies:

```cpp
class MockGeometryEngine : public IGeometryEngine {
public:
  MOCK_METHOD(std::optional<Point>, calculateIntersection, (const Line&, const Line&), (override));
  MOCK_METHOD(Solid, importStep, (const std::string&), (override));
  // Other required methods...
};

TEST(ViewportTest, DisplaysImportedModel) {
  // Setup mock
  MockGeometryEngine mockEngine;
  Solid testSolid = create_test_solid();
  EXPECT_CALL(mockEngine, importStep(testing::_))
      .WillOnce(testing::Return(testSolid));
      
  // Create viewport with mock engine
  ViewportWidget viewport(&mockEngine);
  
  // Trigger import
  viewport.importModel("test.step");
  
  // Verify model is displayed
  EXPECT_TRUE(viewport.hasModel());
  EXPECT_EQ(viewport.getModelFaceCount(), testSolid.face_count());
}
```

---

## 5. Integration Testing

Integration tests verify that components work together correctly:

```cpp
TEST(WorkflowTest, CompleteToolpathGeneration) {
  // 1. Import a model
  auto model = import_step_file(TEST_DATA_DIR "/turning_part.step");
  ASSERT_TRUE(model.is_valid());
  
  // 2. Set up turning parameters
  TurningParameters params;
  params.stock_diameter = 50.0;
  params.stock_length = 100.0;
  params.tool_diameter = 10.0;
  params.feed_rate = 0.2;
  params.cutting_speed = 200.0;
  
  // 3. Generate toolpath
  auto toolpath = generate_turning_toolpath(model, params);
  ASSERT_TRUE(toolpath.is_valid());
  
  // 4. Generate G-code
  GCodeGenerator generator;
  std::string gcode = generator.generate(toolpath);
  
  // 5. Verify G-code contains expected commands
  EXPECT_TRUE(gcode.find("G0") != std::string::npos);
  EXPECT_TRUE(gcode.find("G1") != std::string::npos);
  EXPECT_TRUE(gcode.find("M30") != std::string::npos);
}
```

---

## 6. Regression Testing

For regression tests, maintain a set of test cases that have caused issues in the past:

```cpp
TEST(RegressionTest, Issue42_ToolpathGapAtCorner) {
  // Load the specific model that caused the issue
  auto model = import_step_file(TEST_DATA_DIR "/regression/issue42_model.step");
  
  // Set up the exact parameters that triggered the bug
  TurningParameters params;
  params.stock_diameter = 25.4;
  params.tool_radius = 0.8;
  // Other parameters...
  
  // Generate toolpath
  auto toolpath = generate_turning_toolpath(model, params);
  
  // Verify the specific issue is fixed
  EXPECT_TRUE(verify_toolpath_continuity(toolpath));
  EXPECT_FALSE(has_gaps_at_corners(toolpath));
}
```

---

## 7. Performance Testing

For critical algorithms, include performance tests:

```cpp
TEST(PerformanceTest, LargeModelToolpathGeneration) {
  // Load a large test model
  auto large_model = import_step_file(TEST_DATA_DIR "/performance/large_assembly.step");
  
  // Set typical parameters
  TurningParameters params = get_default_turning_parameters();
  
  // Measure time
  auto start_time = std::chrono::high_resolution_clock::now();
  
  auto toolpath = generate_turning_toolpath(large_model, params);
  
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  
  // Verify performance is within acceptable limits
  EXPECT_TRUE(toolpath.is_valid());
  EXPECT_LT(duration.count(), 5000);  // Expect < 5 seconds
  
  // Log performance for tracking over time
  std::cout << "Large model toolpath generation took " << duration.count() 
            << " ms for " << large_model.face_count() << " faces." << std::endl;
}
```

---

## 8. Continuous Integration

### 8.1 CI Configuration

The CI pipeline runs all tests on each pull request and on the main branch:

```yaml
# Excerpt from .github/workflows/ci.yml
test:
  runs-on: ${{ matrix.os }}
  strategy:
    matrix:
      os: [ubuntu-latest, windows-latest, macos-latest]
      build_type: [Debug, Release]
  
  steps:
    - uses: actions/checkout@v3
    
    # Setup dependencies...
    
    - name: Configure
      run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
    
    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }}
    
    - name: Test
      working-directory: build
      run: ctest --output-on-failure -C ${{ matrix.build_type }}
```

### 8.2 Test Data in CI

For test data in CI:

1. Keep test data files small (<1MB where possible)
2. Include them in the repository under `tests/data/`
3. For larger files, consider using Git LFS or downloading them during CI

---

## 9. Test Coverage

### 9.1 Measuring Coverage

IntuiCAM uses [gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) and [lcov](http://ltp.sourceforge.net/coverage/lcov.php) to measure test coverage:

```bash
# Build with coverage information
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build

# Run tests
cd build
ctest

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### 9.2 Coverage Targets

IntuiCAM aims for the following coverage targets:

* Core algorithms: ≥ 90%
* Utility functions: ≥ 80%
* Overall code base: ≥ 75%

---

## 10. Best Practices

1. **Write tests first**: When implementing new features, write tests first to clarify requirements
2. **Test edge cases**: Always include tests for corner cases and error conditions
3. **Keep tests fast**: Unit tests should execute quickly to encourage frequent running
4. **Maintain independence**: Tests should not depend on each other's state
5. **Use descriptive names**: Test names should describe what they're testing and the expected outcome
6. **One assertion per test**: Prefer multiple focused tests over a single test with many assertions
7. **Test behavior, not implementation**: Focus on testing the public API, not internal details
8. **Keep GUI tests stable**: Avoid brittle tests that depend on exact pixel positions or timing

By following these guidelines, IntuiCAM maintains a robust test suite that ensures reliability and facilitates future development. 