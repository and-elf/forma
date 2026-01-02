#pragma once

#include "core/fs/i_file_system.hpp"
#include "core/io/stream_io.hpp"
#include "../plugins/tracer/src/tracer_plugin.hpp"

namespace forma {

struct HostContext {
    std::unique_ptr<forma::fs::IFileSystem> filesystem;
    forma::tracer::TracerPlugin* tracer = nullptr;
    forma::io::StreamIO stream_io;

    HostContext() = default;
    HostContext(std::unique_ptr<forma::fs::IFileSystem> fs, forma::tracer::TracerPlugin* t)
        : filesystem(std::move(fs)), tracer(t) {}

    void initialize_stream_io() {
        if (filesystem) {
            stream_io = forma::io::StreamIO::from_filesystem(*filesystem);
        } else {
            stream_io = forma::io::StreamIO::defaults();
        }
    }
};

} // namespace forma
