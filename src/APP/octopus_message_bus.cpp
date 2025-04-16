// octopus_message_bus.cpp
//
// Implementation of the OctopusMessageBus class for asynchronous inter-thread communication.
// Implements a publish-subscribe mechanism for handling DataMessages efficiently.
//
// Author: [ad 47]
// Created: 2025-04-11

#include "octopus_ipc_message_bus.hpp"
#include <iostream>
#include <memory>
#include <chrono>
OctopusMessageBus::OctopusMessageBus(size_t thread_count, size_t max_queue_size)
    : threadPool_(thread_count, max_queue_size, TaskOverflowStrategy::Block), running_(false)
{
    // Constructor initializes the message bus with a thread pool.
    // Parameters:
    // - thread_count: Number of worker threads to start initially.
    // - max_queue_size: Maximum number of tasks allowed in the queue.
    // The TaskOverflowStrategy::Block ensures that tasks are blocked when the queue is full.
}

OctopusMessageBus::~OctopusMessageBus()
{
    // Destructor stops the message bus and joins all threads.
    stop();
}

static OctopusMessageBus& OctopusMessageBus::instance()
{
    // Singleton pattern ensures only one instance of the message bus exists.
    static OctopusMessageBus instance(4, 100); // Default to 4 threads and a queue size of 100.
    return instance;
}

OctopusMessageBus::SubscriptionToken OctopusMessageBus::subscribe(uint8_t group, MessageCallback callback)
{
    // Subscribe to messages of a specific group.
    // Parameters:
    // - group: Group ID to subscribe to.
    // - callback: Function to call when a message in the group is received.
    // Returns a unique SubscriptionToken to manage the subscription.
    std::lock_guard<std::mutex> lock(subMutex_);
    SubscriptionToken token = nextToken_++;
    subscribers_[group][token] = std::move(callback);
    return token;
}

void OctopusMessageBus::unsubscribe(uint8_t group, SubscriptionToken token)
{
    // Unsubscribe from a message group using a subscription token.
    // Parameters:
    // - group: Group ID to unsubscribe from.
    // - token: Subscription token returned during subscription.
    std::lock_guard<std::mutex> lock(subMutex_);
    auto groupIt = subscribers_.find(group);
    if (groupIt != subscribers_.end())
    {
        groupIt->second.erase(token);
    }
}

void OctopusMessageBus::publish(const DataMessage& message)
{
    // Publish a message to the bus.
    // Parameters:
    // - message: The message to dispatch to subscribers.
    // Adds the message to the queue and notifies the dispatcher thread.
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(message);
    }
    queueCondVar_.notify_one();
}

void OctopusMessageBus::start()
{
    // Start the internal dispatcher threads.
    // Initializes and starts the background threads responsible for processing messages.
    if (running_)
        return;

    running_ = true;
    dispatcherThreads_.reserve(threadPool_.get_thread_count());
    for (size_t i = 0; i < threadPool_.get_thread_count(); ++i)
    {
        dispatcherThreads_.emplace_back(&OctopusMessageBus::workerLoop, this);
    }
}

void OctopusMessageBus::stop()
{
    // Stop the dispatcher and clear resources.
    // Stops the background dispatcher threads and clears all pending messages from the queue.
    if (!running_)
        return;

    running_ = false;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::queue<DataMessage> empty;
        std::swap(messageQueue_, empty);
    }
    queueCondVar_.notify_all();
    for (auto& thread : dispatcherThreads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    dispatcherThreads_.clear();
}

void OctopusMessageBus::workerLoop()
{
    // The message dispatching loop executed by each dispatcher thread.
    // Continuously processes messages in the queue and dispatches them to subscribers.
    while (running_)
    {
        DataMessage message;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondVar_.wait(lock, [this] { return !messageQueue_.empty() || !running_; });
            if (!running_)
                break;
            message = std::move(messageQueue_.front());
            messageQueue_.pop();
        }

        auto groupIt = subscribers_.find(message.group);
        if (groupIt != subscribers_.end())
        {
            for (const auto& [token, callback] : groupIt->second)
            {
                // Enqueue the callback to the thread pool for execution.
                threadPool_.enqueue([callback, message] { callback(message); });
            }
        }
    }
}
