/*******************************************************************************
 * @file octopus_task_manager_ptl.c
 * @brief C file for managing Octopus protocol tasks.
 *
 * This file defines the structure, constants, and function prototypes for handling
 * communication tasks between the MCU (Microcontroller Unit) and the APP (Application)
 * within the Octopus platform. It provides the necessary protocol definitions to
 * ensure smooth task management and communication across the platform.
 *
 * The Octopus protocol facilitates seamless interaction between hardware and software,
 * enabling efficient data exchange and task synchronization. This header file serves
 * as the interface for the task manager, providing the necessary tools to integrate
 * protocol handling into the Octopus platform.
 *
 * @version 1.0.0
 * @author   Octopus Team
 * @date     2024-12-12
 *******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"     // Include platform-specific header for hardware platform details
#include "octopus_log.h"          // Include logging functions for debugging
#include "octopus_task_manager.h" // Include task manager for scheduling tasks

#include "octopus_uart_ptl_2.h"    // Include UART protocol header
#include "octopus_uart_hal.h"    // Include UART hardware abstraction layer header
#include "octopus_tickcounter.h" // Include timer utility functions
#include "octopus_msgqueue.h"    // Include message queue header for task communication

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */
//#define TEST_LOG_DEBUG_PTL_RX_FRAME // Enable debugging for receiving frames
//#define TEST_LOG_DEBUG_PTL_TX_FRAME // Enable debugging for transmitting frames

/*******************************************************************************
 * MACROS
 */

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * LOCAL FUNCTIONS DECLARATION
 */

// Declare function prototypes for various tasks and processing functions

void ptl_2_remove_none_header_data(ptl_2_proc_buff_t *buffer); // Remove data that is not part of the header
void ptl_2_find_valid_frame(ptl_2_proc_buff_t *buffer);        // Find a valid frame from the received data
void ptl_2_proc_valid_frame(ptl_2_proc_buff_t *buffer, uint16_t length); // Process the valid frame
void ptl_2_frame_analysis_handler(void);
void ptl_2_clear_revice_buff(void);

void ptl_2_tx_event_message_handler(void); // Handle the transmission event message
void ptl_2_rx_event_message_handler(void);

void ptl_2_hal_tx(uint8_t *buffer, uint16_t length);
bool ptl_2_is_sleep_enable(void);                         // Check if sleep mode is enabled
void ptl_2_error_detect(void);                            // Detect communication errors
/*******************************************************************************
 * STATIC VARIABLES
 */


//static ptl_2_proc_buff_t l_t_tx_proc_buf;
static ptl_2_proc_buff_t ptl_2_proc_buff;

static uint32_t l_t_ptl_rx_main_timer;
static uint32_t l_t_ptl_tx_main_timer;
static uint32_t l_t_ptl_error_detect_timer;
static bool lb_com_error = false;

static ptl_2_module_info_t l_t_module_info[PTL_MODULE_SUPPORT_CNT];
static uint8_t l_u8_next_empty_module = 0;


/*******************************************************************************
 * GLOBAL FUNCTIONS IMPLEMENTATION
 */

void ptl_2_register_module(ptl_2_module_t module,ptl_2_module_receive_handler_t receive_handler)
{
    if (l_u8_next_empty_module < PTL_MODULE_SUPPORT_CNT)
    {
        l_t_module_info[l_u8_next_empty_module].module = module;
        l_t_module_info[l_u8_next_empty_module].receive_handler = receive_handler;
        l_u8_next_empty_module ++;
    }
}

ptl_2_module_info_t * ptl_2_get_module(ptl_2_module_t module)
{
    ptl_2_module_info_t *module_info = NULL;

    for (uint8_t i = 0; i < l_u8_next_empty_module; i ++)
    {
        if (l_t_module_info[i].module == (module))
        {
            module_info = &l_t_module_info[i];
            break;
        }
    }

    return module_info;
}

// Initialize UART communication for the task
void ptl_2_init_running(void)
{
    LOG_LEVEL("ptl_init_running\r\n");
    OTMS(TASK_ID_PTL_2, OTMS_S_INVALID);
}

// Start the UART communication for the task
void ptl_2_start_running(void)
{
    LOG_LEVEL("ptl_start_running\r\n");
    OTMS(TASK_ID_PTL_2, OTMS_S_ASSERT_RUN);
}

// Assert that UART communication is running
void ptl_2_assert_running(void)
{
		OTMS(TASK_ID_PTL_2, OTMS_S_RUNNING);
		StartTickCounter(&l_t_ptl_rx_main_timer);
		StartTickCounter(&l_t_ptl_tx_main_timer);
		StartTickCounter(&l_t_ptl_error_detect_timer);
		lb_com_error = false;}

// Main running function for UART communication
void ptl_2_running(void)
{
    if (true == ptl_2_is_sleep_enable())
    {
        OTMS(TASK_ID_PTL_2, OTMS_S_POST_RUN);
    }
    else
    {
        ptl_2_tx_event_message_handler();
        ptl_2_rx_event_message_handler();
			
        ///ptl_2_frame_analysis_handler();
        ptl_2_proc_valid_frame(&ptl_2_proc_buff,ptl_2_proc_buff.size);
        ///ptl_2_error_detect();
    }
}

// Post-running function for UART communication
void ptl_2_post_running(void)
{
    if (true == ptl_2_is_sleep_enable())
    {
    }
    else
    {
        OTMS(TASK_ID_PTL_2, OTMS_S_RUNNING);
    }
}

// Stop the UART communication task
void ptl_2_stop_running(void)
{
    OTMS(TASK_ID_PTL_2, OTMS_S_INVALID);
}

// Check if there is a communication error
bool ptl_2_is_com_error(void)
{
    return lb_com_error;
}

/**
 * Check if sleep mode is enabled based on UART pending state.
 * Returns true if no task is currently running.
 */
bool ptl_2_is_sleep_enable(void)
{
    // Check if there are no tasks running
    //if (l_t_ptl_running_req_mask == PTL_RUNNING_NONE)
    //{
        //return true; // Sleep mode is enabled
    //}
    //else
    {
        return false; // Sleep mode is not enabled, tasks are running
    }
}

void ptl_2_receive_handler(uint8_t data)
{
// Check if the platform is CST OSAL RTOS.
#ifdef PLATFORM_CST_OSAL_RTOS
    // Ensure that the received data does not exceed the maximum buffer size.
    if (l_t_rx_proc_buf.size < PTL_FRAME_MAX_SIZE)
    {
        // Store the received byte of data in the buffer at the current position.
        l_t_rx_proc_buf.buff[l_t_rx_proc_buf.size] = data;

        // Increment the size of the buffer to account for the new byte.
        l_t_rx_proc_buf.size++;
    }
#endif
}

/**
 * Handles the event message for transmitting data via UART.
 * Checks if enough time has passed since the last transmission and sends the data if necessary.
 */
void ptl_2_tx_event_message_handler(void)
{

}

/**
 * Handler for receiving UART data byte-by-byte.
 * Calls the UART reception handler to store the received data.
 */
void ptl_2_rx_event_message_handler(void)
{
    // Call the HAL function to process received byte
    // #ifdef PLATFORM_ITE_OPEN_RTOS
    /// uint8_t data = 0;
    uint8_t count = 0;
    while (1)
    {
        if (ptl_2_proc_buff.size < PTL_FRAME_MAX_SIZE)
        {
            count = hal_com_uart_get_fifo_data_2(&ptl_2_proc_buff.buffer[ptl_2_proc_buff.size], PTL_FRAME_MAX_SIZE - ptl_2_proc_buff.size);
            if (count <= 0)
                break;
            ptl_2_proc_buff.size = ptl_2_proc_buff.size + count;
        }
        else
        {
            break;
        }
    }
    // #endif
}

/**
 * Analyzes the received frame by first removing any non-header data,
 * and then processing any valid frames.
 */
void ptl_2_frame_analysis_handler(void)
{
    // Remove data that is not part of the header
    ptl_2_remove_none_header_data(&ptl_2_proc_buff);

    // Process the valid frame found in the buffer
    ptl_2_find_valid_frame(&ptl_2_proc_buff);
}

/**
 * Removes any data from the buffer that is not part of the header.
 * Only keeps the data after the SOC_TO_MCU_PTL_HEADER byte.
 */
void ptl_2_remove_none_header_data(ptl_2_proc_buff_t *proc_buff)
{
// If the first byte is SOC_TO_MCU_PTL_HEADER, no action needed
#ifdef TASK_MANAGER_STATE_MACHINE_MCU
    if (proc_buff->buffer[0] == SOC_TO_MCU_PTL_HEADER)
        return;
    // Search for SOC_TO_MCU_PTL_HEADER and remove data before it
    for (uint16_t i = 0; i < proc_buff->size; i++)
    {
        if (proc_buff->buffer[i] == SOC_TO_MCU_PTL_HEADER)
        {
            // Shift data to remove the position before the header
            for (uint16_t j = i; j < proc_buff->size; j++)
            {
                proc_buff->buffer[j - i] = proc_buff->buffer[j];
            }
            proc_buff->size -= i;
            return;
        }
    }
#endif
#ifdef TASK_MANAGER_STATE_MACHINE_SOC
    if (proc_buff->buffer[0] == MCU_TO_SOC_PTL_HEADER)
        return;
    for (uint16_t i = 0; i < proc_buff->size; i++)
    {
        if (proc_buff->buffer[i] == MCU_TO_SOC_PTL_HEADER)
        {
            // remove data before header
            for (uint16_t j = i; j < proc_buff->size; j++)
            {
                proc_buff->buffer[j - i] = proc_buff->buffer[j];
            }
            proc_buff->size -= i;
            return;
        }
    }

#endif
    // If _PTL_HEADER is not found, clear the buffer
    proc_buff->size = 0;
}

/**
 * Finds a valid frame in the received buffer.
 * Validates the header and frame checksums.
 */
void ptl_2_find_valid_frame(ptl_2_proc_buff_t *proc_buff)
{
    uint8_t datalen = 0;            // Data length
    uint8_t framelen = 0;           // Frame length
    uint8_t crc;                    // Header checksum
    uint8_t crc_read = 0;           // Read checksum value
    bool head_crc_ok = false;       // Flag for header checksum validity
    bool frame_crc_ok = false;      // Flag for frame checksum validity
    bool found = false;             // Flag to indicate if a valid frame is found
    bool header_invalid = false;    // Flag for invalid header detection
    uint16_t next_valid_offset = 0; // Next valid frame offset
    uint16_t offset = 0;            // Header offset

    // Iterate through the received buffer to find a valid frame
    for (uint16_t i = 0; i < proc_buff->size; i++)
    {
#ifdef TASK_MANAGER_STATE_MACHINE_MCU
        if (proc_buff->buffer[i] == SOC_TO_MCU_PTL_HEADER)
        {
#else
        if (proc_buff->buffer[i] == MCU_TO_SOC_PTL_HEADER)
        {
#endif
            offset = i;
            datalen = proc_buff->buffer[i + 3];
            framelen = datalen + PTL_FRAME_HEADER_SIZE + 1;

            // Ensure minimum frame size is met
            if ((framelen < PTL_FRAME_MIN_SIZE) && (proc_buff->size >= PTL_FRAME_MIN_SIZE) && (offset == 0))
            {
                next_valid_offset = PTL_FRAME_MIN_SIZE;
            }
            else if ((framelen >= PTL_FRAME_MIN_SIZE) && (framelen <= (proc_buff->size - offset)))
            {
                // Validate header checksum
                crc = ptl_get_checksum(&proc_buff->buffer[offset], PTL_FRAME_HEADER_SIZE - 1);
                crc_read = proc_buff->buffer[offset + PTL_FRAME_HEADER_SIZE - 1];
                head_crc_ok = (crc == crc_read);

                if (head_crc_ok)
                {
                    // Validate frame checksum
                    crc = ptl_get_checksum(&(proc_buff->buffer[offset + PTL_FRAME_HEADER_SIZE - 1]), (datalen + 1));
                    crc_read = proc_buff->buffer[offset + PTL_FRAME_HEADER_SIZE + datalen];
                    frame_crc_ok = (crc == crc_read);
                }

                // If both header and frame checksums are valid, process the frame
                if (head_crc_ok && frame_crc_ok)
                {
                    next_valid_offset = offset + framelen;
                    found = true;
                    break;
                }
                else
                {
                    // If the first header is invalid, mark it and skip to next frame
                    if ((offset == 0) || (header_invalid == true))
                    {
                        header_invalid = true;
                        next_valid_offset = framelen + offset;
                    }
                }
            }
            else
            {
                // No valid frame found yet, continue searching
            }
        }
    }

    // If a valid frame is found, process it
    if (found == true)
    {
        //ptl_2_proc_valid_frame(proc_buff->buffer + offset, framelen);
    }

    // Shift buffer data if a valid frame is found
    if (next_valid_offset != 0)
    {
        for (uint16_t i = next_valid_offset; i < proc_buff->size; i++)
        {
            proc_buff->buffer[i - next_valid_offset] = proc_buff->buffer[i];
        }
        proc_buff->size = proc_buff->size - next_valid_offset;
    }
}

/**
 * Processes the valid frame after it has been extracted from the buffer.
 * The payload is passed to the appropriate module handler.
 */
void ptl_2_proc_valid_frame(ptl_2_proc_buff_t *ptl_2_proc_buff, uint16_t length)
{
		if (0 == ptl_2_proc_buff->size)
		{
			return;
		}

	#ifdef TEST_LOG_DEBUG_PTL_RX_FRAME
		  if(length > 0)
			{
			LOG_LEVEL("ptl_2_proc_valid_frame data[]= ");
			LOG_BUFF(ptl_2_proc_buff->buffer, length);
		 
			}
	#endif

		for (uint8_t i = 0; i < l_u8_next_empty_module; i++)
		{
				bool res = l_t_module_info[i].receive_handler(ptl_2_proc_buff);
			
				if (res)
				{
						ptl_2_proc_buff->size = 0;
					  ptl_2_clear_revice_buff();
						return;
				}
		}
		//ptl_2_clear_revice_buff();
}
void ptl_2_clear_revice_buff(void)
{
    ptl_2_proc_buff.size = 0;
}
/**
 * Detects communication errors based on the error detection timer.
 * If the opposite side is running and the timeout expires, mark an error.
 */
void ptl_2_error_detect(void)
{
    if (GetTickCounter(&l_t_ptl_error_detect_timer) > 5000)
    {
        StartTickCounter(&l_t_ptl_error_detect_timer);
        ///if (lb_opposite_running)
        ///{
        ///    lb_com_error = true;
        ///}
    }
}


/**
 * Sends data over UART using the hardware abstraction layer.
 */
void ptl_2_hal_tx(uint8_t *buffer, uint16_t length)
{
    hal_com_uart_send_buffer_2(buffer, length);
}
