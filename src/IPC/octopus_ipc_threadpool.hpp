/**
 * @file octopus_ipc_threadpool.hpp
 * @brief Simple and readable C++11 thread pool implementation
 *
 * This thread pool allows you to enqueue tasks (std::function<void()>) for asynchronous
 * execution using a fixed number of worker threads. Useful for high-frequency callbacks
 * or background tasks in systems like IPC communication.
 *
 * @author ak47
 * @date 2025-04-08
 */
#pragma once

#include <vector>
#include <thread>
#include <deque> // 使用 deque 替代 queue
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>

/**
 * @class ThreadPool
 * @brief A simple C++11 thread pool to execute tasks asynchronously
 */
class ThreadPool
{
public:
    /**
     * @brief Constructor that starts a specified number of threads
     * @param thread_count Number of worker threads to spawn (default = 4)
     * @param max_queue_size Maximum size of the task queue (default = 20)
     */
    explicit ThreadPool(size_t thread_count = 4, size_t max_queue_size = 20);

    /**
     * @brief Destructor that shuts down the thread pool and joins all threads
     */
    ~ThreadPool();

    // Disable copy construction and assignment
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    /**
     * @brief Enqueue a new task into the thread pool
     * @param task A function with no return value and no arguments to be executed asynchronously
     */
    void enqueue(const std::function<void()> &task);

    /**
     * @brief Enqueue a task that returns a result
     * @param task A function that returns a value of type T, to be executed asynchronously
     * @return A future object representing the result of the task
     */
    template <typename T>
    std::future<T> enqueueWithResult(std::function<T()> task);

    /**
     * @brief Increase the number of worker threads in the pool
     * @param count Number of threads to add
     */
    void increase_threads(size_t count);

    /**
     * @brief Decrease the number of worker threads in the pool
     * @param count Number of threads to remove
     */
    void decrease_threads(size_t count);

    /**
     * @brief Get the current number of active worker threads
     * @return Current active thread count
     */
    size_t get_thread_count() const;
    size_t get_task_queue_size() const;
    /**
     * @brief Periodically checks the health of the thread pool and adjusts threads if needed
     */
    void health_check();

    void print_pool_status() const;

private:
    /**
     * @brief The main function that each worker thread runs
     */
    void worker_loop();

private:
    std::vector<std::thread> workers_;             ///< Vector of worker threads
    std::deque<std::function<void()>> task_queue_; ///< Queue of tasks
    std::mutex queue_mutex_;                       ///< Mutex to protect task queue
    std::condition_variable task_cv_;              ///< Condition variable to notify workers
    std::atomic<bool> is_running_;                 ///< Flag to indicate if thread pool is running
    size_t active_thread_count_;                   ///< Active thread count
    size_t max_queue_size_;                        ///< Maximum allowed task queue size
    size_t idle_thread_threshold_;                 ///< Threshold for idle threads before reducing them
};
