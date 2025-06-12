# Getting Started with IntuiCAM

Welcome to IntuiCAM! This guide will help you get started with the application after installation.

## Prerequisites

Before you begin, ensure you have successfully built IntuiCAM following the [Installation Guide](installation.md) or [Windows Setup Guide](windows_setup.md).

## Launching IntuiCAM

### GUI Application

Launch the Qt-based graphical interface:

**Windows:**
```powershell
cd build
.\Release\IntuiCAMGui.exe
```

**Linux/macOS:**
```bash
cd build
./bin/IntuiCAMGui
```

### Command Line Interface

For batch processing or scripting, use the CLI:

**Windows:**
```powershell
cd build
.\Release\IntuiCAMCli.exe --help
```

**Linux/macOS:**
```bash
cd build
./bin/IntuiCAMCli --help
```

## First Steps

🚧 **Note**: IntuiCAM is currently in active development. The user interface and workflows are being implemented.
### Part Setup and Default Steps
When you import a model, IntuiCAM opens the **Part Setup** tab. The top portion contains orientation and chuck settings. Below that, a timeline displays four predefined machining steps:

1. **Contouring**
2. **Threading**
3. **Chamfering**
4. **Parting**

These steps are created automatically and can be toggled on or off before you generate the toolpath.


### Current Status

- ✅ **Build System**: Complete and verified
- ✅ **Core Libraries**: Architecture established
- ✅ **Dependencies**: Qt, OpenCASCADE, VTK integrated
- 🔄 **GUI Implementation**: In progress
- 🔄 **CAM Algorithms**: In development
- 🔄 **STEP Import**: Planned
- ✅ **Toolpath Generation**: Automatic default steps implemented
- 🔄 **G-Code Export**: Planned

### Development Roadmap

1. **Phase 1** (Current): Build system and dependencies
2. **Phase 2**: Basic GUI and file import/export
3. **Phase 3**: Core CAM algorithms (turning operations)
4. **Phase 4**: Toolpath visualization
5. **Phase 5**: G-Code generation and post-processing

## Contributing

IntuiCAM is an open-source project and welcomes contributions! See the [Development Guide](development.md) for information on:

- Setting up the development environment
- Coding standards and conventions
- Submitting pull requests
- Reporting issues

## Support

- **Documentation**: [docs/](index.md)
- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions

Stay tuned for updates as we continue developing IntuiCAM's CAM capabilities!
