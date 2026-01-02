#pragma once

#include <cstddef>
#include <memory>

namespace forma::io {

// RAII write stream interface — writes are flushed on destruction.
struct IWriteStream {
    virtual ~IWriteStream() = default;
    // Write up to `len` bytes from `data`. Returns number of bytes written.
    virtual std::size_t write(const void* data, std::size_t len) = 0;
};

using WriteStreamPtr = std::unique_ptr<IWriteStream>;

} // namespace forma::io
