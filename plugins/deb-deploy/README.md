# Forma Debian Release Plugin

Generates Debian .deb packages from built applications. This plugin handles packaging, not UI definitions - it takes your compiled application and creates a proper Debian package for distribution.

## Features

- **Not a UI tool**: This plugin is for packaging, not for defining application logic
- Generates standard Debian package structure from built binaries
- Creates `DEBIAN/control` files with proper metadata
- Supports maintenance scripts (postinst, prerm, postrm, preinst)
- Generates copyright files following Debian standards
- Handles complex dependency specifications

## Key Concept

**This plugin operates on compiled/built applications, not Forma source files**. You define your UI in `.fml` files, compile them with Forma, then use this plugin to package the result for distribution.

```
Your workflow:
1. Write UI in app.fml
2. Compile: forma compile app.fml → binary
3. Package: deb-deploy plugin → .deb file
4. Distribute: apt install your-app.deb
```

## Features

### 1. Create a package configuration file (`package.cfg`):

```ini
# Package metadata
name = myapp
version = 1.2.3
maintainer = Your Name <you@example.com>
description = My awesome application
architecture = amd64
section = utils
priority = optional
homepage = https://example.com

# Dependencies
depends = libc6 (>= 2.34), libssl3, python3

# Copyright
copyright = 2025 Your Company
license = MIT

# Post-installation script
[postinst]
systemctl daemon-reload
systemctl enable myapp.service

# Pre-removal script
[prerm]
systemctl stop myapp.service
systemctl disable myapp.service
```

### 2. Build your Forma application:

```bash
forma compile myapp.fml -o build/usr/bin/myapp
```

### 3. Run the deb-deploy plugin:

```bash
# Plugin will create DEBIAN/ structure in build directory
forma-release-deb --build-dir build/ --source-dir dist/ --config package.cfg

# Build the actual .deb file
dpkg-deb --build build/ myapp_1.2.3_amd64.deb
```

## Package Structure

The plugin generates:

```
build/
├── DEBIAN/
│   ├── control           # Package metadata
│   ├── postinst          # Post-install script
│   └── prerm             # Pre-removal script
└── usr/
    ├── bin/              # Your application binaries
    ├── lib/              # Libraries
    └── share/
        └── doc/
            └── myapp/
                └── copyright
```

## Configuration File Format

### Metadata Fields

- `name`: Package name (lowercase, no spaces)
- `version`: Version string (e.g., "1.2.3")
- `maintainer`: Maintainer name and email
- `description`: Short package description  
- `architecture`: Target architecture (amd64, arm64, armhf, all, etc.)
- `section`: Package category (utils, admin, net, graphics, etc.)
- `priority`: Priority level (required, important, standard, optional, extra)
- `homepage`: Project website URL
- `depends`: Comma-separated list of dependencies with optional version constraints
- `copyright`: Copyright holder information
- `license`: License type (MIT, GPL-3.0, Apache-2.0, etc.)

### Script Sections

Scripts are defined in sections and will be executed with `#!/bin/bash`:

- `[postinst]`: Run after package installation
- `[prerm]`: Run before package removal
- `[postrm]`: Run after package removal  
- `[preinst]`: Run before package installation

## Building

```bash
mkdir build && cd build
cmake ..
make forma_deb_deploy
```

## Testing

```bash
cd build
make deb_deploy_tests
./plugins/deb-deploy/deb_deploy_tests
```

## Example

See `examples/package.cfg` for a complete configuration example.

## Features

- Generate `DEBIAN/control` files with package metadata
- Create post-installation scripts (`postinst`)
- Create pre-removal scripts (`prerm`)
- Generate copyright files
- Support for dependencies specification

## Usage

Define your package in a Forma document:

```forma
Package {
    name: "myapp"
    version: "1.0.0"
    maintainer: "Your Name <you@example.com>"
    description: "My awesome application"
    architecture: "amd64"
    section: "utils"
    priority: "optional"
}

Dependency {
    packages: "libc6, libssl3, python3"
}

PostInstall {
    commands: "systemctl enable myapp.service"
}

PreRemove {
    commands: "systemctl stop myapp.service"
}

Copyright {
    holder: "2025 Your Company"
    license: "MIT"
}
```

Then compile with the deb-deploy plugin:

```bash
forma compile mypackage.fml --plugin deb-deploy --output build/
dpkg-deb --build build/
```

## Package Structure

The plugin generates standard Debian package structure:

```
build/
├── DEBIAN/
│   ├── control
│   ├── postinst
│   └── prerm
└── usr/
    └── share/
        └── doc/
            └── package/
                └── copyright
```

## Supported Properties

### Package
- `name`: Package name
- `version`: Version string (e.g., "1.2.3")
- `maintainer`: Maintainer name and email
- `description`: Short package description
- `architecture`: Target architecture (amd64, arm64, all, etc.)
- `section`: Package section (utils, admin, net, etc.)
- `priority`: Priority level (required, important, standard, optional, extra)

### Dependency
- `packages`: Comma-separated list of dependencies

### PostInstall
- `commands`: Shell commands to run after installation

### PreRemove
- `commands`: Shell commands to run before removal

### Copyright
- `holder`: Copyright holder information
- `license`: License type (MIT, GPL-3.0, Apache-2.0, etc.)

## Building

```bash
mkdir build && cd build
cmake ..
make forma_deb_deploy
```

## Testing

```bash
cd build
make deb_deploy_tests
./plugins/deb-deploy/deb_deploy_tests
```
