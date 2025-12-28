# VSCode Extension Setup Guide

Complete guide to installing and using the Forma VSCode extension.

## Prerequisites

- Visual Studio Code 1.75 or later
- Node.js 18+ and npm
- C++ compiler (for building language server)
- CMake 3.20+

## Installation Methods

### Method 1: Quick Install (Recommended)

```bash
# 1. Build the language server
cd /path/to/forma
mkdir -p build && cd build
cmake ..
cmake --build . --target forma_lsp_server_stdio

# 2. Set up extension
cd ../vscode-extension
./setup.sh

# 3. Install in VSCode
npm run package
code --install-extension forma-language-*.vsix
```

### Method 2: Development Mode

For testing and development:

```bash
cd /path/to/forma/vscode-extension
npm install
npm run compile

# Open VSCode
code .

# Press F5 to launch Extension Development Host
```

### Method 3: Manual Build

```bash
# Build language server
cd /path/to/forma
./vscode-extension/build-lsp-server.sh

# Build extension
cd vscode-extension
npm install
npm run compile
npm run package

# Install
code --install-extension forma-language-*.vsix
```

## Verification

1. Open VSCode
2. Create a test file `test.forma`:
   ```forma
   Button {
       text: "Hello Forma"
       width: 100
       height: 50
   }
   ```
3. Check status bar shows "✓ Forma" (green checkmark)
4. Verify syntax highlighting is active
5. Make an error and check for red squiggles

## Configuration

### Default Settings

The extension works out-of-the-box if your project structure is:
```
your-project/
├── build/
│   └── forma_lsp_server_stdio   ← Auto-detected
└── *.forma files
```

### Custom Language Server Path

If the language server is in a different location:

1. Open VSCode Settings (Cmd/Ctrl + ,)
2. Search for "forma"
3. Set "Forma: Language Server Path":

```json
{
    "forma.languageServer.path": "/custom/path/to/forma_lsp_server_stdio"
}
```

Or edit `.vscode/settings.json` in your workspace:

```json
{
    "forma.languageServer.path": "${workspaceFolder}/custom/build/forma_lsp_server_stdio",
    "forma.trace.server": "verbose"  // For debugging
}
```

### Recommended Settings

Add to your user settings for better experience:

```json
{
    "[forma]": {
        "editor.tabSize": 4,
        "editor.insertSpaces": true,
        "editor.formatOnSave": false,
        "editor.semanticHighlighting.enabled": true
    },
    "files.associations": {
        "*.forma": "forma",
        "*.fml": "forma"
    }
}
```

## Features

### Syntax Highlighting

Automatic highlighting for:
- Keywords (`class`, `property`, `event`, `method`, `enum`, `when`)
- Types (`int`, `float`, `bool`, `string`)
- Strings and numbers
- Comments (line and block)
- Operators and punctuation

### Diagnostics

Real-time error checking:
- Syntax errors (red squiggles)
- Type errors
- Undefined references
- Semantic warnings

### Code Completion

Auto-complete for:
- Keywords
- Property names
- Type names
- Built-in types

### Navigation

- **Go to Definition**: Cmd/Ctrl + Click on symbols
- **Find References**: Right-click → Find All References
- **Outline View**: Symbol tree in sidebar

### Commands

Access via Command Palette (Cmd/Ctrl + Shift + P):

- `Forma: Restart Language Server` - Restart LSP server

## Troubleshooting

### Language Server Not Starting

**Problem**: Status bar shows error or "Forma" without checkmark

**Solutions**:

1. Check language server is built:
   ```bash
   ls -la build/forma_lsp_server_stdio
   # Should show executable file
   ```

2. View error details:
   - Open: View → Output
   - Select: "Forma Language Server" from dropdown
   - Look for error messages

3. Test server manually:
   ```bash
   ./build/forma_lsp_server_stdio
   # Should start and wait for input
   # Press Ctrl+C to exit
   ```

4. Set explicit path in settings:
   ```json
   {
       "forma.languageServer.path": "/absolute/path/to/forma_lsp_server_stdio"
   }
   ```

### No Syntax Highlighting

**Problem**: File shows plain text

**Solutions**:

1. Check file extension is `.forma` or `.fml`

2. Manually set language mode:
   - Click language indicator in status bar
   - Type "Forma"
   - Select "Forma"

3. Verify extension is installed:
   - Extensions sidebar (Cmd/Ctrl + Shift + X)
   - Search "Forma"
   - Should show "Forma Language Support" as installed

4. Reload window:
   - Command Palette → "Developer: Reload Window"

### Diagnostics Not Showing

**Problem**: Errors don't appear with red squiggles

**Solutions**:

1. Check language server is running:
   - Status bar should show "✓ Forma"

2. Enable verbose logging:
   ```json
   {
       "forma.trace.server": "verbose"
   }
   ```
   Then check Output panel

3. Restart language server:
   - Command Palette → "Forma: Restart Language Server"

4. Check file is not too large:
   - Language server has limits on file size
   - Try splitting large files

### Build Errors

**Problem**: `npm install` or `npm run compile` fails

**Solutions**:

1. Check Node.js version:
   ```bash
   node --version  # Should be 18+
   ```

2. Clear npm cache:
   ```bash
   rm -rf node_modules package-lock.json
   npm install
   ```

3. Update TypeScript:
   ```bash
   npm install -D typescript@latest
   ```

4. Check for port conflicts (if running dev mode):
   - Kill other VSCode instances
   - Try again

## Advanced Usage

### Multi-root Workspaces

For projects with multiple Forma codebases:

```json
{
    "folders": [
        { "path": "./project-a" },
        { "path": "./project-b" }
    ],
    "settings": {
        "forma.languageServer.path": "/shared/path/forma_lsp_server_stdio"
    }
}
```

### Custom Build Tasks

Add to `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Forma LSP Server",
            "type": "shell",
            "command": "cmake --build build --target forma_lsp_server_stdio",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```

### Debugging

To debug the extension itself:

1. Open `vscode-extension/` in VSCode
2. Set breakpoints in `src/extension.ts`
3. Press F5 to launch Extension Development Host
4. Breakpoints will hit when extension activates

To debug the language server:

1. Modify server to wait for debugger:
   ```cpp
   // At start of main() in lsp_server_stdio.cpp
   std::cerr << "PID: " << getpid() << std::endl;
   sleep(10);  // Time to attach debugger
   ```

2. Build and note PID from Output panel

3. Attach gdb:
   ```bash
   gdb -p <PID>
   ```

## Uninstalling

```bash
code --uninstall-extension forma.forma-language
```

Or via VSCode:
- Extensions sidebar
- Find "Forma Language Support"
- Click gear icon → Uninstall

## Getting Help

- **Issues**: https://github.com/forma-lang/forma/issues
- **Discussions**: https://github.com/forma-lang/forma/discussions
- **Documentation**: See [DEVELOPMENT.md](DEVELOPMENT.md)

## Next Steps

- Try the [examples](../examples/)
- Read [syntax.md](../syntax.md) for language reference
- Explore [plugins](../plugins/) for code generation
- Join the community discussions
