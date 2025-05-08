#ifndef BoundedBufferQueue_H
#define BoundedBufferQueue_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

template <typename T>
class BoundedBufferQueue
{
private:
    uint32_t maxSize_;
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable itemAdded_;
    std::condition_variable itemRemoved_;

public:
    BoundedBufferQueue(uint32_t maxSize) : maxSize_(maxSize){};
    BoundedBufferQueue() : maxSize_(3){}; // default size

    uint32_t getSize()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        return queue_.size();
    }

    uint32_t getMaxSize()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        return maxSize_;
    }

    void setMaxSize(uint32_t max)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        maxSize_ = max;
    }

    void produce(T const &item)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        while (queue_.size() >= maxSize_)
        {
            itemRemoved_.wait(lk);
        }
        queue_.push(item);
        itemAdded_.notify_one();
    }

    void produce(T &&item)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        while (queue_.size() >= maxSize_)
        {
            itemRemoved_.wait(lk);
        }
        queue_.push(std::move(item));
        itemAdded_.notify_one();
    }

    T consume()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        while (queue_.empty())
        {
            itemAdded_.wait(lk);
        }
        T item = std::move(queue_.front());
        queue_.pop();
        itemRemoved_.notify_one();
        return item;
    }
};

#endif