
/*******************************************************************************
 * @file     octopus_task_manager_flash.h
 * @brief    Header file for managing Flash operations in the Octopus project.
 * @details  This file provides function declarations for reading and writing
 *           data to and from Flash memory. It also includes functions for
 *           printing data in a hexadecimal format for debugging purposes.
 * @version  1.0
 * @date     2024-12-12
 * @author   Octopus Team
 ******************************************************************************/
#ifndef __OCTOPUS_TASK_MANAGER_FLASH_H__
#define __OCTOPUS_TASK_MANAGER_FLASH_H__

#include "octopus_platform.h"  ///< Include platform-specific configurations
#include "octopus_flash_hal.h"  ///< Include Flash Hardware Abstraction Layer (HAL) for low-level operations

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/** 
 * @brief    Declarations of local functions for Flash operations.
 * @details  These functions include Flash read/write operations and debugging
 *           utilities for printing Flash data in hexadecimal format.
 */
extern void PrintfBuffHex(const char *fun, int line, char *str, uint8_t *dat, int len);  
/**< Prints a buffer in hexadecimal format for debugging. */
extern void FlashReadToBuff(uint32_t addr, uint8_t *buf, uint32_t len);   
/**< Reads data from Flash memory into a buffer. */
extern void FlashWriteBuffTo(uint32_t addr, uint8_t *buf, uint32_t len);  
/**< Writes data from a buffer to Flash memory. */
extern void E2ROMReadToBuff(uint32_t addr, uint8_t *buf, uint32_t length);
extern void E2ROMWriteBuffTo(uint32_t addr, uint8_t *buf, uint32_t length);
#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_FLASH_H__ */
