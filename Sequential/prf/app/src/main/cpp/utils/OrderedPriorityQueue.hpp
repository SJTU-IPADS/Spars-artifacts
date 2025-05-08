#ifndef PriorityQueue_H
#define PriorityQueue_H

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>

template <typename T>
class OrderedPriorityQueue
{
private:
    std::priority_queue<T, std::vector<T>, std::greater<T>> queue_;
    std::mutex mtx_;
    std::condition_variable itemAdded_;
    uint32_t priorityReturned_;

public:
    OrderedPriorityQueue() {
        priorityReturned_ = 0;
    };

    uint32_t getSize()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        return queue_.size();
    }

    void reset() // 调用时务必确保queue_为空
    {
        std::unique_lock<std::mutex> lk(mtx_);
        priorityReturned_ = 0;
    }

    void produce(T const &item)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        queue_.push(item);
        itemAdded_.notify_one();
    }

    void produce(T &&item)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        queue_.push(std::move(item));
        itemAdded_.notify_one();
    }

    T consume()
    {
        std::unique_lock<std::mutex> lk(mtx_);

        while (queue_.empty() || queue_.top() != priorityReturned_)
        {
            itemAdded_.wait(lk);
        }

        T item = std::move(const_cast<T&>(queue_.top()));

        priorityReturned_++;
        queue_.pop();

        return item;
    }
};

#endif