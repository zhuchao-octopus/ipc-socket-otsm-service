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

#ifndef __OCTOPUS_TASK_MANAGER_PTL_2_H__
#define __OCTOPUS_TASK_MANAGER_PTL_2_H__
/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * MACROS
 *******************************************************************************/
#define PTL_FRAME_MAX_SIZE 255  ///< Maximum frame size
#define PTL_TX_TIMEOUT                  50
#define PTL_MODULE_SUPPORT_CNT          16
/*******************************************************************************
 * ENUMERATIONS
 *******************************************************************************/

// Protocol types
typedef enum {
    SETTING_PTL_BAFANG        = 0x00,  ///< Protocol for Bafang
    SETTING_PTL_LINGHUILIION2 = 0x01,  ///< Protocol for Linghuiliion2
    SETTING_PTL_KEY_DISP      = 0x02,  ///< Protocol for KEY_DISP (KDS)

    SETTING_PTL_BEGIN = SETTING_PTL_BAFANG,
    SETTING_PTL_END   = SETTING_PTL_KEY_DISP,
} SETTING_PTL;

/* ============================== UART PTL ============================== */
#define PTL_FRAME_MAX_SIZE 255

typedef SETTING_PTL ptl_2_module_t;

typedef struct {
    uint16_t size;
    uint8_t  buffer[PTL_FRAME_MAX_SIZE];
} ptl_2_proc_buff_t;

typedef bool (*ptl_2_module_receive_handler_t)(ptl_2_proc_buff_t *buffer);

typedef struct
{
    ptl_2_module_t             module;
    ptl_2_module_receive_handler_t receive_handler;
}ptl_2_module_info_t;

void ptl_2_init_running(void);
void ptl_2_start_running(void);
void ptl_2_assert_running(void);
void ptl_2_running(void);
void ptl_2_post_running(void);
void ptl_2_stop_running(void);

bool ptl_2_is_com_error(void);

void ptl_2_register_module(ptl_2_module_t module, ptl_2_module_receive_handler_t receive_handler);
void ptl_2_clear_revice_buff(void);
int  ptl_2_send(const uint8_t* dataptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __OCTOPUS_TASK_MANAGER_PTL_H__ */
