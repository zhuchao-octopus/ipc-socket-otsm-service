#ifndef OCTOPUS_MESSAGE_BUS_C_INTERFACE_H
#define OCTOPUS_MESSAGE_BUS_C_INTERFACE_H
#include "octopus_message_bus.hpp"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef uint64_t SubscriptionToken;
    /**
     * @brief Subscribe to a message group using a C-style callback.
     *
     * This function wraps the C-style callback in a lambda and registers it to the message bus.
     * It returns a token that must be used to unsubscribe later.
     *
     * @param group The message group ID to subscribe to.
     * @param callback The C-style callback function to be invoked when a message arrives.
     * @param user_data Pointer to user-defined data that will be passed to the callback.
     * @return SubscriptionToken A non-zero token for successful subscription, or 0 if failed.
     */
    SubscriptionToken ipc_subscribe(uint8_t group, void (*callback)(const DataMessage *, void *));

    /**
     * @brief Unsubscribe a previously registered callback using its token.
     *
     * The token must be the one returned from the subscribe() call.
     *
     * @param group The message group ID to unsubscribe from.
     * @param token The token received when subscribing.
     */
    void ipc_unsubscribe(uint8_t group, OctopusMessageBus::SubscriptionToken token);

    /**
     * @brief Publish a message to the message bus.
     *
     * The message will be dispatched to all subscribers registered for its group.
     *
     * @param msg Pointer to a valid DataMessage object to be published.
     */
    void ipc_publish(const DataMessage *msg);
    void ipc_post_message(const DataMessage *msg);
#ifdef __cplusplus
}
#endif

#endif // OCTOPUS_MESSAGE_BUS_C_INTERFACE_H
