# Forma VSCode Extension

Complete Visual Studio Code extension for the Forma programming language.

## Features

✅ **Syntax Highlighting** - Full TextMate grammar for `.forma` and `.fml` files
✅ **Language Server Protocol** - Advanced IDE features powered by forma_lsp_server_stdio
✅ **Real-time Diagnostics** - Errors and warnings as you type
✅ **Auto-completion** - Context-aware code suggestions  
✅ **Hover Information** - Type information and documentation
✅ **Go to Definition** - Navigate to symbol definitions
✅ **Symbol Navigation** - Workspace-wide symbol search

## Quick Start

### 1. Build the Language Server

```bash
cd /path/to/forma
mkdir -p build && cd build
cmake ..
cmake --build . --target forma_lsp_server_stdio
```

This creates `build/forma_lsp_server_stdio` which the extension will use.

### 2. Install the Extension

#### Option A: Development Mode (Testing)

```bash
cd vscode-extension
npm install
npm run compile
```

Then in VSCode:
- Press `F5` to open Extension Development Host
- Open a `.forma` file to test

#### Option B: Package and Install

```bash
cd vscode-extension
npm install
npm run package
code --install-extension forma-language-*.vsix
```

### 3. Configure (Optional)

The extension auto-detects the language server in common locations:
- `<workspace>/build/forma_lsp_server_stdio`
- `<workspace>/plugins/lsp-server/build/forma_lsp_server_stdio`

To specify a custom path, add to your VSCode settings:

```json
{
    "forma.languageServer.path": "/custom/path/to/forma_lsp_server_stdio"
}
```

## Development

### Prerequisites

- Node.js 18+
- TypeScript 5.0+
- VSCode 1.75+

### Building

```bash
npm install        # Install dependencies
npm run compile    # Compile TypeScript
npm run watch      # Watch mode for development
npm run lint       # Run linter
```

### Debugging

1. Open `vscode-extension` folder in VSCode
2. Press `F5` to launch Extension Development Host
3. Set breakpoints in `src/extension.ts`
4. Open a `.forma` file in the development window

### Project Structure

```
vscode-extension/
├── src/
│   └── extension.ts              # Main extension code
├── syntaxes/
│   └── forma.tmLanguage.json     # TextMate grammar
├── language-configuration.json   # Brackets, comments, etc.
├── package.json                  # Extension manifest
├── tsconfig.json                 # TypeScript config
└── README.md                     # User documentation
```

## Commands

- **Forma: Restart Language Server** - Restart the LSP server without reloading VSCode

## Troubleshooting

### Language server not starting

1. Verify the server is built:
   ```bash
   ls -la build/forma_lsp_server_stdio
   ```

2. Check the Output panel: View → Output → "Forma Language Server"

3. Verify the path in settings:
   ```json
   {
       "forma.languageServer.path": "/absolute/path/to/forma_lsp_server_stdio"
   }
   ```

### No syntax highlighting

1. Ensure file has `.forma` or `.fml` extension
2. Try setting language manually: View → Command Palette → "Change Language Mode" → "Forma"

### Diagnostics not showing

1. Check if the language server is running (status bar shows "✓ Forma")
2. Restart the language server: Command Palette → "Forma: Restart Language Server"
3. Check for errors in the Output panel

## Publishing

To publish to the VSCode marketplace:

1. Update version in `package.json`
2. Update `CHANGELOG.md`
3. Package the extension:
   ```bash
   npm run package
   ```
4. Publish:
   ```bash
   npx vsce publish
   ```

## License

MIT License - See LICENSE file in the root of the Forma project.

## Contributing

This extension is part of the [Forma programming language project](https://github.com/yourusername/forma).

Bug reports and contributions welcome!
