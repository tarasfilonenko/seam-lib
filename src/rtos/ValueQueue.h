#pragma once
// ─────────────────────────────────────────────
// seam::rtos::ValueQueue<T>
//
// A FreeRTOS queue wrapper for plain structs.
// Writer takes const ref — copied into queue buffer.
// Reader returns optional<T> — nullopt on timeout.
// Only one Writer and one Reader may exist at a time.
// ─────────────────────────────────────────────

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <optional>

namespace seam {
namespace rtos {

template<typename T>
class ValueQueue {
public:

    // ── Writer ────────────────────────────────
    struct Writer {
        Writer(Writer&&) = default;
        Writer& operator=(Writer&&) = default;
        Writer(const Writer&) = delete;
        Writer& operator=(const Writer&) = delete;

        bool send(const T& item,
                  TickType_t timeout = portMAX_DELAY) {
            return xQueueSend(_handle, &item, timeout) == pdTRUE;
        }

    private:
        friend class ValueQueue<T>;
        explicit Writer(QueueHandle_t h) : _handle(h) {}
        QueueHandle_t _handle;
    };

    // ── Reader ────────────────────────────────
    struct Reader {
        Reader(Reader&&) = default;
        Reader& operator=(Reader&&) = default;
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;

        std::optional<T> receive(TickType_t timeout = portMAX_DELAY) {
            T item{};
            if (xQueueReceive(_handle, &item, timeout) == pdTRUE) {
                return item;
            }
            return std::nullopt;
        }

    private:
        friend class ValueQueue<T>;
        explicit Reader(QueueHandle_t h) : _handle(h) {}
        QueueHandle_t _handle;
    };

    // ── Queue ─────────────────────────────────
    explicit ValueQueue(size_t depth) {
        _handle = xQueueCreate(depth, sizeof(T));
        configASSERT(_handle != nullptr);
        _writer.emplace(_handle);
        _reader.emplace(_handle);
    }

    ~ValueQueue() {
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

    ValueQueue(const ValueQueue&) = delete;
    ValueQueue& operator=(const ValueQueue&) = delete;
    ValueQueue(ValueQueue&&) = delete;
    ValueQueue& operator=(ValueQueue&&) = delete;

private:
    QueueHandle_t         _handle = nullptr;
    std::optional<Writer> _writer;
    std::optional<Reader> _reader;
};

} // namespace rtos
} // namespace seam