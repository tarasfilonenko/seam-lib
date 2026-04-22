#pragma once
// ─────────────────────────────────────────────
// seam::rtos::OwningQueue<T>
//
// A FreeRTOS queue wrapper for heap-allocated objects.
// Writer takes unique_ptr — releases ownership into queue.
// Reader returns unique_ptr — grants ownership to caller.
// Only one Writer and one Reader may exist at a time.
// ─────────────────────────────────────────────

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <memory>
#include <optional>

namespace seam {
namespace rtos {

template<typename T>
class OwningQueue {
public:

    // ── Writer ────────────────────────────────
    struct Writer {
        Writer(Writer&&) = default;
        Writer& operator=(Writer&&) = default;
        Writer(const Writer&) = delete;
        Writer& operator=(const Writer&) = delete;

        bool send(std::unique_ptr<T> item,
                  TickType_t timeout = portMAX_DELAY) {
            T* raw = item.release();
            if (xQueueSend(_handle, &raw, timeout) == pdTRUE) {
                return true;
            }
            delete raw;  // queue full — reclaim ownership
            return false;
        }

    private:
        friend class OwningQueue<T>;
        explicit Writer(QueueHandle_t h) : _handle(h) {}
        QueueHandle_t _handle;
    };

    // ── Reader ────────────────────────────────
    struct Reader {
        Reader(Reader&&) = default;
        Reader& operator=(Reader&&) = default;
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;

        std::unique_ptr<T> receive(TickType_t timeout = portMAX_DELAY) {
            T* raw = nullptr;
            if (xQueueReceive(_handle, &raw, timeout) == pdTRUE) {
                return std::unique_ptr<T>(raw);
            }
            return nullptr;
        }

    private:
        friend class OwningQueue<T>;
        explicit Reader(QueueHandle_t h) : _handle(h) {}
        QueueHandle_t _handle;
    };

    // ── Queue ─────────────────────────────────
    explicit OwningQueue(size_t depth) {
        _handle = xQueueCreate(depth, sizeof(T*));
        configASSERT(_handle != nullptr);
        _writer = Writer(_handle);
        _reader = Reader(_handle);
    }

    ~OwningQueue() {
        if (_handle) vQueueDelete(_handle);
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

    OwningQueue(const OwningQueue&) = delete;
    OwningQueue& operator=(const OwningQueue&) = delete;
    OwningQueue(OwningQueue&&) = delete;
    OwningQueue& operator=(OwningQueue&&) = delete;

private:
    QueueHandle_t         _handle = nullptr;
    std::optional<Writer> _writer;
    std::optional<Reader> _reader;
};

} // namespace rtos
} // namespace seam