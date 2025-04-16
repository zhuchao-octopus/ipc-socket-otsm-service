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
 * @brief A lightweight asynchronous message bus for decoupled component communication.
 * 
 * The OctopusMessageBus provides a mechanism for components to communicate asynchronously via messages. It supports
 * subscribing to message groups, publishing messages to those groups, and handling the dispatching of messages
 * to subscribed callbacks.
 * 
 * The message bus is implemented as a thread-safe singleton and uses an internal message queue to decouple
 * message publication and processing.
 */
#include "octopus_ipc_threadpool.hpp"
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>

/**
 * @class OctopusMessageBus
 * @brief A message bus that supports publishing and subscribing messages with multi-threaded processing.
 *
 * This class provides a message bus system where components can subscribe to specific message groups
 * and publish messages to those groups. It utilizes a thread pool to handle message dispatching
 * asynchronously, improving performance in multi-threaded environments.
 */
class OctopusMessageBus
{
public:
    /**
     * @brief Type alias for a callback function to handle incoming messages.
     *
     * This callback is invoked when a message of the corresponding group is published.
     */
    using MessageCallback = std::function<void(const DataMessage&)>;

    /**
     * @brief Type alias for a subscription token that uniquely identifies a subscription.
     *
     * A subscription token is generated for each callback, which can later be used to unsubscribe the callback.
     */
    using SubscriptionToken = uint64_t;

    /**
     * @brief Construct the OctopusMessageBus with a specified number of threads and maximum queue size.
     * @param thread_count Number of threads in the pool.
     * @param max_queue_size Maximum number of messages allowed in the dispatch queue.
     */
    OctopusMessageBus(size_t thread_count, size_t max_queue_size);
    static OctopusMessageBus& instance();
    /**
     * @brief Destructor to stop the dispatcher and clean up resources.
     */
    ~OctopusMessageBus();

    /**
     * @brief Subscribe to messages of a specific group.
     *
     * This function allows a component to subscribe to a specific message group. When a message of the specified
     * group is published, the provided callback will be invoked.
     *
     * A subscription token is returned, which can be used later to unsubscribe the callback.
     *
     * @param group Group ID to subscribe to.
     * @param callback Callback to invoke when a matching message is received.
     * @return SubscriptionToken A unique token that identifies the subscription.
     */
    SubscriptionToken subscribe(uint8_t group, MessageCallback callback);

    /**
     * @brief Unsubscribe from a message group using a subscription token.
     *
     * This function allows a component to unsubscribe from a message group using the token that was
     * provided during subscription. It removes the associated callback from the group.
     *
     * @param group Group ID to unsubscribe from.
     * @param token The subscription token returned during the subscription.
     */
    void unsubscribe(uint8_t group, SubscriptionToken token);

    /**
     * @brief Publish a message to the bus.
     *
     * This function allows a component to publish a message to the bus. The message will be dispatched asynchronously
     * to all subscribers of the corresponding group.
     *
     * @param message The message to dispatch to subscribers.
     */
    void publish(const DataMessage& message);

    /**
     * @brief Start the internal dispatcher threads.
     *
     * Starts the background threads that will process the messages in the queue and dispatch them to the appropriate
     * subscribers. This function must be called to begin message processing.
     */
    void start();

    /**
     * @brief Stop the dispatcher and clear resources.
     *
     * Stops the background dispatcher threads and clears all pending messages from the queue.
     */
    void stop();

private:
    /**
     * @brief Main worker function executed by each thread.
     *
     * Each thread waits for messages and processes them asynchronously.
     */
    void workerLoop();

    std::atomic<SubscriptionToken> nextToken_{1}; ///< Counter for generating unique subscription tokens

    /**
     * @brief A map of subscribers for each message group.
     *
     * This map stores callbacks for each group. The group ID is the key, and the value is another map that
     * associates subscription tokens with their corresponding callbacks.
     */
    std::unordered_map<uint8_t, std::unordered_map<SubscriptionToken, MessageCallback>> subscribers_;

    /**
     * @brief Mutex to protect access to the subscriber list.
     *
     * This mutex ensures thread-safety when subscribing or unsubscribing callbacks.
     */
    std::mutex subMutex_;

    /**
     * @brief Message queue for incoming messages.
     *
     * This queue holds the messages that are published to the bus and need to be dispatched to subscribers.
     */
    std::queue<DataMessage> messageQueue_;

    /**
     * @brief Mutex to protect access to the message queue.
     *
     * This mutex ensures thread-safety when adding or removing messages from the queue.
     */
    std::mutex queueMutex_;

    /**
     * @brief Condition variable to notify the dispatcher threads when new messages are available.
     *
     * The dispatcher threads are notified whenever a new message is published to the queue.
     */
    std::condition_variable queueCondVar_;

    /**
     * @brief Atomic flag to control the running state of the dispatcher threads.
     *
     * This flag indicates whether the dispatcher threads are currently running or have been stopped.
     */
    std::atomic<bool> running_;

    /**
     * @brief Vector of dispatcher threads responsible for processing messages.
     *
     * These threads continuously process messages from the queue and dispatch them to the appropriate subscribers.
     */
    std::vector<std::thread> dispatcherThreads_;

    /**
     * @brief The thread pool used for dispatching messages asynchronously.
     *
     * This thread pool manages a pool of threads to handle message dispatching tasks.
     */
    OctopusThreadPool threadPool_;
};
