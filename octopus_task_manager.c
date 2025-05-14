/*******************************************************************************
 * @file        octopus_task_manager.c
 * @brief       Task manager module for managing multiple tasks and their states
 *
 * This file implements a task manager that controls the lifecycle of tasks
 * in a system. Tasks can transition between various predefined states,
 * such as INIT, START, RUN, and STOP. Each task has its own state and
 * corresponding state-specific function handlers. The task manager provides
 * APIs for initializing, starting, stopping, and running tasks.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *
 * @note        This module assumes a fixed number of tasks (`TASK_ID_MAX_NUM`)
 *              and relies on a configuration (`otms_t`) for task-specific
 *              state handling.
 ******************************************************************************/

/******************************************************************************
 * INCLUDES
 */
#include "octopus_task_manager.h" // Include task manager for scheduling tasks
#include "octopus_log.h"          // Include logging functions for debugging
#include "octopus_system.h"
#include "octopus_gpio.h"
#include "octopus_carinfor.h"
#include "octopus_ble.h"
#include "octopus_key.h"
#include "octopus_update_soc.h"
#include "octopus_update_mcu.h"
#include "octopus_ipc.h"
#include "octopus_bafang.h"
#include "octopus_uart_ptl_1.h"    // Include UART protocol header
#include "octopus_uart_ptl_2.h"    // Include UART protocol header
/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* Add any necessary macros here */

/*******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

/* Define any custom types here */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** Static configuration for all tasks in the OTMS. */
const static otms_t lat_otms_config[TASK_ID_MAX_NUM] = {
#if 1
    [TASK_ID_PTL_1] = {
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
		
	#ifdef TASK_MANAGER_STATE_MACHINE_PTL2
    [TASK_ID_PTL_2] = {
        .state_limit = OTMS_S_INVALID,
        .func = {
            [OTMS_S_INIT] = ptl_2_init_running,
            [OTMS_S_START] = ptl_2_start_running,
            [OTMS_S_ASSERT_RUN] = ptl_2_assert_running,
            [OTMS_S_RUNNING] = ptl_2_running,
            [OTMS_S_POST_RUN] = ptl_2_post_running,
            [OTMS_S_STOP] = ptl_2_stop_running,
        },
    },
	#endif
		
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
   #ifdef TASK_MANAGER_STATE_MACHINE_GPIO
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
    #endif
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

#ifdef TASK_MANAGER_STATE_MACHINE_CAN
 [TASK_ID_CAN] = {
            .state_limit = OTMS_S_INVALID,
            .func = {
                [OTMS_S_INIT] = app_can_init_running,
                [OTMS_S_START] = app_can_start_running,
                [OTMS_S_ASSERT_RUN] = app_can_assert_running,
                [OTMS_S_RUNNING] = app_can_running,
                [OTMS_S_POST_RUN] = app_can_post_running,
                [OTMS_S_STOP] = app_can_stop_running,
            },
        },
#endif

#ifdef TASK_MANAGER_STATE_MACHINE_BAFANG
 [TASK_ID_PTL_BAFANG] = {
        .state_limit = OTMS_S_INVALID,
        .func = {
            [OTMS_S_INIT] = app_bafang_ptl_init_running,
            [OTMS_S_START] = app_bafang_ptl_start_running,
            [OTMS_S_ASSERT_RUN] = app_bafang_ptl_assert_running,
            [OTMS_S_RUNNING] = app_bafang_ptl_running,
            [OTMS_S_POST_RUN] = app_bafang_ptl_post_running,
            [OTMS_S_STOP] = app_bafang_ptl_stop_running,
        },
    },
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
/* Add any constants here */

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Executes the state-specific function for a given task.
 *
 * @param task_id ID of the task to execute.
 * @param state   State to execute for the task.
 */
inline static void otms_exec_state(otms_id_t task_id, otms_state_t state);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/* Add any global variables here */

/*******************************************************************************
 * STATIC VARIABLES
 ******************************************************************************/

/**
 * @brief Array to store the current state of each task.
 */
static otms_state_t otms_task_state[TASK_ID_MAX_NUM];

/*******************************************************************************
 * EXTERNAL VARIABLES
 ******************************************************************************/

/* Add any external variables here */

/*******************************************************************************
 * FUNCTION IMPLEMENTATIONS
 ******************************************************************************/

/**
 * @brief Gets the task manager configuration.
 *
 * @return Pointer to the task manager configuration.
 */
const otms_t *otms_get_config(void)
{
    return lat_otms_config;
}

/**
 * @brief Initializes the task manager and sets all tasks to the INIT state.
 */
void task_manager_init(void)
{
    otms_id_t i;
    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        otms_exec_state(i, OTMS_S_INIT); // Set each task to the INIT state.
    }
}

/**
 * @brief Starts all tasks by transitioning them to the START state.
 */
void task_manager_start(void)
{
    otms_id_t i;
    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        otms_exec_state(i, OTMS_S_START); // Set each task to the START state.
    }
}

/**
 * @brief Stops all tasks by transitioning them to the STOP state.
 */
void task_manager_stop(void)
{
    otms_id_t i;

    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        otms_exec_state(i, OTMS_S_STOP); // Set each task to the STOP state.
    }
}

/**
 * @brief Runs the current state-specific function for all tasks.
 */
void task_manager_run(void)
{
    otms_id_t i;

    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        otms_exec_state(i, otms_task_state[i]); // Execute the current state for each task.
    }
}

/**
 * @brief Sets the state of a specific task.
 *
 * @param task_id ID of the task.
 * @param state   New state to set for the task.
 */
void otms_set_state(otms_id_t task_id, otms_state_t state)
{
    if (task_id < TASK_ID_MAX_NUM)
    {
        otms_task_state[task_id] = state; // Update the task state.
    }
    else
    {
        MY_ASSERT(0); // Invalid task ID.
    }
}

/**
 * @brief Gets the current state of a specific task.
 *
 * @param task_id ID of the task.
 * @return Current state of the task.
 */
otms_state_t otms_get_state(otms_id_t task_id)
{
    otms_state_t state;

    if (task_id < TASK_ID_MAX_NUM)
    {
        state = otms_task_state[task_id];
    }
    else
    {
        state = OTMS_S_INVALID; // Invalid state.
        MY_ASSERT(0);           // Invalid task ID.
    }
    return state;
}

/**
 * @brief Handles transitions when entering the RUN state.
 */
void otms_on_enter_run(void)
{
    otms_id_t i = 0;

    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        if (otms_get_state(i) > OTMS_S_POST_RUN)
        {
            otms_set_state(i, OTMS_S_START); // Reset tasks to the START state.
        }
    }
}

/**
 * @brief Handles transitions when exiting the POST_RUN state.
 */
void otms_on_exit_post_run(void)
{
    otms_id_t i = 0;

    for (i = 0; i < TASK_ID_MAX_NUM; i++)
    {
        otms_set_state(i, OTMS_S_STOP); // Transition tasks to the STOP state.
    }
}

/**
 * @brief Executes the state-specific function for a given task.
 *
 * @param task_id ID of the task to execute.
 * @param state   State to execute for the task.
 */
static void otms_exec_state(otms_id_t task_id, otms_state_t state)
{
    const otms_t *cfg = otms_get_config();

    if ((task_id < TASK_ID_MAX_NUM) && (NULL != cfg) && (cfg[task_id].state_limit > state))
    {
        if (NULL != cfg[task_id].func[state])
        {
            cfg[task_id].func[state](); // Call the state-specific function.
        }
    }
}
