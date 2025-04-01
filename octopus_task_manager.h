/*******************************************************************************
 * @file    octopus_task_manager.h
 * @brief   Task manager header file for the Octopus platform.
 *          Provides declarations for task state machine management, including
 *          initialization, state transitions, and task-specific configurations.
 *
 * @details This file defines macros, data structures, and function prototypes
 *          for managing tasks within the Octopus system. Each task operates as
 *          a finite state machine with well-defined states and transitions.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *
 * @note    Ensure that task-specific state functions are implemented and linked
 *          correctly in the corresponding source files.
 ******************************************************************************/

#ifndef __OCTOPUS_TASK_MANAGER_H__
#define __OCTOPUS_TASK_MANAGER_H__

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"   // Platform-specific configurations.
#include "octopus_system.h"     // System-level functions and definitions.
#include "octopus_gpio.h"       // GPIO control functions.
#include "octopus_carinfor.h"   // Car information management functions.
#include "octopus_ble.h"        // BLE communication functions.
#include "octopus_key.h"        // Key input management.
#include "octopus_uart_ptl.h"   // UART protocol functions.
#include "octopus_update_soc.h" // SOC update functions.
#include "octopus_update_mcu.h" // MCU update functions.

#include "octopus_ipc_socket.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 *
 * Macros for enabling or disabling debug-specific functionality in this file.
 */

/*******************************************************************************
 * MACROS
 */

/** Task states for the Octopus Task Manager State Machine (OTMS). */
#define OTMS_S_INIT (0x00U)       /**< Initial state. */
#define OTMS_S_START (0x01U)      /**< Start state. */
#define OTMS_S_ASSERT_RUN (0x02U) /**< State to verify running conditions. */
#define OTMS_S_RUNNING (0x03U)    /**< Running state. */
#define OTMS_S_POST_RUN (0x04U)   /**< Post-run state for cleanup or preparation. */
#define OTMS_S_STOP (0x05U)       /**< Stop state. */
#define OTMS_S_INVALID (0xFFU)    /**< Invalid state. */

#define OTMS_S_COUNT (6U) /** Total number of task states. */

    /*******************************************************************************
     * TYPEDEFS
     */

    /** Task ID type, used to identify individual tasks. */
    typedef int32_t otms_id_t;

    /** Task state type, representing the current state of a task. */
    typedef uint8_t otms_state_t;

    /** Function pointer type for state-specific task functions. */
    typedef void (*otms_state_func_t)(void);

    /**
     * Task State Machine (OTMS) structure.
     * Defines the state transition table for each task.
     */
    typedef struct
    {
        otms_state_t state_limit;                   /**< Maximum valid state for the task. */
        const otms_state_func_t func[OTMS_S_COUNT]; /**< Array of function pointers for state-specific behavior. */
    } otms_t;

    /** Enum for defining unique task IDs. */
    typedef enum
    {
        TASK_ID_PTL = 0, /**< Protocol handling task. */
        TASK_ID_SYSTEM,  /**< System task. */
        TASK_ID_GPIO,    /**< GPIO management task. */

        TASK_ID_CAR_INFOR, /**< Car information management task. */

        TASK_ID_BLE,        /**< BLE communication task. */
        TASK_ID_KEY,        /**< Key input handling task. */
        TASK_ID_UPDATE_MCU, /**< MCU update task. */

        TASK_ID_UPDATE_SOC, /**< SOC update task. */

        TASK_ID_IPC_SOCKET,
        TASK_ID_MAX_NUM /**< Maximum number of tasks. */
    } TaskModule_t;

    /*******************************************************************************
     * CONSTANTS
     */

    /** Global configuration for the Octopus Task Manager State Machine (OTMS). */
    const otms_t *otms_get_config(void);

    /** Static configuration for all tasks in the OTMS. */
    const static otms_t lat_otms_config[TASK_ID_MAX_NUM] = {
#if 1
        [TASK_ID_PTL] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = ptl_init_running,
                [OTMS_S_START] = ptl_start_running,
                [OTMS_S_ASSERT_RUN] = ptl_assert_running,
                [OTMS_S_RUNNING] = ptl_running,
                [OTMS_S_POST_RUN] = ptl_post_running,
                [OTMS_S_STOP] = ptl_stop_running,
            },
        },

        [TASK_ID_SYSTEM] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_system_init_running,
                [OTMS_S_START] = app_system_start_running,
                [OTMS_S_ASSERT_RUN] = app_system_assert_running,
                [OTMS_S_RUNNING] = app_system_running,
                [OTMS_S_POST_RUN] = app_system_post_running,
                [OTMS_S_STOP] = app_system_stop_running,
            },
        },

        [TASK_ID_GPIO] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_gpio_init_running,
                [OTMS_S_START] = app_gpio_start_running,
                [OTMS_S_ASSERT_RUN] = app_gpio_assert_running,
                [OTMS_S_RUNNING] = app_gpio_running,
                [OTMS_S_POST_RUN] = app_gpio_post_running,
                [OTMS_S_STOP] = app_gpio_stop_running,
            },
        },

        [TASK_ID_CAR_INFOR] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_carinfo_init_running,
                [OTMS_S_START] = app_carinfo_start_running,
                [OTMS_S_ASSERT_RUN] = app_carinfo_assert_running,
                [OTMS_S_RUNNING] = app_carinfo_running,
                [OTMS_S_POST_RUN] = app_carinfo_post_running,
                [OTMS_S_STOP] = app_carinfo_stop_running,
            },
        },
#endif

#ifdef TASK_MANAGER_STATE_MACHINE_BLE
        [TASK_ID_BLE] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_ble_init_running,
                [OTMS_S_START] = app_ble_start_running,
                [OTMS_S_ASSERT_RUN] = app_ble_assert_running,
                [OTMS_S_RUNNING] = app_ble_running,
                [OTMS_S_POST_RUN] = app_ble_post_running,
                [OTMS_S_STOP] = app_ble_stop_running,
            },
        },
#endif

#ifdef TASK_MANAGER_STATE_MACHINE_KEY
        [TASK_ID_KEY] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_key_init_running,
                [OTMS_S_START] = app_key_start_running,
                [OTMS_S_ASSERT_RUN] = app_key_assert_running,
                [OTMS_S_RUNNING] = app_key_running,
                [OTMS_S_POST_RUN] = app_key_post_running,
                [OTMS_S_STOP] = app_key_stop_running,
            },
        },
#endif

#ifdef TASK_MANAGER_STATE_MACHINE_UPDATE
#ifdef TASK_MANAGER_STATE_MACHINE_MCU
        [TASK_ID_UPDATE_MCU] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_update_mcu_init_running,
                [OTMS_S_START] = app_update_mcu_start_running,
                [OTMS_S_ASSERT_RUN] = app_update_mcu_assert_running,
                [OTMS_S_RUNNING] = app_update_mcu_running,
                [OTMS_S_POST_RUN] = app_update_mcu_post_running,
                [OTMS_S_STOP] = app_update_mcu_stop_running,
            },
        },
#elif defined(TASK_MANAGER_STATE_MACHINE_SOC)
        [TASK_ID_UPDATE_SOC] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_update_soc_init_running,
                [OTMS_S_START] = app_update_soc_start_running,
                [OTMS_S_ASSERT_RUN] = app_update_soc_assert_running,
                [OTMS_S_RUNNING] = app_update_soc_running,
                [OTMS_S_POST_RUN] = app_update_soc_post_running,
                [OTMS_S_STOP] = app_update_soc_stop_running,
            },
        },
#endif
#endif

#ifdef TASK_MANAGER_STATE_MACHINE_IPC_SOCKET
        [TASK_ID_IPC_SOCKET] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_ipc_socket_init_running,
                [OTMS_S_START] = app_ipc_socket_start_running,
                [OTMS_S_ASSERT_RUN] = app_ipc_socket_assert_running,
                [OTMS_S_RUNNING] = app_ipc_socket_running,
                [OTMS_S_POST_RUN] = app_ipc_socket_post_running,
                [OTMS_S_STOP] = app_ipc_socket_stop_running,
            },
        },
#endif
    };

    /*******************************************************************************
     * GLOBAL VARIABLES DECLARATION
     */

    /*******************************************************************************
     * GLOBAL FUNCTIONS DECLARATION
     */

    /**
     * @brief Initializes the task manager and sets all tasks to the INIT state.
     */
    void task_manager_init(void);

    /**
     * @brief Starts all tasks by setting them to the START state.
     */
    void task_manager_start(void);

    /**
     * @brief Stops all tasks by setting them to the STOP state.
     */
    void task_manager_stop(void);

    /**
     * @brief Executes the current state for all tasks in the task manager.
     */
    void task_manager_run(void);

    /**
     * @brief Handles transitions to the RUN state for all tasks.
     */
    void otms_on_enter_run(void);

    /**
     * @brief Handles transitions from the POST_RUN state for all tasks.
     */
    void otms_on_exit_post_run(void);

    /**
     * @brief Sets the state of a specified task.
     * @param task_id ID of the task to set.
     * @param state   State to assign to the task.
     */
    void otms_set_state(otms_id_t task_id, otms_state_t state);

    /**
     * @brief Retrieves the current state of a specified task.
     * @param task_id ID of the task to query.
     * @return The current state of the task.
     */
    otms_state_t otms_get_state(otms_id_t task_id);

/** Macro to trigger a state transition for a specific task. */
#define OTMS(task_id, state) otms_set_state(task_id, state)

/** Macro to retrieve the current state of a specific task. */
#define GET_OTMS_STATE(task_id) otms_get_state(task_id)

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_H__ */
