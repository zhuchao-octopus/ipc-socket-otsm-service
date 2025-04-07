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

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Function pointer type for handling server responses.
     * @param response The received response data.
     * @param size The size of the response vector.
     */
    typedef void (*ResponseCallback)(const std::vector<int> &response, int size);

    /**
     * @brief Registers a callback function to be called when a response is received.
     * @param callback Function pointer to the callback.
     */
    void register_ipc_socket_callback(ResponseCallback callback);

    /**
     * @brief Initializes the client connection and starts the response receiver thread.
     * This function is automatically called when the shared library is loaded.
     */
    void app_main();

    /**
     * @brief Cleans up the client connection and stops the response receiver thread.
     * This function is automatically called when the shared library is unloaded.
     */
    void exit_cleanup();

#ifdef __cplusplus
}
#endif

void app_send_query(uint8_t group, uint8_t msg);

void app_send_query(uint8_t group, uint8_t msg, const std::vector<uint8_t> &query_array);

void app_send_command(uint8_t group, uint8_t msg);

void app_send_command(uint8_t group, uint8_t msg, const std::vector<uint8_t> &query_array);
#endif // CLIENT_HPP
