# VSCode Test Coverage Setup

This guide explains how to run tests with coverage visualization in VSCode.

## Required Extensions

Install these VSCode extensions (VSCode will prompt you automatically):

1. **CMake Tools** (`ms-vscode.cmake-tools`) - Build and test integration
2. **C/C++** (`ms-vscode.cpptools`) - C++ language support
3. **Coverage Gutters** (`ryanluker.vscode-coverage-gutters`) - Coverage visualization

## Quick Start

### Option 1: Using VSCode Test Explorer

1. Open VSCode in the forma workspace
2. The CMake Tools extension will auto-configure with coverage enabled
3. Open the **Testing** view (beaker icon in sidebar)
4. Click the **Run Tests** button at the top
5. Tests will run and coverage data will be generated

### Option 2: Using Tasks

1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
2. Type "Tasks: Run Task"
3. Select **"Run Tests with Coverage"**
4. Coverage report will be generated automatically

### Option 3: Using Terminal

```bash
# Build with coverage
cmake -B build -DFORMA_BUILD_TESTS=ON -DFORMA_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests and generate coverage
cmake --build build --target coverage
```

## Viewing Coverage

### In-Editor Coverage Display

1. After running tests with coverage, press `Ctrl+Shift+P`
2. Type "Coverage Gutters: Display Coverage"
3. Or click the "Watch" button in the status bar
4. Coverage will be shown inline in your code:
   - ✅ Green: Line is covered
   - ❌ Red: Line is not covered
   - No marker: Line is not executable

### HTML Report

Open `build/coverage.html` in your browser for detailed coverage report:

```bash
xdg-open build/coverage.html  # Linux
open build/coverage.html      # macOS
```

## Generated Files

After running coverage:

- `build/coverage.lcov` - Coverage data for VSCode (used by Coverage Gutters)
- `build/coverage.html` - Main HTML report (with links to detailed files)
- `build/coverage.*.html` - Per-file HTML coverage reports
- `build/coverage.txt` - Text summary of coverage statistics

## CMake Configuration

The `.vscode/settings.json` automatically configures:

```json
{
  "cmake.configureSettings": {
    "FORMA_BUILD_TESTS": "ON",
    "FORMA_ENABLE_COVERAGE": "ON",
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

## Current Coverage

Run from terminal to see current coverage:

```bash
cat build/coverage.txt
```

Example output:
```
lines: 30.9% (290 out of 938)
functions: 48.3% (43 out of 89)
branches: 16.3% (235 out of 1446)
```

## Troubleshooting

### Tests not showing in Test Explorer

1. Make sure CMake Tools extension is installed
2. Open Command Palette: "CMake: Configure"
3. Reload window if needed

### Coverage not displaying

1. Make sure Coverage Gutters extension is installed
2. Run tests with coverage: `cmake --build build --target coverage`
3. Check that `build/coverage.lcov` exists
4. Click "Watch" in the status bar to enable coverage display

### Coverage shows 0%

Make sure you're building in Debug mode with coverage flags:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DFORMA_ENABLE_COVERAGE=ON
```

## Advanced: Coverage for Specific Files

To see coverage for a specific file:

1. Open the file in VSCode
2. Make sure coverage is displayed (green/red highlights)
3. Hover over line numbers to see execution counts
4. Click on coverage in status bar for file coverage percentage
