# Forma Language Support for Visual Studio Code

This extension provides language support for Forma, a QML-inspired declarative programming language.

## Features

- **Syntax Highlighting**: Full syntax highlighting for `.forma` and `.fml` files
- **Language Server Protocol (LSP)**: Integration with the Forma LSP server for advanced features:
  - Code completion
  - Diagnostics (errors and warnings)
  - Symbol navigation
  - Hover information
  - Go to definition

## Requirements

This extension requires the `forma_lsp_server` executable to be built and available. 

### Building the Language Server

```bash
cd /path/to/forma
mkdir build && cd build
cmake ..
cmake --build .
```

The `forma_lsp_server` executable will be in the `build` directory.

## Extension Settings

This extension contributes the following settings:

* `forma.languageServer.path`: Path to the forma_lsp_server executable. If empty, will try to find it in PATH.
* `forma.languageServer.port`: Port for the Forma LSP server HTTP endpoint (default: 8080).
* `forma.trace.server`: Traces the communication between VS Code and the language server.

## Usage

1. Open a `.forma` or `.fml` file
2. The extension will automatically activate and start the language server
3. Enjoy syntax highlighting and IntelliSense features!

### Example Forma Code

```forma
// Define a custom button class
class CustomButton {
    property text: string;
    property color: string;
    property onClick: event;
}

// Create an instance
myButton: CustomButton {
    text: "Click Me";
    color: "blue";
    
    when onClick {
        _ => {
            text: "Clicked!";
        }
    }
}
```

## Commands

- `Forma: Restart Language Server` - Restart the Forma language server

## Known Issues

- The language server must be built separately before the extension can function
- Initial startup might take a moment while connecting to the language server

## Release Notes

### 0.1.0

Initial release:
- Syntax highlighting for Forma language
- Language server integration
- Basic IntelliSense support

## Contributing

This extension is part of the [Forma programming language project](https://github.com/yourusername/forma).

## License

MIT License - See LICENSE file for details
