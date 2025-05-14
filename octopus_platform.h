/*******************************************************************************
 * File Name: octopus_task_manager_platform.h
 * @version  1.0.0
 * @date     2024-12-11
 * @author   Octopus Team
 * Description:
 *   This file provides platform-specific configurations and utility macros
 *   for the Octopus Task Manager project. It supports multiple platforms
 *   and RTOS environments, allowing developers to switch between different
 *   setups based on project requirements.
 *
 * Features:
 *   - Platform selection macros to switch between ITE and CST platforms.
 *   - Inclusion of standard and platform-specific libraries.
 *   - Definition of general-purpose macros for bit manipulation, system timing,
 *     and mathematical operations.
 *   - Abstracted delay and system tick retrieval for supported platforms.
 *
 * Usage:
 *   1. Define the appropriate platform macro (e.g., PLATFORM_ITE_OPEN_RTOS)
 *      before including this file.
 *   2. Use the provided macros and utility functions as needed for platform-
 *      specific development.
 *
 * Notes:
 *   - Ensure all required libraries and headers are available for the target
 *     platform.
 *   - Validate compatibility between the selected platform and RTOS.
 *   - This file is designed for internal use within the Octopus Task Manager
 *     project.
 *
 * License:
 * Copyright (c) 2024 Octopus Team
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 *

 ******************************************************************************/

#ifndef ___OCTOPUS_TASK_MANAGER_PLATFORM_H___
#define ___OCTOPUS_TASK_MANAGER_PLATFORM_H___

/*******************************************************************************
 * PROJECT SWITCH MACROS
 * Define which platform and RTOS to use by enabling the corresponding macro.
 ******************************************************************************/
// #define PLATFORM_ITE_OPEN_RTOS   // Enable ITE platform with OPEN RTOS
// #define PLATFORM_CST_OSAL_RTOS   // Uncomment to use CST platform with OSAL RTOS
// #define PLATFORM_X86_WIND_RTOS   // Uncomment to use XB6 platform with WIND RTOS
//#define PLATFORM_STM32_RTOS
#define PLATFORM_LINUX_RISC         // X86 ARM linux

/***********************************************************************************
 * @brief Task Manager state machine modes.
 */
//#define TASK_MANAGER_STATE_MACHINE_MCU 1 /**< Main control mode. */
#define TASK_MANAGER_STATE_MACHINE_SOC 1 /**< (Reserved) SOC mode. */

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

//#define TASK_MANAGER_STATE_MACHINE_SIF 1 /**< Secondary interface mode. */
//#define TASK_MANAGER_STATE_MACHINE_BLE 1 
//#define TASK_MANAGER_STATE_MACHINE_BMS 1 
//#define TASK_MANAGER_STATE_MACHINE_UPDATE 1 
//#define TASK_MANAGER_STATE_MACHINE_KEY 1 
//#define TASK_MANAGER_STATE_MACHINE_GPIO 1 
//#define TASK_MANAGER_STATE_MACHINE_CAN 1 

#define TASK_MANAGER_STATE_MACHINE_IPC_SOCKET 1 

///////////////////////////////////////////////////////////////////////////////////
//#define USE_EEROM_FOR_DATA_SAVING
/***********************************************************************************
 * INCLUDE FILES
 * Include necessary standard libraries and platform-specific headers.
 ******************************************************************************/
#include <stddef.h>  // Standard definitions for NULL and size_t
#include <stdint.h>  // Standard integer type definitions
#include <stdbool.h> // Boolean type definitions
#include <stdio.h>   // Standard input/output functions
#include <stdarg.h>  // Variable argument list handling
#include <stdlib.h>  // Standard library functions
#include <string.h>  // String manipulation functions
#include <assert.h>  // Debugging support for assertions
#include <time.h>    // Time manipulation functions
#include <stdlib.h>  // for rand()

#include "octopus_log.h"       			// Include logging functions for debugging
#include "octopus_uart_ptl_1.h"    // Include UART protocol header

#ifdef PLATFORM_ITE_OPEN_RTOS
#include <sys/ioctl.h>         // System I/O control definitions
#include <sys/time.h>          // Time-related functions for UNIX systems
#include <pthread.h>           // POSIX thread support
#include "ite/ith.h"           // ITE hardware-specific definitions
#include "ite/itp.h"           // ITE platform-specific definitions
#include "uart/uart.h"         // UART communication module
#include "saradc/saradc.h"     // SAR ADC module for analog input
#include "math.h"              // Standard mathematical functions
#include "SDL/SDL.h"           // SDL library for multimedia applications
#include "openrtos/FreeRTOS.h" // FreeRTOS kernel for real-time tasks
#include "openrtos/queue.h"    // FreeRTOS queue handling
#include <unistd.h>            // POSIX API for file and process handling
#elif defined(PLATFORM_CST_OSAL_RTOS)
#include "OSAL.h"              // OS abstraction layer
#include "OSAL_PwrMgr.h"       // OS power management utilities
#include "OSAL_Memory.h"       // OS memory management functions
#include "gatt.h"              // Bluetooth Generic Attribute Profile
#include "hci.h"               // Host Controller Interface for Bluetooth
#include "gapgattserver.h"     // GAP GATT server management
#include "gattservapp.h"       // GATT service application interface
#include "gatt_profile_uuid.h" // GATT profile UUID definitions
#include "linkdb.h"            // Link database for Bluetooth connections
#include "peripheral.h"        // Peripheral device support utilities
#include "gapbondmgr.h"        // GAP bonding manager for secure connections
#include "hidkbdservice.h"     // HID keyboard service support
#include "hiddev.h"            // HID device implementation
#include "global_config.h"     // Global configuration settings
#include "hidkbd.h"            // HID keyboard implementation
#include "uart.h"              // UART communication module
#include "ll.h"                // Link layer support for Bluetooth
#include "ll_common.h"         // Common link layer definitions
#include "dma.h"               // Direct Memory Access handling
#include "key.h"               // Key input handling utilities
#include "spi.h"               // SPI communication interface
#include "gpio.h"              // General-Purpose Input/Output support
#include "flash.h"             // Flash memory management utilities
#include "clock.h"             // System clock management utilities
#include "error.h"             // Error handling utilities
#include "pwrmgr.h"            // Power management utilities
#include "rom_sym_def.h"       // ROM symbol definitions
#include "sdk_config.h"        // SDK configuration file
#include "types.h"             // Basic type definitions
#elif defined(PLATFORM_LINUX_RISC)
#include <pthread.h>
#include <unistd.h>
#include "../HAL/octopus_serialport_c.h"

#else 
#include "../src/native_devices.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 * Define macros for enabling or disabling debug functionality.
 ******************************************************************************/

/*******************************************************************************
 * GENERAL MACROS
 * Define common bit manipulation macros and constants.
 ******************************************************************************/
#ifdef PLATFORM_CST_OSAL_RTOS
#define GET_SYSTEM_TICK_COUNT (hal_systick() * 625 / 1000) // Convert system ticks to milliseconds
#define DELAY_US(us) (WaitUs(us))                          // Introduce delay in microseconds

#elif defined(PLATFORM_ITE_OPEN_RTOS)
#define CFG_OTSM_STACK_SIZE (200112L)          // Stack size for Octopus Task Manager
#define GET_SYSTEM_TICK_COUNT (SDL_GetTicks()) // Retrieve system tick count in milliseconds
#define DELAY_US(us) (usleep(us))              // Introduce delay in microseconds

#elif defined(PLATFORM_LINUX_RISC)
#define CFG_OTSM_STACK_SIZE (1024 * 1024) //(200112L)// Stack size for Octopus Task Manager
#define DELAY_US(us) (usleep(us))         // Define empty macro for unsupported platforms
#define GET_SYSTEM_TICK_COUNT ({                                                      \
    struct timespec ts;                                                               \
    unsigned long long tick_count = 0;                                                \
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)                                     \
    {                                                                                 \
        tick_count = (unsigned long long)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000); \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
        perror("clock_gettime");                                                      \
    }                                                                                 \
    tick_count;                                                                       \
}) // Return zero for unsupported platforms

#else
extern uint32_t system_tick_ms;
extern volatile uint32_t system_timer_tick_50us;
#define GET_SYSTEM_TICK_COUNT system_tick_ms // Return zero for unsupported platforms
#endif

/*******************************************************************************
 * BIT MANIPULATION MACROS
 * Define macros for setting, clearing, toggling, and extracting bit values.
 ******************************************************************************/
#define BIT_0 0x01 // Bit mask for bit 0
#define BIT_1 0x02 // Bit mask for bit 1
#define BIT_2 0x04 // Bit mask for bit 2
#define BIT_3 0x08 // Bit mask for bit 3
#define BIT_4 0x10 // Bit mask for bit 4
#define BIT_5 0x20 // Bit mask for bit 5
#define BIT_6 0x40 // Bit mask for bit 6
#define BIT_7 0x80 // Bit mask for bit 7

// Macros for setting, clearing, toggling, and checking bits
#define SetBit(VAR, Place) ((VAR) |= (1 << (Place)))          // Set specified bit
#define ClrBit(VAR, Place) ((VAR) &= ~(1 << (Place)))         // Clear specified bit
#define ValBit(VAR, Place) ((VAR) & (1 << (Place)))           // Get value of specified bit
#define ChgBit(VAR, Place) ((VAR) ^= (1 << (Place)))          // Toggle specified bit
#define GetBit(VAR, Place) (((VAR) & (1 << (Place))) ? 1 : 0) // Check bit state

// Macros for byte and word manipulation
#define LSB_BIT(BYTE) ((BYTE) & 0x0F)        // Extract least significant nibble
#define MSB_BIT(BYTE) (((BYTE) >> 4) & 0x0F) // Extract most significant nibble

#define LSB_WORD(a) ((uint8_t)((a) & 0xFF))        // Extract least significant byte
#define MSB_WORD(a) ((uint8_t)(((a) >> 8) & 0xFF)) // Extract most significant byte

#define MK_LSB(a) (uint8_t)(a & 0xFF)                              // Extract the least significant byte (LSB) from a 16-bit word
#define MK_MSB(a) (uint8_t)((a >> 8) & 0xFF)                       // Extract the most significant byte (MSB) from a 16-bit word
#define MK_LSBWORD(a) (uint16_t)(a & 0xFFFF)                       // Extract the least significant word (LSB) from a 32-bit word
#define MK_MSBWORD(a) (uint16_t)((a >> 16) & 0xFFFF)               // Extract the most significant word (MSB) from a 32-bit word
#define MK_BYTE(MSB, LSB) ((uint8_t)(((MSB) << 4) | (LSB)))        // Combine two 4-bit values (MSB and LSB) into a single byte (alternative method)
#define MK_WORD(MSB, LSB) (uint16_t)(((uint16_t)MSB << 8) + LSB)   // Combine two 8-bit values (MSB and LSB) into a 16-bit word
#define MK_DWORD(MSB, LSB) (uint32_t)(((uint32_t)MSB << 16) + LSB) // Combine two 16-bit values (MSB and LSB) into a 32-bit double word
#define MK_SIG_WORD(a) (*(int16_t *)(&a))                          // Interpret a word as a signed 16-bit value

/*******************************************************************************
 * CONSTANTS
 * Define mathematical constants and other useful values.
 ******************************************************************************/
#define PI_FLOAT (3.14159f) // Value of �� as a floating-point constant

/*******************************************************************************
* FUNCTION DECLARATIONS
* Declare any external functions used in this file.
******************************************************************************/

#define MY_ASSERT(expr) do { if (!(expr)) { LOG_LEVEL("ASSERT FAILED: %s, FILE: %s, LINE: %d\n", #expr, __FILE__, __LINE__); while (1); } } while (0)


#ifdef __cplusplus
}
#endif

#endif // ___OCTOPUS_TASK_MANAGER_PLATFORM_H___
