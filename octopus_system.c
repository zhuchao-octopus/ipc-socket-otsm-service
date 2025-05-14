/*******************************************************************************
 * @file     octopus_system.c
 * @brief    Implements the system control logic for managing states,
 *           message handling, and UART communication in an embedded application.
 *
 * This source file is responsible for initializing and controlling the system's
 * state machine. It handles incoming and outgoing messages through a UART
 * interface, processes system-level events, and manages the power state
 * and handshake procedures with both the MCU and external applications.
 *
 * The code uses a modular design to interface with other system components,
 * ensuring flexibility and scalability.
 *
 * @version  1.0.0
 * @date     2024-12-11
 * @author   Octopus Team
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "octopus_platform.h"     // Include platform-specific header for hardware platform details
#include "octopus_log.h"          // Include logging functions for debugging
#include "octopus_task_manager.h" // Include task manager for scheduling tasks
#include "octopus_gpio.h"
#include "octopus_system.h"
#include "octopus_tickcounter.h"
#include "octopus_msgqueue.h"
#include "octopus_flash_hal.h"

/*******************************************************************************
 * Debug Switch Macros
 * Define debug levels or other switches as required.
 ******************************************************************************/

/*******************************************************************************
 * MACROS
 * The following macros define key IDs and their respective actions.
 */
#define SYSTEM_POWER_ON_VALUE 1
#define SYSTEM_POWER_OFF_VALUE 0
/*******************************************************************************
 * Local Function Declarations
 * Declare static functions used only within this file.
 ******************************************************************************/
static bool system_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool system_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

void app_power_on_off(bool onoff);

/*******************************************************************************
 * Global Variables
 * Define variables accessible across multiple files if needed.
 ******************************************************************************/

/*******************************************************************************
 * Local Variables
 * Define static variables used only within this file.
 ******************************************************************************/
static mb_state_t lt_mb_state;         // Current state of the message buffer
static uint8_t l_u8_mpu_status = 0;    // Tracks the status of the MPU
static uint8_t l_u8_power_off_req = 0; // Tracks if a power-off request is pending
static uint32_t l_t_msg_wait_10_timer; // Timer for 10 ms message waiting period

/*******************************************************************************
 * Global Function Implementations
 ******************************************************************************/

/**
 * @brief Initializes the system for running.
 *
 * This function registers the system module with the communication layer
 * and transitions the system task to an invalid state.
 */
void app_system_init_running(void)
{
    LOG_LEVEL("app_system_init_running\r\n");
    OTMS(TASK_ID_SYSTEM, OTMS_S_INVALID);

    //ptl_register_module(P2M_MOD_DEBUG, debug_send_handler, debug_receive_handler);
#ifdef TASK_MANAGER_STATE_MACHINE_MCU
    ptl_register_module(MCU_TO_SOC_MOD_SYSTEM, system_send_handler, system_receive_handler);
#elif defined(TASK_MANAGER_STATE_MACHINE_SOC)
    ptl_register_module(SOC_TO_MCU_MOD_SYSTEM, system_send_handler, system_receive_handler);
#else
    ptl_register_module(MCU_TO_SOC_MOD_SYSTEM, system_send_handler, system_receive_handler);
#endif
}

/**
 * @brief Starts the system.
 *
 * This function transitions the system task to the "assert run" state.
 */
void app_system_start_running(void)
{
    LOG_LEVEL("app_system_start_running\r\n");
    OTMS(TASK_ID_SYSTEM, OTMS_S_ASSERT_RUN);
}

/**
 * @brief Asserts the system is running.
 *
 * This function starts timers, requests the system to start running,
 * and transitions the system task to the running state.
 */
void app_system_assert_running(void)
{
    StartTickCounter(&l_t_msg_wait_10_timer);

#ifdef TASK_MANAGER_STATE_MACHINE_MCU
    ptl_reqest_running(MCU_TO_SOC_MOD_SYSTEM);
#elif defined(TASK_MANAGER_STATE_MACHINE_SOC)
    ptl_reqest_running(SOC_TO_MCU_MOD_SYSTEM);
#endif

    OTMS(TASK_ID_SYSTEM, OTMS_S_RUNNING);
}

/**
 * @brief Main system running state handler.
 *
 * Processes system messages and handles events like power on/off and device events.
 */
void app_system_running(void)
{
    if (GetTickCounter(&l_t_msg_wait_10_timer) < 10)
        return;
    StartTickCounter(&l_t_msg_wait_10_timer);

    Msg_t *msg = get_message(TASK_ID_SYSTEM);
    if (msg->id == NO_MSG)
        return;

    switch (msg->id)
    {
    case MSG_DEVICE_NORMAL_EVENT:
        break;

    case MSG_DEVICE_ACC_EVENT:
				LOG_LEVEL("Event: MSG_DEVICE_ACC_EVENT\r\n");
        app_power_on_off(msg->param2);
        break;

    case MSG_DEVICE_POWER_EVENT:
				LOG_LEVEL("Event MSG_DEVICE_POWER_EVENT\r\n");
        if (msg->param1 == CMD_MODSYSTEM_POWER_ON)
            app_power_on_off(SYSTEM_POWER_ON_VALUE);
        else if (msg->param1 == CMD_MODSYSTEM_POWER_OFF)
            app_power_on_off(SYSTEM_POWER_OFF_VALUE);
        break;
    }
}

void app_system_post_running(void)
{
}

void app_system_stop_running(void)
{
    OTMS(TASK_ID_SYSTEM, OTMS_S_INVALID);
}

/**
 * @brief Handles the system power on/off state.
 *
 * Logs the power state and updates GPIO pins accordingly.
 *
 * @param onoff Boolean indicating power state (true for on, false for off).
 */
void app_power_on_off(bool onoff)
{
}
/*******************************************************************************
 * FUNCTION: system_send_handler
 *
 * DESCRIPTION:
 * Handles the sending of system-related commands based on the frame type and parameters.
 * This function processes commands for system, setup, and app modules and sends appropriate responses.
 *
 * PARAMETERS:
 * - frame_type: Type of the frame (MCU_TO_SOC_MOD_SYSTEM, SOC_TO_MCU_MOD_SYSTEM, MCU_TO_SOC_MOD_SETUP).
 * - param1: Command identifier.
 * - param2: Additional parameter for the command.
 * - buff: Pointer to the buffer where the frame should be built.
 *
 * RETURNS:
 * - true if the command was processed successfully, false otherwise.
 ******************************************************************************/
bool system_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    MY_ASSERT(buff);         // Ensure the buffer is valid
    uint8_t tmp[8] = {0}; // Temporary buffer for command parameters

    // Handle commands for MCU_TO_SOC_MOD_SYSTEM frame type
    if (MCU_TO_SOC_MOD_SYSTEM == frame_type)
    {
        switch (param1)
        {
        case CMD_MODSYSTEM_HANDSHAKE:
            tmp[0] = 0;
            tmp[1] = 0;
            LOG_LEVEL("system handshake frame_type=%02x param1=%02x param2=%02x\n",frame_type, tmp[0], tmp[1]);
            ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, tmp, 2, buff);
            return true;

        case CMD_MODSYSTEM_ACC_STATE:
            // Acknowledgement, no additional action needed
            ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_ACC_STATE, tmp, 1, buff);
            return true;

        case CMD_MODSYSTEM_POWER_OFF:
            // Acknowledgement, no additional action needed
            tmp[0] = 0;
            tmp[1] = 0;
            ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_POWER_OFF, tmp, 2, buff);
            return true;

        case CMD_MODSETUP_UPDATE_TIME:
            // Update time command, sending date and time values
            tmp[0] = 23; // year
            tmp[1] = 3;  // month
            tmp[2] = 25; // day
            tmp[3] = 8;  // hour
            tmp[4] = 55; // minute
            tmp[5] = 0;  // second
            ptl_build_frame(MCU_TO_SOC_MOD_SETUP, CMD_MODSETUP_UPDATE_TIME, tmp, 6, buff);
            return true;

        case CMD_MODSETUP_KEY:
            tmp[0] = MSB_WORD(param2); // Key code (most significant byte)
            tmp[1] = LSB_WORD(param2); // Key state (least significant byte)
            tmp[2] = 0;                // Reserved byte
            LOG_LEVEL("CMD_MODSETUP_KEY  key %02x state %02x\n", tmp[0], tmp[1]);
            ptl_build_frame(MCU_TO_SOC_MOD_SETUP, CMD_MODSETUP_KEY, tmp, 3, buff);
            return true;

        default:
            break;
        }
    }
    // Handle commands for SOC_TO_MCU_MOD_SYSTEM frame type
    else if (SOC_TO_MCU_MOD_SYSTEM == frame_type)
    {
        switch (param1)
        {
        case CMD_MODSYSTEM_HANDSHAKE:
            tmp[0] = 0;
            tmp[1] = 0;
            LOG_LEVEL("system handshake param1=%02x param2=%02x\n", tmp[0], tmp[1]);
            ptl_build_frame(SOC_TO_MCU_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, tmp, 2, buff);
            return true;

        case CMD_MODSYSTEM_APP_STATE:
            tmp[0] = l_u8_mpu_status; // Send MPU status
            tmp[1] = 0x01;            // Additional status byte
            ptl_build_frame(SOC_TO_MCU_MOD_SYSTEM, CMD_MODSYSTEM_APP_STATE, tmp, 2, buff);
            return true;

        default:
            break;
        }
    }
    // Handle commands for MCU_TO_SOC_MOD_SETUP frame type
    else if (MCU_TO_SOC_MOD_SETUP == frame_type)
    {
        switch (param1)
        {
        case CMD_MODSETUP_SET_TIME:
            return false; // No action required for this command

        default:
            break;
        }
    }
    return false; // Command not processed
}

/*******************************************************************************
 * FUNCTION: system_receive_handler
 *
 * DESCRIPTION:
 * Handles the reception of system-related commands based on the payload and sends appropriate responses.
 *
 * PARAMETERS:
 * - payload: Pointer to the received payload data.
 * - ackbuff: Pointer to the buffer where the acknowledgment frame will be built.
 *
 * RETURNS:
 * - true if the command was processed successfully, false otherwise.
 ******************************************************************************/
bool system_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    MY_ASSERT(payload); // Ensure payload is valid
    MY_ASSERT(ackbuff); // Ensure acknowledgment buffer is valid
    uint8_t tmp;     // Temporary variable for holding command data

    // Handle received commands for MCU_TO_SOC_MOD_SYSTEM frame type
    if (MCU_TO_SOC_MOD_SYSTEM == payload->frame_type)
    {
        switch (payload->cmd)
        {
        case CMD_MODSYSTEM_HANDSHAKE:
            LOG_LEVEL("system got handshake from xxx payload->frame_type=%02x\r\n",payload->frame_type);
            //ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, (uint8_t *)VER_STR, sizeof(VER_STR), ackbuff);
            return false;

        case CMD_MODSYSTEM_ACC_STATE:
            LOG_LEVEL("CMD_MODSYSTEM_ACC_STATE\r\n");
            //ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, (uint8_t *)VER_STR, sizeof(VER_STR), ackbuff);
            return false;

        case CMD_MODSYSTEM_APP_STATE:
            LOG_LEVEL("CMD_MODSYSTEM_APP_STATE l_u8_mpu_status = %d\r\n", payload->data[0]);
            tmp = 0x01;
            ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_APP_STATE, &tmp, 1, ackbuff);
            return true;

        case CMD_MODSYSTEM_POWER_OFF:
            return false; // Acknowledgment, no action required

        case CMD_MODSETUP_UPDATE_TIME:
            tmp = 0x01;
            ptl_build_frame(SOC_TO_MCU_MOD_SETUP, CMD_MODSETUP_UPDATE_TIME, &tmp, 1, ackbuff);
            return false;

        case CMD_MODSETUP_SET_TIME:
            tmp = 0x01;
            ptl_build_frame(MCU_TO_SOC_MOD_SETUP, CMD_MODSETUP_SET_TIME, &tmp, 1, ackbuff);
            return true;

        case CMD_MODSETUP_KEY:
            tmp = 0x01;
            ptl_build_frame(SOC_TO_MCU_MOD_SETUP, CMD_MODSETUP_KEY, &tmp, 1, ackbuff);
            return false;

        default:
            break;
        }
    }
		
	  if (SOC_TO_MCU_MOD_SYSTEM == payload->frame_type)
		 {
			 switch (payload->cmd)
        {
        case CMD_MODSYSTEM_HANDSHAKE:
            LOG_LEVEL("system got handshake from xxx payload->frame_type=%02x\r\n",payload->frame_type);
            //ptl_build_frame(MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, (uint8_t *)VER_STR, sizeof(VER_STR), ackbuff);
            return false; 
				 default:
            break;
				}
		 }

    return false; // Command not processed
}

/*******************************************************************************
 * FUNCTION: system_handshake_with_mcu
 *
 * DESCRIPTION:
 * Initiates a handshake with the MCU by sending the handshake command.
 ******************************************************************************/
void system_handshake_with_mcu(void)
{
    LOG_LEVEL("system send handshake data to xxx\r\n");
    send_message(TASK_ID_PTL_1, SOC_TO_MCU_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, 0);
}

/*******************************************************************************
 * FUNCTION: system_handshake_with_app
 *
 * DESCRIPTION:
 * Initiates a handshake with the app by sending the handshake command.
 ******************************************************************************/
void system_handshake_with_app(void)
{
    LOG_LEVEL("system send handshake data to xxx\r\n");
    send_message(TASK_ID_PTL_1, MCU_TO_SOC_MOD_SYSTEM, CMD_MODSYSTEM_HANDSHAKE, 0);
}

/*******************************************************************************
 * FUNCTION: system_set_mpu_status
 *
 * DESCRIPTION:
 * Sets the MPU status and sends an application state update message.
 *
 * PARAMETERS:
 * - status: The status value to set for the MPU.
 ******************************************************************************/
void system_set_mpu_status(uint8_t status)
{
    LOG_LEVEL("system set mpu status=%d \r\n", status);
    l_u8_mpu_status = status;
    send_message(TASK_ID_SYSTEM, CMD_MODSYSTEM_APP_STATE, l_u8_mpu_status, l_u8_mpu_status);
}

/*******************************************************************************
 * FUNCTION: system_get_mpu_status
 *
 * DESCRIPTION:
 * Retrieves the current MPU status.
 *
 * RETURNS:
 * - The current MPU status value.
 ******************************************************************************/
uint8_t system_get_mpu_status(void)
{
    return l_u8_mpu_status;
}

/*******************************************************************************
 * FUNCTION: system_get_power_off_req
 *
 * DESCRIPTION:
 * Checks if a power off request has been made.
 *
 * RETURNS:
 * - true if power off is requested, false otherwise.
 ******************************************************************************/
bool system_get_power_off_req(void)
{
    return l_u8_power_off_req;
}

/*******************************************************************************
 * FUNCTION: system_get_mb_state
 *
 * DESCRIPTION:
 * Retrieves the current ModBus state.
 *
 * RETURNS:
 * - The current ModBus state value.
 ******************************************************************************/
mb_state_t system_get_mb_state(void)
{
    return lt_mb_state;
}
