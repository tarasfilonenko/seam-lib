#pragma once
// ─────────────────────────────────────────────
// seam::rtos::RingBuffer
//
// A FreeRTOS ring buffer wrapper for raw bytes.
// Writer copies bytes into the buffer.
// Reader receives a pointer into internal buffer memory —
// caller MUST call returnItem() when done, otherwise
// the ring buffer never reclaims that space.
// Only one Writer and one Reader may exist at a time.
// ─────────────────────────────────────────────

#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace seam {
namespace rtos {

class RingBuffer {
public:

    // ── Writer ────────────────────────────────
    struct Writer {
        Writer(Writer&&) = default;
        Writer& operator=(Writer&&) = default;
        Writer(const Writer&) = delete;
        Writer& operator=(const Writer&) = delete;

        bool write(const uint8_t* data, size_t len,
                   TickType_t timeout = portMAX_DELAY) {
            return xRingbufferSend(_handle, data, len, timeout) == pdTRUE;
        }

    private:
        friend class RingBuffer;
        explicit Writer(RingbufHandle_t h) : _handle(h) {}
        RingbufHandle_t _handle;
    };

    // ── Reader ────────────────────────────────
    struct Reader {
        Reader(Reader&&) = default;
        Reader& operator=(Reader&&) = default;
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;

        // Returns pointer into internal ring buffer memory.
        // Returns nullptr on timeout.
        // MUST call returnItem() when done processing.
        uint8_t* receive(size_t* out_len,
                         TickType_t timeout = portMAX_DELAY) {
            return static_cast<uint8_t*>(
                xRingbufferReceive(_handle, out_len, timeout)
            );
        }

        // Must be called after every successful receive()
        void returnItem(void* item) {
            vRingbufferReturnItem(_handle, item);
        }

    private:
        friend class RingBuffer;
        explicit Reader(RingbufHandle_t h) : _handle(h) {}
        RingbufHandle_t _handle;
    };

    // ── RingBuffer ────────────────────────────
    explicit RingBuffer(size_t size) {
        _handle = xRingbufferCreate(size, RINGBUF_TYPE_BYTEBUF);
        configASSERT(_handle != nullptr);
        _writer = Writer(_handle);
        _reader = Reader(_handle);
    }

    ~RingBuffer() {
        if (_handle) vRingbufferDelete(_handle);
    }

    Writer takeWriter() {
        configASSERT(_writer.has_value());
        Writer w = std::move(_writer.value());
        _writer.reset();
        return w;
    }

    Reader takeReader() {
        configASSERT(_reader.has_value());
        Reader r = std::move(_reader.value());
        _reader.reset();
        return r;
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

private:
    RingbufHandle_t         _handle = nullptr;
    std::optional<Writer>   _writer;
    std::optional<Reader>   _reader;
};

} // namespace rtos
} // namespace seam