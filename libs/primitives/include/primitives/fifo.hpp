#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace pebble::primitives {

class FifoQueueEmptyError: public std::logic_error {
public:
    using std::logic_error::logic_error;
};

class FifoQueueFullError: public std::logic_error {
public:
    using std::logic_error::logic_error;
};

/* FifoQueue<T> -- fixed-capacity, ring-buffer-backed FIFO */
template<typename T>
class FifoQueue {
public:
    FifoQueue() = delete;
    FifoQueue(std::size_t capacity): capacity_{capacity}, storage_(capacity) {
        if(capacity_ == 0)
            throw std::invalid_argument{"FifoQueue<T> capacity must be > 0"};
    }

    void push(T v) {
        if(full()) throw FifoQueueFullError{"FifoQueue<T>::push() called when queue is full"};
        storage_[tail_] = v;
        tail_ = (tail_ + 1) % capacity_;
        size_++;
    }

    T pop() {
        if(empty()) throw FifoQueueEmptyError{"FifoQueue<T>::pop() called when queue is empty"};
        T v = std::move(storage_[head_]);
        head_ = (head_ + 1) % capacity_;
        size_--;
        return v;
    }

    const T& front() const {
        if(empty()) throw FifoQueueEmptyError{"FifoQueue<T>::front() called when queue is empty"};
        return storage_[head_];
    }

    bool full() const noexcept { return size_ == capacity_; }
    bool empty() const noexcept { return size_ == 0; }
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t capacity_;
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
    std::vector<T> storage_;
};

}  // namespace pebble::foundation