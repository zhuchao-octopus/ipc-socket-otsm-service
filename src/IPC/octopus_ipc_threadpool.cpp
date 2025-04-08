/**
 * @file octopus_ipc_threadpool.cpp
 * @brief Implementation of a simple C++11 thread pool
 *
 * This file provides the implementation of the ThreadPool class defined in
 * octopus_ipc_threadpool.hpp. It manages a pool of worker threads that
 * execute submitted tasks asynchronously.
 *
 * @author ak47
 * @date 2025-04-08
 */
#include "octopus_ipc_threadpool.hpp"
#include <iostream>
#include <chrono>

ThreadPool::ThreadPool(size_t thread_count, size_t max_queue_size)
    : is_running_(true), active_thread_count_(thread_count), max_queue_size_(max_queue_size)
{
    // Start worker threads
    for (size_t i = 0; i < thread_count; ++i)
    {
        workers_.emplace_back([this]()
                              { worker_loop(); });
    }
}

ThreadPool::~ThreadPool()
{
    // Stop all worker threads
    is_running_ = false;
    task_cv_.notify_all();

    for (std::thread &worker : workers_)
    {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::enqueue(const std::function<void()> &task)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (task_queue_.size() < max_queue_size_)
        {
            task_queue_.emplace_back(task);
        }
        else
        {
            // Log or handle task dropping (optional)
            std::cerr << "Task queue is full, skipping task." << std::endl;
            return; // If queue is full, we skip adding the task
        }
    }
    task_cv_.notify_one();
}

template <typename T>
std::future<T> ThreadPool::enqueueWithResult(std::function<T()> task)
{
    auto packaged_task = std::make_shared<std::packaged_task<T()>>(std::move(task));
    std::future<T> result = packaged_task->get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (task_queue_.size() < max_queue_size_)
        {
            task_queue_.emplace_back([packaged_task]()
                                     { (*packaged_task)(); });
        }
        else
        {
            // Log or handle task dropping (optional)
            std::cerr << "Task queue is full, skipping task." << std::endl;
            return std::future<T>(); // Return an empty future if the task is skipped
        }
    }

    task_cv_.notify_one();
    return result;
}

void ThreadPool::worker_loop()
{
    while (is_running_)
    {
        std::function<void()> task;

        // Wait for task to be available
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_cv_.wait(lock, [this]
                          { return !task_queue_.empty() || !is_running_; });

            if (!is_running_ && task_queue_.empty())
                return;

            task = std::move(task_queue_.front());
            task_queue_.pop_front();
        }

        // Execute the task
        task();
    }
}

void ThreadPool::increase_threads(size_t count)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (size_t i = 0; i < count; ++i)
    {
        workers_.emplace_back([this]()
                              { worker_loop(); });
        active_thread_count_++;
    }
}

void ThreadPool::decrease_threads(size_t count)
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (size_t i = 0; i < count; ++i)
    {
        // Terminate a worker thread safely (worker thread will finish the current task and exit)
        is_running_ = false;
        task_cv_.notify_all();
        if (!workers_.empty())
        {
            workers_.back().join();
            workers_.pop_back();
            active_thread_count_--;
        }
    }
}

size_t ThreadPool::get_thread_count() const
{
    return active_thread_count_;
}

size_t ThreadPool::get_task_queue_size() const
{
    //std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

void ThreadPool::health_check()
{
    // Example: Check the task queue and thread pool health periodically (every 5 seconds)
    static auto last_check = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() >= 5)
    {
        last_check = now;

        // Check if task queue is too large or if thread count needs adjustment
        if (task_queue_.size() > max_queue_size_ * 0.8)
        {
            // Queue is getting large, consider adding more threads
            increase_threads(2); // Example: Add 2 more threads
        }
        else if (active_thread_count_ > 2 && task_queue_.empty())
        {
            // Queue is empty, consider removing threads
            decrease_threads(2); // Example: Remove 2 threads
        }
    }
}

void ThreadPool::print_pool_status() const
{
    std::cout << "Thread Pool Status: Is running: " << (is_running_ ? "Yes" : "No")
              << " | Active threads: " << active_thread_count_
              << " | Task queue size: " << get_task_queue_size() << std::endl;
}


