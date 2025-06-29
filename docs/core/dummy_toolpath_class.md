# Dummy Toolpath Example

This short guide demonstrates how toolpaths are implemented in the core library
and provides a minimal operation that you can use as a template for your own
experiments.

IntuiCAM's toolpaths are generated by **operations**. Each operation derives from
`IntuiCAM::Toolpath::Operation` and implements two virtual methods:

```cpp
std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part);
bool validate() const;
```

The `Toolpath` class stores a sequence of low-level movements. Operations fill
these movements according to their parameters and the input part geometry.

## DummyOperation

`DummyOperation` lives in `core/toolpath` and is intentionally simple. It
creates a toolpath containing a single rapid move followed by one cutting move.
The code below omits error checking for brevity.

```cpp
class DummyOperation : public Operation {
public:
    struct Parameters {
        Geometry::Point3D startPosition{0,0,0};
        Geometry::Point3D endPosition{10,0,0};
        double feedRate{100.0};
    };

    explicit DummyOperation(const std::string& name,
                            std::shared_ptr<Tool> tool);

    void setParameters(const Parameters& p);
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part&) override;
    bool validate() const override;
};
```

The implementation simply adds two moves:

```cpp
std::unique_ptr<Toolpath> DummyOperation::generateToolpath(const Geometry::Part&)
{
    auto tp = std::make_unique<Toolpath>(name_, tool_);
    tp->addRapidMove(params_.startPosition);                // approach
    tp->addLinearMove(params_.endPosition, params_.feedRate); // cut
    return tp;
}
```

Because `DummyOperation` follows the same pattern as the real operations
(facing, roughing, …), it can serve as a starting point for new toolpath types.
Simply copy the class and extend `generateToolpath()` with your own logic.

## Usage

Include `DummyOperation.h`, create an instance with a tool, set the parameters
and call `generateToolpath()`:

```cpp
auto tool = std::make_shared<Tool>(Tool::Type::Turning, "DemoTool");
DummyOperation op("Demo", tool);
DummyOperation::Parameters p;
p.startPosition = {0,0,5};
p.endPosition   = {50,0,5};
op.setParameters(p);
auto tp = op.generateToolpath(myPart);
```

This produces a minimal path that can be inspected or fed into the rest of the
pipeline. Use this class as a reference when implementing more advanced
operations.
