# Tracer Plugin

A logging and diagnostic plugin for the Forma compiler pipeline.

## Features

- **Multiple verbosity levels**: Silent, Normal, Verbose, Debug
- **Structured output**: Hierarchical stage tracking with indentation
- **Statistics reporting**: File sizes, counts, and metrics
- **Error highlighting**: Always-visible error and warning messages
- **Progress tracking**: Visual feedback with ▶ and ✓ markers

## Usage

```cpp
#include "plugins/tracer/src/tracer_plugin.hpp"

using namespace forma::tracer;

// Get global tracer instance
auto& tracer = get_tracer();

// Set verbosity level
tracer.set_level(TraceLevel::Verbose);

// Track compilation stages
tracer.begin_stage("Parsing");
// ... do parsing work ...
tracer.end_stage();

// Log messages
tracer.info("Processing complete");
tracer.verbose("Detailed statistics");
tracer.debug("Internal state dump");
tracer.error("Fatal error occurred");
tracer.warning("Potential issue detected");

// Report statistics
tracer.stat("File size", 1234);
tracer.stat("Output", "main.c");
```

## Trace Levels

- **Silent**: No output except errors
- **Normal**: Stage markers and completion messages
- **Verbose**: All of Normal plus detailed statistics
- **Debug**: All of Verbose plus internal debugging information

## Architecture

The tracer follows a singleton pattern with a global instance accessible via `get_tracer()`. It can be configured at runtime via command-line flags:

- `-v`, `--verbose`: Set to Verbose level
- `--debug`: Set to Debug level

## Integration

The tracer is integrated throughout the Forma compilation pipeline:

1. **File Reading**: Reports file paths and sizes
2. **Parsing**: Reports type, enum, event, and instance counts
3. **Type Checking**: Reports diagnostics with severity levels
4. **Asset Collection**: Reports discovered assets and URIs
5. **Code Generation**: Reports renderer type and output size
