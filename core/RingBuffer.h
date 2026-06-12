#pragma once
#include <atomic>
#include <cstring>
#include <vector>
#include <algorithm>

// lock-free single-producer single-consumer ring buffer
// designed for real-time audio routing between capture and render threads
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : buffer_(capacity), capacity_(capacity), head_(0), tail_(0) {}

    // producer: write audio data into the buffer
    // returns number of bytes actually written
    size_t write(const uint8_t* data, size_t bytes) {
        auto currentHead = head_.load(std::memory_order_relaxed);
        auto currentTail = tail_.load(std::memory_order_acquire);

        auto available = availableWrite(currentHead, currentTail);
        auto toWrite = std::min(bytes, available);
        if (toWrite == 0) return 0;

        auto headIdx = currentHead % capacity_;
        auto firstChunk = std::min(toWrite, capacity_ - headIdx);
        std::memcpy(buffer_.data() + headIdx, data, firstChunk);

        if (firstChunk < toWrite)
            std::memcpy(buffer_.data(), data + firstChunk, toWrite - firstChunk);

        head_.store(currentHead + toWrite, std::memory_order_release);
        return toWrite;
    }

    // consumer: read audio data from the buffer
    // returns number of bytes actually read
    size_t read(uint8_t* dest, size_t bytes) {
        auto currentTail = tail_.load(std::memory_order_relaxed);
        auto currentHead = head_.load(std::memory_order_acquire);

        auto available = availableRead(currentHead, currentTail);
        auto toRead = std::min(bytes, available);
        if (toRead == 0) return 0;

        auto tailIdx = currentTail % capacity_;
        auto firstChunk = std::min(toRead, capacity_ - tailIdx);
        std::memcpy(dest, buffer_.data() + tailIdx, firstChunk);

        if (firstChunk < toRead)
            std::memcpy(dest + firstChunk, buffer_.data(), toRead - firstChunk);

        tail_.store(currentTail + toRead, std::memory_order_release);
        return toRead;
    }

    // peek without consuming - useful for multi-consumer scenarios
    // each consumer tracks their own read position externally
    size_t peek(uint8_t* dest, size_t bytes, size_t readPos) const {
        auto currentHead = head_.load(std::memory_order_acquire);

        if (readPos > currentHead) return 0;

        auto available = currentHead - readPos;
        if (available > capacity_) {
            // reader fell too far behind, data was overwritten
            return 0;
        }

        auto toRead = std::min(bytes, available);
        auto posIdx = readPos % capacity_;
        auto firstChunk = std::min(toRead, capacity_ - posIdx);
        std::memcpy(dest, buffer_.data() + posIdx, firstChunk);

        if (firstChunk < toRead)
            std::memcpy(dest + firstChunk, buffer_.data(), toRead - firstChunk);

        return toRead;
    }

    size_t getWritePos() const { return head_.load(std::memory_order_acquire); }
    size_t getReadPos() const { return tail_.load(std::memory_order_acquire); }
    size_t getCapacity() const { return capacity_; }

    void reset() {
        head_.store(0, std::memory_order_release);
        tail_.store(0, std::memory_order_release);
    }

private:
    size_t availableWrite(size_t head, size_t tail) const {
        auto used = head - tail;
        return capacity_ - used - 1; // -1 to distinguish full from empty
    }

    size_t availableRead(size_t head, size_t tail) const {
        return head - tail;
    }

    std::vector<uint8_t> buffer_;
    size_t capacity_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};
