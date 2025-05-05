# Development Guide

Welcome to the IntuiCAM development guide. This document helps new contributors set up their environment, understand coding conventions, and follow best practices for contributing code, tests, and documentation.

---

## 1. Development Environment Setup

1. **Clone the repository**:

   ```bash
   git clone https://github.com/YourOrg/IntuiCAM.git
   cd IntuiCAM
   ```
2. **Prerequisites**:

   * C++17/20–capable compiler (GCC ≥ 9, Clang ≥ 10, MSVC 2019+)
   * CMake ≥ 3.16
   * Qt5 or Qt6 (for GUI modules)
   * Python 3.7+ (for optional scripting bindings)
   * `git`, `clang-format`, `clang-tidy` (optional but recommended)
3. **Install dependencies** (on Ubuntu example):

   ```bash
   sudo apt update
   sudo apt install build-essential cmake qtbase5-dev python3-dev clang-format clang-tidy
   ```
4. **Build**:

   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . -- -j$(nproc)
   ```
5. **Run tests**:

   ```bash
   ctest --output-on-failure
   ```

---

## 2. Coding Standards

* **Language**: Modern C++17/20. Use RAII and smart pointers; avoid raw owning pointers.
* **Style**: Follow Google C++ Style Guide conventions:

  * Classes in `CamelCase`, methods and variables in `snake_case`.
  * Braces on the same line for functions and control statements.
  * 2‑spaces indent, max 100 characters per line.
* **Formatting**: Run `clang-format -i` on changed files. CI will enforce style.
* **Linting**: Use `clang-tidy` to catch common issues. Fix warnings before commit.
* **Documentation**: Public APIs must have Doxygen comments:

  ```cpp
  /// Computes turning toolpath for the given stock and parameters.
  /// \param stock  Stock geometry object.
  /// \param params  Operation parameters.
  /// \return Generated toolpath curve list.
  Toolpath compute_turning_path(const Stock& stock, const TurnParams& params);
  ```

---

## 3. Branching & Commits

* **Main branches**:

  * `main` (stable, always green CI)
  * `develop` (latest development, merge features here)
* **Feature branches**: `feature/<short-description>`
* **Bugfix branches**: `bugfix/<issue-number>-<short-desc>`
* **Commits**:

  * Write clear, imperative subject lines (e.g., "Add facing operation support").
  * Include issue reference (`#123`) when applicable.
  * Limit subject to 50 characters; use body to explain "what" and "why".

---

## 4. Testing & Continuous Integration

* **Unit tests**:

  * Located under `core/tests/` and `tests/`.
  * Use GoogleTest framework.
  * Aim for high coverage on core algorithms (toolpath generation, file import/export).
* **Integration tests**:

  * End-to-end scenarios using sample STEP files and expected G-Code.
* **CI**:

  * GitHub Actions workflows in `.github/workflows/ci.yml`.
  * Runs build, tests, `clang-format` check, and `clang-tidy` analysis across Windows, Linux, macOS.

---

## 5. Pull Request Process

1. **Fork & branch** off `develop`.
2. **Implement** your feature/fix with tests.
3. **Update documentation** if needed (README, docs/ files).
4. **Run CI** locally: build, tests, format, lint.
5. **Push** your branch and open a PR against `develop`.
6. **Review**: maintainers will review code, request changes if necessary.
7. **Merge**: after approval and passing CI, PR will be merged.

---

## 6. Reporting Issues

* Use GitHub **Issues** for bugs and feature requests.
* Provide clear descriptions, steps to reproduce, and relevant logs or screenshots.
* Tag issues with appropriate labels (`bug`, `enhancement`, `question`).

---

## 7. Documentation Contributions

* Documentation lives under `docs/`. Follow the **Markdown** style:

  * Use fenced code blocks and relative links.
  * Add new topics under appropriate headings (installation, core, gui, etc.).
* For code snippets or API docs, ensure Doxygen comments in source are up to date.
* After editing docs, build the repo to verify no broken links.

---

## 8. Community & Communication

* **Discussions**: Use GitHub Discussions for general questions and proposals.
* **Code of Conduct**: Be respectful and inclusive. See `CODE_OF_CONDUCT.md`.
* **Chat**: (Optional) Slack/Discord invite link can be added here.

Thank you for contributing to IntuiCAM! Your work helps make desktop CNC turning accessible to everyone.
