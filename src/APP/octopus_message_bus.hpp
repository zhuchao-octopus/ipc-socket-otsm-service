// octopus_message_bus.hpp
//
// Message Bus for asynchronous inter-thread communication.
// Provides a publish-subscribe mechanism based on DataMessage structure.
// Designed for high-frequency, high-performance internal message passing.
//
// Author: [Your Name]
// Created: 2025-04-11

#pragma once

#include "../IPC/octopus_ipc_ptl.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
/////////////////////////////////////////////////////////////////////////////////////////////////
/*
┌────────────┐      publish()      ┌──────────────┐
│ Producer A │ ──────────────────▶ │ MessageBus  │
└────────────┘                     │              │
                                   │   ┌───────┐   │
┌────────────┐      publish()      │   │Queue1 │◀──┐
│ Producer B │ ──────────────────▶ │   └───────┘   │
└────────────┘                     │      ↓        │
                                   │ ┌───────────┐ │
                                   │ │ Dispatcher│ │──▶ callbackA(msg)
                                   │ └───────────┘ │
                                   └──────────────┘
*/
/////////////////////////////////////////////////////////////////////////////////////////////////
// octopus_message_bus.hpp
//
// Message Bus for asynchronous inter-thread communication.
// Provides a publish-subscribe mechanism based on DataMessage structure.
// Designed for high-frequency, high-performance internal message passing.
//
// Author: [Your Name]
// Created: 2025-04-11

/**
 * @brief OctopusMessageBus enables publish-subscribe messaging between components.
 * 
 * Subscribers can register interest in specific message groups, and published messages
 * are delivered asynchronously to registered callbacks.
 */
class OctopusMessageBus {
public:
    using MessageCallback = std::function<void(const DataMessage&)>;

    /**
     * @brief Construct a new OctopusMessageBus object.
     */
    OctopusMessageBus();

    /**
     * @brief Destroy the OctopusMessageBus object and stop dispatcher.
     */
    ~OctopusMessageBus();

    /**
     * @brief Subscribe to messages of a specific group.
     * 
     * @param group Group ID to subscribe to.
     * @param callback Function to call when message with that group is received.
     */
    void subscribe(uint8_t group, MessageCallback callback);

    /**
     * @brief Unsubscribe from messages of a specific group.
     * 
     * @param group Group ID to unsubscribe from.
     * @param callback Function that was registered to the group.
     */
    void unsubscribe(uint8_t group, MessageCallback callback);

    /**
     * @brief Publish a message to the bus.
     * 
     * This message will be asynchronously dispatched to all matching subscribers.
     * 
     * @param message The message to publish.
     */
    void publish(const DataMessage& message);

    /**
     * @brief Starts the internal dispatcher thread.
     */
    void start();

    /**
     * @brief Stops the dispatcher and clears all pending messages.
     */
    void stop();

private:
    void dispatchLoop();

    std::unordered_map<uint8_t, std::vector<MessageCallback>> subscribers_; ///< Callbacks per group
    std::mutex subMutex_; ///< Protects subscriber list

    std::queue<DataMessage> messageQueue_; ///< Incoming message queue
    std::mutex queueMutex_; ///< Protects the message queue
    std::condition_variable queueCondVar_; ///< Notifies dispatcher thread

    std::atomic<bool> running_; ///< Flag to control dispatcher
    std::thread dispatcherThread_; ///< Background dispatcher thread
};
