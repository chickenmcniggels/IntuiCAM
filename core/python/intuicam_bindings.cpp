#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

// Include all module headers
#include <IntuiCAM/Common/Types.h>
#include <IntuiCAM/Common/Version.h>
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Geometry/StepLoader.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/PostProcessor/Types.h>
#include <IntuiCAM/Simulation/Types.h>

namespace py = pybind11;

// Forward declarations for module binding functions
void bind_common(py::module& m);
void bind_geometry(py::module& m);
void bind_toolpath(py::module& m);
void bind_postprocessor(py::module& m);
void bind_simulation(py::module& m);

PYBIND11_MODULE(intuicam_py, m) {
    m.doc() = "IntuiCAM Python bindings - Complete CAM library for CNC turning";
    
    // Version information
    m.attr("__version__") = INTUICAM_VERSION_STRING;
    m.attr("__author__") = "IntuiCAM Development Team";
    
    // Create submodules
    auto common_module = m.def_submodule("common", "Common utilities and types");
    auto geometry_module = m.def_submodule("geometry", "Geometry handling and STEP import");
    auto toolpath_module = m.def_submodule("toolpath", "Toolpath generation algorithms");
    auto postprocessor_module = m.def_submodule("postprocessor", "G-code generation");
    auto simulation_module = m.def_submodule("simulation", "Material removal simulation");
    
    // Bind each module
    bind_common(common_module);
    bind_geometry(geometry_module);
    bind_toolpath(toolpath_module);
    bind_postprocessor(postprocessor_module);
    bind_simulation(simulation_module);
    
    // Convenience functions at top level
    m.def("load_step_file", &IntuiCAM::Geometry::StepLoader::importStepFile,
          "Load a STEP file and return the imported parts",
          py::arg("file_path"));
    
    m.def("create_facing_operation", [](const std::string& name) {
        auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
            IntuiCAM::Toolpath::Tool::Type::Facing, "Default Facing Tool");
        return std::make_unique<IntuiCAM::Toolpath::FacingOperation>(name, tool);
    }, "Create a facing operation with default tool");
    
    m.def("create_roughing_operation", [](const std::string& name) {
        auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
            IntuiCAM::Toolpath::Tool::Type::Turning, "Default Roughing Tool");
        return std::make_unique<IntuiCAM::Toolpath::RoughingOperation>(name, tool);
    }, "Create a roughing operation with default tool");
}

// Common module bindings
void bind_common(py::module& m) {
    using namespace IntuiCAM::Common;
    
    // Enums
    py::enum_<LogLevel>(m, "LogLevel")
        .value("Debug", LogLevel::Debug)
        .value("Info", LogLevel::Info)
        .value("Warning", LogLevel::Warning)
        .value("Error", LogLevel::Error)
        .value("Critical", LogLevel::Critical);
    
    py::enum_<LengthUnit>(m, "LengthUnit")
        .value("Millimeter", LengthUnit::Millimeter)
        .value("Inch", LengthUnit::Inch)
        .value("Meter", LengthUnit::Meter);
    
    py::enum_<AngleUnit>(m, "AngleUnit")
        .value("Degree", AngleUnit::Degree)
        .value("Radian", AngleUnit::Radian);
    
    // Exception classes
    py::register_exception<Exception>(m, "IntuiCAMException");
    py::register_exception<GeometryException>(m, "GeometryException");
    py::register_exception<ToolpathException>(m, "ToolpathException");
    py::register_exception<SimulationException>(m, "SimulationException");
    
    // UnitConverter
    py::class_<UnitConverter>(m, "UnitConverter")
        .def_static("convert_length", &UnitConverter::convertLength)
        .def_static("convert_angle", &UnitConverter::convertAngle)
        .def_static("get_length_unit_string", &UnitConverter::getLengthUnitString)
        .def_static("get_angle_unit_string", &UnitConverter::getAngleUnitString);
    
    // Math utilities
    auto math_module = m.def_submodule("math", "Mathematical utilities");
    math_module.attr("PI") = Math::PI;
    math_module.attr("EPSILON") = Math::EPSILON;
    math_module.def("is_equal", &Math::isEqual, py::arg("a"), py::arg("b"), py::arg("tolerance") = Math::EPSILON);
    math_module.def("is_zero", &Math::isZero, py::arg("value"), py::arg("tolerance") = Math::EPSILON);
    math_module.def("clamp", &Math::clamp);
    math_module.def("lerp", &Math::lerp);
    math_module.def("deg_to_rad", &Math::degToRad);
    math_module.def("rad_to_deg", &Math::radToDeg);
}

// Geometry module bindings
void bind_geometry(py::module& m) {
    using namespace IntuiCAM::Geometry;
    
    // Point3D
    py::class_<Point3D>(m, "Point3D")
        .def(py::init<>())
        .def(py::init<double, double, double>())
        .def_readwrite("x", &Point3D::x)
        .def_readwrite("y", &Point3D::y)
        .def_readwrite("z", &Point3D::z)
        .def("__repr__", [](const Point3D& p) {
            return "Point3D(" + std::to_string(p.x) + ", " + 
                   std::to_string(p.y) + ", " + std::to_string(p.z) + ")";
        });
    
    // Vector3D
    py::class_<Vector3D>(m, "Vector3D")
        .def(py::init<>())
        .def(py::init<double, double, double>())
        .def_readwrite("x", &Vector3D::x)
        .def_readwrite("y", &Vector3D::y)
        .def_readwrite("z", &Vector3D::z)
        .def("normalized", &Vector3D::normalized)
        .def("magnitude", &Vector3D::magnitude);
    
    // BoundingBox
    py::class_<BoundingBox>(m, "BoundingBox")
        .def(py::init<>())
        .def(py::init<const Point3D&, const Point3D&>())
        .def_readwrite("min", &BoundingBox::min)
        .def_readwrite("max", &BoundingBox::max)
        .def("contains", &BoundingBox::contains)
        .def("intersects", &BoundingBox::intersects)
        .def("size", &BoundingBox::size)
        .def("center", &BoundingBox::center);
    
    // StepLoader
    py::class_<StepLoader::ImportResult>(m, "ImportResult")
        .def_readonly("success", &StepLoader::ImportResult::success)
        .def_readonly("error_message", &StepLoader::ImportResult::errorMessage);
    
    py::class_<StepLoader>(m, "StepLoader")
        .def_static("import_step_file", &StepLoader::importStepFile)
        .def_static("validate_step_file", &StepLoader::validateStepFile)
        .def_static("get_supported_formats", &StepLoader::getSupportedFormats);
}

// Toolpath module bindings
void bind_toolpath(py::module& m) {
    using namespace IntuiCAM::Toolpath;
    
    // Tool::Type enum
    py::enum_<Tool::Type>(m, "ToolType")
        .value("Turning", Tool::Type::Turning)
        .value("Facing", Tool::Type::Facing)
        .value("Parting", Tool::Type::Parting)
        .value("Threading", Tool::Type::Threading)
        .value("Grooving", Tool::Type::Grooving);
    
    // MovementType enum
    py::enum_<MovementType>(m, "MovementType")
        .value("Rapid", MovementType::Rapid)
        .value("Linear", MovementType::Linear)
        .value("CircularCW", MovementType::CircularCW)
        .value("CircularCCW", MovementType::CircularCCW)
        .value("Dwell", MovementType::Dwell)
        .value("ToolChange", MovementType::ToolChange);
    
    // Tool class
    py::class_<Tool, std::shared_ptr<Tool>>(m, "Tool")
        .def(py::init<Tool::Type, const std::string&>())
        .def("get_type", &Tool::getType)
        .def("get_name", &Tool::getName);
    
    // Movement struct
    py::class_<Movement>(m, "Movement")
        .def(py::init<MovementType, const IntuiCAM::Geometry::Point3D&>())
        .def_readwrite("type", &Movement::type)
        .def_readwrite("position", &Movement::position)
        .def_readwrite("feed_rate", &Movement::feedRate)
        .def_readwrite("spindle_speed", &Movement::spindleSpeed)
        .def_readwrite("comment", &Movement::comment);
    
    // Toolpath class
    py::class_<Toolpath>(m, "Toolpath")
        .def(py::init<const std::string&, std::shared_ptr<Tool>>())
        .def("add_movement", &Toolpath::addMovement)
        .def("add_rapid_move", &Toolpath::addRapidMove)
        .def("add_linear_move", &Toolpath::addLinearMove)
        .def("get_movements", &Toolpath::getMovements, py::return_value_policy::reference)
        .def("get_name", &Toolpath::getName)
        .def("get_movement_count", &Toolpath::getMovementCount)
        .def("estimate_machining_time", &Toolpath::estimateMachiningTime);
}

// PostProcessor module bindings
void bind_postprocessor(py::module& m) {
    using namespace IntuiCAM::PostProcessor;
    
    // MachineType enum
    py::enum_<PostProcessor::MachineType>(m, "MachineType")
        .value("GenericLathe", PostProcessor::MachineType::GenericLathe)
        .value("Fanuc", PostProcessor::MachineType::Fanuc)
        .value("Haas", PostProcessor::MachineType::Haas)
        .value("Mazak", PostProcessor::MachineType::Mazak)
        .value("Okuma", PostProcessor::MachineType::Okuma)
        .value("Siemens", PostProcessor::MachineType::Siemens);
    
    // PostProcessor class
    py::class_<PostProcessor>(m, "PostProcessor")
        .def(py::init<PostProcessor::MachineType>())
        .def("process", py::overload_cast<const IntuiCAM::Toolpath::Toolpath&>(&PostProcessor::process))
        .def_static("create_for_machine", &PostProcessor::createForMachine)
        .def_static("get_supported_machines", &PostProcessor::getSupportedMachines)
        .def_static("get_machine_name", &PostProcessor::getMachineName);
}

// Simulation module bindings
void bind_simulation(py::module& m) {
    using namespace IntuiCAM::Simulation;
    
    // MaterialSimulator class
    py::class_<MaterialSimulator>(m, "MaterialSimulator")
        .def(py::init<>())
        .def("simulate", py::overload_cast<const IntuiCAM::Toolpath::Toolpath&>(&MaterialSimulator::simulate))
        .def("calculate_machining_time", &MaterialSimulator::calculateMachiningTime);
    
    // CollisionDetector::CollisionType enum
    py::enum_<CollisionDetector::CollisionType>(m, "CollisionType")
        .value("ToolChuck", CollisionDetector::CollisionType::ToolChuck)
        .value("ToolStock", CollisionDetector::CollisionType::ToolStock)
        .value("ToolTailstock", CollisionDetector::CollisionType::ToolTailstock)
        .value("RapidMove", CollisionDetector::CollisionType::RapidMove);
} 