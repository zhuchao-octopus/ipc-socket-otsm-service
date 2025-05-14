/*******************************************************************************
 * @file     octopus_task_manager_gpio.h
 * @brief    Header file for managing GPIO (General Purpose Input/Output) pins in the Octopus project.
 * @details  This file provides functions for handling GPIO operations, including
 *           initializing, starting, and managing GPIO tasks. It also defines the
 *           structures for tracking the status of keys and GPIO pins.
 * @version  1.0
 * @date     2024-12-12
 * @author   Octopus Team
 ******************************************************************************/
#ifndef __OCTOPUS_TASK_MANAGER_GPIO_H__
#define __OCTOPUS_TASK_MANAGER_GPIO_H__

#include "octopus_platform.h"  // Include platform-specific configurations
#include "octopus_gpio_hal.h"   // Include GPIO HAL for hardware-specific functionality

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/** 
 * @brief    GPIO status redundancy macros.
 * @details  These macros define redundancy levels for GPIO status and key status.
 */
#define GPIO_POLLING_PERIOD_MS          				   20
 
#define GPIO_STATUS_REDUNDANCY                      8       ///< Redundancy level for GPIO status.

#define GPIO_KEY_STATUS_REDUNDANCY                  3       ///< Redundancy level for key status.
#define GPIO_KEY_STATUS_MAX_REDUNDANCY              255     ///< Maximum redundancy level for key status.

#define GPIO_KEY_STATUS_PRESS_PERIOD								(GPIO_KEY_STATUS_REDUNDANCY*1)   //100 ms
#define GPIO_KEY_STATUS_LONG_PRESS_PERIOD						(GPIO_KEY_STATUS_REDUNDANCY*10)  //1 s
#define GPIO_KEY_STATUS_LONG_LONG_PRESS_PERIOD			(GPIO_KEY_STATUS_REDUNDANCY*30)  //5 s  
////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************
 * @brief    Type definition for tracking the status of a GPIO key.
 * @details  This structure is used to store the status of a GPIO key, including
 *           whether the key is pressed, long-pressed, released, or dispatched.
 */
typedef struct {
    uint8_t key;             			///< The GPIO key identifier.
    bool pressed;            			///< Whether the key is currently pressed.
    bool release;           			///< Whether the key has been released.
    bool event_dispatched;   			///< Whether the key event has been dispatched.
	  uint8_t state;
    uint8_t long_press_duration ; ///< Duration for long-press detection.
    uint8_t press_count;     			///< Counter for tracking press actions.
} GPIO_KEY_STATUS;

/*******************************************************************************
 * @brief    Type definition for tracking the status of a GPIO pin.
 * @details  This structure is used to store the on/off status, changes, and
 *           counts for GPIO pin events.
 */
typedef struct
{
    bool offon;          ///< GPIO pin status (0 for off, 1 for on).
    bool changed;        ///< Indicates if the GPIO pin status has changed.
    uint8_t count1;      ///< A counter used for GPIO pin event tracking.
    uint8_t count2;      ///< Another counter used for tracking GPIO events.
} GPIO_STATUS;

/*******************************************************************************
 * @brief    Function declarations for managing GPIO tasks.
 * @details  These functions are used to initialize, start, and manage GPIO-related
 *           tasks during runtime.
 */

void app_gpio_init_running(void);     ///< Initializes GPIO tasks at runtime.
void app_gpio_start_running(void);    ///< Starts GPIO tasks.
void app_gpio_assert_running(void);   ///< Asserts GPIO tasks are running.
void app_gpio_running(void);          ///< Manages GPIO tasks during runtime.
void app_gpio_post_running(void);     ///< Handles post-processing of GPIO tasks.
void app_gpio_stop_running(void);     ///< Stops GPIO tasks during runtime.

bool IsAccOn(void);                    ///< Checks if the ACC (Accessory) pin is ON.
GPIO_KEY_STATUS* get_key_status_by_key(uint8_t key);

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_GPIO_H__ */
