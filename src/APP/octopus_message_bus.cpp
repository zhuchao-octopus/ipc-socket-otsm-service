// octopus_message_bus.cpp
//
// Implementation of the OctopusMessageBus class for asynchronous inter-thread communication.
// Implements a publish-subscribe mechanism for handling DataMessages efficiently.
//
// Author: [Your Name]
// Created: 2025-04-11

#include "octopus_message_bus.hpp"
#include <iostream>
#include <chrono>

// Constructor
OctopusMessageBus::OctopusMessageBus() : running_(false) {}

// Destructor
OctopusMessageBus::~OctopusMessageBus() {
    stop();
}

// Subscribe to a specific message group
void OctopusMessageBus::subscribe(uint8_t group, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(subMutex_);
    subscribers_[group].push_back(callback);
}

// Unsubscribe from a specific message group
void OctopusMessageBus::unsubscribe(uint8_t group, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(subMutex_);
    auto& callbacks = subscribers_[group];
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
}

// Publish a message to the bus
void OctopusMessageBus::publish(const DataMessage& message) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    messageQueue_.push(message);
    queueCondVar_.notify_one(); // Notify the dispatcher thread to process the message
}

// Internal dispatcher loop that processes incoming messages
void OctopusMessageBus::dispatchLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        queueCondVar_.wait(lock, [this] { return !messageQueue_.empty() || !running_; });

        // If there's no message, continue the loop
        if (!running_) break;

        DataMessage message = messageQueue_.front();
        messageQueue_.pop();
        lock.unlock(); // Unlock the queue while processing the message

        // Find subscribers and invoke the callbacks for the group
        std::lock_guard<std::mutex> subLock(subMutex_);
        auto group = message.group;
        if (subscribers_.find(group) != subscribers_.end()) {
            for (const auto& callback : subscribers_[group]) {
                callback(message); // Invoke the callback
            }
        }
    }
}

// Start the dispatcher thread
void OctopusMessageBus::start() {
    if (!running_) {
        running_ = true;
        dispatcherThread_ = std::thread(&OctopusMessageBus::dispatchLoop, this);
    }
}

// Stop the dispatcher thread
void OctopusMessageBus::stop() {
    if (running_) {
        running_ = false;
        queueCondVar_.notify_all(); // Wake up dispatcher thread
        if (dispatcherThread_.joinable()) {
            dispatcherThread_.join();
        }
    }
}
