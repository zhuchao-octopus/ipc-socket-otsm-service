/*******************************************************************************
 * @file     octopus.h
 * @brief    Header file for the Octopus Task Manager.
 *           This file provides function declarations, macros, and type definitions
 *           for managing tasks and event loops in the Octopus system.
 *           It supports platform-specific configurations and RTOS choices.
 *
 * @note     This file is part of the Octopus project and should be included
 *           in the task manager implementation files.
 *           It allows users to define task states, events, and handle
 *           the task scheduling mechanisms.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team

 ******************************************************************************/

#ifndef ___OCTOPUS_TASK_MANAGER_OCTOPUS_H___
#define ___OCTOPUS_TASK_MANAGER_OCTOPUS_H___

/*******************************************************************************
 * INCLUDE FILES
 * Include standard libraries and platform-specific headers.
 */
 #include "octopus_platform.h"       // Platform-specific configurations and settings
 #include "octopus_task_manager.h"   // Task management functions and definitions
 #include "octopus_uart_ptl_1.h"       // UART protocol handling
 #include "octopus_tickcounter.h"    // Timer and tick counter management
 #include "octopus_msgqueue.h" // Message queue for inter-task communication
 #include "octopus_log.h"            // Octopus-specific logging utilities

 #include "octopus_timer_hal.h"      // Hardware Abstraction Layer (HAL) for timers
 #include "octopus_uart_hal.h" // HAL for UART communication
 #include "octopus_gpio_hal.h"       // HAL for GPIO (General-Purpose Input/Output)
 #include "octopus_flash_hal.h" // HAL for flash memory management
 #include "octopus_ble_hal.h"        // HAL for Bluetooth Low Energy (BLE)

 #include "octopus_carinfor.h"       // Car information management structures and functions
 #include "octopus_sif.h"            // SIF (Serial Interface Framework) protocol interface
 #include "octopus_key.h"            // Key input handling and processing
 #include "octopus_bms.h"


#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * DEBUG SWITCH MACROS
     * Add macros for enabling or disabling debug functionality.
     */

    /*******************************************************************************
     * GENERAL MACROS
     * Define common bit manipulation macros and constants.
     */

    /*******************************************************************************
     * TYPE DEFINITIONS
     * Define any necessary types or structs here.
     */

    /*******************************************************************************
     * CONSTANTS
     * Define any necessary constant values here.
     */

    /*******************************************************************************
     * FUNCTION DECLARATIONS
     * Declare external functions used in the task manager.
     */

    // Retrieves the main user task ID.
    uint8_t GetTaskManagerStateMachineId(void);

    // Initializes the task manager's main task.
    void TaskManagerStateMachineInit(void);

// The main event loop for the task manager.
#ifdef PLATFORM_CST_OSAL_RTOS
 uint16_t TaskManagerStateEventLoop(uint8_t task_id, uint16_t events);

#elif defined(PLATFORM_ITE_OPEN_RTOS)

void *TaskManagerStateEventLoop(void *arg);

#elif defined(PLATFORM_LINUX_RISC)

void TaskManagerStateStopRunning();

#else
 void TaskManagerStateMachineInit(void);
 void TaskManagerStateEventLoop(void *arg);
#endif

#ifdef __cplusplus
}
#endif

#endif // ___OCTOPUS_TASK_MANAGER_OCTOPUS_H___
