/*******************************************************************************
 * @file     octopus_ipc_socket.h
 * @brief    Provides system management functions for task manager, including
 *           version information, state machine management, and MCU update states.
 *
 * This header file defines the necessary API to manage the system state
 * within the Octopus Task Manager framework. It includes management of the
 * main control state machine, MCU update states, and platform-specific state
 * transitions for the system's MPU (Microprocessor Unit) and boot sequence.
 *
 * The file also includes macros for versioning and the system's state
 * definitions to aid in the initialization, startup, and state transitions of
 * the task manager. Additionally, the system status for various stages of
 * application execution is provided.
 *
 * @note     This file assumes the use of platform-specific configurations
 *           for the task manager's operation and MCU update procedures.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *******************************************************************************/

#ifndef __OCTOPUS_IPC_SOCKET_H__
#define __OCTOPUS_IPC_SOCKET_H__

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "octopus_platform.h"

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
    /*******************************************************************************
     * Callback function declaration
     * Define a function pointer type for the callback that takes an integer.
     ******************************************************************************/
    // Define a type for the callback that accepts an integer
    typedef void (*CarInforCallback_t)(int cmd);

    /*******************************************************************************
     * GLOBAL FUNCTIONS DECLARATION
     ******************************************************************************/

    /**
     * @brief Initialize the application system.
     */
    void app_ipc_socket_init_running(void);

    /**
     * @brief Start running the application system.
     */
    void app_ipc_socket_start_running(void);

    /**
     * @brief Assert and maintain the running state of the application system.
     */
    void app_ipc_socket_assert_running(void);

    /**
     * @brief Execute the main running logic of the application system.
     */
    void app_ipc_socket_running(void);

    /**
     * @brief Post-run procedures for the application system.
     */
    void app_ipc_socket_post_running(void);

    void app_ipc_socket_stop_running(void);

    void register_car_infor_callback(CarInforCallback_t callback);

    void update_push_interval_ms(uint16_t delay_ms);

    void otsm_do_ipc_Command(uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_IPC_SOCKET_H__ */
