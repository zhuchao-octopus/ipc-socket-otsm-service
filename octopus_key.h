/*******************************************************************************
 * @file octopus_task_manager_key.h
 * @brief Header file for managing key/button input handling within the Octopus Task Manager.
 * 
 * This file defines macros, types, and function declarations for managing key/button 
 * input events such as detecting key presses, key releases, long presses, and double presses. 
 * It includes function declarations for the initialization, starting, and stopping of key input processing, 
 * as well as handling various key states.
 * 
 * Key events can be used to navigate through menus, confirm selections, or power on/off the system.
 * 
 * @note The key mapping (e.g., OCTOPUS_KEY_0) and key codes should be customized based on the actual hardware.
 * 
 * @ingroup APP:SUB_TYPE
 * @version  1.0
 * @date     2024-12-12
 * @author   Octopus Team
 ******************************************************************************/

#ifndef __OCTOPUS_TASK_MANAGER_KEY_H__
#define __OCTOPUS_TASK_MANAGER_KEY_H__


/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"   // Platform-specific configurations and definitions

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * MACROS
 * The following macros define key IDs and their respective actions.
 */

// Key IDs (these should correspond to the physical keys on the device)
#define OCTOPUS_KEY_0 0
#define OCTOPUS_KEY_1 1
#define OCTOPUS_KEY_2 2
#define OCTOPUS_KEY_3 3
#define OCTOPUS_KEY_4 4
#define OCTOPUS_KEY_5 5
#define OCTOPUS_KEY_6 6
#define OCTOPUS_KEY_7 7
#define OCTOPUS_KEY_8 8
#define OCTOPUS_KEY_9 9
#define OCTOPUS_KEY_10 10
#define OCTOPUS_KEY_11 11
#define OCTOPUS_KEY_12 12
#define OCTOPUS_KEY_13 13
#define OCTOPUS_KEY_14 14
#define OCTOPUS_KEY_POWER 20
/*******************************************************************************
 * TYPEDEFS
 * The following defines key codes representing different actions or states of the keys.
 */
 //key action
#define KEY_ACTION_NONE          0 
#define KEY_ACTION_DOWN          1
#define	KEY_ACTION_UP            2
#define	KEY_ACTION_LONG          3
#define	KEY_ACTION_DUBDOW        4
#define	KEY_ACTION_DOUBLE        5

// Key codes
#define KEY_CODE_IDLE               (0x00)  /**< No key action detected */
#define KEY_CODE_MENU               (0x01)  /**< Menu button pressed */
#define KEY_CODE_UP                 (0x02)  /**< Up button pressed */
#define KEY_CODE_DOWN               (0x03)  /**< Down button pressed */
#define KEY_CODE_OK                 (0x04)  /**< OK button pressed */
#define KEY_CODE_LEFT               (0x05)  /**< Left button pressed */
#define KEY_CODE_RIGHT              (0x06)  /**< Right button pressed */
#define KEY_CODE_BACK               (0x07)  /**< Back button pressed */
#define KEY_CODE_INFO               (0x08)  /**< Info button pressed */
#define KEY_CODE_ILL                (0x09)  /**< Invalid key action */
#define KEY_CODE_POWER              (0x0A)  /**< Power button pressed */

// Key states
#define KEY_STATE_NONE         		(0x00)  /**< Key is released */
#define KEY_STATE_RELEASED          (0x01)  /**< Key is released */
#define KEY_STATE_PRESSED           (0x02)  /**< Key is pressed */
#define KEY_STATE_LONG_PRESSED      (0x03)  /**< Key is long pressed */
#define KEY_STATE_LONG_LONG_PRESSED (0x04)  /**< Key is double pressed */

/*******************************************************************************
 * DEBUG SWITCH MACROS
*/
#ifdef TASK_MANAGER_STATE_MACHINE_KEY

/*******************************************************************************
 * GLOBAL FUNCTIONS DECLARATIONS
 * The following functions manage the state of key handling in the system.
 */

/**
 * @brief Initializes the key input handling system.
 * 
 * This function sets up the necessary configurations and peripherals to start handling key events.
 */
void app_key_init_running(void);

/**
 * @brief Starts the key input handling process.
 * 
 * This function begins the process of detecting and processing key events, allowing the system 
 * to react to key presses.
 */
void app_key_start_running(void);

/**
 * @brief Asserts that the key input system is running.
 * 
 * This function ensures that the key input system is properly initialized and running.
 */
void app_key_assert_running(void);

/**
 * @brief Handles key input events in the running state.
 * 
 * This function processes the key events in real-time, updating the system based on key actions.
 */
void app_key_running(void);

/**
 * @brief Posts key input events for further processing.
 * 
 * This function queues key input events for future processing or handling by other tasks.
 */
void app_key_post_running(void);

/**
 * @brief Stops the key input handling process.
 * 
 * This function stops the key input event detection, effectively disabling the key input system.
 */
void app_key_stop_running(void);

#ifdef __cplusplus
}
#endif

#endif

#endif /* __OCTOPUS_TASK_MANAGER_KEY_H__ */
