/*******************************************************************************
 * @file     octopus_ipc_socket.c
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

#include "octopus_ipc_socket.h"
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
static bool ipc_socket_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
static bool ipc_socket_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);
void notify_message_to_ipc_socket(int cmd_parameter);
/*******************************************************************************
 * Global Variables
 * Define variables accessible across multiple files if needed.
 ******************************************************************************/
static CarInforCallback_t CarInforCallback = NULL;

/*******************************************************************************
 * Local Variables
 * Define static variables used only within this file.
 ******************************************************************************/
static mb_state_t lt_mb_state;         // Current state of the message buffer
static uint8_t l_u8_mpu_status = 0;    // Tracks the status of the MPU
static uint8_t l_u8_power_off_req = 0; // Tracks if a power-off request is pending
static uint32_t l_t_msg_wait_10_timer; // Timer for 10 ms message waiting period
static uint32_t l_t_msg_wait_500_timer;
/*******************************************************************************
 * Global Function Implementations
 ******************************************************************************/
void register_car_infor_callback(CarInforCallback_t callback)
{
    CarInforCallback = callback;
}
/**
 * @brief Initializes the system for running.
 *
 * This function registers the system module with the communication layer
 * and transitions the system task to an invalid state.
 */
void app_ipc_socket_init_running(void)
{
    LOG_LEVEL("app_ipc_socket_init_running\r\n");
    OTMS(TASK_ID_IPC_SOCKET, OTMS_S_INVALID);
    ptl_register_module(A2M_MOD_IPC, ipc_socket_send_handler, ipc_socket_receive_handler);
}

/**
 * @brief Starts the system.
 *
 * This function transitions the system task to the "assert run" state.
 */
void app_ipc_socket_start_running(void)
{
    LOG_LEVEL("app_ipc_socket_start_running\r\n");
    OTMS(TASK_ID_IPC_SOCKET, OTMS_S_ASSERT_RUN);
}

/**
 * @brief Asserts the system is running.
 *
 * This function starts timers, requests the system to start running,
 * and transitions the system task to the running state.
 */
void app_ipc_socket_assert_running(void)
{
    StartTickCounter(&l_t_msg_wait_10_timer);
    StartTickCounter(&l_t_msg_wait_500_timer);
    ptl_reqest_running(A2M_MOD_IPC);
    OTMS(TASK_ID_IPC_SOCKET, OTMS_S_RUNNING);
}

/**
 * @brief Main system running state handler.
 *
 * Processes system messages and handles events like power on/off and device events.
 */
void app_ipc_socket_running(void)
{
    if (GetTickCounter(&l_t_msg_wait_10_timer) < 10)
        return;
    StartTickCounter(&l_t_msg_wait_10_timer);

    Msg_t *msg = get_message(TASK_ID_IPC_SOCKET);
    if (msg->id == NO_MSG)
    {
        // if (GetTickCounter(&l_t_msg_wait_500_timer) >= 50)
        //if (GetTickCounter(&l_t_msg_wait_500_timer) >= 1000*30)
        if (GetTickCounter(&l_t_msg_wait_500_timer) >= 2000)
        {
            notify_message_to_ipc_socket(CMD_GET_INDICATOR_INFO);
            // notify_message_to_ipc_socket(CMD_GET_METER_INFO);
            // notify_message_to_ipc_socket(CMD_GET_DRIVINFO_INFO);
            StartTickCounter(&l_t_msg_wait_500_timer);
        }
        return;
    }

    switch (msg->id)
    {
    case MSG_DEVICE_CAN_EVENT:
        notify_message_to_ipc_socket(msg->param1);
        break;
    }
}

void app_ipc_socket_post_running(void)
{
}

void app_ipc_socket_stop_running(void)
{
    OTMS(TASK_ID_IPC_SOCKET, OTMS_S_INVALID);
}

void notify_message_to_ipc_socket(int cmd_parameter)
{
    if (CarInforCallback)
    {
        CarInforCallback(cmd_parameter);
    }
}
/*******************************************************************************
 * FUNCTION: ipc_socket_send_handler
 *
 * DESCRIPTION:
 * Handles the sending of system-related commands based on the frame type and parameters.
 * This function processes commands for system, setup, and app modules and sends appropriate responses.
 *
 * PARAMETERS:
 * - frame_type: Type of the frame (M2A_MOD_SYSTEM, A2M_MOD_SYSTEM, M2A_MOD_SETUP).
 * - param1: Command identifier.
 * - param2: Additional parameter for the command.
 * - buff: Pointer to the buffer where the frame should be built.
 *
 * RETURNS:
 * - true if the command was processed successfully, false otherwise.
 ******************************************************************************/
bool ipc_socket_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    assert(buff);         // Ensure the buffer is valid
    uint8_t tmp[8] = {0}; // Temporary buffer for command parameters

    // Handle commands for A2M_MOD_SYSTEM frame type
    if (A2M_MOD_IPC == frame_type)
    {
        switch (param1)
        {
        case CMD_MODSYSTEM_APP_STATE:
            tmp[0] = l_u8_mpu_status; // Send MPU status
            tmp[1] = 0x01;            // Additional status byte
            ptl_build_frame(A2M_MOD_SYSTEM, CMD_MODSYSTEM_APP_STATE, tmp, 2, buff);
            return true;
        default:
            break;
        }
    }
    return false; // Command not processed
}

/*******************************************************************************
 * FUNCTION: ipc_socket_receive_handler
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
bool ipc_socket_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    assert(payload); // Ensure payload is valid
    assert(ackbuff); // Ensure acknowledgment buffer is valid
    uint8_t tmp;     // Temporary variable for holding command data

    // Handle received commands for M2A_MOD_SYSTEM frame type
    // if(M2A_MOD_IPC == payload->frame_type)
    {
    }

    return false; // Command not processed
}

//__attribute__((visibility("default")))
int ipc_socket_doCommand(uint8_t *data, uint8_t length)
{
    LOG_LEVEL("ipc_socket_doCommand called with length: %d\n", length);
    for (int i = 0; i < length; i++)
    {
        LOG_LEVEL("data[%d] = %d\n", i, data[i]);
    }
    return 0;
}
