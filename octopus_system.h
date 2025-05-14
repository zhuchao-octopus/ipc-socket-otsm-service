/*******************************************************************************
* @file     octopus_task_manager_system.h
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



#ifndef __OCTOPUS_TASK_MANAGER_SYSTEM_H__
#define __OCTOPUS_TASK_MANAGER_SYSTEM_H__

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "octopus_platform.h"

#ifdef __cplusplus
extern "C"{
#endif

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Version information.
 */
#define OTMS_VERSION "v1.0.5"                         /**< Current version of the Task Manager System. */
#define OTMS_RELEASE_DATA_TIME  __DATE__ " " __TIME__ /**< Compilation date and time. */

/**
 * @brief Version strings for the application, hardware, and project.
 */
#define APP_VER_STR OTMS_VERSION //"v1.0.0"                   /**< Application version. */
#define HW_VER_STR  "v1.0.0"                   /**< Hardware version. */
#define PRJ_VER_STR "otsm"                 /**< Project version. */
#define VER_STR "MCU:" APP_VER_STR "HW:" HW_VER_STR "PRJ:" PRJ_VER_STR "\n" /**< Combined version string. */



/**
 * @brief System MPU (Microprocessor Unit) state definitions.
 */
#define SYSTEM_MPU_STATE_INIT           (0x00) /**< Initial state. */
#define SYSTEM_MPU_STATE_ENTER_PLAY     (0x01) /**< Entering playback animation. */
#define SYSTEM_MPU_STATE_ENTER_FINISH   (0x02) /**< Playback animation complete. */
#define SYSTEM_MPU_STATE_COMPLETED      (0x03) /**< Loading completed. */
#define SYSTEM_MPU_STATE_LEAVE_PLAY     (0x04) /**< Exiting playback animation. */
#define SYSTEM_MPU_STATE_LEAVE_FINISH   (0x05) /**< Exit animation complete. */

/*******************************************************************************
 * TYPEDEFS
 ******************************************************************************/

/**
 * @brief MCU (Microcontroller Unit) update state enumeration.
 */
typedef enum {
    MCU_UPDATE_ST_INIT          = (0x00),      /**< Update initialization. */
    MCU_UPDATE_ST_CHECK_FILE    = (0x01),      /**< Check update file. */
    MCU_UPDATE_ST_WAIT_CONFIRM  = (0x02),      /**< Wait for update confirmation. */
    MCU_UPDATE_ST_START         = (0x03),      /**< Start the update process. */
    MCU_UPDATE_ST_WAIT_BOOT     = (0x04),      /**< Wait for MCU to boot. */
    MCU_UPDATE_ST_TRANSFER      = (0x05),      /**< Transfer update data. */
    MCU_UPDATE_ST_COMPLETED     = (0x06),      /**< Update completed successfully. */
    MCU_UPDATE_ST_FAILED        = (0x07),      /**< Update failed. */
} mcu_update_state_t;

/**
 * @brief Mainboard (MB) state enumeration.
 */
typedef enum MB_STATE {
    MB_ST_INIT = 0,                            /**< Initialization state. */
    MB_ST_LOWPOWER,                            /**< Low-power state. */
    MB_ST_BOOTING,                             /**< Booting state. */
    MB_ST_STANDBY,                             /**< Standby state. */
    MB_ST_ON,                                  /**< Fully operational state. */
    MB_ST_PARTIAL,                             /**< Partial operation state. */
    MB_ST_SHUTDOWN,                            /**< Shutdown process. */
    MB_ST_OFF,                                 /**< Power-off state. */
    MB_ST_STOP,
} mb_state_t;

/*******************************************************************************
 * GLOBAL FUNCTIONS DECLARATION
 ******************************************************************************/

/**
 * @brief Initialize the application system.
 */
void app_system_init_running(void);

/**
 * @brief Start running the application system.
 */
void app_system_start_running(void);

/**
 * @brief Assert and maintain the running state of the application system.
 */
void app_system_assert_running(void);

/**
 * @brief Execute the main running logic of the application system.
 */
void app_system_running(void);

/**
 * @brief Post-run procedures for the application system.
 */
void app_system_post_running(void);

/**
 * @brief Stop the application system.
 */
void app_system_stop_running(void);

/**
 * @brief Set the current MPU (Microprocessor Unit) status.
 * @param status Status value to set.
 */
void system_set_mpu_status(uint8_t status);

/**
 * @brief Get the power-off request status.
 * @return True if power-off is requested, false otherwise.
 */
bool system_get_power_off_req(void);

/**
 * @brief Get the current MPU (Microprocessor Unit) status.
 * @return Current MPU status.
 */
uint8_t system_get_mpu_status(void);

/**
 * @brief Perform a handshake with the application layer.
 */
void system_handshake_with_app(void);

/**
 * @brief Perform a handshake with the MCU (Microcontroller Unit).
 */
void system_handshake_with_mcu(void);

/**
 * @brief Get the current mainboard state.
 * @return Current mainboard state.
 */
mb_state_t system_get_mb_state(void);

void app_power_on_off(bool onoff);
#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_SYSTEM_H__ */

