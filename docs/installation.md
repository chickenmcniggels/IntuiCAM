
# Installation

This guide shows how to get IntuiCAM up and running on Windows using **vcpkg** to manage dependencies (including Open CASCADE). If you prefer a manual SDK install, see [Alternative: manual OCCT install](#alternative-manual-occt-install).

---

## Prerequisites

- **Windows 10/11** (x64)  
- **Git**  
- **CMake** ≥ 3.16  
- **C++17-capable compiler** (MSVC 2019+ or MinGW-w64 x64)  
- **Qt6 (Widgets)** 6.9.0  
  - Example install prefix: `C:\Qt\6.9.0\mingw_64`  
- **vcpkg** (for OCCT and other libs)

---

## 1. Clone IntuiCAM

```bat
git clone https://github.com/your-org/IntuiCAM.git
cd IntuiCAM
```

---

## 2. Bootstrap and configure vcpkg

If you don’t have **vcpkg** installed yet, do:

```bat
cd C:\
git clone https://github.com/microsoft/vcpkg.git vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

Now you have the `vcpkg` tool in `C:\vcpkg\vcpkg.exe`.

---

## 3. Install dependencies via vcpkg

From the root of your vcpkg checkout:

```bat
C:\vcpkg\vcpkg.exe install opencascade:x64-windows
```

You can also install other dependencies later (e.g. `.\vcpkg install boost:x64-windows`).

---

## 4. Configure CMake with the vcpkg toolchain

Back in your IntuiCAM folder:

```bat
cd path\to\IntuiCAM
mkdir build
cd build
cmake .. ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.0/mingw_64" ^
  -DCMAKE_BUILD_TYPE=Release
```

- `CMAKE_TOOLCHAIN_FILE` tells CMake to use vcpkg’s integration.
- `CMAKE_PREFIX_PATH` points to your Qt6 install.

---

## 5. Build and run

bat
cmake --build . --config Release
.\gui\Release\IntuiCAM_GUI.exe


If everything went well, you should see the IntuiCAM window with a “Welcome to IntuiCAM” message.

---

## Alternative: Manual Open CASCADE SDK install

If you don’t want to use vcpkg, you can download the OCCT Windows SDK:

1. Download `OpenCASCADE-<version>-win64-vc15.zip` from  
   https://www.opencascade.com/content/latest-release
2. Unzip to `C:\OpenCASCADE-<version>\`
3. Set environment variable:
   ```bat
   setx CASROOT "C:\OpenCASCADE-<version>"
   setx PATH "%CASROOT%\bin;%PATH%"
   ```  
4. In your CMake command, replace the `-DCMAKE_TOOLCHAIN_FILE` line with:
   ```bat
   -DCMAKE_PREFIX_PATH="C:/OpenCASCADE-<version>"
   ```

CMake will then find OCCT via its own `OpenCASCADEConfig.cmake`.

---

## Troubleshooting

- **Qt not found**: Make sure `CMAKE_PREFIX_PATH` matches your Qt install.
- **vcpkg not applied**: Verify the path to `vcpkg.cmake` and rerun CMake from a clean build folder.
- **Compiler errors**: Ensure you’re using a 64-bit toolchain (matches `x64-windows` triplet).

For further help, see [Development Guide](development.md) or open an issue on GitHub. 