/**
 * @file octopus_ipc_app_client.hpp
 * @brief Header file for client-side communication with the server.
 *
 * Provides an interface for the UI layer to send queries and receive responses via a callback.
 * Supports asynchronous response handling through a registered callback function.
 *
 * Author: ak47
 * Organization: octopus
 * Date Time: 2025/03/13 21:00
 */
#ifndef __OCTOPUS_IPC_APP_HPP__
#define __OCTOPUS_IPC_APP_HPP__

#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <csignal>
#include <chrono>
#include "octopus_message_bus.hpp"
#include "octopus_message_bus_c_api.h"
#include "../IPC/octopus_ipc_ptl.hpp"
#include "../IPC/octopus_logger.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Function pointer type for handling server responses.
     * @param response The received response data.
     * @param size The size of the response vector.
     */

    typedef void (*OctopusAppResponseCallback)(const DataMessage &query_msg, int size);

    /**
     * @brief Registers a callback function to be called when a response is received.
     * @param callback Function pointer to the callback.
     */

    void ipc_register_socket_callback(std::string func_name, OctopusAppResponseCallback callback);
    void ipc_unregister_socket_callback(OctopusAppResponseCallback callback);

    /**
     * @brief Initializes the client connection and starts the response receiver thread.
     * This function is automatically called when the shared library is loaded.
     */
    void ipc_app_main();

    /**
     * @brief Cleans up the client connection and stops the response receiver thread.
     * This function is automatically called when the shared library is unloaded.
     */
    void ipc_exit_cleanup();

    void ipc_send_message(DataMessage &message);
    void ipc_send_message_queue(DataMessage &message);
    void ipc_send_message_queue_delayed(DataMessage& message, int delay_ms);

#ifdef __cplusplus
}
#endif

#endif // CLIENT_HPP
