# Forma
Inspired by QML, Forma uses FML, which on the surface looks like a tweaked QML. This is where the languages separate.

## Destinct pipeline
* Tokenizer
* IR
* Audio system
* Theme
* renderer / code generation
* lsp-api
* build system
* tracing / logging

All if there are separate libararies, either statically linked or dynamically loaded using `--plugin` flag to forma binary.

The tokenizer and IR is static, everything else is pluggable and up to the user.

### Tokenizer
Compile-time tested

### IR
Compile-time tested

### Audio system
Optional interface for handling audio
* Miniaudio
* OpenAL
* dummy

### Theme
Optional interface for handling themes. Compile-time tested

### renderer / code generation
Optional interface for emitting compilable code. Compile-time tested
* JS
* SDL
* LVGL
* Vulkan
* etc.
### lsp-api
Optional interface for integattion with ide's. Compile-time tested

### build system
Optional interface for emitting build system code.  Compile-time tested
* CMake
* Meson
* Bazel

### tracing / logging

# Configuration
Toml is used to configure each part of the code generation pipeline

## Example
```
[targets.desktop.extensions]
renderer = ["forma-renderer-js"]
theme    = ["forma-theme-dark"]
runtime  = ["forma-audio-dummy"]
ide      = ["forma-ide-lsp"]

[targets.embedded.extensions]
renderer = ["forma-renderer-lvgl"]
theme    = ["forma-theme-oem"]
runtime  = ["forma-audio-none"]


[build.cmake]
generator = "Ninja"
build_type = "Release"

[build.custom]
script = "scripts/build_embedded.sh"

[renderer.forma-renderer-lvgl]
language = c

[paths]
include = [
  "ui",
  "common",
  "../shared-ui"
]

```

# Cli
the forma binary can override the toml configuration

```
forma \
  --mode lsp \
  --renderer js \
  --plugin forma-renderer-js \
  --plugin forma-vfs \
  --project ./app
```

```
forma \
  -I ./ui \
  -I ./common \
  -I /opt/forma/packages \
  --project ./app
```

```
forma \
  --mode lsp \
  --renderer js \
  --plugin forma-renderer-js \
  --plugin forma-vfs \
  --project ./app
```

```
forma \
  --mode build \
  --renderer lvgl \
  --plugin forma-build-cmake \
  --target stm32
```

# Constexpr EVERYTHING
The Tokenizer is constexpr
The IR is constexp
The 

# Syntax
In Forma, we can define backend data types straight from within markup
The format specification is defined in [syntax.md]

## Examples
```
Rectangle {
  color: Forma.Color.Red
  width: 100
}
```

```
Theme Dark {
  Color Primary = #1e1e1e
  Color Accent  = #569cd6

  Style Button {
    background: Primary
    color: #ffffff
    radius: 6
  }

  Style Label {
    color: #dddddd
  }
}
```

```
class TodoModel {
  property Array<Todo> todos
  event Added(Todo)
  fn add(Todo)
}
```

```
x: when foo.state {
    INIT    => 0
    RUNNING => 42
    STOPPED => -1
}


# Preview
* Exists only in tooling modes
* Injects values at evaluation time
* Never affects codegen or runtime backends
* Think of it as dependency injection for the UI preview.

```
View {
  model: backend.model() or preview {
    { text: "Buy milk", done: false }
    { text: "Ship Forma", done: true }
  }
}
```

# Architecture
## Limits
Hard limits on property count,
There are seven major parts of the langauge; Tokenizer, IR, 