/**
 * @file octopus_ipc_threadpool.cpp
 * @brief Implementation of a simple C++11 thread pool with dynamic scaling and graceful thread removal.
 *
 * This file provides the implementation of the ThreadPool class defined in
 * octopus_ipc_threadpool.hpp. It manages a pool of worker threads that
 * execute submitted tasks asynchronously, and supports dynamic scaling.
 *
 * @author ak47
 * @date 2025-04-08
 */
#include "octopus_ipc_threadpool.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include <deque>
#include <mutex>

OctopusThreadPool::OctopusThreadPool(size_t thread_count, size_t max_queue_size, TaskOverflowStrategy strategy)
    : is_running_(true),
      active_thread_count_(thread_count),
      max_queue_size_(max_queue_size),
      is_scaling_(false),
      threads_to_terminate_(0),
      overflow_strategy_(strategy)
{
    // Launch initial worker threads
    for (size_t i = 0; i < thread_count; ++i)
    {
        workers_.emplace_back([this]()
                              { worker_loop(); });
    }

    // Log the initial status of the thread pool
    std::cout << "[ThreadPoolPlus] Initialized with " << thread_count << " thread(s)." << std::endl;
}

OctopusThreadPool::~OctopusThreadPool()
{
    // Stop all worker threads
    is_running_ = false;
    task_cv_.notify_all();

    // Join all threads
    for (std::thread &worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    // Log the shutdown of the thread pool
    std::cout << "[ThreadPoolPlus] Shutting down thread pool." << std::endl;
}

void OctopusThreadPool::enqueue(const std::function<void()> &task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        switch (overflow_strategy_)
        {
        case TaskOverflowStrategy::DropOldest:
            if (task_queue_.size() >= max_queue_size_)
            {
                std::cerr << "[ThreadPoolPlus] Queue full. Dropping oldest task." << std::endl;
                task_queue_.pop_front(); // 丢最老
            }
            task_queue_.emplace_back(task);
            break;

        case TaskOverflowStrategy::DropNewest:
            if (task_queue_.size() >= max_queue_size_)
            {
                std::cerr << "[ThreadPoolPlus] Queue full. Dropping newest task." << std::endl;
                return; // 丢当前
            }
            task_queue_.emplace_back(task);
            break;

        case TaskOverflowStrategy::Block:
            // 等待队列腾出空间
            while (task_queue_.size() >= max_queue_size_)
            {
                std::cerr << "[ThreadPoolPlus] Queue full. Waiting..." << std::endl;
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                lock.lock();
            }
            task_queue_.emplace_back(task);
            break;
        }
    }
    task_cv_.notify_one();
}

void OctopusThreadPool::worker_loop()
{
    while (is_running_)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_cv_.wait(lock, [this]()
                          { return !task_queue_.empty() || !is_running_ || threads_to_terminate_ > 0; });

            // Handle shutdown
            if (!is_running_ && task_queue_.empty())
            {
                return;
            }

            // Handle dynamic shrink
            if (task_queue_.empty() && threads_to_terminate_ > 0)
            {
                threads_to_terminate_--;
                active_thread_count_--;
                std::cout << "[ThreadPoolPlus] Thread exiting. Active thread count: " << active_thread_count_ << std::endl;
                return;
            }

            task = std::move(task_queue_.front());
            task_queue_.pop_front();
            /// std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        try
        {
            task();
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ThreadPoolPlus] Task threw exception: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[ThreadPoolPlus] Task threw unknown exception." << std::endl;
        }
    }
}

void OctopusThreadPool::health_check()
{
    static auto last_check = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() < 600)
    {
        return; // Avoid checking too frequently
    }

    last_check = now;

    if (is_scaling_.exchange(true))
    {
        return; // Skip if already scaling
    }

    size_t current_queue_size = get_task_queue_size();

    // Log the current health status of the queue
    std::cout << "[ThreadPoolPlus] Health check: Queue size = " << current_queue_size
              << ", Active threads = " << active_thread_count_ << std::endl;

    if (current_queue_size > max_queue_size_ * 0.8)
    {
        // Queue is overloaded, add more threads
        std::cout << "[ThreadPoolPlus] Queue is overloaded, adding more threads..." << std::endl;
        add_threads(2); // Add 2 more threads as an example
    }
    else if (current_queue_size == 0 && active_thread_count_ > 2)
    {
        // Queue is empty, remove idle threads if more than 2 threads exist
        std::cout << "[ThreadPoolPlus] Queue is empty, removing idle threads..." << std::endl;
        remove_threads(1); // Remove 1 thread as an example
    }

    is_scaling_ = false;
}

void OctopusThreadPool::add_threads(size_t count)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (size_t i = 0; i < count; ++i)
    {
        workers_.emplace_back([this]()
                              { worker_loop(); });
        active_thread_count_++;
    }

    // Log when new threads are added
    std::cout << "[ThreadPoolPlus] Added " << count << " thread(s). Active thread count: " << active_thread_count_ << std::endl;
}

void OctopusThreadPool::remove_threads(size_t count)
{
    // Optional: Removing threads is not safe unless threads cooperate to self-terminate
    // This function may require additional flags or safe signaling logic
    // For now, it is just a placeholder and not yet implemented

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (count > active_thread_count_)
        {
            count = active_thread_count_; // Don't remove more than we have
        }
        threads_to_terminate_ += count;
    }

    // Wake up idle threads to allow them to self-terminate
    for (size_t i = 0; i < count; ++i)
    {
        task_cv_.notify_one();
    }

    std::cout << "[ThreadPoolPlus] Scheduled removal of " << count << " thread(s)."
              << " Will shrink to " << (active_thread_count_ - count) << " threads." << std::endl;
}

size_t OctopusThreadPool::get_thread_count() const
{
    return active_thread_count_;
}

size_t OctopusThreadPool::get_task_queue_size() const
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

void OctopusThreadPool::print_pool_status() const
{
    std::cout << "[ThreadPoolPlus] Status: Running: " << (is_running_ ? "Yes" : "No")
              << " | Active Threads: " << get_thread_count()
              << " | Queue Size: " << get_task_queue_size() << std::endl;
}
