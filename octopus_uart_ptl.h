/*******************************************************************************
 * @file octopus_task_manager_ptl.h
 * @brief Header file for managing Octopus protocol tasks.
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

#ifndef __OCTOPUS_TASK_MANAGER_PTL_H__
#define __OCTOPUS_TASK_MANAGER_PTL_H__
/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 *
 * Define debug macros here for enabling or disabling debugging functionality.
 */

/*******************************************************************************
 * MACROS
 */
#define PTL_UART_CHANNEL 1      ///< UART channel for communication
#define PTL_FRAME_HEADER_SIZE 5 ///< Size of the frame header
#define PTL_FRAME_MIN_SIZE 7    ///< Minimum size of the frame
#define PTL_FRAME_MAX_SIZE 255  ///< Maximum size of the frame
#define PTL_FRAME_DATA_START 5  ///< Start index of the data in the frame
#define PTL_HEADER 0xFA         ///< Frame header identifier

    /*******************************************************************************
     * TYPEDEFS
     */

    /**
     * @brief Enumeration for UART channels.
     */
    typedef enum UartChannel
    {
        UartChannel_Uart0 = 0, ///< UART channel 0
        UartChannel_Uart1 = 1, ///< UART channel 1
        UartChannel_Uart2 = 2  ///< UART channel 2
    } UartChannel;

    /**
     * @brief Enumeration for frame headers.
     */
    typedef enum
    {
        M2A_PTL_HEADER = 0x55, ///< MCU -> APP frame header
        A2M_PTL_HEADER = 0xAA, ///< APP -> MCU frame header
        DBG_PTL_HEADER = 0xFF  ///< for debug
    } ptl_frame_header_t;

    /**
     * @brief Enumeration for module IDs.
     */
    typedef enum
    {
        /* MCU -> APP(SOC) module IDs */
        M2A_MOD_SYSTEM = 0x00,    ///< System initialization
        M2A_MOD_UPDATE = 0x01,    ///< System update
        M2A_MOD_TRANSFER = 0x02,  ///< Data transfer
        M2A_MOD_METER = 0x03,     ///< Meter data
        M2A_MOD_INDICATOR = 0x04, ///< Indicator data
        M2A_MOD_DRIV_INFO = 0x05, ///< Driving information
        M2A_MOD_SETUP = 0x06,     ///< Settings
        M2A_MOD_KEY = 0x07,       ///< KEY

        /* APP(SOC) -> MCU module IDs */
        A2M_MOD_SYSTEM = 0x80,    ///< System initialization
        A2M_MOD_UPDATE = 0x81,    ///< System update
        A2M_MOD_TRANSFER = 0x82,  ///< Data transfer
        A2M_MOD_METER = 0x83,     ///< Meter data
        A2M_MOD_INDICATOR = 0x84, ///< Indicator data
        A2M_MOD_DRIV_INFO = 0x85, ///< Driving information
        A2M_MOD_SETUP = 0x86,     ///< Settings
        A2M_MOD_KEY = 0x87,       ///< KEY
        A2M_MOD_IPC = 0x88,       ///< IPC socket

        P2M_MOD_DEBUG = 0xF0, ///< DEBUG
    } ptl_frame_type_t;

        /* Range definitions for validity checks */
        #define M2A_MOD_START M2A_MOD_SYSTEM
        #define M2A_MOD_END M2A_MOD_KEY
        #define A2M_MOD_START A2M_MOD_SYSTEM
        #define A2M_MOD_END A2M_MOD_KEY

    /**
     * @brief Enumeration for commands within each module.
     */
    typedef enum
    {
        /* MOD_SYSTEM commands */
        CMD_MODSYSTEM_HANDSHAKE = 0x00, ///< System handshake
        CMD_MODSYSTEM_ACC_STATE = 0x01, ///< ACC state
        CMD_MODSYSTEM_APP_STATE = 0x02, ///< Application state
        CMD_MODSYSTEM_POWER_ON = 0x03,  ///< Power on
        CMD_MODSYSTEM_POWER_OFF = 0x04, ///< Power off

        /* MOD_UPDATE commands */
        CMD_MODUPDATE_CHECK_FW_STATE = 0x06,  ///< Check firmware state
        CMD_MODUPDATE_UPDATE_FW_STATE = 0x07, ///< Update firmware state
        CMD_MODUPDATE_ENTER_FW_UPDATE = 0x08, ///< Enter firmware update mode
        CMD_MODUPDATE_EXIT_FW_UPDATE = 0x09,  ///< Exit firmware update mode
        CMD_MODUPDATE_SEND_FW_DATA = 0x0A,    ///< Send firmware data
        CMD_MODUPDATE_REBOOT = 0x0B,          ///< Reboot system

        /* MOD_TRANSFER commands */
        CMD_MODTRANSFER_A2M = 0x0C, ///< A2M data transfer
        CMD_MODTRANSFER_M2A = 0x0D, ///< M2A data transfer

        /* MOD_METER commands */
        CMD_MODMETER_RPM_SPEED = 0x0E,    ///< RPM and speed
        CMD_MODMETER_FUEL_TEMPTER = 0x0F, ///< Fuel and temperature
        CMD_MODMETER_SOC = 0x10,          ///< State of charge (SOC)

        /* MOD_INDICATOR commands */
        CMD_MODINDICATOR_INDICATOR = 0x11,  ///< Indicator status
        CMD_MODINDICATOR_ERROR_INFO = 0x12, ///< Error information

        /* MOD_DRIV_INFO commands */
        CMD_MODDRIVINFO_ODO = 0x14,             ///< Odometer data
        CMD_MODDRIVINFO_DRIV_DATA = 0x15,       ///< Driving data
        CMD_MODDRIVINFO_GEAR = 0x16,            ///< Gear information
        CMD_MODDRIVINFO_NAVI = 0x17,            ///< Navigation data
        CMD_MODDRIVINFO_DRIV_DATA_CLEAR = 0x18, ///< Clear driving data

        /* MOD_SETUP commands */
        CMD_MODSETUP_UPDATE_TIME = 0x19, ///< Update time
        CMD_MODSETUP_SET_TIME = 0x1A,    ///< Set time
        CMD_MODSETUP_KEY = 0x1B          ///< Key input
    } ptl_frame_cmd_t;

    /**
     * @brief Protocol processing buffer.
     */
    typedef struct
    {
        uint16_t size;                    ///< Buffer size
        uint8_t buff[PTL_FRAME_MAX_SIZE]; ///< Data buffer
    } ptl_proc_buff_t;

    /**
     * @brief Protocol frame payload.
     */
    typedef struct
    {
        ptl_frame_type_t frame_type; ///< Frame type
        ptl_frame_cmd_t cmd;         ///< Command
        uint8_t data_len;            ///< Length of the data
        uint8_t *data;               ///< Pointer to the data
    } ptl_frame_payload_t;

    /**
     * @brief Module handlers for sending and receiving frames.
     */
    typedef bool (*module_send_handler_t)(ptl_frame_type_t frame_type, uint16_t param1, uint16_t param2, ptl_proc_buff_t *buff);
    typedef bool (*module_receive_handler_t)(ptl_frame_payload_t *payload, ptl_proc_buff_t *ackbuff);

    /*******************************************************************************
     * GLOBAL FUNCTIONS DECLARATION
     */
    /* Protocol state management */
    /**
     * Initializes the protocol in the running state.
     */
    void ptl_init_running(void);

    /**
     * Starts the protocol in the running state.
     */
    void ptl_start_running(void);

    /**
     * Asserts that the protocol is running.
     */
    void ptl_assert_running(void);

    /**
     * Performs the actions for the protocol in the running state.
     */
    void ptl_running(void);

    /**
     * Handles post-running actions for the protocol.
     */
    void ptl_post_running(void);

    /**
     * Stops the protocol from running.
     */
    void ptl_stop_running(void);

    /* Protocol control functions */
    /**
     * Requests to start the protocol running from a specific source.
     * @param source The source requesting to start the protocol.
     * @return True if the request is successful, false otherwise.
     */
    bool ptl_reqest_running(uint8_t source);

    /**
     * Releases the protocol from running by a specific source.
     * @param source The source releasing the protocol.
     * @return True if the release is successful, false otherwise.
     */
    bool ptl_release_running(uint8_t source);

    /**
     * Sets the opposite running state of the protocol.
     * @param running The opposite state to set.
     */
    void ptl_set_opposite_running(bool running);

    /**
     * Checks if there is a communication error in the protocol.
     * @return True if a communication error exists, false otherwise.
     */
    bool ptl_is_com_error(void);

    /* Protocol frame operations */
    /**
     * Registers a module with the specified frame type and associated send and receive handlers.
     * @param frame_type The type of the frame.
     * @param send_handler The function to handle sending data.
     * @param receive_handler The function to handle receiving data.
     */
    void ptl_register_module(ptl_frame_type_t frame_type, module_send_handler_t send_handler, module_receive_handler_t receive_handler);

    /**
     * Builds a protocol frame with the specified parameters.
     * @param frame_type The type of the frame.
     * @param cmd The command for the frame.
     * @param data The data to include in the frame.
     * @param datelen The length of the data.
     * @param framebuff The buffer to store the built frame.
     */
    void ptl_build_frame(ptl_frame_type_t frame_type, ptl_frame_cmd_t cmd, uint8_t *data, uint8_t datelen, ptl_proc_buff_t *framebuff);

    /**
     * Builds the header of a protocol frame with the specified parameters.
     * @param frame_type The type of the frame.
     * @param cmd The command for the frame.
     * @param datalen The length of the data.
     * @param buff The buffer to store the built header.
     */
    void ptl_build_frame_header(ptl_frame_type_t frame_type, ptl_frame_cmd_t cmd, uint8_t datalen, ptl_proc_buff_t *buff);

    /**
     * Handles the reception of data for the protocol.
     * @param data The received data byte.
     */
    void ptl_receive_handler(uint8_t data);

    /**
     * Analyzes the received protocol frame for proper handling.
     */
    void ptl_frame_analysis_handler(void);

    /**
     * Calculates the checksum for the given data.
     * @param data The data for which the checksum will be calculated.
     * @param len The length of the data.
     * @return The calculated checksum.
     */
    uint8_t ptl_get_checksum(uint8_t *data, uint8_t len);

    void ptl_help(void);

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_PTL_H__ */
