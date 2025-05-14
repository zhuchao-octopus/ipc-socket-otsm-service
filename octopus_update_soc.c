/*******************************************************************************
 * @file    octopus_mcu_update.c
 * @brief   This file contains functions and handlers for the MCU firmware update process,
 *          including communication with the MCU through UART, checking the firmware file, 
 *          and handling the update process state machine.
 *          It also includes mechanisms to manage the upgrade cycle, error handling, 
 *          and interaction with the application layer.
 * 
 * @details This file is part of the Octopus platform, which is designed to handle MCU firmware
 *          updates over a communication interface. The update process is divided into several
 *          states, including initialization, file verification, confirmation, data transfer,
 *          and completion. The communication between the host and the MCU is done via UART.
 *          The firmware update file is in Intel Hex format, and data is transmitted in packets.
 * 
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 * 
 * @note    Ensure that the MCU is in a proper state before starting the update process.
 *          Timeouts and retries are implemented to handle communication issues.
 * 
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"  			// Include platform-specific header for hardware platform details
#include "octopus_log.h"       			// Include logging functions for debugging
#include "octopus_task_manager.h" 	// Include task manager for scheduling tasks

#include "octopus_update_soc.h" 
#include "octopus_tickcounter.h" 		// Include tick counter for timing operations
#include "octopus_msgqueue.h"  			// Include message queue for inter-process communication

#ifdef PLATFORM_ITE_OPEN_RTOS
#include "../update/intelhexupdate.h"
#endif
/*******************************************************************************
 * DEBUG SWITCH MACROS
 */
#ifdef TASK_MANAGER_STATE_MACHINE_UPDATE
/*******************************************************************************
 * MACROS
 */
#define MCU_UPDATE_DISK_PATH "E"  // Define the disk path for firmware update storage
#define MCU_UPDATE_FILE_PATH (MCU_UPDATE_DISK_PATH ":/KD070E01_MCU.hex")  // Define the full path to the firmware file for MCU update

#define MCU_UPDATE_CONFIRM_TIME          (30 * 1000)  // Set the time to wait for firmware update confirmation (30 seconds)
#define MCU_UPDATE_CHECK_FW_STATE_TIME   (1 * 1000)   // Set the time interval to check the firmware state (1 second)


/*******************************************************************************
 * TYPEDEFS
 */
typedef enum {
    MCU_FW_STATE_NORMAL        = 0x00,        // Define the normal operation state for the MCU (running as expected)
    MCU_FW_STATE_WAIT_BOOT     = 0x01,        // Define the state where the MCU is entering firmware update mode (waiting for boot)
    MCU_FW_STATE_WAIT_TRANSFER = 0x02,        // Define the state where the MCU is ready to receive data packets (waiting for transfer)
} ptl_mcu_fw_state_t;   // Define an enum to represent different firmware states of the MCU

typedef struct
{
    uint32_t addr;      // Define the address where data will be written in memory
    uint8_t  buff[48];  // Define a buffer to hold the data that will be written
    uint8_t count;      // Define the number of bytes of data in the buffer to be written
} program_buf_t;        // Define a struct for holding the program data buffer and the write address


/*******************************************************************************
 * CONSTANTS
 */

#define CFG_UPDATE_STACK_SIZE (200112L)  // Define the stack size for the firmware update process (200112 bytes)

/*******************************************************************************
 * LOCAL FUNCTIONS DECLARATION
 */
static bool app_update_send_handler(ptl_frame_type_t module, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);  // Declare function to handle sending update commands
static bool app_update_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);  // Declare function to handle receiving update commands
static void app_update_state_handler(void);  // Declare function to handle the MCU firmware update state

#ifdef PLATFORM_ITE_OPEN_RTOS
static void app_read_hex_file_callback(uint32_t addr, uint8_t* binbuff, uint8_t count);  // Declare callback function for reading data from a hex file
#ifdef TASK_MANAGER_STATE_MACHINE_UPDATE
static void* app_read_hex_file_task(void* arg);  // Declare task function for reading hex file data
#endif
#endif
/*******************************************************************************
 * GLOBAL VARIABLES
 */

/*******************************************************************************
 * STATIC VARIABLES
 */

static mcu_update_state_t lt_mcu_status = (mcu_update_state_t)0;  // Define static variable to hold the MCU update status

#ifdef PLATFORM_ITE_OPEN_RTOS
static ptl_mcu_fw_state_t mcu_fw_state = MCU_FW_STATE_NORMAL;  // Define static variable to hold the MCU firmware state (initialized to normal)
static bool l_b_flag_update_success = false;     // Flag to indicate if the MCU update was successful
static bool l_b_flag_enter_fw_update = false;    // Flag to indicate if the MCU is entering firmware update mode
static bool l_b_flag_transfer_next = false;      // Flag to indicate if the next data frame is ready for transfer
static bool l_b_flag_transfer_complete = false;  // Flag to indicate if the data transfer is complete
static bool l_b_flag_update_failed = false;      // Flag to indicate if the MCU update failed
static bool l_b_flag_transfer_retry = false;     // Flag to indicate if a transfer retry is required
static bool l_b_flag_update_reboot_req = false;  // Flag to indicate if a reboot request is made after the update
static uint32_t l_tmr_check_fw_state;           // Define a timer for checking the firmware state
static uint32_t l_i_mcu_file_res = 0;           // Variable to hold the result of the MCU firmware file processing
#endif

//static bool l_b_flag_confirm_start = false;      // Flag to indicate if the start of the update process is confirmed
static uint32_t l_u32_mcu_fw_total_line = 0;    // Variable to hold the total number of lines in the MCU firmware
static uint32_t l_u32_mcu_fw_curr_line = 0;     // Variable to hold the current line number being processed in the MCU firmware
static uint32_t l_tmr_confirm;                  // Define a timer for confirming the upgrade process
static program_buf_t lt_mcu_program_buf;        // Define a buffer to store data for the MCU program

/*******************************************************************************
 * EXTERNAL VARIABLES
 */
/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */

/**
 * Initializes the update process and registers the update module handlers.
 */
void app_update_soc_init_running(void)
{
    LOG_LEVEL("app_update_init\r\n");  // Log the initialization of the update process
    ptl_register_module(SOC_TO_MCU_MOD_UPDATE, app_update_send_handler, app_update_receive_handler);  // Register the send and receive handlers for the update module
    OTMS(TASK_ID_UPDATE_SOC, OTMS_S_INVALID);  // Set the update task status to invalid
}

/**
 * Starts the update process by asserting the update task as running.
 */
void app_update_soc_start_running(void)
{
    OTMS(TASK_ID_UPDATE_SOC, OTMS_S_ASSERT_RUN);  // Set the update task status to assert run (start the update process)
}

/**
 * Asserts the running state of the update task if the system's memory board is on.
 */
void app_update_soc_assert_running(void)
{
    if (MB_ST_ON <= system_get_mb_state())  // Check if the memory board state is ON
    {
        ptl_reqest_running(SOC_TO_MCU_MOD_UPDATE);  // Request the update module to start running
        OTMS(TASK_ID_UPDATE_SOC, OTMS_S_RUNNING);  // Set the update task status to running
    }
}

/**
 * Executes the MCU update state processing logic.
 */
void app_update_soc_running(void)
{
    app_update_state_handler();  // Process the current MCU update state
}

/**
 * Ends the update process, releases the running state, and asserts run if the memory board is not stopped.
 */
void app_update_soc_post_running(void)
{
    ptl_release_running(SOC_TO_MCU_MOD_UPDATE);  // Release the update module from running state
    if(MB_ST_STOP != system_get_mb_state())  // Check if the memory board state is not stopped
    {
        OTMS(TASK_ID_UPDATE_SOC, OTMS_S_ASSERT_RUN);  // Assert the update task status to running
    }
}

/**
 * Stops the update process and sets the task status to invalid.
 */
void app_update_soc_stop_running(void)
{
    LOG_LEVEL("app_update_stop_running\r\n");  // Log the stopping of the update process
    OTMS(TASK_ID_UPDATE_SOC, OTMS_S_INVALID);  // Set the update task status to invalid (stop the update process)
}

/**
 * Confirms the start of the update process.
 */
void app_update_confirm()
{
    LOG_LEVEL("update_set_confirm\r\n");  // Log the confirmation of the update start
    //l_b_flag_confirm_start = true;  // Set the flag indicating that the update confirmation has started
}

/**
 * Returns the remaining time for MCU update confirmation.
 */
uint32_t app_update_get_confirm_time(void)
{
    return MCU_UPDATE_CONFIRM_TIME - GetTickCounter(&l_tmr_confirm);  // Calculate the remaining confirmation time
}

/**
 * Returns the total number of lines in the MCU firmware file.
 */
uint32_t app_update_get_fw_total_line(void)
{
    return l_u32_mcu_fw_total_line;  // Return the total line count of the MCU firmware
}

/**
 * Returns the current line number of the MCU firmware being processed.
 */
uint32_t app_update_get_fw_curr_line(void)
{
    return l_u32_mcu_fw_curr_line;  // Return the current line number of the MCU firmware
}

/**
 * Returns the error code if any occurred during the update process.
 */
#ifdef TASK_MANAGER_STATE_MACHINE_SOC
uint32_t app_update_get_error_code(void)
{
    return l_i_mcu_file_res;  // Return the error code from the MCU firmware processing
}
#endif
/**
 * Returns the current status of the MCU update process.
 */
mcu_update_state_t app_update_get_status(void)
{
    return lt_mcu_status;  // Return the current MCU update status
}


/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/**
 * Handles the sending of update module frames based on the command received.
 * This function constructs the update frame and passes it to the `com_uart_build_frame` function for transmission.
 */
static bool app_update_send_handler(ptl_frame_type_t frame_type,uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff)
{
    MY_ASSERT(buff);  // Ensure the buffer pointer is valid

    uint8_t tmp[64] = {0};  // Temporary buffer to hold data to be sent
    LOG_LEVEL("update_module_send_handler MOD %02x CMD %02x\r\n", frame_type, param1);  // Log the module and command for debugging

    if(SOC_TO_MCU_MOD_UPDATE == frame_type)  // Check if the module is the update module
    {
        switch(param1)  // Handle the command based on the type
        {
        case CMD_MODUPDATE_CHECK_FW_STATE:  // Command to check the firmware state
            tmp[0] = 0x00;  // Set the first byte to 0 (used for the check firmware state command)
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_CHECK_FW_STATE, tmp, 1, buff);  // Build and send the frame
            return true;  // Indicate the frame was successfully sent
        case CMD_MODUPDATE_ENTER_FW_UPDATE:  // Command to enter firmware update mode
            tmp[0] = 0x00;  // Set the first byte to 0 (used for entering firmware update mode)
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_ENTER_FW_UPDATE, tmp, 1, buff);  // Build and send the frame
            return true;  // Indicate the frame was successfully sent
        case CMD_MODUPDATE_EXIT_FW_UPDATE:  // Command to exit firmware update mode
            tmp[0] = 0x00;  // Set the first byte to 0 (used for exiting firmware update mode)
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_EXIT_FW_UPDATE, tmp, 1, buff);  // Build and send the frame
            return true;  // Indicate the frame was successfully sent
        case CMD_MODUPDATE_SEND_FW_DATA:  // Command to send firmware data
        {
            uint32_t addr = lt_mcu_program_buf.addr;  // Get the address from the MCU program buffer
            uint16_t addr_mw = MK_MSBWORD(addr);  // Get the most significant word of the address
            uint16_t addr_lw = MK_LSBWORD(addr);  // Get the least significant word of the address
            tmp[0] = MK_MSB(addr_mw);  // Set the most significant byte of the address in the buffer
            tmp[1] = MK_LSB(addr_mw);  // Set the least significant byte of the most significant word of the address in the buffer
            tmp[2] = MK_MSB(addr_lw);  // Set the most significant byte of the least significant word of the address in the buffer
            tmp[3] = MK_LSB(addr_lw);  // Set the least significant byte of the least significant word of the address in the buffer
            tmp[4] = lt_mcu_program_buf.count;  // Set the number of bytes to be sent from the MCU program buffer

            // Copy the firmware data from the buffer into the temporary buffer
            for (int i = 0; i < lt_mcu_program_buf.count; i++)
            {
                tmp[5 + i] = lt_mcu_program_buf.buff[i];  // Copy each byte of the firmware data
            }

            // Build and send the frame with the firmware data
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_SEND_FW_DATA, tmp, (5 + lt_mcu_program_buf.count), buff);
            return true;  // Indicate the frame was successfully sent
        }
        case CMD_MODUPDATE_REBOOT:  // Command to request a reboot
        {
            tmp[0] = param2;  // Set the reboot parameter in the buffer
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_REBOOT, tmp, 1, buff);  // Build and send the reboot frame
            return true;  // Indicate the frame was successfully sent
        }
        default:  // Default case for unrecognized commands
            break;  // No action for unknown commands
        }
    }
    return false;  // Return false if the module is not the update module or the command is not recognized
}

/**
 * Handles the receiving of update module frames based on the payload received.
 * This function processes the response to the update commands and updates the flags or state accordingly.
 */
static bool app_update_receive_handler(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff)
{
	  #ifdef PLATFORM_ITE_OPEN_RTOS
    MY_ASSERT(payload);  // Ensure the payload pointer is valid
    MY_ASSERT(ackbuff);  // Ensure the acknowledgment buffer pointer is valid

    uint8_t tmp = 0;  // Temporary variable to hold data for acknowledgment

    if(MCU_TO_SOC_MOD_UPDATE == payload->frame_type)  // Check if the module is the update module (from module A to module M)
    {
        switch(payload->cmd)  // Handle the command based on the payload's command
        {
        case CMD_MODUPDATE_CHECK_FW_STATE:  // Command to check the firmware state
            mcu_fw_state = (ptl_mcu_fw_state_t)payload->data[0];  // Update the MCU firmware state from the payload data
            l_b_flag_enter_fw_update = (MCU_FW_STATE_WAIT_TRANSFER == mcu_fw_state);  // Set the flag to enter firmware update if the state indicates transfer wait
            // ACK, no further action required for this command
            return false;  // Return false since no acknowledgment frame is needed
        case CMD_MODUPDATE_UPDATE_FW_STATE:  // Command to update the firmware state
            mcu_fw_state = (ptl_mcu_fw_state_t)payload->data[0];  // Update the MCU firmware state from the payload data
            l_b_flag_enter_fw_update = (MCU_FW_STATE_WAIT_TRANSFER == mcu_fw_state);  // Set the flag to enter firmware update if the state indicates transfer wait
            // Send ACK with data 0x01 indicating the state update is successful
            tmp = 0x01;  // Set acknowledgment byte to 0x01
            ptl_build_frame(SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_UPDATE_FW_STATE, &tmp, 1, ackbuff);  // Build and send acknowledgment frame
            return true;  // Return true indicating acknowledgment was sent
        case CMD_MODUPDATE_ENTER_FW_UPDATE:  // Command to enter firmware update mode
            l_b_flag_enter_fw_update = (0x01 == payload->data[0]);  // Set the flag to enter firmware update mode if the data is 0x01
            // ACK, no further action required for this command
            return false;  // Return false since no acknowledgment frame is needed
        case CMD_MODUPDATE_EXIT_FW_UPDATE:  // Command to exit firmware update mode
            l_b_flag_update_success = (0x00 == payload->data[0]);  // Set the success flag if the data is 0x00 (indicating success)
            l_b_flag_update_failed = (0xFE == payload->data[0]);  // Set the failure flag if the data is 0xFE (indicating failure)
            // ACK, no further action required for this command
            return false;  // Return false since no acknowledgment frame is needed
        case CMD_MODUPDATE_SEND_FW_DATA:  // Command to send firmware data
            l_b_flag_transfer_next = (0xFF == payload->data[0]);  // Set the flag for transferring the next frame if the data is 0xFF
            l_b_flag_transfer_retry = (0xFE == payload->data[0]);  // Set the retry flag if the data is 0xFE (indicating retry)
            // ACK, no further action required for this command
            return false;  // Return false since no acknowledgment frame is needed
        default:  // Default case for unrecognized commands
            break;  // No action for unknown commands
        }
    }
		#endif
    return false;  // Return false if the module is not the update module or the command is not recognized
}

/**
 * Callback function to handle the reading of a hex file's data.
 * This function processes the address and data, prints it, and manages the transfer flow.
 */
#ifdef PLATFORM_ITE_OPEN_RTOS
static void app_read_hex_file_callback(uint32_t addr, uint8_t* binbuff, uint8_t count)
{
    // Print the address, count of bytes, and the data read from the hex file
    LOG_LEVEL("read addr:0X%08X count:%d data:", addr, count);
    for (int i = 0; i < count; i++)  // Iterate through the data buffer
    {
        LOG_("%02X ", binbuff[i]);  // Print each byte of the data in hexadecimal
    }
    LOG_("\r\n");

    DELAY_US(1 * 1000);  // Sleep for 1 millisecond to avoid overloading the system

    if (l_b_flag_transfer_next) {  // Check if the flag for transferring the next frame is set
        l_u32_mcu_fw_curr_line++;  // Increment the current line number of the MCU firmware

        lt_mcu_program_buf.addr = addr;  // Set the address in the program buffer
        lt_mcu_program_buf.count = count;  // Set the data count in the program buffer
        for (int i = 0; i < lt_mcu_program_buf.count; i++)  // Copy the data from binbuff to the program buffer
        {
            lt_mcu_program_buf.buff[i] = binbuff[i];
        }

        l_b_flag_transfer_next = false;  // Reset the flag indicating to transfer the next frame
        send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_SEND_FW_DATA, 0);  // Send a message to request the next firmware data transfer

        while (!l_b_flag_transfer_next && !l_b_flag_transfer_retry)  // Wait until either transfer next or retry flag is set
        {
            usleep(1 * 1000);  // Sleep for 1 millisecond
        }
    }
    else if (l_b_flag_transfer_retry) {  // If retry is needed
        l_b_flag_transfer_retry = false;  // Reset the retry flag
        send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_SEND_FW_DATA, 0);  // Send a message to retry the firmware data transfer

        while (!l_b_flag_transfer_next && !l_b_flag_transfer_retry)  // Wait until either transfer next or retry flag is set
        {
            usleep(10 * 1000);  // Sleep for 10 milliseconds for retry
        }
    }
}

/**
 * Task to read the hex file and initiate the transfer process.
 * This function reads the MCU update file and triggers the callback for each chunk of data.
 */
#ifdef TASK_MANAGER_STATE_MACHINE_UPDATE
static void* app_read_hex_file_task(void* arg)
{
    l_b_flag_transfer_complete = false;  // Reset the transfer complete flag before starting
    l_i_mcu_file_res = readIntelHexFile(MCU_UPDATE_FILE_PATH, app_read_hex_file_callback);  // Read the hex file and call the callback function for each chunk
    l_b_flag_transfer_complete = true;  // Set the transfer complete flag once the file is read
    return NULL;  // Return null as the task is completed
}
#endif
#endif
/**
 * This function handles the MCU firmware update process through multiple states.
 * It transitions between states depending on the result of checks, timers, and the current update status.
 */
static void app_update_state_handler(void)
{
	 #ifdef TASK_MANAGER_STATE_MACHINE_SOC
    switch (lt_mcu_status)  // Switch statement based on the current MCU update status
    {
    case MCU_UPDATE_ST_INIT:  // Initial state, setting up flags and timers for the update
        LOG_LEVEL("MCU_UPDATE_ST_INIT \r\n");  // Log the state
        //l_b_flag_confirm_start = false;   // Flag indicating confirmation start
        l_b_flag_enter_fw_update = false;  // Flag indicating whether to enter firmware update
        l_b_flag_transfer_next = false;    // Flag for the next transfer
        l_b_flag_transfer_retry = false;   // Flag for retrying transfer
        l_b_flag_transfer_complete = false;  // Flag indicating transfer completion
        l_b_flag_update_success = false;   // Flag indicating successful update
        l_b_flag_update_failed = false;    // Flag indicating failed update

        l_u32_mcu_fw_total_line = 0;   // Initialize total firmware lines to 0
        l_u32_mcu_fw_curr_line = 0;    // Initialize current firmware line to 0

        // Transition to the next state: checking the firmware file
        lt_mcu_status = MCU_UPDATE_ST_CHECK_FILE;

        // Start timer for checking firmware state
        StartTickCounter(&l_tmr_check_fw_state);
        break;

    case MCU_UPDATE_ST_CHECK_FILE:  // Check if the firmware update file is valid
        if (checkPatchValid(MCU_UPDATE_FILE_PATH))  // Check if the patch file is valid
        {
            LOG_LEVEL("MCU_UPDATE_ST_CHECK_FILE checkPatchValid \r\n");
            int res = checkIntelHexFile(MCU_UPDATE_FILE_PATH);  // Validate the Intel Hex file format
            if (res > 0)
            {
                LOG_LEVEL("MCU_UPDATE_ST_CHECK_FILE checkIntelHexFile \r\n");

                l_u32_mcu_fw_total_line = res;  // Set total number of firmware lines
                l_u32_mcu_fw_curr_line = 0;    // Reset current line number

                // Transition to waiting for user confirmation for the update
                StartTickCounter(&l_tmr_confirm);  // Start timer for waiting for confirmation
                lt_mcu_status = MCU_UPDATE_ST_WAIT_CONFIRM;
            }
        }
        break;

    case MCU_UPDATE_ST_WAIT_CONFIRM:  // Wait for user confirmation to proceed with the update
        if (GetTickCounter(&l_tmr_check_fw_state) >= MCU_UPDATE_CHECK_FW_STATE_TIME)  // Timer to check firmware state
        {
            StartTickCounter(&l_tmr_check_fw_state);  // Restart the timer
            send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_CHECK_FW_STATE, 0);  // Send check firmware state command
        }

        if ((GetTickCounter(&l_tmr_confirm) >= MCU_UPDATE_CONFIRM_TIME) || l_b_flag_enter_fw_update)  // Check if confirmation timeout or flag set
        {
            LOG_LEVEL("MCU_UPDATE_ST_WAIT_CONFIRM MCU_UPDATE_CONFIRM_TIME \r\n");
            if (checkPatchValid(MCU_UPDATE_FILE_PATH))  // Validate the patch file again
            {
                LOG_LEVEL("MCU_UPDATE_ST_WAIT_CONFIRM patch valid \r\n");
                // Transition to start the update process
                lt_mcu_status = MCU_UPDATE_ST_START;
            }
        }
        else
        {
            if (!checkPatchValid(MCU_UPDATE_FILE_PATH))  // If patch is invalid, reset to initialization
            {
                LOG_LEVEL("MCU_UPDATE_ST_WAIT_CONFIRM patch invalid\r\n");
                lt_mcu_status = MCU_UPDATE_ST_INIT;  // Transition back to initialization state
            }
        }
        break;

    case MCU_UPDATE_ST_START:  // Start the firmware update process
        LOG_LEVEL("MCU_UPDATE_ST_START \r\n");
        send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_ENTER_FW_UPDATE, 0);  // Send command to enter firmware update mode
        
        l_b_flag_transfer_next = true;  // Set flag to transfer the next chunk
        l_b_flag_transfer_retry = false;  // Reset retry flag

        // Transition to waiting for the MCU to enter boot mode for firmware update
        lt_mcu_status = MCU_UPDATE_ST_WAIT_BOOT;
        break;

    case MCU_UPDATE_ST_WAIT_BOOT:  // Wait for the MCU to enter boot mode for firmware update
        if (GetTickCounter(&l_tmr_check_fw_state) >= MCU_UPDATE_CHECK_FW_STATE_TIME)  // Timer to check firmware state
        {
            StartTickCounter(&l_tmr_check_fw_state);  // Restart the timer
            send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_CHECK_FW_STATE, 0);  // Send check firmware state command
        }

        if (mcu_fw_state == MCU_FW_STATE_WAIT_TRANSFER)  // Check if MCU is ready to receive firmware transfer
        {
            LOG_LEVEL("MCU_UPDATE_ST_WAIT_BOOT jump TRANSFER \r\n");
            // Start the thread to read the Intel Hex file and begin data transfer
					#ifdef PLATFORM_ITE_OPEN_RTOS
                    #ifdef TASK_MANAGER_STATE_MACHINE_UPDATE
            pthread_t task;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  // Set thread to detached state
            pthread_attr_setstacksize(&attr, CFG_UPDATE_STACK_SIZE);  // Set the stack size for the thread
            pthread_create(&task, &attr, app_read_hex_file_task, NULL);  // Create the thread to read and transfer data
					#endif
                    #endif
            // Transition to the data transfer state
            lt_mcu_status = MCU_UPDATE_ST_TRANSFER;
        }
        break;

    case MCU_UPDATE_ST_TRANSFER:  // Data transfer state
        if (l_b_flag_transfer_complete)  // Check if transfer is complete
        {
            l_b_flag_transfer_complete = false;  // Reset transfer complete flag
            if (l_i_mcu_file_res > 0)  // If the file was successfully read
            {
                send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_EXIT_FW_UPDATE, 0);  // Send command to exit firmware update mode
            }
            else
            {
                l_b_flag_update_failed = true;  // Mark the update as failed if file read failed
            }
        }
        if (l_b_flag_update_success)  // If the update was successful
        {
            l_b_flag_update_success = false;  // Reset success flag
            l_b_flag_update_reboot_req = true;  // Flag to request reboot after successful update
            lt_mcu_status = MCU_UPDATE_ST_COMPLETED;  // Transition to completed state
        }
        if (l_b_flag_update_failed)  // If the update failed
        {
            l_b_flag_update_failed = false;  // Reset failure flag
            l_b_flag_update_reboot_req = true;  // Flag to request reboot after failure
            lt_mcu_status = MCU_UPDATE_ST_FAILED;  // Transition to failed state
        }
        break;

    case MCU_UPDATE_ST_COMPLETED:  // Firmware update completed successfully
        if (!checkPatchValid(MCU_UPDATE_FILE_PATH))  // Check if the patch file is valid
        {
            if (l_b_flag_update_reboot_req)  // If reboot is requested
            {
                l_b_flag_update_reboot_req = false;  // Reset reboot request flag
                send_message(TASK_ID_PTL, SOC_TO_MCU_MOD_UPDATE, CMD_MODUPDATE_REBOOT, 0x01);  // Send reboot command
            }
        }
        break;

    case MCU_UPDATE_ST_FAILED:  // Firmware update failed state
        // Log the failure state (optional)
        break;
    }
		#endif
}

#endif

