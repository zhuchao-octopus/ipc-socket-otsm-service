/*******************************************************************************
 * @file    octopus_mcu_update.c
 * @brief   MCU update management and reboot state machine implementation.
 *
 * This file contains the implementation for managing MCU firmware updates,
 * handling reboot sequences, and interacting with various hardware modules.
 *
 * @details
 * - Provides initialization and control functions for the MCU update task.
 * - Implements a state machine for managing reboot sequences.
 * - Handles communication with other system modules for update and reboot processes.
 * - Offers support for flash memory operations during firmware updates.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *
 * @note    Ensure all dependent modules are correctly initialized before using
 *          the functions in this file.
 */

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"         // Include platform-specific hardware details
#include "octopus_log.h"              // Include logging module for debugging purposes
#include "octopus_task_manager.h"     // Include task manager for scheduling tasks
#include "octopus_update_mcu.h"       // Include header for MCU update management
#include "octopus_tickcounter.h"      // Include tick counter for timing operations
#include "octopus_msgqueue.h"         // Include message queue for communication
#include "octopus_flash.h"            // Include flash memory handling utilities

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */

#ifdef TASK_MANAGER_STATE_MACHINE_UPDATE // Ensure this part is included only if the macro is defined

/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */
// Define a buffer structure used for flash programming during firmware updates
typedef struct {
    uint32_t addr;      								// Address in flash memory to write data
    uint8_t  buff[48];  								// Data buffer for temporary storage
    uint8_t  count;     								// Number of valid bytes in the buffer
} program_buf_t;

// Define enumeration for MCU reboot state machine states
typedef enum {
    MCU_REBOOT_ST_IDLE       = (0x00),  // Idle state
    MCU_REBOOT_ST_INIT       = (0x01),  // Initialization state
    MCU_REBOOT_ST_SOC_OFF    = (0x02),  // State where the SOC is turned off
    MCU_REBOOT_ST_SOC_WAIT   = (0x03),  // Waiting for SOC to be off
    MCU_REBOOT_ST_SOC_ON     = (0x04),  // State where the SOC is turned on
    MCU_REBOOT_ST_MCU_RESET  = (0x05),  // State to reset the MCU
} mcu_reboot_state_t;
/*******************************************************************************
 * CONSTANTS
 */

// Define constants for reboot tags
#define MCU_REBOOT_TAG_IDLE  	 (0xFF)  // Idle tag, indicating no reboot required
#define MCU_REBOOT_TAG_MCU   	 (0x00)  // Tag for rebooting only the MCU
#define MCU_REBOOT_TAG_MAC   	 (0x01)  // Tag for rebooting the entire machine
#define MCU_REBOOT_TAG_SOC   	 (0x02)  // Tag for rebooting the SOC

/*******************************************************************************
 * GLOBAL VARIABLES
 */


/*******************************************************************************
 * STATIC VARIABLES
 */
// Declare static variables for managing reboot process
//static uint8_t l_u8_acc_status = 0;

static uint8_t l_u8_reboot = MCU_REBOOT_TAG_IDLE;          				// Current reboot tag
static mcu_reboot_state_t l_t_reboot_status = MCU_REBOOT_ST_IDLE; // Current reboot state
static uint32_t l_tmr_reboot = 0;                         				// Timer for reboot delay

/*******************************************************************************
 * EXTERNAL VARIABLES
 */


/*******************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
// Function declarations
static bool update_module_send_handler(ptl_frame_type_t module, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff); // Handle outgoing messages
static bool update_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff); // Handle incoming messages
static void mcu_reboot_state_proc(void); // Process reboot state machine

/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */

/**
 * @brief Initialize the MCU update module and register communication handlers.
 */
void app_update_mcu_init_running(void) {
    ptl_register_module(MCU_TO_SOC_MOD_UPDATE, update_module_send_handler, update_module_receive_handler); // Register handlers for module communication
    OTMS(TASK_ID_UPDATE_MCU, OTMS_S_INVALID); // Set the task state to invalid (not running)
}

/**
 * @brief Start the MCU update task by setting it to the assert run state.
 */
void app_update_mcu_start_running(void) {
    OTMS(TASK_ID_UPDATE_MCU, OTMS_S_ASSERT_RUN); // Assert task state to running
}

/**
 * @brief Assert the MCU update task to a running state, ensuring it is active.
 */
void app_update_mcu_assert_running(void) {
    ptl_reqest_running(MCU_TO_SOC_MOD_UPDATE); // Request module to be active
    OTMS(TASK_ID_UPDATE_MCU, OTMS_S_RUNNING); // Set task state to running
}

/**
 * @brief Main task function for managing the MCU update process.
 */
void app_update_mcu_running(void) {
    mcu_reboot_state_proc(); // Call the reboot state machine processing function
}

/**
 * @brief Post-task actions after the MCU update task completes.
 */
void app_update_mcu_post_running(void) {
    ptl_release_running(MCU_TO_SOC_MOD_UPDATE); // Release the active module
    OTMS(TASK_ID_UPDATE_MCU, OTMS_S_ASSERT_RUN); // Reset task state to assert run
}

/**
 * @brief Stop the MCU update task by invalidating its state.
 */
void app_update_mcu_stop_running(void) {
    OTMS(TASK_ID_UPDATE_MCU, OTMS_S_INVALID); // Set the task state to invalid
} 

/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
// Function to handle sending update module requests based on the module type and parameters
bool update_module_send_handler(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    // DEV_ASSERT(buff);  // Uncommented: this would assert that the 'buff' pointer is not NULL.
    
    uint8_t tmp[8] = {0};  // Initialize a temporary buffer 'tmp' of 8 bytes, all set to 0.
    
    // The following commented print statement would log the function execution for debugging purposes:
    // PRINT("update_module_send_handler  MOD %02x  CMD %02x PRARM %04x\n", module, cmd, param);  
    // Check if the module type is MCU_TO_SOC_MOD_UPDATE (which indicates an update operation).
    if (MCU_TO_SOC_MOD_UPDATE == frame_type)
    {
        // Handle the specific update request based on 'param1'.
        switch (param1)
        {
            case CMD_MODUPDATE_UPDATE_FW_STATE:  // Case for firmware state update
            {
                tmp[0] = 0x00;  // Set the first byte of 'tmp' to 0x00, indicating that the MCU is running normally.
                // Build the frame with the update information (MCU_TO_SOC_MOD_UPDATE, firmware state update command, and the state data).
                ptl_build_frame(MCU_TO_SOC_MOD_UPDATE, CMD_MODUPDATE_UPDATE_FW_STATE, tmp, 1, buff);
                return true;    // Successfully processed the update request.
            }
            default:
                break;  // No action for unrecognized 'param1' values.
        }
    }
    // Return false if the module is not for updates or no matching command is found.
    return false;
}

// Function to handle receiving update module requests, process the payload, and send acknowledgment.
bool update_module_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
    // DEV_ASSERT(payload);  // Uncommented: This would assert that the 'payload' pointer is not NULL.
    // DEV_ASSERT(ackbuff);  // Uncommented: This would assert that the 'ackbuff' pointer is not NULL.
    
    uint8_t tmp = 0;  // Temporary variable to hold status or response data.
    
    // Check if the frame type of the received payload is SOC_TO_MCU_MOD_UPDATE (indicating it's an update operation).
    if (SOC_TO_MCU_MOD_UPDATE == payload->frame_type)
    {
        // The following commented print statement would log the frame details for debugging:
        // LOG_LEVEL("MOD %02x CMD %02x pm1 %02x pm2 %02x\r\n", payload->module, payload->cmd, payload->data[0], payload->data[1]);

        // Switch based on the command in the received payload.
        switch (payload->cmd)
        {
            case CMD_MODUPDATE_CHECK_FW_STATE:   // Command for checking the firmware state.
            {
                LOG_LEVEL("CMD_MODUPDATE_CHECK_FW_STATE\r\n");
                tmp = 0x00; // Set tmp to 0x00 to indicate that the MCU is running normally.
                // Build a frame with the response (firmware state check) and send it back via 'ackbuff'.
                ptl_build_frame(MCU_TO_SOC_MOD_UPDATE, CMD_MODUPDATE_CHECK_FW_STATE, &tmp, 1, ackbuff);
                return true;  // Successfully handled the request.
            }
            case CMD_MODUPDATE_UPDATE_FW_STATE:  // Command for updating the firmware state.
            {
                LOG_LEVEL("CMD_MODUPDATE_UPDATE_FW_STATE \r\n");
                // This command acknowledges the request but does nothing as per the current implementation.
                return false;  // No action required.
            }
            case CMD_MODUPDATE_ENTER_FW_UPDATE:  // Command to enter firmware update mode.
            {
                LOG_LEVEL("CMD_MODUPDATE_ENTER_FW_UPDATE \r\n");
                // Indicate that the system should reboot into update mode.
                l_u8_reboot = MCU_REBOOT_TAG_MCU;
                l_t_reboot_status = MCU_REBOOT_ST_INIT;
                StartTickCounter(&l_tmr_reboot);  // Start the reboot timer.

                tmp = 0x01;  // Indicate success (entering firmware update mode).
                // Build a response frame and send it back via 'ackbuff'.
                ptl_build_frame(MCU_TO_SOC_MOD_UPDATE, CMD_MODUPDATE_ENTER_FW_UPDATE, &tmp, 1, ackbuff);
                return true;  // Successfully entered firmware update mode.
            }
            case CMD_MODUPDATE_EXIT_FW_UPDATE:   // Command to exit firmware update mode.
            {
                LOG_LEVEL("CMD_MODUPDATE_EXIT_FW_UPDATE\r\n");
                tmp = 0x00; // Indicate successful exit from update mode.
                // Build a response frame and send it back via 'ackbuff'.
                ptl_build_frame(MCU_TO_SOC_MOD_UPDATE, CMD_MODUPDATE_EXIT_FW_UPDATE, &tmp, 1, ackbuff);
                return true;  // Successfully exited update mode.
            }
            case CMD_MODUPDATE_REBOOT:           // Command for rebooting the system.
            {
                LOG_LEVEL("CMD_MODUPDATE_REBOOT\r\n");

                // Reboot options are commented out, but indicate how the system should handle different reboot scenarios.
                #if 0
                0x00: Reboot only the main controller.
                0x01: Reboot the entire system by cutting power.
                0x02: Reboot only the core board by cutting power.
                #endif

                l_u8_reboot = payload->data[0]; // Set reboot state based on payload data.
                l_t_reboot_status = MCU_REBOOT_ST_INIT;  // Set reboot status to initialization.
                StartTickCounter(&l_tmr_reboot);  // Start the reboot timer.

                tmp = 0x01; // Acknowledge the success of the reboot request.
                // Build a response frame and send it back via 'ackbuff'.
                ptl_build_frame(MCU_TO_SOC_MOD_UPDATE, CMD_MODUPDATE_REBOOT, &tmp, 1, ackbuff);
                return true;  // Successfully processed reboot request.
            }
            default:
                break;  // No action for unrecognized commands.
        }
    }
    
    return false;  // Return false if the frame type doesn't match or no matching command is found.
}

// Function to process the MCU reboot state machine. It handles different stages of the reboot process.
static void mcu_reboot_state_proc(void)
{
    // Switch based on the current reboot status.
    switch (l_t_reboot_status) {
    
        // State when the MCU is idle, no reboot actions are needed.
        case MCU_REBOOT_ST_IDLE:
            break;  // No actions are performed in the idle state.

        // Initial state when the reboot is first triggered.
        case MCU_REBOOT_ST_INIT:
            // If the reboot timer has elapsed more than 200ms, transition to the next state.
            if (GetTickCounter(&l_tmr_reboot) > 200)
            {
                // Check if the reboot target is MCU, if so, transition to MCU reset state.
                if (MCU_REBOOT_TAG_MCU == l_u8_reboot)
                {
                    l_t_reboot_status = MCU_REBOOT_ST_MCU_RESET;
                }
                // If the reboot target is the entire system (SOC off), transition to SOC power-off state.
                else
                {
                    l_t_reboot_status = MCU_REBOOT_ST_SOC_OFF;
                }
            }
            break;

        // State to power off the System-on-Chip (SOC).
        case MCU_REBOOT_ST_SOC_OFF:
            // TODO: Add code to power off the SOC.
            // Start the timer again to track the delay for SOC power-off.
            StartTickCounter(&l_tmr_reboot);
            l_t_reboot_status = MCU_REBOOT_ST_SOC_WAIT;  // Move to SOC wait state.
            break;

        // State where the system waits after SOC power off.
        case MCU_REBOOT_ST_SOC_WAIT:
            // If the timer exceeds 200ms, decide whether to reset the MCU or power on the SOC.
            if (GetTickCounter(&l_tmr_reboot) > 200)
            {
                // If the reboot target is MAC (system), transition to MCU reset state.
                if (MCU_REBOOT_TAG_MAC == l_u8_reboot)
                {
                    l_t_reboot_status = MCU_REBOOT_ST_MCU_RESET;
                }
                // Otherwise, power on the SOC and transition to the SOC on state.
                else
                {
                    l_t_reboot_status = MCU_REBOOT_ST_SOC_ON;
                }
            }
            break;

        // State to power on the SOC.
        case MCU_REBOOT_ST_SOC_ON:
            // TODO: Add code to power on the SOC.
            l_t_reboot_status = MCU_REBOOT_ST_IDLE;  // After powering on, return to idle state.
            break;

        // State to reset the MCU.
        case MCU_REBOOT_ST_MCU_RESET:
            // LOG_LEVEL("MCU_REBOOT_ST_MCU_RESET\r\n");
            // IAP_RebootToBootloader(); // Reboot to bootloader for firmware update, etc.
            break;
    }
}

#endif
