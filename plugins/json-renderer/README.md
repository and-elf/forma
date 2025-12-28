# JSON Renderer Plugin

Exports Forma UI documents as JSON for web applications, debugging, and tooling.

## Building

```bash
cd plugins/json-renderer
g++ -std=c++20 -fPIC -shared -Wl,--export-dynamic json_renderer.cpp -o libjson_renderer.so
```

## Usage

```bash
# Load the plugin
./forma --plugin plugins/json-renderer/libjson_renderer.so --renderer json input.forma

# List plugin info
./forma --plugin plugins/json-renderer/libjson_renderer.so --list-plugins
```

## Output Example

Given a Forma file:
```forma
Button {
    text: "Click Me"
    x: 10
    y: 20
}
```

Would output:
```json
{
  "instances": [
    {
      "type": "Button",
      "properties": {
        "text": "Click Me",
        "x": 10,
        "y": 20
      }
    }
  ]
}
```

## Capabilities

- **Renderer**: true
- **Theme**: false
- **Audio**: false
- **Build**: false

## Use Cases

- Web-based Forma viewers
- Debugging and inspection
- Cross-platform UI transfer
- Integration with JavaScript frameworks
- Documentation generation
