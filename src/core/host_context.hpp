#pragma once

#include "core/fs/i_file_system.hpp"
#include "../plugins/tracer/src/tracer_plugin.hpp"

namespace forma {

struct HostContext {
    std::unique_ptr<forma::fs::IFileSystem> filesystem;
    forma::tracer::TracerPlugin* tracer = nullptr;

    HostContext() = default;
    HostContext(std::unique_ptr<forma::fs::IFileSystem> fs, forma::tracer::TracerPlugin* t)
        : filesystem(std::move(fs)), tracer(t) {}
};

} // namespace forma
